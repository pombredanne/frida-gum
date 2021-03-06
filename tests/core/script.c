/*
 * Copyright (C) 2010-2012 Ole André Vadla Ravnås <ole.andre.ravnas@tillitech.com>
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

#include "script-fixture.c"

TEST_LIST_BEGIN (script)
  SCRIPT_TESTENTRY (invalid_script_should_return_null)
  SCRIPT_TESTENTRY (message_can_be_sent)
  SCRIPT_TESTENTRY (message_can_be_received)
  SCRIPT_TESTENTRY (recv_may_specify_desired_message_type)
  SCRIPT_TESTENTRY (recv_can_be_waited_for)
  SCRIPT_TESTENTRY (thread_can_be_forced_to_sleep)
  SCRIPT_TESTENTRY (timeout_can_be_scheduled)
  SCRIPT_TESTENTRY (timeout_can_be_cancelled)
  SCRIPT_TESTENTRY (interval_can_be_scheduled)
  SCRIPT_TESTENTRY (interval_can_be_cancelled)
  SCRIPT_TESTENTRY (argument_can_be_read)
  SCRIPT_TESTENTRY (argument_can_be_replaced)
  SCRIPT_TESTENTRY (return_value_can_be_read)
  SCRIPT_TESTENTRY (invocations_are_bound_on_tls_object)
  SCRIPT_TESTENTRY (invocations_provide_call_depth)
  SCRIPT_TESTENTRY (callbacks_can_be_detached);
  SCRIPT_TESTENTRY (function_can_be_replaced)
  SCRIPT_TESTENTRY (function_can_be_reverted)
  SCRIPT_TESTENTRY (interceptor_performance)
  SCRIPT_TESTENTRY (pointer_can_be_read)
  SCRIPT_TESTENTRY (pointer_can_be_written)
  SCRIPT_TESTENTRY (memory_can_be_allocated)
  SCRIPT_TESTENTRY (s8_can_be_read)
  SCRIPT_TESTENTRY (u8_can_be_read)
  SCRIPT_TESTENTRY (u8_can_be_written)
  SCRIPT_TESTENTRY (s16_can_be_read)
  SCRIPT_TESTENTRY (u16_can_be_read)
  SCRIPT_TESTENTRY (s32_can_be_read)
  SCRIPT_TESTENTRY (u32_can_be_read)
  SCRIPT_TESTENTRY (s64_can_be_read)
  SCRIPT_TESTENTRY (u64_can_be_read)
  SCRIPT_TESTENTRY (byte_array_can_be_read)
  SCRIPT_TESTENTRY (utf8_string_can_be_read)
  SCRIPT_TESTENTRY (utf8_string_can_be_written)
  SCRIPT_TESTENTRY (utf8_string_can_be_allocated)
  SCRIPT_TESTENTRY (utf16_string_can_be_read)
  SCRIPT_TESTENTRY (utf16_string_can_be_allocated)
#ifdef G_OS_WIN32
  SCRIPT_TESTENTRY (ansi_string_can_be_read)
  SCRIPT_TESTENTRY (ansi_string_can_be_allocated)
#endif
  SCRIPT_TESTENTRY (invalid_read_results_in_exception)
  SCRIPT_TESTENTRY (invalid_write_results_in_exception)
  SCRIPT_TESTENTRY (memory_can_be_scanned)
  SCRIPT_TESTENTRY (memory_scan_should_be_interruptible)
  SCRIPT_TESTENTRY (memory_scan_handles_unreadable_memory)
  SCRIPT_TESTENTRY (process_arch_is_available)
  SCRIPT_TESTENTRY (process_platform_is_available)
#ifndef HAVE_ANDROID
  SCRIPT_TESTENTRY (process_threads_can_be_enumerated)
#endif
  SCRIPT_TESTENTRY (process_modules_can_be_enumerated)
  SCRIPT_TESTENTRY (process_ranges_can_be_enumerated)
  SCRIPT_TESTENTRY (module_exports_can_be_enumerated)
  SCRIPT_TESTENTRY (module_exports_enumeration_performance)
  SCRIPT_TESTENTRY (module_ranges_can_be_enumerated)
  SCRIPT_TESTENTRY (module_base_address_can_be_found)
  SCRIPT_TESTENTRY (module_export_can_be_found_by_name)
  SCRIPT_TESTENTRY (socket_type_can_be_inspected)
#ifndef HAVE_ANDROID
  SCRIPT_TESTENTRY (socket_endpoints_can_be_inspected)
#endif
  SCRIPT_TESTENTRY (native_function_can_be_invoked)
  SCRIPT_TESTENTRY (native_callback_can_be_invoked)
  SCRIPT_TESTENTRY (file_can_be_written_to)
#ifdef HAVE_I386
  SCRIPT_TESTENTRY (execution_can_be_traced)
  SCRIPT_TESTENTRY (call_can_be_probed)
#endif
  SCRIPT_TESTENTRY (script_can_be_reloaded)
TEST_LIST_END ()

SCRIPT_TESTCASE (native_function_can_be_invoked)
{
  gchar str[7];

  strcpy (str, "badger");
  COMPILE_AND_LOAD_SCRIPT (
      "var toupper = new NativeFunction(" GUM_PTR_CONST ", "
          "'int', ['pointer', 'int']);"
      "send(toupper(" GUM_PTR_CONST ", 3));"
      "send(toupper(" GUM_PTR_CONST ", -1));",
      gum_toupper, str, str);
  EXPECT_SEND_MESSAGE_WITH ("3");
  EXPECT_SEND_MESSAGE_WITH ("-6");
  EXPECT_NO_MESSAGES ();
  g_assert_cmpstr (str, ==, "BADGER");
}

SCRIPT_TESTCASE (native_callback_can_be_invoked)
{
  TestScriptMessageItem * item;
  gint (* toupper_impl) (gchar * str, gint limit);
  gchar str[7];

  COMPILE_AND_LOAD_SCRIPT (
      "toupper = new NativeCallback(function (str, limit) {"
      "  var count = 0;"
      "  while (count < limit || limit === -1) {"
      "    var p = str.add(count);"
      "    var b = Memory.readU8(p);"
      "    if (b === 0)"
      "      break;"
      "    Memory.writeU8(p, String.fromCharCode(b).toUpperCase().charCodeAt(0));"
      "    count++;"
      "  }"
      "  return (limit === -1) ? -count : count;"
      "}, 'int', ['pointer', 'int']);"
      "send(toupper);");

  item = test_script_fixture_pop_message (fixture);
  sscanf (item->message, "{\"type\":\"send\",\"payload\":"
      "\"0x%" G_GSIZE_MODIFIER "x\"}", (gsize *) &toupper_impl);
  g_assert (toupper_impl != NULL);
  test_script_message_item_free (item);

  strcpy (str, "badger");
  g_assert_cmpint (toupper_impl (str, 3), ==, 3);
  g_assert_cmpstr (str, ==, "BADger");
  g_assert_cmpint (toupper_impl (str, -1), ==, -6);
  g_assert_cmpstr (str, ==, "BADGER");
}

static gint
gum_toupper (gchar * str,
             gint limit)
{
  gint count = 0;
  gchar * c;

  for (c = str; *c != '\0' && (count < limit || limit == -1); c++, count++)
  {
    *c = g_ascii_toupper (*c);
  }

  return (limit == -1) ? -count : count;
}

SCRIPT_TESTCASE (file_can_be_written_to)
{
  gchar d00d[4] = { 0x64, 0x30, 0x30, 0x64 };

  if (!g_test_slow ())
  {
    g_print ("<skipping, run in slow mode> ");
    return;
  }

  COMPILE_AND_LOAD_SCRIPT (
      "var log = new File(\"/tmp/script-test.log\", 'a');"
      "log.write(\"Hello \");"
      "log.write(Memory.readByteArray(" GUM_PTR_CONST ", 4));"
      "log.write(\"!\\n\");"
      "log.close();",
      d00d);
  EXPECT_NO_MESSAGES ();
}

SCRIPT_TESTCASE (socket_type_can_be_inspected)
{
  int fd;
  struct sockaddr_in addr = { 0, };

  fd = socket (AF_INET, SOCK_STREAM, 0);
  COMPILE_AND_LOAD_SCRIPT ("send(Socket.type(%d));", fd);
  EXPECT_SEND_MESSAGE_WITH ("\"tcp\"");
  addr.sin_family = AF_INET;
  addr.sin_port = htons (39876);
  addr.sin_addr.s_addr = INADDR_ANY;
  bind (fd, (struct sockaddr *) &addr, sizeof (addr));
  COMPILE_AND_LOAD_SCRIPT ("send(Socket.type(%d));", fd);
  EXPECT_SEND_MESSAGE_WITH ("\"tcp\"");
  GUM_CLOSE_SOCKET (fd);

  fd = socket (AF_INET, SOCK_DGRAM, 0);
  COMPILE_AND_LOAD_SCRIPT ("send(Socket.type(%d));", fd);
  EXPECT_SEND_MESSAGE_WITH ("\"udp\"");
  GUM_CLOSE_SOCKET (fd);

  fd = socket (AF_INET6, SOCK_STREAM, 0);
  COMPILE_AND_LOAD_SCRIPT ("send(Socket.type(%d));", fd);
  EXPECT_SEND_MESSAGE_WITH ("\"tcp6\"");
  GUM_CLOSE_SOCKET (fd);

  fd = socket (AF_INET6, SOCK_DGRAM, 0);
  COMPILE_AND_LOAD_SCRIPT ("send(Socket.type(%d));", fd);
  EXPECT_SEND_MESSAGE_WITH ("\"udp6\"");
  GUM_CLOSE_SOCKET (fd);

  COMPILE_AND_LOAD_SCRIPT ("send(Socket.type(-1));");
  EXPECT_SEND_MESSAGE_WITH ("null");

#ifndef G_OS_WIN32
  fd = socket (AF_UNIX, SOCK_STREAM, 0);
  COMPILE_AND_LOAD_SCRIPT ("send(Socket.type(%d));", fd);
  EXPECT_SEND_MESSAGE_WITH ("\"unix:stream\"");
  close (fd);

  fd = socket (AF_UNIX, SOCK_DGRAM, 0);
  COMPILE_AND_LOAD_SCRIPT ("send(Socket.type(%d));", fd);
  EXPECT_SEND_MESSAGE_WITH ("\"unix:dgram\"");
  close (fd);

  fd = open ("/etc/hosts", O_RDONLY);
  g_assert (fd >= 0);
  COMPILE_AND_LOAD_SCRIPT ("send(Socket.type(%d));", fd);
  EXPECT_SEND_MESSAGE_WITH ("null");
  close (fd);
#endif
}

#ifndef HAVE_ANDROID

SCRIPT_TESTCASE (socket_endpoints_can_be_inspected)
{
  GSocketFamily family[] = { G_SOCKET_FAMILY_IPV4, G_SOCKET_FAMILY_IPV6 };
  guint i;
  GMainContext * context;
  int fd;

  context = g_main_context_get_thread_default ();

  for (i = 0; i != G_N_ELEMENTS (family); i++)
  {
    GSocketService * service;
    guint16 client_port, server_port;
    GSocketAddress * client_address, * server_address;
    GInetAddress * loopback;
    GSocket * socket;

    service = g_socket_service_new ();
    g_signal_connect (service, "incoming", G_CALLBACK (on_incoming_connection),
        NULL);
    server_port = g_socket_listener_add_any_inet_port (
        G_SOCKET_LISTENER (service), NULL, NULL);
    g_socket_service_start (service);
    loopback = g_inet_address_new_loopback (family[i]);
    server_address = g_inet_socket_address_new (loopback, server_port);
    g_object_unref (loopback);

    socket = g_socket_new (family[i], G_SOCKET_TYPE_STREAM,
        G_SOCKET_PROTOCOL_TCP, NULL);
    fd = g_socket_get_fd (socket);

    COMPILE_AND_LOAD_SCRIPT ("send(Socket.peerAddress(%d));", fd);
    EXPECT_SEND_MESSAGE_WITH ("null");

    while (g_main_context_pending (context))
      g_main_context_iteration (context, FALSE);

    g_assert (g_socket_connect (socket, server_address, NULL, NULL));

    g_object_get (socket, "local-address", &client_address, NULL);
    client_port =
        g_inet_socket_address_get_port (G_INET_SOCKET_ADDRESS (client_address));

    while (g_main_context_pending (context))
      g_main_context_iteration (context, FALSE);

    COMPILE_AND_LOAD_SCRIPT (
        "var addr = Socket.localAddress(%d);"
        "send([typeof addr.ip, addr.port]);", fd);
    EXPECT_SEND_MESSAGE_WITH ("[\"string\",%u]", client_port);

    COMPILE_AND_LOAD_SCRIPT (
        "var addr = Socket.peerAddress(%d);"
        "send([typeof addr.ip, addr.port]);", fd);
    EXPECT_SEND_MESSAGE_WITH ("[\"string\",%u]", server_port);

    g_socket_close (socket, NULL);
    g_socket_service_stop (service);
    while (g_main_context_pending (context))
      g_main_context_iteration (context, FALSE);

    g_object_unref (socket);

    g_object_unref (client_address);
    g_object_unref (server_address);
    g_object_unref (service);
  }

#ifdef HAVE_DARWIN
  {
    struct sockaddr_un address;
    socklen_t len;

    fd = socket (AF_UNIX, SOCK_STREAM, 0);

    address.sun_family = AF_UNIX;
    strcpy (address.sun_path, "/tmp/gum-script-test");
    unlink (address.sun_path);
    address.sun_len = sizeof (address) - sizeof (address.sun_path) +
        strlen (address.sun_path) + 1;
    len = address.sun_len;
    bind (fd, (struct sockaddr *) &address, len);

    COMPILE_AND_LOAD_SCRIPT ("send(Socket.localAddress(%d));", fd);
    EXPECT_SEND_MESSAGE_WITH ("{\"path\":\"\"}");
    close (fd);

    unlink (address.sun_path);
  }
#endif
}

static gboolean
on_incoming_connection (GSocketService * service,
                        GSocketConnection * connection,
                        GObject * source_object,
                        gpointer user_data)
{
  GInputStream * input;
  void * buf;

  input = g_io_stream_get_input_stream (G_IO_STREAM (connection));
  buf = g_malloc (1);
  g_input_stream_read_async (input, buf, 1, G_PRIORITY_DEFAULT, NULL,
      on_read_ready, NULL);

  return TRUE;
}

static void
on_read_ready (GObject * source_object,
               GAsyncResult * res,
               gpointer user_data)
{
  GError * error = NULL;
  g_input_stream_read_finish (G_INPUT_STREAM (source_object), res, &error);
  g_clear_error (&error);
}

#endif /* !HAVE_ANDROID */

