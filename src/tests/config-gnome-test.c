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
#include <gio/gio.h>

typedef struct {
  GSettings *proxy_settings;
  GSettings *http_proxy_settings;
  GSettings *https_proxy_settings;
  GSettings *ftp_proxy_settings;
} Fixture;

enum {
  GNOME_PROXY_MODE_NONE = 0,
  GNOME_PROXY_MODE_MANUAL,
  GNOME_PROXY_MODE_AUTO
};

typedef struct {
  int mode;
  const char *proxy;
  int proxy_port;
  const char *url;
  const char *expected_return;
  gboolean success;
} ConfigGnomeTest;

static const ConfigGnomeTest config_gnome_test_set[] = {
  { GNOME_PROXY_MODE_MANUAL, "127.0.0.1", 8080, "https://www.example.com", "http://127.0.0.1:8080", TRUE},
  { GNOME_PROXY_MODE_MANUAL, "127.0.0.1", 8080, "http://www.example.com", "http://127.0.0.1:8080", TRUE},
  { GNOME_PROXY_MODE_MANUAL, "127.0.0.1", 8080, "ftp://www.example.com", "http://127.0.0.1:8080", TRUE},
  { GNOME_PROXY_MODE_MANUAL, "127.0.0.1", 8080, "http://localhost:1234", "http://127.0.0.1:8080", TRUE},
};

static void
fixture_setup (Fixture       *self,
               gconstpointer  data)
{
  self->proxy_settings = g_settings_new ("org.gnome.system.proxy");
  self->http_proxy_settings = g_settings_new ("org.gnome.system.proxy.http");
  self->https_proxy_settings = g_settings_new ("org.gnome.system.proxy.https");
  self->ftp_proxy_settings = g_settings_new ("org.gnome.system.proxy.ftp");
}

static void
fixture_teardown (Fixture       *fixture,
                  gconstpointer  data)
{
}

static void
test_config_gnome (Fixture    *self,
                   const void *user_data)
{
  int idx;

  for (idx = 0; idx < G_N_ELEMENTS (config_gnome_test_set); idx++) {
    g_autoptr (PxManager) manager = NULL;
    g_autoptr (GError) error = NULL;
    g_autoptr (GUri) uri = NULL;
    g_auto (GStrv) config = NULL;
    ConfigGnomeTest test = config_gnome_test_set[idx];

    g_settings_set_enum (self->proxy_settings, "mode", test.mode);
    g_settings_set_string (self->http_proxy_settings, "host", test.proxy);
    g_settings_set_int (self->http_proxy_settings, "port", test.proxy_port);
    g_settings_set_string (self->https_proxy_settings, "host", test.proxy);
    g_settings_set_int (self->https_proxy_settings, "port", test.proxy_port);
    g_settings_set_string (self->ftp_proxy_settings, "host", test.proxy);
    g_settings_set_int (self->ftp_proxy_settings, "port", test.proxy_port);

    manager = px_test_manager_new ("config-gnome");
    g_clear_error (&error);

    uri = g_uri_parse (test.url, G_URI_FLAGS_PARSE_RELAXED, &error);
    if (!uri) {
      g_warning ("Could not parse url '%s': %s", test.url, error ? error->message : "");
      g_assert_not_reached ();
    }

    config = px_manager_get_configuration (manager, uri, &error);
    if (test.success)
      g_assert_cmpstr (config[0], ==, test.expected_return);
    else
      g_assert_cmpstr (config[0], !=, test.expected_return);

    g_clear_object (&manager);
  }
}

int
main (int    argc,
      char **argv)
{
  g_test_init (&argc, &argv, NULL);

  g_test_add ("/config/gnome", Fixture, NULL, fixture_setup, test_config_gnome, fixture_teardown);

  return g_test_run ();
}
