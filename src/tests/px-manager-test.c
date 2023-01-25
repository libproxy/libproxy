/*******************************************************************************
 * libproxy - A library for proxy configuration
 * Copyright (C) 2022-2023 Jan-Michael Brummer <jan.brummer@tabos.org>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 ******************************************************************************/

#include "px-manager.h"
#include "px-manager-helper.h"

#include <glib.h>

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
  fixture->loop = g_main_loop_new (NULL, FALSE);

  if (data) {
    g_autofree char *path = g_test_build_filename (G_TEST_DIST, "data", data, NULL);
    if (!g_setenv ("PX_CONFIG_SYSCONFIG", path, TRUE)) {
      g_warning ("Failed to set environment");
      return;
    }
  }

  fixture->manager = px_test_manager_new ("config-sysconfig");
}

static void
fixture_teardown (Fixture       *fixture,
                  gconstpointer  data)
{
  g_unsetenv ("PX_CONFIG_SYSCONFIG");
  g_clear_object (&fixture->manager);
}

static gpointer
download_pac (gpointer data)
{
  Fixture *self = data;
  GBytes *pac;

  pac = px_manager_pac_download (self->manager, "http://127.0.0.1:1983");
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
  g_assert_cmpstr (config[0], ==, "PROXY 127.0.0.1:1983");

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

  return g_test_run ();
}