#ifdef HAVE_I386

SCRIPT_TESTCASE (execution_can_be_traced)
{
  GMainContext * context;

  if (!g_test_slow ())
  {
    g_print ("<skipping, run in slow mode> ");
    return;
  }

  context = g_main_context_get_thread_default ();

  COMPILE_AND_LOAD_SCRIPT (
    "var me = Process.getCurrentThreadId();"
    "Stalker.follow(me, {"
    "  events: {"
    "    call: true,"
    "    ret: false,"
    "    exec: false"
    "  },"
    "  onReceive: function(events) {"
    "    send(events.length > 0);"
    "  },"
    "  onCallSummary: function(summary) {"
    "    send(Object.keys(summary).length > 0);"
    "  }"
    "});"
    "recv('stop', function(message) {"
    "  Stalker.unfollow();"
    "});");
  g_usleep (1);
  EXPECT_NO_MESSAGES ();
  POST_MESSAGE ("{\"type\":\"stop\"}");
  while (g_main_context_pending (context))
    g_main_context_iteration (context, FALSE);
  EXPECT_SEND_MESSAGE_WITH ("true");
  EXPECT_SEND_MESSAGE_WITH ("true");
}

SCRIPT_TESTCASE (call_can_be_probed)
{
  if (!g_test_slow ())
  {
    g_print ("<skipping, run in slow mode> ");
    return;
  }

  COMPILE_AND_LOAD_SCRIPT ("Stalker.follow();"
    "Stalker.addCallProbe(" GUM_PTR_CONST ", function(args) {"
    "  send(args[0].toInt32());"
    "});"
    "recv('stop', function(message) {"
    "  Stalker.unfollow();"
    "});", target_function_int);
  EXPECT_NO_MESSAGES ();
  target_function_int (1337);
  EXPECT_SEND_MESSAGE_WITH ("1337");
  POST_MESSAGE ("{\"type\":\"stop\"}");
}

