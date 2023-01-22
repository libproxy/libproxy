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
server_callback (SoupServer *server,
                 SoupServerMessage *msg,
                 const char        *path,
                 GHashTable        *query,
                 gpointer           data)
{
  soup_server_message_set_status (SOUP_SERVER_MESSAGE (msg), SOUP_STATUS_OK, NULL);
}

static void
fixture_setup (Fixture       *fixture,
               gconstpointer  data)
{
  fixture->loop = g_main_loop_new (NULL, FALSE);
  fixture->manager = px_test_manager_new (NULL);
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

  pac = px_manager_pac_download (self->manager, "http://127.0.0.1:1983");
  g_assert_nonnull (pac);

  g_main_loop_quit (self->loop);

  return NULL;
}

static void
test_pac_download (Fixture    *self,
                   const void *user_data)
{
  g_thread_new ("test", (GThreadFunc)download_pac, self);
  /* pac = px_manager_pac_download (self->manager, "http://127.0.0.1:1983"); */
  /* g_assert_nonnull (pac); */
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

  g_test_add ("/pac/download", Fixture, NULL, fixture_setup, test_pac_download, fixture_teardown);

  return g_test_run ();
}
