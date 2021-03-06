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

#ifndef __GUM_INTERCEPTOR_PRIV_H__
#define __GUM_INTERCEPTOR_PRIV_H__

#include "guminterceptor.h"

#include "gumarray.h"
#include "gumcodeallocator.h"
#include "gumspinlock.h"
#include "gumtls.h"

typedef struct _FunctionContext          FunctionContext;

struct _FunctionContext
{
  GumInterceptor * interceptor;

  gpointer function_address;

  GumCodeAllocator * allocator;
  GumCodeSlice * trampoline_slice;
  volatile gint * trampoline_usage_counter;

  gpointer on_enter_trampoline;
  guint8 overwritten_prologue[32];
  guint overwritten_prologue_len;

  gpointer on_leave_trampoline;

  GumArray * listener_entries;

  gpointer replacement_function_data;
};

extern GumTlsKey _gum_interceptor_guard_key;

G_GNUC_INTERNAL void _gum_interceptor_deinit (void);

gboolean _gum_function_context_on_enter (FunctionContext * function_ctx,
    GumCpuContext * cpu_context, gpointer * caller_ret_addr);
void _gum_function_context_on_leave (FunctionContext * function_ctx,
    GumCpuContext * cpu_context, gpointer * caller_ret_addr);

gboolean _gum_function_context_try_begin_invocation (
    FunctionContext * function_ctx, gpointer caller_ret_addr,
    const GumCpuContext * cpu_context);
gpointer _gum_function_context_end_invocation (void);

void _gum_function_context_make_monitor_trampoline (FunctionContext * ctx);
void _gum_function_context_make_replace_trampoline (FunctionContext * ctx,
    gpointer replacement_function);
void _gum_function_context_destroy_trampoline (FunctionContext * ctx);
void _gum_function_context_activate_trampoline (FunctionContext * ctx);
void _gum_function_context_deactivate_trampoline (FunctionContext * ctx);

gpointer _gum_interceptor_resolve_redirect (gpointer address);
gboolean _gum_interceptor_can_intercept (gpointer function_address);

gpointer _gum_interceptor_invocation_get_nth_argument (
    GumInvocationContext * context, guint n);
void _gum_interceptor_invocation_replace_nth_argument (
    GumInvocationContext * context, guint n, gpointer value);
gpointer _gum_interceptor_invocation_get_return_value (
    GumInvocationContext * context);

#endif