#endif /* HAVE_I386 */

SCRIPT_TESTCASE (process_arch_is_available)
{
  COMPILE_AND_LOAD_SCRIPT ("send(Process.arch);");
#if defined (HAVE_I386)
# if GLIB_SIZEOF_VOID_P == 4
  EXPECT_SEND_MESSAGE_WITH ("\"ia32\"");
# else
  EXPECT_SEND_MESSAGE_WITH ("\"x64\"");
# endif
#elif defined (HAVE_ARM)
  EXPECT_SEND_MESSAGE_WITH ("\"arm\"");
#endif
}

SCRIPT_TESTCASE (process_platform_is_available)
{
  COMPILE_AND_LOAD_SCRIPT ("send(Process.platform);");
#if defined (HAVE_LINUX)
  EXPECT_SEND_MESSAGE_WITH ("\"linux\"");
#elif defined (HAVE_DARWIN)
  EXPECT_SEND_MESSAGE_WITH ("\"darwin\"");
#elif defined (G_OS_WIN32)
  EXPECT_SEND_MESSAGE_WITH ("\"windows\"");
#endif
}

#ifndef HAVE_ANDROID
SCRIPT_TESTCASE (process_threads_can_be_enumerated)
{
  COMPILE_AND_LOAD_SCRIPT (
      "Process.enumerateThreads({"
        "onMatch: function(thread) {"
        "  send('onMatch');"
        "  return 'stop';"
        "},"
        "onComplete: function() {"
        "  send('onComplete');"
        "}"
      "});");
  EXPECT_SEND_MESSAGE_WITH ("\"onMatch\"");
  EXPECT_SEND_MESSAGE_WITH ("\"onComplete\"");
}
#endif

SCRIPT_TESTCASE (process_modules_can_be_enumerated)
{
  COMPILE_AND_LOAD_SCRIPT (
      "Process.enumerateModules({"
        "onMatch: function(name, address, size, path) {"
        "  send('onMatch');"
        "  return 'stop';"
        "},"
        "onComplete: function() {"
        "  send('onComplete');"
        "}"
      "});");
  EXPECT_SEND_MESSAGE_WITH ("\"onMatch\"");
  EXPECT_SEND_MESSAGE_WITH ("\"onComplete\"");
}

SCRIPT_TESTCASE (process_ranges_can_be_enumerated)
{
  COMPILE_AND_LOAD_SCRIPT (
      "Process.enumerateRanges('--x', {"
        "onMatch: function(address, size, prot) {"
        "  send('onMatch');"
        "  return 'stop';"
        "},"
        "onComplete: function() {"
        "  send('onComplete');"
        "}"
      "});");
  EXPECT_SEND_MESSAGE_WITH ("\"onMatch\"");
  EXPECT_SEND_MESSAGE_WITH ("\"onComplete\"");
}

