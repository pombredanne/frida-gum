/*
 * Copyright (C) 2008-2010 Ole André Vadla Ravnås <ole.andre.ravnas@tillitech.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "gumallocationtracker.h"

#include <string.h>

#include "gumallocationblock.h"
#include "gumallocationgroup.h"
#include "gummemory.h"
#include "gumreturnaddress.h"
#include "gumbacktracer.h"
#include "gumhash.h"

G_DEFINE_TYPE (GumAllocationTracker, gum_allocation_tracker, G_TYPE_OBJECT);

typedef struct _GumAllocationTrackerBlock GumAllocationTrackerBlock;

enum
{
  PROP_0,
  PROP_BACKTRACER,
};

struct _GumAllocationTrackerPrivate
{
  gboolean disposed;

  GMutex * mutex;

  volatile gint enabled;

  GumAllocationTrackerFilterFunction filter_func;
  gpointer filter_func_user_data;

  guint block_count;
  guint block_total_size;
  GumHashTable * known_blocks_ht;
  GumHashTable * block_groups_ht;

  GumBacktracerIface * backtracer_interface;
  GumBacktracer * backtracer_instance;
};

struct _GumAllocationTrackerBlock
{
  guint size;
  GumReturnAddress return_addresses[1];
};

#define GUM_ALLOCATION_TRACKER_LOCK(t) g_mutex_lock (t->priv->mutex)
#define GUM_ALLOCATION_TRACKER_UNLOCK(t) g_mutex_unlock (t->priv->mutex)

static void gum_allocation_tracker_constructed (GObject * object);
static void gum_allocation_tracker_set_property (GObject * object,
    guint property_id, const GValue * value, GParamSpec * pspec);
static void gum_allocation_tracker_get_property (GObject * object,
    guint property_id, GValue * value, GParamSpec * pspec);
static void gum_allocation_tracker_dispose (GObject * object);
static void gum_allocation_tracker_finalize (GObject * object);

static void gum_allocation_tracker_size_stats_add_block (
    GumAllocationTracker * self, guint size);
static void gum_allocation_tracker_size_stats_remove_block (
    GumAllocationTracker * self, guint size);

static void
gum_allocation_tracker_class_init (GumAllocationTrackerClass * klass)
{
  GObjectClass * object_class = G_OBJECT_CLASS (klass);
  GParamSpec * pspec;

  g_type_class_add_private (klass, sizeof (GumAllocationTrackerPrivate));

  object_class->set_property = gum_allocation_tracker_set_property;
  object_class->get_property = gum_allocation_tracker_get_property;
  object_class->dispose = gum_allocation_tracker_dispose;
  object_class->finalize = gum_allocation_tracker_finalize;
  object_class->constructed = gum_allocation_tracker_constructed;

  pspec = g_param_spec_object ("backtracer", "Backtracer",
      "Backtracer Implementation", GUM_TYPE_BACKTRACER,
      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_CONSTRUCT_ONLY);
  g_object_class_install_property (object_class, PROP_BACKTRACER, pspec);
}

static void
gum_allocation_tracker_init (GumAllocationTracker * self)
{
  GumAllocationTrackerPrivate * priv;

  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GUM_TYPE_ALLOCATION_TRACKER,
      GumAllocationTrackerPrivate);
  priv = self->priv;

  priv->mutex = g_mutex_new ();
}

static void
gum_allocation_tracker_constructed (GObject * object)
{
  GumAllocationTracker * self = GUM_ALLOCATION_TRACKER (object);
  GumAllocationTrackerPrivate * priv = self->priv;

  if (priv->backtracer_instance != NULL)
  {
    priv->known_blocks_ht = gum_hash_table_new_full (NULL, NULL, NULL,
        gum_free);
  }
  else
  {
    priv->known_blocks_ht = gum_hash_table_new (NULL, NULL);
  }

  priv->block_groups_ht = gum_hash_table_new_full (NULL, NULL, NULL,
      (GDestroyNotify) gum_allocation_group_free);
}

static void
gum_allocation_tracker_set_property (GObject * object,
                                     guint property_id,
                                     const GValue * value,
                                     GParamSpec * pspec)
{
  GumAllocationTracker * self = GUM_ALLOCATION_TRACKER (object);
  GumAllocationTrackerPrivate * priv = self->priv;

  switch (property_id)
  {
    case PROP_BACKTRACER:
      if (priv->backtracer_instance != NULL)
        g_object_unref (priv->backtracer_instance);
      priv->backtracer_instance = g_value_dup_object (value);

      if (priv->backtracer_instance != NULL)
      {
        priv->backtracer_interface =
            GUM_BACKTRACER_GET_INTERFACE (priv->backtracer_instance);
      }
      else
      {
        priv->backtracer_interface = NULL;
      }

      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
gum_allocation_tracker_get_property (GObject * object,
                                     guint property_id,
                                     GValue * value,
                                     GParamSpec * pspec)
{
  GumAllocationTracker * self = GUM_ALLOCATION_TRACKER (object);
  GumAllocationTrackerPrivate * priv = self->priv;

  switch (property_id)
  {
    case PROP_BACKTRACER:
      g_value_set_object (value, priv->backtracer_instance);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
gum_allocation_tracker_dispose (GObject * object)
{
  GumAllocationTracker * self = GUM_ALLOCATION_TRACKER (object);
  GumAllocationTrackerPrivate * priv = self->priv;

  if (!priv->disposed)
  {
    priv->disposed = TRUE;

    if (priv->backtracer_instance != NULL)
    {
      g_object_unref (priv->backtracer_instance);
      priv->backtracer_instance = NULL;
    }
    priv->backtracer_interface = NULL;

    gum_hash_table_unref (priv->known_blocks_ht);
    priv->known_blocks_ht = NULL;

    gum_hash_table_unref (priv->block_groups_ht);
    priv->block_groups_ht = NULL;
  }

  G_OBJECT_CLASS (gum_allocation_tracker_parent_class)->dispose (object);
}

static void
gum_allocation_tracker_finalize (GObject * object)
{
  GumAllocationTracker * self = GUM_ALLOCATION_TRACKER (object);

  g_mutex_free (self->priv->mutex);
  self->priv->mutex = NULL;

  G_OBJECT_CLASS (gum_allocation_tracker_parent_class)->finalize (object);
}

GumAllocationTracker *
gum_allocation_tracker_new (void)
{
  return gum_allocation_tracker_new_with_backtracer (NULL);
}

GumAllocationTracker *
gum_allocation_tracker_new_with_backtracer (GumBacktracer * backtracer)
{
  return GUM_ALLOCATION_TRACKER (g_object_new (GUM_TYPE_ALLOCATION_TRACKER,
      "backtracer", backtracer,
      NULL));
}

void
gum_allocation_tracker_set_filter_function (GumAllocationTracker * self,
                                            GumAllocationTrackerFilterFunction filter,
                                            gpointer user_data)
{
  GumAllocationTrackerPrivate * priv = self->priv;

  g_assert (g_atomic_int_get (&priv->enabled) == FALSE);

  priv->filter_func = filter;
  priv->filter_func_user_data = user_data;
}

void
gum_allocation_tracker_begin (GumAllocationTracker * self)
{
  GumAllocationTrackerPrivate * priv = self->priv;

  GUM_ALLOCATION_TRACKER_LOCK (self);
  priv->block_count = 0;
  priv->block_total_size = 0;
  gum_hash_table_remove_all (priv->known_blocks_ht);
  GUM_ALLOCATION_TRACKER_UNLOCK (self);

  g_atomic_int_set (&priv->enabled, TRUE);
}

void
gum_allocation_tracker_end (GumAllocationTracker * self)
{
  GumAllocationTrackerPrivate * priv = self->priv;

  g_atomic_int_set (&priv->enabled, FALSE);

  GUM_ALLOCATION_TRACKER_LOCK (self);
  priv->block_count = 0;
  priv->block_total_size = 0;
  gum_hash_table_remove_all (priv->known_blocks_ht);
  gum_hash_table_remove_all (priv->block_groups_ht);
  GUM_ALLOCATION_TRACKER_UNLOCK (self);
}

guint
gum_allocation_tracker_peek_block_count (GumAllocationTracker * self)
{
  return self->priv->block_count;
}

guint
gum_allocation_tracker_peek_block_total_size (GumAllocationTracker * self)
{
  return self->priv->block_total_size;
}

GumList *
gum_allocation_tracker_peek_block_list (GumAllocationTracker * self)
{
  GumList * blocks = NULL;
  GumHashTableIter iter;
  gpointer key, value;

  GUM_ALLOCATION_TRACKER_LOCK (self);
  gum_hash_table_iter_init (&iter, self->priv->known_blocks_ht);
  while (gum_hash_table_iter_next (&iter, &key, &value))
  {
    if (self->priv->backtracer_instance != NULL)
    {
      GumAllocationTrackerBlock * tb = (GumAllocationTrackerBlock *) value;
      GumAllocationBlock * block;
      guint i;

      block = gum_allocation_block_new (key, tb->size);

      for (i = 0; tb->return_addresses[i] != NULL; i++)
        block->return_addresses.items[i] = tb->return_addresses[i];
      block->return_addresses.len = i;

      blocks = gum_list_prepend (blocks, block);
    }
    else
    {
      blocks = gum_list_prepend (blocks,
          gum_allocation_block_new (key, GPOINTER_TO_UINT (value)));
    }
  }
  GUM_ALLOCATION_TRACKER_UNLOCK (self);

  return blocks;
}

GumList *
gum_allocation_tracker_peek_block_groups (GumAllocationTracker * self)
{
  GumList * groups, * cur;

  GUM_ALLOCATION_TRACKER_LOCK (self);
  groups = gum_hash_table_get_values (self->priv->block_groups_ht);
  for (cur = groups; cur != NULL; cur = cur->next)
    cur->data = gum_allocation_group_copy ((GumAllocationGroup *) cur->data);
  GUM_ALLOCATION_TRACKER_UNLOCK (self);

  return groups;
}

void
gum_allocation_tracker_on_malloc (GumAllocationTracker * self,
                                  gpointer address,
                                  guint size)
{
  gum_allocation_tracker_on_malloc_full (self, address, size, NULL);
}

void
gum_allocation_tracker_on_free (GumAllocationTracker * self,
                                gpointer address)
{
  gum_allocation_tracker_on_free_full (self, address, NULL);
}

void
gum_allocation_tracker_on_realloc (GumAllocationTracker * self,
                                   gpointer old_address,
                                   gpointer new_address,
                                   guint new_size)
{
  gum_allocation_tracker_on_realloc_full (self, old_address, new_address,
      new_size, NULL);
}

void
gum_allocation_tracker_on_malloc_full (GumAllocationTracker * self,
                                       gpointer address,
                                       guint size,
                                       const GumCpuContext * cpu_context)
{
  GumAllocationTrackerPrivate * priv = self->priv;
  gpointer value;

  if (!g_atomic_int_get (&priv->enabled))
    return;

  if (priv->backtracer_instance != NULL)
  {
    gboolean do_backtrace = TRUE;
    GumReturnAddressArray return_addresses;
    GumAllocationTrackerBlock * block;

    if (priv->filter_func != NULL)
    {
      do_backtrace = priv->filter_func (self, address, size,
          priv->filter_func_user_data);
    }

    if (do_backtrace)
    {
      priv->backtracer_interface->generate (priv->backtracer_instance,
          cpu_context, &return_addresses);
    }
    else
    {
      return_addresses.len = 0;
    }

    block = (GumAllocationTrackerBlock *)
        gum_malloc (sizeof (GumAllocationTrackerBlock) +
            (return_addresses.len * sizeof (GumReturnAddress)));
    block->size = size;
    block->return_addresses[return_addresses.len] = NULL;

    if (return_addresses.len > 0)
    {
      memcpy (block->return_addresses, &return_addresses.items,
          return_addresses.len * sizeof (GumReturnAddress));
    }

    value = block;
  }
  else
  {
    value = GUINT_TO_POINTER (size);
  }

  GUM_ALLOCATION_TRACKER_LOCK (self);

  gum_hash_table_insert (priv->known_blocks_ht, address, value);

  gum_allocation_tracker_size_stats_add_block (self, size);

  GUM_ALLOCATION_TRACKER_UNLOCK (self);
}

void
gum_allocation_tracker_on_free_full (GumAllocationTracker * self,
                                     gpointer address,
                                     const GumCpuContext * cpu_context)
{
  GumAllocationTrackerPrivate * priv = self->priv;
  gpointer value;

  (void) cpu_context;

  if (!g_atomic_int_get (&priv->enabled))
    return;

  GUM_ALLOCATION_TRACKER_LOCK (self);

  value = gum_hash_table_lookup (priv->known_blocks_ht, address);
  if (value != NULL)
  {
    guint size;

    if (priv->backtracer_instance != NULL)
      size = ((GumAllocationTrackerBlock *) value)->size;
    else
      size = GPOINTER_TO_UINT (value);

    gum_allocation_tracker_size_stats_remove_block (self, size);

    gum_hash_table_remove (priv->known_blocks_ht, address);
  }

  GUM_ALLOCATION_TRACKER_UNLOCK (self);
}

void
gum_allocation_tracker_on_realloc_full (GumAllocationTracker * self,
                                        gpointer old_address,
                                        gpointer new_address,
                                        guint new_size,
                                        const GumCpuContext * cpu_context)
{
  GumAllocationTrackerPrivate * priv = self->priv;

  if (!g_atomic_int_get (&priv->enabled))
    return;

  if (old_address != NULL)
  {
    if (new_size != 0)
    {
      gpointer value;

      GUM_ALLOCATION_TRACKER_LOCK (self);

      value = gum_hash_table_lookup (priv->known_blocks_ht, old_address);
      if (value != NULL)
      {
        guint old_size;

        gum_hash_table_steal (priv->known_blocks_ht, old_address);

        if (priv->backtracer_instance != NULL)
        {
          GumAllocationTrackerBlock * block;

          block = (GumAllocationTrackerBlock *) value;

          gum_hash_table_insert (priv->known_blocks_ht, new_address, block);

          old_size = block->size;
          block->size = new_size;
        }
        else
        {
          gum_hash_table_insert (priv->known_blocks_ht, new_address,
              GUINT_TO_POINTER (new_size));

          old_size = GPOINTER_TO_UINT (value);
        }

        gum_allocation_tracker_size_stats_remove_block (self, old_size);
        gum_allocation_tracker_size_stats_add_block (self, new_size);
      }

      GUM_ALLOCATION_TRACKER_UNLOCK (self);
    }
    else
    {
      gum_allocation_tracker_on_free_full (self, old_address, cpu_context);
    }
  }
  else
  {
    gum_allocation_tracker_on_malloc_full (self, new_address, new_size,
        cpu_context);
  }
}

static void
gum_allocation_tracker_size_stats_add_block (GumAllocationTracker * self,
                                             guint size)
{
  GumAllocationTrackerPrivate * priv = self->priv;
  GumAllocationGroup * group;

  priv->block_count++;
  priv->block_total_size += size;

  group = (GumAllocationGroup *)
      gum_hash_table_lookup (priv->block_groups_ht, GUINT_TO_POINTER (size));

  if (group == NULL)
  {
    group = gum_allocation_group_new (size);
    gum_hash_table_insert (priv->block_groups_ht, GUINT_TO_POINTER (size),
        group);
  }

  group->alive_now++;
  if (group->alive_now > group->alive_peak)
    group->alive_peak = group->alive_now;
  group->total_peak++;
}

static void
gum_allocation_tracker_size_stats_remove_block (GumAllocationTracker * self,
                                                guint size)
{
  GumAllocationTrackerPrivate * priv = self->priv;
  GumAllocationGroup * group;

  priv->block_count--;
  priv->block_total_size -= size;

  group = (GumAllocationGroup *) gum_hash_table_lookup (priv->block_groups_ht,
      GUINT_TO_POINTER (size));
  group->alive_now--;
}
