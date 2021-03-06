/*
 * Copyright (C) 2008-2010 Ole André Vadla Ravnås <ole.andre.ravnas@tillitech.com>
 * Copyright (C) 2008 Christian Berentsen <jc.berentsen@gmail.com>
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

#ifndef __GUM_SYMBOL_UTIL_H__
#define __GUM_SYMBOL_UTIL_H__

#include <gum/gummemory.h>

typedef struct _GumSymbolDetails GumSymbolDetails;

struct _GumSymbolDetails
{
  GumAddress address;
  gchar module_name[GUM_MAX_PATH + 1];
  gchar symbol_name[GUM_MAX_SYMBOL_NAME + 1];
  gchar file_name[GUM_MAX_PATH + 1];
  guint line_number;
};

G_BEGIN_DECLS

GUM_API gboolean gum_symbol_details_from_address (gpointer address,
    GumSymbolDetails * details);
GUM_API gchar * gum_symbol_name_from_address (gpointer address);

GUM_API gpointer gum_find_function (const gchar * name);
GUM_API GArray * gum_find_functions_named (const gchar * name);
GUM_API GArray * gum_find_functions_matching (const gchar * str);

G_END_DECLS

#endif