SCRIPT_TESTCASE (module_exports_can_be_enumerated)
{
  COMPILE_AND_LOAD_SCRIPT (
      "Module.enumerateExports(\"%s\", {"
        "onMatch: function(name, address) {"
        "  send('onMatch');"
        "  return 'stop';"
        "},"
        "onComplete: function() {"
        "  send('onComplete');"
        "}"
      "});", SYSTEM_MODULE_NAME);
  EXPECT_SEND_MESSAGE_WITH ("\"onMatch\"");
  EXPECT_SEND_MESSAGE_WITH ("\"onComplete\"");
}

SCRIPT_TESTCASE (module_exports_enumeration_performance)
{
  TestScriptMessageItem * item;
  gint duration;

  COMPILE_AND_LOAD_SCRIPT (
      "var start = new Date();"
      "Module.enumerateExports(\"%s\", {"
        "onMatch: function(name, address) {"
        "},"
        "onComplete: function() {"
        "}"
      "});"
      "send((new Date()).getTime() - start.getTime());",
      SYSTEM_MODULE_NAME);
  item = test_script_fixture_pop_message (fixture);
  sscanf (item->message, "{\"type\":\"send\",\"payload\":%d}", &duration);
  g_print ("<%d ms> ", duration);
  test_script_message_item_free (item);
}

SCRIPT_TESTCASE (module_ranges_can_be_enumerated)
{
  COMPILE_AND_LOAD_SCRIPT (
      "Module.enumerateRanges(\"%s\", '--x', {"
        "onMatch: function(address, size, prot) {"
        "  send('onMatch');"
        "  return 'stop';"
        "},"
        "onComplete: function() {"
        "  send('onComplete');"
        "}"
      "});", SYSTEM_MODULE_NAME);
  EXPECT_SEND_MESSAGE_WITH ("\"onMatch\"");
  EXPECT_SEND_MESSAGE_WITH ("\"onComplete\"");
}

SCRIPT_TESTCASE (module_base_address_can_be_found)
{
  COMPILE_AND_LOAD_SCRIPT (
      "send(Module.findBaseAddress('%s') !== null);", SYSTEM_MODULE_NAME);
  EXPECT_SEND_MESSAGE_WITH ("true");
}

SCRIPT_TESTCASE (module_export_can_be_found_by_name)
{
#ifdef G_OS_WIN32
  HMODULE mod;
  gpointer actual_address;
  char actual_address_str[32];

  mod = GetModuleHandle (_T ("kernel32.dll"));
  g_assert (mod != NULL);
  actual_address = GetProcAddress (mod, "Sleep");
  g_assert (actual_address != NULL);
  sprintf_s (actual_address_str, sizeof (actual_address_str),
      "\"%" G_GSIZE_MODIFIER "x\"", GPOINTER_TO_SIZE (actual_address));

  COMPILE_AND_LOAD_SCRIPT (
      "send(Module.findExportByName('kernel32.dll', 'Sleep').toString(16));");
  EXPECT_SEND_MESSAGE_WITH (actual_address_str);
#else
  COMPILE_AND_LOAD_SCRIPT (
      "send(Module.findExportByName('%s', '%s') !== null);",
      SYSTEM_MODULE_NAME, SYSTEM_MODULE_EXPORT);
  EXPECT_SEND_MESSAGE_WITH ("true");
#endif
}

SCRIPT_TESTCASE (invalid_script_should_return_null)
{
  GError * err = NULL;

  g_assert (gum_script_from_string ("'", NULL) == NULL);

  g_assert (gum_script_from_string ("'", &err) == NULL);
  g_assert (err != NULL);
  g_assert_cmpstr (err->message, ==,
      "Script(line 1): SyntaxError: Unexpected token ILLEGAL");
}

SCRIPT_TESTCASE (message_can_be_sent)
{
  COMPILE_AND_LOAD_SCRIPT ("send(1234);");
  EXPECT_SEND_MESSAGE_WITH ("1234");
}

SCRIPT_TESTCASE (message_can_be_received)
{
  COMPILE_AND_LOAD_SCRIPT (
      "recv(function(message) {"
      "  if (message.type == 'ping')"
      "    send('pong');"
      "});");
  EXPECT_NO_MESSAGES ();
  POST_MESSAGE ("{\"type\":\"ping\"}");
  EXPECT_SEND_MESSAGE_WITH ("\"pong\"");
}

SCRIPT_TESTCASE (recv_may_specify_desired_message_type)
{
  COMPILE_AND_LOAD_SCRIPT (
      "recv('wobble', function(message) {"
      "  send('wibble');"
      "});"
      "recv('ping', function(message) {"
      "  send('pong');"
      "});");
  EXPECT_NO_MESSAGES ();
  POST_MESSAGE ("{\"type\":\"ping\"}");
  EXPECT_SEND_MESSAGE_WITH ("\"pong\"");
}

typedef struct _GumInvokeTargetContext GumInvokeTargetContext;

struct _GumInvokeTargetContext
{
  GumScript * script;
  volatile gboolean started;
  volatile gboolean finished;
};

SCRIPT_TESTCASE (recv_can_be_waited_for)
{
  GThread * worker_thread;
  GumInvokeTargetContext ctx;

  COMPILE_AND_LOAD_SCRIPT (
      "Interceptor.attach(" GUM_PTR_CONST ", {"
      "  onEnter: function(args) {"
      "    op = recv('poke', function(pokeMessage) {"
      "      send('pokeBack');"
      "    });"
      "    op.wait();"
      "    send('pokeReceived');"
      "  }"
      "});", target_function_int);
  EXPECT_NO_MESSAGES ();

  ctx.script = fixture->script;
  ctx.started = FALSE;
  ctx.finished = FALSE;
  worker_thread = g_thread_create (invoke_target_function_int_worker,
      &ctx, TRUE, NULL);
  while (!ctx.started)
    g_usleep (G_USEC_PER_SEC / 200);

  g_usleep (G_USEC_PER_SEC / 25);
  EXPECT_NO_MESSAGES ();
  g_assert (!ctx.finished);

  POST_MESSAGE ("{\"type\":\"poke\"}");
  g_thread_join (worker_thread);
  g_assert (ctx.finished);
  EXPECT_SEND_MESSAGE_WITH ("\"pokeBack\"");
  EXPECT_SEND_MESSAGE_WITH ("\"pokeReceived\"");
  EXPECT_NO_MESSAGES ();
}

static gpointer
invoke_target_function_int_worker (gpointer data)
{
  GumInvokeTargetContext * ctx = (GumInvokeTargetContext *) data;

  ctx->started = TRUE;
  target_function_int (42);
  ctx->finished = TRUE;

  return NULL;
}

