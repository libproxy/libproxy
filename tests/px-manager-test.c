/* px-manager-test.c
 *
 * Copyright 2022-2023 The Libproxy Team
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "px-manager.h"
#include "px-manager-helper.h"

#include <gio/gio.h>

#define SERVER_PORT 1983

typedef struct {
  GMainLoop *loop;
  PxManager *manager;
} Fixture;

static void
send_error (GOutputStream *out,
            int            error_code,
            const char    *reason)
{
  char *res;

  res = g_strdup_printf ("HTTP/1.0 %d %s\r\n\r\n"
			 "<html><head><title>%d %s</title></head>"
			 "<body>%s</body></html>",
			 error_code, reason,
			 error_code, reason,
			 reason);
  g_output_stream_write_all (out, res, strlen (res), NULL, NULL, NULL);
  g_free (res);
}

static gboolean
on_incoming (GSocketService    *service,
             GSocketConnection *connection,
             GObject           *source_object)
{
  GOutputStream *out = NULL;
  GInputStream *in = NULL;
  g_autoptr (GDataInputStream) data = NULL;
  g_autoptr (GFile) f = NULL;
  g_autoptr (GError) error = NULL;
  g_autoptr (GFileInputStream) file_in = NULL;
  g_autoptr (GString) s = NULL;
  g_autoptr (GFileInfo) info = NULL;
  g_autofree char *line = NULL;
  g_autofree char *unescaped = NULL;
  g_autofree char *path = NULL;
  char *escaped;
  char *version;
  char *tmp;

  in = g_io_stream_get_input_stream (G_IO_STREAM (connection));
  out = g_io_stream_get_output_stream (G_IO_STREAM (connection));

  data = g_data_input_stream_new (in);
  /* Be tolerant of input */
  g_data_input_stream_set_newline_type (data, G_DATA_STREAM_NEWLINE_TYPE_ANY);

  line = g_data_input_stream_read_line (data, NULL, NULL, NULL);

  if (line == NULL) {
    send_error (out, 400, "Invalid request");
    goto out;
  }

  if (!g_str_has_prefix (line, "GET ")) {
    send_error (out, 501, "Only GET implemented");
    goto out;
  }

  escaped = line + 4; /* Skip "GET " */

  version = NULL;
  tmp = strchr (escaped, ' ');
  if (tmp == NULL) {
    send_error (out, 400, "Bad Request");
    goto out;
  }
  *tmp = 0;

  version = tmp + 1;
  if (!g_str_has_prefix (version, "HTTP/1.")) {
    send_error(out, 505, "HTTP Version Not Supported");
    goto out;
  }

  unescaped = g_uri_unescape_string (escaped, NULL);
  path = g_test_build_filename (G_TEST_DIST, "data", unescaped, NULL);
  f = g_file_new_for_path (path);

  error = NULL;
  file_in = g_file_read (f, NULL, &error);
  if (file_in == NULL) {
    send_error (out, 404, error->message);
    goto out;
  }

  s = g_string_new ("HTTP/1.0 200 OK\r\n");

  info = g_file_input_stream_query_info (file_in,
					 G_FILE_ATTRIBUTE_STANDARD_SIZE ","
					 G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE,
					 NULL, NULL);
  if (info) {
    const char *content_type;
    char *mime_type;

    if (g_file_info_has_attribute (info, G_FILE_ATTRIBUTE_STANDARD_SIZE))
      g_string_append_printf (s, "Content-Length: %"G_GINT64_FORMAT"\r\n", g_file_info_get_size (info));

    if (g_file_info_has_attribute (info, G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE)) {
      content_type = g_file_info_get_content_type (info);
      if (content_type) {
        mime_type = g_content_type_get_mime_type (content_type);
        if (mime_type) {
          g_string_append_printf (s, "Content-Type: %s\r\n", mime_type);
          g_free (mime_type);
        }
      }
    }
  }
  g_string_append (s, "\r\n");

  if (g_output_stream_write_all (out, s->str, s->len, NULL, NULL, NULL)) {
    g_output_stream_splice (out, G_INPUT_STREAM (file_in), 0, NULL, NULL);
  }

out:
  return TRUE;
}

static void
fixture_setup (Fixture       *fixture,
               gconstpointer  data)
{
  g_autofree char *path = NULL;

  fixture->loop = g_main_loop_new (NULL, FALSE);

  if (data)
    path = g_test_build_filename (G_TEST_DIST, "data", data, NULL);

  fixture->manager = px_test_manager_new ("config-sysconfig", path);
}

static void
fixture_teardown (Fixture       *fixture,
                  gconstpointer  data)
{
  g_clear_object (&fixture->manager);
}

static gpointer
download_pac (gpointer data)
{
  Fixture *self = data;
  GBytes *pac;

  pac = px_manager_pac_download (self->manager, "http://127.0.0.1:1983/px-manager-sample.pac");
  g_assert_nonnull (pac);

  g_main_loop_quit (self->loop);

  return NULL;
}

