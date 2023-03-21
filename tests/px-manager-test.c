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

#include <libsoup/soup.h>

#define SERVER_PORT 1983

typedef struct {
  GMainLoop *loop;
  PxManager *manager;
} Fixture;

static void
server_callback (SoupServer        *server,
                 SoupServerMessage *msg,
                 const char        *path,
                 GHashTable        *query,
                 gpointer           data)
{
  g_print ("%s: path %s\n", __FUNCTION__, path);
  soup_server_message_set_status (SOUP_SERVER_MESSAGE (msg), SOUP_STATUS_OK, NULL);

  if (g_strcmp0 (path, "/test.pac") == 0) {
    g_autofree char *pac = g_test_build_filename (G_TEST_DIST, "data", "px-manager-sample.pac", NULL);
    g_autofree char *pac_data = NULL;
    g_autoptr (GError) error = NULL;
    gsize len;

    if (!g_file_get_contents (pac, &pac_data, &len, &error)) {
      g_warning ("Could not read pac file: %s", error ? error->message : "");
      return;
    }
    soup_server_message_set_response (msg, "text/plain", SOUP_MEMORY_COPY, pac_data, len);
  }
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

  pac = px_manager_pac_download (self->manager, "http://127.0.0.1:1983/test.pac");
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

  config = px_manager_get_proxies_sync (self->manager, "", NULL);
  g_assert_nonnull (config);
  g_assert_cmpstr (config[0], ==, "direct://");

  config = px_manager_get_proxies_sync (self->manager, "nonsense", NULL);
  g_assert_nonnull (config);
  g_assert_cmpstr (config[0], ==, "direct://");

  config = px_manager_get_proxies_sync (self->manager, "https://www.example.com", NULL);
  g_assert_nonnull (config);
  g_assert_cmpstr (config[0], ==, "direct://");
}

static void
test_get_proxies_nonpac (Fixture    *self,
                         const void *user_data)
{
  g_auto (GStrv) config = NULL;

  config = px_manager_get_proxies_sync (self->manager, "https://www.example.com", NULL);
  g_assert_nonnull (config);
  g_assert_cmpstr (config[0], ==, "http://127.0.0.1:1983");
}

static gpointer
get_proxies_pac (gpointer data)
{
  Fixture *self = data;
  g_auto (GStrv) config = NULL;

  config = px_manager_get_proxies_sync (self->manager, "https://www.example.com", NULL);
  g_assert_nonnull (config);
  g_assert_cmpstr (config[0], ==, "http://127.0.0.1:1983");

  config = px_manager_get_proxies_sync (self->manager, "https://192.168.10.4", NULL);
  g_assert_nonnull (config);
  g_assert_cmpstr (config[0], ==, "socks://127.0.0.1:1983");

  config = px_manager_get_proxies_sync (self->manager, "https://192.168.10.5", NULL);
  g_assert_nonnull (config);
  g_assert_cmpstr (config[0], ==, "socks4://127.0.0.1:1983");

  config = px_manager_get_proxies_sync (self->manager, "https://192.168.10.6", NULL);
  g_assert_nonnull (config);
  g_assert_cmpstr (config[0], ==, "socks4a://127.0.0.1:1983");

  config = px_manager_get_proxies_sync (self->manager, "https://192.168.10.7", NULL);
  g_assert_nonnull (config);
  g_assert_cmpstr (config[0], ==, "socks5://127.0.0.1:1983");

  g_main_loop_quit (self->loop);

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

static gpointer
get_wpad (gpointer data)
{
  Fixture *self = data;
  g_auto (GStrv) config = NULL;

  config = px_manager_get_proxies_sync (self->manager, "https://www.example.com", NULL);
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
  g_autoptr (GUri) uri = g_uri_parse("http://10.10.1.12", G_URI_FLAGS_PARSE_RELAXED, NULL);
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
  g_autoptr (GUri) uri = g_uri_parse("http://10.10.1.12:22", G_URI_FLAGS_PARSE_RELAXED, NULL);
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
  g_autoptr (GUri) uri = g_uri_parse("http://18.10.1.12", G_URI_FLAGS_PARSE_RELAXED, NULL);
  char **ignore_list = g_malloc0 (sizeof (char *) * 2);
  gboolean ret;

  /* Invalid test */
  ignore_list[0] = g_strdup ("<local>");
  ignore_list[1] = NULL;

  ret = px_manager_is_ignore (uri, ignore_list);
  g_assert_false (ret);
  g_uri_unref (uri);

  /* Valid test */
  uri = g_uri_parse("http://127.0.0.1", G_URI_FLAGS_PARSE_RELAXED, NULL);
  ret = px_manager_is_ignore (uri, ignore_list);
  g_assert_false (ret);

  g_free (ignore_list[0]);
  g_free (ignore_list);
}

int
main (int    argc,
      char **argv)
{
  SoupServer *server = NULL;
  g_autoptr (GError) error = NULL;

  g_test_init (&argc, &argv, NULL);

  server = soup_server_new (NULL, NULL);
  if (!soup_server_listen_local (server, SERVER_PORT, SOUP_SERVER_LISTEN_IPV4_ONLY, &error)) {
    g_warning ("Could not create local server: %s", error ? error->message : "");
    return -1;
  }

  soup_server_add_handler (server, NULL, server_callback, NULL, NULL);

  g_test_add ("/pac/download", Fixture, "px-manager-direct", fixture_setup, test_pac_download, fixture_teardown);
  g_test_add ("/pac/get_proxies_direct", Fixture, "px-manager-direct", fixture_setup, test_get_proxies_direct, fixture_teardown);
  g_test_add ("/pac/get_proxies_nonpac", Fixture, "px-manager-nonpac", fixture_setup, test_get_proxies_nonpac, fixture_teardown);
  g_test_add ("/pac/get_proxies_pac", Fixture, "px-manager-pac", fixture_setup, test_get_proxies_pac, fixture_teardown);
  g_test_add ("/pac/wpad", Fixture, "px-manager-wpad", fixture_setup, test_get_wpad, fixture_teardown);

  g_test_add ("/ignore/domain", Fixture, "px-manager-ignore", fixture_setup, test_ignore_domain, fixture_teardown);
  g_test_add ("/ignore/domain_port", Fixture, "px-manager-ignore", fixture_setup, test_ignore_domain_port, fixture_teardown);
  g_test_add ("/ignore/hostname", Fixture, "px-manager-ignore", fixture_setup, test_ignore_hostname, fixture_teardown);

  return g_test_run ();
}