SCRIPT_TESTCASE (thread_can_be_forced_to_sleep)
{
  GTimer * timer = g_timer_new ();
  COMPILE_AND_LOAD_SCRIPT ("Thread.sleep(0.25);");
  g_assert_cmpfloat (g_timer_elapsed (timer, NULL), >=, 0.2f);
  EXPECT_NO_MESSAGES ();
  g_timer_destroy (timer);
}

SCRIPT_TESTCASE (timeout_can_be_scheduled)
{
  GMainContext * context;

  context = g_main_context_get_thread_default ();

  COMPILE_AND_LOAD_SCRIPT (
      "setTimeout(function() {"
      "  send(1337);"
      "}, 20);");
  EXPECT_NO_MESSAGES ();
  while (g_main_context_pending (context))
    g_main_context_iteration (context, FALSE);
  EXPECT_NO_MESSAGES ();

  g_usleep (25000);
  while (g_main_context_pending (context))
    g_main_context_iteration (context, FALSE);
  EXPECT_SEND_MESSAGE_WITH ("1337");

  g_usleep (25000);
  while (g_main_context_pending (context))
    g_main_context_iteration (context, FALSE);
  EXPECT_NO_MESSAGES ();
}

SCRIPT_TESTCASE (timeout_can_be_cancelled)
{
  GMainContext * context;

  context = g_main_context_get_thread_default ();

  COMPILE_AND_LOAD_SCRIPT (
      "var timeout = setTimeout(function() {"
      "  send(1337);"
      "}, 20);"
      "clearTimeout(timeout);");
  while (g_main_context_pending (context))
    g_main_context_iteration (context, FALSE);
  g_usleep (25000);
  while (g_main_context_pending (context))
    g_main_context_iteration (context, FALSE);
  EXPECT_NO_MESSAGES ();
}

SCRIPT_TESTCASE (interval_can_be_scheduled)
{
  GMainContext * context;

  context = g_main_context_get_thread_default ();

  COMPILE_AND_LOAD_SCRIPT (
      "setInterval(function() {"
      "  send(1337);"
      "}, 20);");
  EXPECT_NO_MESSAGES ();
  while (g_main_context_pending (context))
    g_main_context_iteration (context, FALSE);
  EXPECT_NO_MESSAGES ();

  g_usleep (25000);
  while (g_main_context_pending (context))
    g_main_context_iteration (context, FALSE);
  EXPECT_SEND_MESSAGE_WITH ("1337");

  g_usleep (25000);
  while (g_main_context_pending (context))
    g_main_context_iteration (context, FALSE);
  EXPECT_SEND_MESSAGE_WITH ("1337");
}

SCRIPT_TESTCASE (interval_can_be_cancelled)
{
  GMainContext * context;

  context = g_main_context_get_thread_default ();

  COMPILE_AND_LOAD_SCRIPT (
      "var count = 1;"
      "var interval = setInterval(function() {"
      "  send(count++);"
      "  if (count == 3)"
      "    clearInterval(interval);"
      "}, 20);");
  while (g_main_context_pending (context))
    g_main_context_iteration (context, FALSE);

  g_usleep (25000);
  while (g_main_context_pending (context))
    g_main_context_iteration (context, FALSE);
  EXPECT_SEND_MESSAGE_WITH ("1");

  g_usleep (25000);
  while (g_main_context_pending (context))
    g_main_context_iteration (context, FALSE);
  EXPECT_SEND_MESSAGE_WITH ("2");

  g_usleep (25000);
  while (g_main_context_pending (context))
    g_main_context_iteration (context, FALSE);
  EXPECT_NO_MESSAGES ();
}

SCRIPT_TESTCASE (argument_can_be_read)
{
  COMPILE_AND_LOAD_SCRIPT (
      "Interceptor.attach(" GUM_PTR_CONST ", {"
      "  onEnter: function(args) {"
      "    send(args[0].toInt32());"
      "  }"
      "});", target_function_int);

  EXPECT_NO_MESSAGES ();

  target_function_int (42);
  EXPECT_SEND_MESSAGE_WITH ("42");

  target_function_int (-42);
  EXPECT_SEND_MESSAGE_WITH ("-42");
}

SCRIPT_TESTCASE (argument_can_be_replaced)
{
  COMPILE_AND_LOAD_SCRIPT (
      "var replacementString = Memory.allocUtf8String('Hei');"
      "Interceptor.attach(" GUM_PTR_CONST ", {"
      "  onEnter: function(args) {"
      "    args[0] = replacementString;"
      "  }"
      "});", target_function_string);

  EXPECT_NO_MESSAGES ();
  g_assert_cmpstr (target_function_string ("Hello"), ==, "Hei");
  EXPECT_NO_MESSAGES ();
}

SCRIPT_TESTCASE (return_value_can_be_read)
{
  COMPILE_AND_LOAD_SCRIPT (
      "Interceptor.attach(" GUM_PTR_CONST ", {"
      "  onLeave: function(retval) {"
      "    send(retval.toInt32());"
      "  }"
      "});", target_function_int);

  EXPECT_NO_MESSAGES ();
  target_function_int (7);
  EXPECT_SEND_MESSAGE_WITH ("315");
}

SCRIPT_TESTCASE (invocations_are_bound_on_tls_object)
{
  COMPILE_AND_LOAD_SCRIPT (
      "Interceptor.attach(" GUM_PTR_CONST ", {"
      "  onEnter: function(args) {"
      "    send(this.value || null);"
      "    this.value = args[0].toInt32();"
      "  },"
      "  onLeave: function(retval) {"
      "    send(this.value || null);"
      "  }"
      "});", target_function_int);

  EXPECT_NO_MESSAGES ();
  target_function_int (7);
  EXPECT_SEND_MESSAGE_WITH ("null");
  EXPECT_SEND_MESSAGE_WITH ("7");
  target_function_int (11);
  EXPECT_SEND_MESSAGE_WITH ("null");
  EXPECT_SEND_MESSAGE_WITH ("11");
}