static void
test_pac_download (Fixture    *self,
                   const void *user_data)
{
  g_autoptr (GThread) thread = NULL;

  thread = g_thread_new ("test", (GThreadFunc)download_pac, self);
  g_main_loop_run (self->loop);
}

static void
test_get_proxies_direct (Fixture    *self,
                         const void *user_data)
{
  g_auto (GStrv) config = NULL;

  config = px_manager_get_proxies_sync (self->manager, "");
  g_assert_nonnull (config);
  g_assert_cmpstr (config[0], ==, "direct://");

  config = px_manager_get_proxies_sync (self->manager, "nonsense");
  g_assert_nonnull (config);
  g_assert_cmpstr (config[0], ==, "direct://");

  config = px_manager_get_proxies_sync (self->manager, "https://www.example.com");
  g_assert_nonnull (config);
  g_assert_cmpstr (config[0], ==, "direct://");
}

static void
test_get_proxies_nonpac (Fixture    *self,
                         const void *user_data)
{
  g_auto (GStrv) config = NULL;

  config = px_manager_get_proxies_sync (self->manager, "https://www.example.com");
  g_assert_nonnull (config);
  g_assert_cmpstr (config[0], ==, "http://127.0.0.1:1983");
}

static gpointer
get_proxies_pac (gpointer data)
{
  Fixture *self = data;
  g_auto (GStrv) config = NULL;

  g_setenv("PX_DEBUG_PACALERT", "1", TRUE);

  config = px_manager_get_proxies_sync (self->manager, "https://www.example.com");
  g_assert_nonnull (config);
  g_assert_cmpstr (config[0], ==, "http://127.0.0.1:1984");

  config = px_manager_get_proxies_sync (self->manager, "https://192.168.10.4");
  g_assert_nonnull (config);
  g_assert_cmpstr (config[0], ==, "socks://127.0.0.1:1983");

  config = px_manager_get_proxies_sync (self->manager, "https://192.168.10.5");
  g_assert_nonnull (config);
  g_assert_cmpstr (config[0], ==, "socks4://127.0.0.1:1983");

  config = px_manager_get_proxies_sync (self->manager, "https://192.168.10.6");
  g_assert_nonnull (config);
  g_assert_cmpstr (config[0], ==, "socks4a://127.0.0.1:1983");

  config = px_manager_get_proxies_sync (self->manager, "https://192.168.10.7");
  g_assert_nonnull (config);
  g_assert_cmpstr (config[0], ==, "socks5://127.0.0.1:1983");

  /* Fallback */
  config = px_manager_get_proxies_sync (self->manager, "https://192.168.11.8");
  g_assert_nonnull (config);
  g_assert_cmpstr (config[0], ==, "direct://");

  /* Invalid return URI */
  config = px_manager_get_proxies_sync (self->manager, "https://192.168.10.8");
  g_assert_nonnull (config);
  g_assert_cmpstr (config[0], ==, "direct://");

  /* Invalid return URI */
  config = px_manager_get_proxies_sync (self->manager, "https://192.168.10.9");
  g_assert_nonnull (config);
  g_assert_cmpstr (config[0], ==, "direct://");

  /* Invalid input url */
  config = px_manager_get_proxies_sync (self->manager, "invalid");
  g_assert_nonnull (config);
  g_assert_cmpstr (config[0], ==, "direct://");

  g_main_loop_quit (self->loop);
  g_unsetenv("PX_DEBUG_PACALERT");

  return NULL;
}

static void
test_get_proxies_pac (Fixture    *self,
                      const void *user_data)
{
  g_autoptr (GThread) thread = NULL;

  thread = g_thread_new ("test", (GThreadFunc)get_proxies_pac, self);
  g_main_loop_run (self->loop);
}

static void
test_get_proxies_pac_debug (Fixture    *self,
                            const void *user_data)
{
  g_autoptr (GThread) thread = NULL;

  g_setenv("PX_DEBUG", "1", TRUE);
  thread = g_thread_new ("test", (GThreadFunc)get_proxies_pac, self);
  g_main_loop_run (self->loop);
  g_unsetenv ("PX_DEBUG");
}

static gpointer
get_wpad (gpointer data)
{
  Fixture *self = data;
  g_auto (GStrv) config = NULL;

  config = px_manager_get_proxies_sync (self->manager, "https://www.example.com");
  g_assert_nonnull (config);
  g_assert_cmpstr (config[0], ==, "direct://");

  g_main_loop_quit (self->loop);

  return NULL;
}

static void
test_get_wpad (Fixture    *self,
               const void *user_data)
{
  g_autoptr (GThread) thread = NULL;

  thread = g_thread_new ("test", (GThreadFunc)get_wpad, self);
  g_main_loop_run (self->loop);
}

