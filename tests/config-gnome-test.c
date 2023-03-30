/* config-gnome-test.c
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

typedef struct {
  GSettings *proxy_settings;
  GSettings *http_proxy_settings;
  GSettings *https_proxy_settings;
  GSettings *ftp_proxy_settings;
  GSettings *socks_proxy_settings;
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
  { GNOME_PROXY_MODE_MANUAL, "127.0.0.1", 8080, "socks://localhost:1234", "socks://127.0.0.1:8080", TRUE},
};

static void
fixture_setup (Fixture       *self,
               gconstpointer  data)
{
  self->proxy_settings = g_settings_new ("org.gnome.system.proxy");
  self->http_proxy_settings = g_settings_new ("org.gnome.system.proxy.http");
  self->https_proxy_settings = g_settings_new ("org.gnome.system.proxy.https");
  self->ftp_proxy_settings = g_settings_new ("org.gnome.system.proxy.ftp");
  self->socks_proxy_settings = g_settings_new ("org.gnome.system.proxy.socks");
}

static void
fixture_teardown (Fixture       *fixture,
                  gconstpointer  data)
{
}

static void
test_config_gnome_manual (Fixture    *self,
                          const void *user_data)
{
  int idx;

  for (idx = 0; idx < G_N_ELEMENTS (config_gnome_test_set); idx++) {
    g_autoptr (PxManager) manager = NULL;
    g_autoptr (GError) error = NULL;
    g_autoptr (GUri) uri = NULL;
    g_auto (GStrv) config = NULL;
    ConfigGnomeTest test = config_gnome_test_set[idx];

    g_settings_set_strv (self->proxy_settings, "ignore-hosts", NULL);
    g_settings_set_enum (self->proxy_settings, "mode", test.mode);
    g_settings_set_string (self->http_proxy_settings, "host", test.proxy);
    g_settings_set_int (self->http_proxy_settings, "port", test.proxy_port);
    g_settings_set_string (self->https_proxy_settings, "host", test.proxy);
    g_settings_set_int (self->https_proxy_settings, "port", test.proxy_port);
    g_settings_set_string (self->ftp_proxy_settings, "host", test.proxy);
    g_settings_set_int (self->ftp_proxy_settings, "port", test.proxy_port);
    g_settings_set_string (self->socks_proxy_settings, "host", test.proxy);
    g_settings_set_int (self->socks_proxy_settings, "port", test.proxy_port);

    manager = px_test_manager_new ("config-gnome", NULL);
    g_clear_error (&error);

    uri = g_uri_parse (test.url, G_URI_FLAGS_PARSE_RELAXED, &error);
    if (!uri) {
      g_warning ("Could not parse url '%s': %s", test.url, error ? error->message : "");
      g_assert_not_reached ();
    }

    config = px_manager_get_configuration (manager, uri, &error);
    g_assert_cmpstr (config[0], ==, test.expected_return);

    g_clear_object (&manager);
  }
}

static void
test_config_gnome_manual_auth (Fixture    *self,
                               const void *user_data)
{
  g_autoptr (PxManager) manager = NULL;
  g_autoptr (GError) error = NULL;
  g_autoptr (GUri) uri = NULL;
  g_auto (GStrv) config = NULL;

  g_settings_set_enum (self->proxy_settings, "mode", GNOME_PROXY_MODE_MANUAL);
  g_settings_set_string (self->http_proxy_settings, "host", "127.0.0.1");
  g_settings_set_int (self->http_proxy_settings, "port", 9876);
  g_settings_set_boolean (self->http_proxy_settings, "use-authentication", TRUE);
  g_settings_set_string (self->http_proxy_settings, "authentication-user", "test");
  g_settings_set_string (self->http_proxy_settings, "authentication-password", "pwd");

  manager = px_test_manager_new ("config-gnome", NULL);
  g_clear_error (&error);

  uri = g_uri_parse ("http://www.example.com", G_URI_FLAGS_PARSE_RELAXED, &error);

  config = px_manager_get_configuration (manager, uri, &error);
  g_assert_cmpstr (config[0], ==, "http://test:pwd@127.0.0.1:9876");
}

static void
test_config_gnome_auto (Fixture    *self,
                        const void *user_data)
{
  g_autoptr (PxManager) manager = NULL;
  g_autoptr (GError) error = NULL;
  g_auto (GStrv) config = NULL;
  g_autoptr (GUri) uri = NULL;

  manager = px_test_manager_new ("config-gnome", NULL);
  g_settings_set_enum (self->proxy_settings, "mode", GNOME_PROXY_MODE_AUTO);
  g_settings_set_string (self->proxy_settings, "autoconfig-url", "");

  uri = g_uri_parse ("https://www.example.com", G_URI_FLAGS_PARSE_RELAXED, &error);
  config = px_manager_get_configuration (manager, uri, &error);
  g_assert_cmpstr (config[0], ==, "wpad://");

  g_settings_set_string (self->proxy_settings, "autoconfig-url", "http://127.0.0.1:3435");
  config = px_manager_get_configuration (manager, uri, &error);
  g_assert_cmpstr (config[0], ==, "pac+http://127.0.0.1:3435");
}

static void
test_config_gnome_fail (Fixture    *self,
                        const void *user_data)
{
  g_autoptr (PxManager) manager = NULL;
  g_autoptr (GError) error = NULL;
  g_auto (GStrv) config = NULL;
  g_autoptr (GUri) uri = NULL;

  /* Disable GNOME support */
  if (!g_setenv ("XDG_CURRENT_DESKTOP", "unknown", TRUE)) {
    g_warning ("Could not set XDG_CURRENT_DESKTOP environment, abort");
    return;
  }

  manager = px_test_manager_new ("config-gnome", NULL);
  g_settings_set_enum (self->proxy_settings, "mode", GNOME_PROXY_MODE_AUTO);
  g_settings_set_string (self->proxy_settings, "autoconfig-url", "");

  uri = g_uri_parse ("https://www.example.com", G_URI_FLAGS_PARSE_RELAXED, &error);
  config = px_manager_get_configuration (manager, uri, &error);
  g_assert_null (config[0]);
}

int
main (int    argc,
      char **argv)
{
  g_test_init (&argc, &argv, NULL);

  g_setenv ("XDG_CURRENT_DESKTOP", "GNOME", TRUE);

  g_test_add ("/config/gnome/manual", Fixture, NULL, fixture_setup, test_config_gnome_manual, fixture_teardown);
  g_test_add ("/config/gnome/manual_auth", Fixture, NULL, fixture_setup, test_config_gnome_manual_auth, fixture_teardown);
  g_test_add ("/config/gnome/auto", Fixture, NULL, fixture_setup, test_config_gnome_auto, fixture_teardown);
  g_test_add ("/config/gnome/fail", Fixture, NULL, fixture_setup, test_config_gnome_fail, fixture_teardown);

  return g_test_run ();
}