SCRIPT_TESTCASE (invocations_provide_call_depth)
{
  COMPILE_AND_LOAD_SCRIPT (
      "Interceptor.attach(" GUM_PTR_CONST ", {"
      "  onEnter: function(args) {"
      "    send('>a' + this.depth);"
      "  },"
      "  onLeave: function(retval) {"
      "    send('<a' + this.depth);"
      "  }"
      "});"
      "Interceptor.attach(" GUM_PTR_CONST ", {"
      "  onEnter: function(args) {"
      "    send('>b' + this.depth);"
      "  },"
      "  onLeave: function(retval) {"
      "    send('<b' + this.depth);"
      "  }"
      "});"
      "Interceptor.attach(" GUM_PTR_CONST ", {"
      "  onEnter: function(args) {"
      "    send('>c' + this.depth);"
      "  },"
      "  onLeave: function(retval) {"
      "    send('<c' + this.depth);"
      "  }"
      "});",
      target_function_nested_a,
      target_function_nested_b,
      target_function_nested_c);

  EXPECT_NO_MESSAGES ();
  target_function_nested_a (7);
  EXPECT_SEND_MESSAGE_WITH ("\">a0\"");
  EXPECT_SEND_MESSAGE_WITH ("\">b1\"");
  EXPECT_SEND_MESSAGE_WITH ("\">c2\"");
  EXPECT_SEND_MESSAGE_WITH ("\"<c2\"");
  EXPECT_SEND_MESSAGE_WITH ("\"<b1\"");
  EXPECT_SEND_MESSAGE_WITH ("\"<a0\"");
  EXPECT_NO_MESSAGES ();
}

SCRIPT_TESTCASE (callbacks_can_be_detached)
{
  COMPILE_AND_LOAD_SCRIPT (
      "Interceptor.attach(" GUM_PTR_CONST ", {"
      "  onEnter: function(args) {"
      "    send(args[0].toInt32());"
      "  }"
      "});"
      "Interceptor.detachAll();",
      target_function_int);

  EXPECT_NO_MESSAGES ();
  target_function_int (42);
  EXPECT_NO_MESSAGES ();
}

SCRIPT_TESTCASE (function_can_be_replaced)
{
  COMPILE_AND_LOAD_SCRIPT (
      "Interceptor.replace(" GUM_PTR_CONST ", new NativeCallback(function (arg) {"
      "  send(arg);"
      "  return 1337;"
      "}, 'int', ['int']));",
      target_function_int);

  EXPECT_NO_MESSAGES ();
  g_assert_cmpint (target_function_int (7), ==, 1337);
  EXPECT_SEND_MESSAGE_WITH ("7");
  EXPECT_NO_MESSAGES ();

  gum_script_unload (fixture->script);
  target_function_int (1);
  EXPECT_NO_MESSAGES ();
}

SCRIPT_TESTCASE (function_can_be_reverted)
{
  COMPILE_AND_LOAD_SCRIPT (
      "Interceptor.replace(" GUM_PTR_CONST ", new NativeCallback(function (arg) {"
      "  send(arg);"
      "  return 1337;"
      "}, 'int', ['int']));"
      "Interceptor.revert(" GUM_PTR_CONST ");",
      target_function_int, target_function_int);

  EXPECT_NO_MESSAGES ();
  target_function_int (7);
  EXPECT_NO_MESSAGES ();
}

SCRIPT_TESTCASE (interceptor_performance)
{
  COMPILE_AND_LOAD_SCRIPT (
      "Interceptor.attach(" GUM_PTR_CONST ", {"
      "  onEnter: function(args) {"
      "  },"
      "  onLeave: function(retval) {"
      "  }"
      "});", target_function_int);
  /* while (TRUE) */
    target_function_int (7);
}

SCRIPT_TESTCASE (memory_can_be_scanned)
{
  guint8 haystack[] = { 0x01, 0x02, 0x13, 0x37, 0x03, 0x13, 0x37 };
  COMPILE_AND_LOAD_SCRIPT (
      "Memory.scan(" GUM_PTR_CONST ", 7, '13 37', {"
        "onMatch: function(address, size) {"
        "  send('onMatch offset=' + address.sub(" GUM_PTR_CONST
             ").toInt32() + ' size=' + size);"
        "},"
        "onComplete: function() {"
        "  send('onComplete');"
        "}"
      "});", haystack, haystack);
  EXPECT_SEND_MESSAGE_WITH ("\"onMatch offset=2 size=2\"");
  EXPECT_SEND_MESSAGE_WITH ("\"onMatch offset=5 size=2\"");
  EXPECT_SEND_MESSAGE_WITH ("\"onComplete\"");
}

SCRIPT_TESTCASE (memory_scan_should_be_interruptible)
{
  guint8 haystack[] = { 0x01, 0x02, 0x13, 0x37, 0x03, 0x13, 0x37 };
  COMPILE_AND_LOAD_SCRIPT (
      "Memory.scan(" GUM_PTR_CONST ", 7, '13 37', {"
        "onMatch: function(address, size) {"
        "  send('onMatch offset=' + address.sub(" GUM_PTR_CONST
             ").toInt32() + ' size=' + size);"
        "  return 'stop';"
        "},"
        "onComplete: function() {"
        "  send('onComplete');"
        "}"
      "});", haystack, haystack);
  EXPECT_SEND_MESSAGE_WITH ("\"onMatch offset=2 size=2\"");
  EXPECT_SEND_MESSAGE_WITH ("\"onComplete\"");
}

SCRIPT_TESTCASE (memory_scan_handles_unreadable_memory)
{
  COMPILE_AND_LOAD_SCRIPT (
      "Memory.scan(ptr(\"1328\"), 7, '13 37', {"
        "onMatch: function(address, size) {"
        "  send('onMatch');"
        "},"
        "onError: function(message) {"
        "  send('onError: ' + message);"
        "},"
        "onComplete: function() {"
        "  send('onComplete');"
        "}"
      "});");
  EXPECT_SEND_MESSAGE_WITH ("\"onError: access violation reading 0x530\"");
  EXPECT_SEND_MESSAGE_WITH ("\"onComplete\"");
}

SCRIPT_TESTCASE (pointer_can_be_read)
{
  gpointer val = GSIZE_TO_POINTER (0x1337000);
  COMPILE_AND_LOAD_SCRIPT (
      "send(Memory.readPointer(" GUM_PTR_CONST ").toString());", &val);
  EXPECT_SEND_MESSAGE_WITH ("\"0x1337000\"");
}

SCRIPT_TESTCASE (pointer_can_be_written)
{
  gpointer val = NULL;
  COMPILE_AND_LOAD_SCRIPT (
      "Memory.writePointer(" GUM_PTR_CONST ", ptr(\"0x1337000\"));", &val);
  g_assert_cmphex (GPOINTER_TO_SIZE (val), ==, 0x1337000);
}

SCRIPT_TESTCASE (memory_can_be_allocated)
{
    COMPILE_AND_LOAD_SCRIPT (
      "var p = Memory.alloc(8);"
      "Memory.writePointer(p, ptr(\"1337\"));"
      "send(Memory.readPointer(p).toInt32() === 1337);");
  EXPECT_SEND_MESSAGE_WITH ("true");
}