static void
test_ignore_domain (Fixture    *self,
                    const void *user_data)
{
  g_autoptr (GUri) uri = g_uri_parse("http://10.10.1.12", G_URI_FLAGS_NONE, NULL);
  char **ignore_list = g_malloc0 (sizeof (char *) * 2);
  gboolean ret;

  /* Invalid test */
  ignore_list[0] = g_strdup ("10.10.1.13");
  ignore_list[1] = NULL;

  ret = px_manager_is_ignore (uri, ignore_list);
  g_assert_false (ret);

  /* Valid test */
  ignore_list[0] = g_strdup ("10.10.1.12");
  ignore_list[1] = NULL;

  ret = px_manager_is_ignore (uri, ignore_list);
  g_assert_true (ret);

  g_free (ignore_list[0]);
  g_free (ignore_list);
}

static void
test_ignore_domain_port (Fixture    *self,
                         const void *user_data)
{
  g_autoptr (GUri) uri = g_uri_parse("http://10.10.1.12:22", G_URI_FLAGS_NONE, NULL);
  char **ignore_list = g_malloc0 (sizeof (char *) * 2);
  gboolean ret;

  /* Invalid test */
  ignore_list[0] = g_strdup ("10.10.1.13");
  ignore_list[1] = NULL;

  ret = px_manager_is_ignore (uri, ignore_list);
  g_free (ignore_list[0]);
  g_assert_false (ret);

  /* Invalid test */
  ignore_list[0] = g_strdup ("10.10.1.12:24");
  ignore_list[1] = NULL;

  ret = px_manager_is_ignore (uri, ignore_list);
  g_free (ignore_list[0]);
  g_assert_false (ret);

  /* Valid test */
  ignore_list[0] = g_strdup ("10.10.1.12:22");
  ignore_list[1] = NULL;

  ret = px_manager_is_ignore (uri, ignore_list);
  g_assert_true (ret);

  g_free (ignore_list[0]);
  g_free (ignore_list);
}

static void
test_ignore_hostname (Fixture    *self,
                      const void *user_data)
{
  g_autoptr (GUri) uri = g_uri_parse("http://18.10.1.12", G_URI_FLAGS_NONE, NULL);
  char **ignore_list = g_malloc0 (sizeof (char *) * 2);
  gboolean ret;

  /* Invalid test */
  ignore_list[0] = g_strdup ("<local>");
  ignore_list[1] = NULL;

  ret = px_manager_is_ignore (uri, ignore_list);
  g_assert_false (ret);
  g_uri_unref (uri);

  /* Valid test */
  uri = g_uri_parse("http://127.0.0.1", G_URI_FLAGS_NONE, NULL);
  ret = px_manager_is_ignore (uri, ignore_list);
  g_assert_false (ret);

  g_free (ignore_list[0]);
  g_free (ignore_list);
}

int
main (int    argc,
      char **argv)
{
  g_autoptr (GSocketService) service = NULL;
  g_autoptr (GError) error = NULL;

  g_test_init (&argc, &argv, NULL);

  service = g_socket_service_new ();
  if (!g_socket_listener_add_inet_port (G_SOCKET_LISTENER (service), SERVER_PORT, NULL, &error)) {
    g_error ("Could not create server socket: %s", error ? error->message : "?");
    return -1;
  }

  g_signal_connect (service, "incoming", G_CALLBACK (on_incoming), NULL);

  g_test_add ("/pac/download", Fixture, "px-manager-direct", fixture_setup, test_pac_download, fixture_teardown);
  g_test_add ("/pac/get_proxies_direct", Fixture, "px-manager-direct", fixture_setup, test_get_proxies_direct, fixture_teardown);
  g_test_add ("/pac/get_proxies_nonpac", Fixture, "px-manager-nonpac", fixture_setup, test_get_proxies_nonpac, fixture_teardown);
  g_test_add ("/pac/get_proxies_pac", Fixture, "px-manager-pac", fixture_setup, test_get_proxies_pac, fixture_teardown);
  g_test_add ("/pac/wpad", Fixture, "px-manager-wpad", fixture_setup, test_get_wpad, fixture_teardown);
  g_test_add ("/pac/get_proxies_pac_debug", Fixture, "px-manager-pac", fixture_setup, test_get_proxies_pac_debug, fixture_teardown);

  g_test_add ("/ignore/domain", Fixture, "px-manager-ignore", fixture_setup, test_ignore_domain, fixture_teardown);
  g_test_add ("/ignore/domain_port", Fixture, "px-manager-ignore", fixture_setup, test_ignore_domain_port, fixture_teardown);
  g_test_add ("/ignore/hostname", Fixture, "px-manager-ignore", fixture_setup, test_ignore_hostname, fixture_teardown);

  return g_test_run ();
}