SCRIPT_TESTCASE (s8_can_be_read)
{
  gint8 val = -42;
  COMPILE_AND_LOAD_SCRIPT ("send(Memory.readS8(" GUM_PTR_CONST "));", &val);
  EXPECT_SEND_MESSAGE_WITH ("-42");
}

SCRIPT_TESTCASE (u8_can_be_read)
{
  guint8 val = 42;
  COMPILE_AND_LOAD_SCRIPT ("send(Memory.readU8(" GUM_PTR_CONST "));", &val);
  EXPECT_SEND_MESSAGE_WITH ("42");
}

SCRIPT_TESTCASE (u8_can_be_written)
{
  guint8 val = 42;
  COMPILE_AND_LOAD_SCRIPT ("Memory.writeU8(" GUM_PTR_CONST ", 37);", &val);
  g_assert_cmpint (val, ==, 37);
}

SCRIPT_TESTCASE (s16_can_be_read)
{
  gint16 val = -12123;
  COMPILE_AND_LOAD_SCRIPT ("send(Memory.readS16(" GUM_PTR_CONST "));", &val);
  EXPECT_SEND_MESSAGE_WITH ("-12123");
}

SCRIPT_TESTCASE (u16_can_be_read)
{
  guint16 val = 12123;
  COMPILE_AND_LOAD_SCRIPT ("send(Memory.readU16(" GUM_PTR_CONST "));", &val);
  EXPECT_SEND_MESSAGE_WITH ("12123");
}

SCRIPT_TESTCASE (s32_can_be_read)
{
  gint32 val = -120123;
  COMPILE_AND_LOAD_SCRIPT ("send(Memory.readS32(" GUM_PTR_CONST "));", &val);
  EXPECT_SEND_MESSAGE_WITH ("-120123");
}

SCRIPT_TESTCASE (u32_can_be_read)
{
  guint32 val = 120123;
  COMPILE_AND_LOAD_SCRIPT ("send(Memory.readU32(" GUM_PTR_CONST "));", &val);
  EXPECT_SEND_MESSAGE_WITH ("120123");
}

SCRIPT_TESTCASE (s64_can_be_read)
{
  gint64 val = G_GINT64_CONSTANT (-1201239876783);
  COMPILE_AND_LOAD_SCRIPT ("send(Memory.readS64(" GUM_PTR_CONST "));", &val);
  EXPECT_SEND_MESSAGE_WITH ("-1201239876783");
}

SCRIPT_TESTCASE (u64_can_be_read)
{
  guint64 val = G_GUINT64_CONSTANT (1201239876783);
  COMPILE_AND_LOAD_SCRIPT ("send(Memory.readU64(" GUM_PTR_CONST "));", &val);
  EXPECT_SEND_MESSAGE_WITH ("1201239876783");
}

SCRIPT_TESTCASE (byte_array_can_be_read)
{
  guint8 val[3] = { 0x13, 0x37, 0x42 };
  COMPILE_AND_LOAD_SCRIPT ("send('stuff', Memory.readByteArray(" GUM_PTR_CONST
      ", 3));", val);
  EXPECT_SEND_MESSAGE_WITH_PAYLOAD_AND_DATA ("\"stuff\"", "13 37 42");
}

SCRIPT_TESTCASE (utf8_string_can_be_read)
{
  const gchar * str = "Bjøærheimsbygd";

  COMPILE_AND_LOAD_SCRIPT ("send(Memory.readUtf8String(" GUM_PTR_CONST "));",
      str);
  EXPECT_SEND_MESSAGE_WITH ("\"Bjøærheimsbygd\"");

  COMPILE_AND_LOAD_SCRIPT ("send(Memory.readUtf8String(" GUM_PTR_CONST
      ", 3));", str);
  EXPECT_SEND_MESSAGE_WITH ("\"Bjø\"");

  COMPILE_AND_LOAD_SCRIPT ("send(Memory.readUtf8String(" GUM_PTR_CONST
      ", 0));", str);
  EXPECT_SEND_MESSAGE_WITH ("\"\"");

  COMPILE_AND_LOAD_SCRIPT ("send(Memory.readUtf8String(" GUM_PTR_CONST
      ", -1));", str);
  EXPECT_SEND_MESSAGE_WITH ("\"Bjøærheimsbygd\"");

  COMPILE_AND_LOAD_SCRIPT ("send(Memory.readUtf8String(ptr(\"0\")));", str);
  EXPECT_SEND_MESSAGE_WITH ("null");
}

SCRIPT_TESTCASE (utf8_string_can_be_written)
{
  gchar str[6];

  strcpy (str, "Hello");
  COMPILE_AND_LOAD_SCRIPT ("Memory.writeUtf8String(" GUM_PTR_CONST ", 'Bye');",
      str);
  g_assert_cmpstr (str, ==, "Bye");
  g_assert_cmphex (str[4], ==, 'o');
  g_assert_cmphex (str[5], ==, '\0');
}

SCRIPT_TESTCASE (utf8_string_can_be_allocated)
{
  COMPILE_AND_LOAD_SCRIPT (
      "send(Memory.readUtf8String(Memory.allocUtf8String('Bjørheimsbygd')));");
  EXPECT_SEND_MESSAGE_WITH ("\"Bjørheimsbygd\"");
}

SCRIPT_TESTCASE (utf16_string_can_be_read)
{
  const gchar * str_utf8 = "Bjørheimsbygd";
  gunichar2 * str = g_utf8_to_utf16 (str_utf8, -1, NULL, NULL, NULL);

  COMPILE_AND_LOAD_SCRIPT ("send(Memory.readUtf16String(" GUM_PTR_CONST "));",
      str);
  EXPECT_SEND_MESSAGE_WITH ("\"Bjørheimsbygd\"");

  COMPILE_AND_LOAD_SCRIPT ("send(Memory.readUtf16String(" GUM_PTR_CONST
      ", 3));", str);
  EXPECT_SEND_MESSAGE_WITH ("\"Bjø\"");

  COMPILE_AND_LOAD_SCRIPT ("send(Memory.readUtf16String(" GUM_PTR_CONST
      ", 0));", str);
  EXPECT_SEND_MESSAGE_WITH ("\"\"");

  COMPILE_AND_LOAD_SCRIPT ("send(Memory.readUtf16String(" GUM_PTR_CONST
      ", -1));", str);
  EXPECT_SEND_MESSAGE_WITH ("\"Bjørheimsbygd\"");

  COMPILE_AND_LOAD_SCRIPT ("send(Memory.readUtf16String(ptr(\"0\")));", str);
  EXPECT_SEND_MESSAGE_WITH ("null");

  g_free (str);
}

SCRIPT_TESTCASE (utf16_string_can_be_allocated)
{
  COMPILE_AND_LOAD_SCRIPT ("send(Memory.readUtf16String("
      "Memory.allocUtf16String('Bjørheimsbygd')));");
  EXPECT_SEND_MESSAGE_WITH ("\"Bjørheimsbygd\"");
}

#ifdef G_OS_WIN32

SCRIPT_TESTCASE (ansi_string_can_be_read)
{
  const gchar * str_utf8 = "Bjørheimsbygd";
  gunichar2 * str_utf16 = g_utf8_to_utf16 (str_utf8, -1, NULL, NULL, NULL);
  gchar str[64];
  WideCharToMultiByte (CP_THREAD_ACP, 0, (LPCWSTR) str_utf16, -1,
      (LPSTR) str, sizeof (str), NULL, NULL);

  COMPILE_AND_LOAD_SCRIPT ("send(Memory.readAnsiString(" GUM_PTR_CONST "));",
      str);
  EXPECT_SEND_MESSAGE_WITH ("\"Bjørheimsbygd\"");

  COMPILE_AND_LOAD_SCRIPT ("send(Memory.readAnsiString(" GUM_PTR_CONST
      ", 3));", str);
  EXPECT_SEND_MESSAGE_WITH ("\"Bjø\"");

  COMPILE_AND_LOAD_SCRIPT ("send(Memory.readAnsiString(" GUM_PTR_CONST
      ", 0));", str);
  EXPECT_SEND_MESSAGE_WITH ("\"\"");

  COMPILE_AND_LOAD_SCRIPT ("send(Memory.readAnsiString(" GUM_PTR_CONST
      ", -1));", str);
  EXPECT_SEND_MESSAGE_WITH ("\"Bjørheimsbygd\"");

  COMPILE_AND_LOAD_SCRIPT ("send(Memory.readAnsiString(ptr(\"0\")));", str);
  EXPECT_SEND_MESSAGE_WITH ("null");

  g_free (str_utf16);
}

SCRIPT_TESTCASE (ansi_string_can_be_allocated)
{
  COMPILE_AND_LOAD_SCRIPT (
      "send(Memory.readAnsiString(Memory.allocAnsiString('Bjørheimsbygd')));");
  EXPECT_SEND_MESSAGE_WITH ("\"Bjørheimsbygd\"");
}

#endif

SCRIPT_TESTCASE (invalid_read_results_in_exception)
{
  const gchar * type_name[] = {
      "Pointer",
      "S8",
      "U8",
      "S16",
      "U16",
      "S32",
      "U32",
      /*
       * We don't know if the compiler will decide to access the lower or higher
       * part first, so we can't know the exact error message for these two.
       * Hence we limit this part of the test to 64 bit builds...
       */
#if GLIB_SIZEOF_VOID_P == 8
      "S64",
      "U64",
#endif
      "Utf8String",
      "Utf16String",
#ifdef G_OS_WIN32
      "AnsiString"
#endif
  };
  guint i;

  for (i = 0; i != G_N_ELEMENTS (type_name); i++)
  {
    gchar * source;

    source = g_strconcat ("Memory.read", type_name[i], "(ptr(\"1328\"));",
        NULL);
    COMPILE_AND_LOAD_SCRIPT (source);
    EXPECT_ERROR_MESSAGE_WITH (1, "Error: access violation reading 0x530");
    g_free (source);
  }
}

SCRIPT_TESTCASE (invalid_write_results_in_exception)
{
  const gchar * primitive_type_name[] = {
      "U8",
  };
  const gchar * string_type_name[] = {
      "Utf8String"
  };
  guint i;

  for (i = 0; i != G_N_ELEMENTS (primitive_type_name); i++)
  {
    gchar * source;

    source = g_strconcat ("Memory.write", primitive_type_name[i],
        "(ptr(\"1328\"), 13);", NULL);
    COMPILE_AND_LOAD_SCRIPT (source);
    EXPECT_ERROR_MESSAGE_WITH (1, "Error: access violation writing to 0x530");
    g_free (source);
  }

  for (i = 0; i != G_N_ELEMENTS (string_type_name); i++)
  {
    gchar * source;

    source = g_strconcat ("Memory.write", string_type_name[i],
        "(ptr(\"1328\"), 'Hey');", NULL);
    COMPILE_AND_LOAD_SCRIPT (source);
    EXPECT_ERROR_MESSAGE_WITH (1, "Error: access violation writing to 0x530");
    g_free (source);
  }
}

SCRIPT_TESTCASE (script_can_be_reloaded)
{
  COMPILE_AND_LOAD_SCRIPT (
      "send(typeof badger);"
      "badger = 42;");
  EXPECT_SEND_MESSAGE_WITH ("\"undefined\"");
  gum_script_load (fixture->script);
  EXPECT_NO_MESSAGES ();
  gum_script_unload (fixture->script);
  gum_script_unload (fixture->script);
  EXPECT_NO_MESSAGES ();
  gum_script_load (fixture->script);
  EXPECT_SEND_MESSAGE_WITH ("\"undefined\"");
}

GUM_NOINLINE static int
target_function_int (int arg)
{
  int result = 0;
  int i;

  for (i = 0; i != 10; i++)
    result += i * arg;

  gum_script_dummy_global_to_trick_optimizer += result;

  return result;
}

GUM_NOINLINE static const gchar *
target_function_string (const gchar * arg)
{
  int i;

  for (i = 0; i != 10; i++)
    gum_script_dummy_global_to_trick_optimizer += i * arg[0];

  return arg;
}

GUM_NOINLINE static int
target_function_nested_a (int arg)
{
  int result = 0;
  int i;

  for (i = 0; i != 7; i++)
    result += i * arg;

  gum_script_dummy_global_to_trick_optimizer += result;

  return target_function_nested_b (result);
}

GUM_NOINLINE static int
target_function_nested_b (int arg)
{
  int result = 0;
  int i;

  for (i = 0; i != 14; i++)
    result += i * arg;

  gum_script_dummy_global_to_trick_optimizer += result;

  return target_function_nested_c (result);
}

GUM_NOINLINE static int
target_function_nested_c (int arg)
{
  int result = 0;
  int i;

  for (i = 0; i != 21; i++)
    result += i * arg;

  gum_script_dummy_global_to_trick_optimizer += result;

  return result;
}
