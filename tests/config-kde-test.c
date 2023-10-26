/* config-kde-test.c
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


typedef struct {
  const char *url;
  const char *proxy;
  gboolean success;
} ConfigKdeTest;

static const ConfigKdeTest config_kde_manual_test_set[] = {
  { "https://www.example.com", "http://127.0.0.1:8080", TRUE},
  { "http://www.example.com", "http://127.0.0.1:8080", TRUE},
  { "ftp://www.example.com", "ftp://127.0.0.1:8080", TRUE},
  { "socks://www.example.com", "socks://127.0.0.1:8080", TRUE},
  { "http://localhost:1234", "http://127.0.0.1:8080", FALSE},
  { "socks://localhost:1234", "http://127.0.0.1:8080", FALSE},
  { "socks://localhost:1234", "socks://127.0.0.1:8080", FALSE},
};

static const ConfigKdeTest config_kde_wpad_test_set[] = {
  { "https://www.example.com", "http://127.0.0.1:8080", TRUE},
  { "http://www.example.com", "http://127.0.0.1:8080", TRUE},
  { "ftp://www.example.com", "ftp://127.0.0.1:8080", TRUE},
  { "http://localhost:1234", "http://127.0.0.1:8080", FALSE},
};

static const ConfigKdeTest config_kde_pac_test_set[] = {
  { "https://www.example.com", "http://127.0.0.1:8080", TRUE},
};

static void
test_config_kde_disabled (void)
{
  int idx;

  for (idx = 0; idx < G_N_ELEMENTS (config_kde_manual_test_set); idx++) {
    g_autoptr (PxManager) manager = NULL;
    g_autoptr (GError) error = NULL;
    g_autoptr (GUri) uri = NULL;
    g_auto (GStrv) config = NULL;
    ConfigKdeTest test = config_kde_manual_test_set[idx];
    g_autofree char *path = g_test_build_filename (G_TEST_DIST, "data", "sample-kde-proxy-disabled", NULL);

    manager = px_test_manager_new ("config-kde", path);
    g_clear_error (&error);

    uri = g_uri_parse (test.url, G_URI_FLAGS_NONE, &error);
    if (!uri) {
      g_warning ("Could not parse url '%s': %s", test.url, error ? error->message : "");
      g_assert_not_reached ();
    }

    config = px_manager_get_configuration (manager, uri);
    g_assert_cmpstr (config[0], !=, test.proxy);

    g_clear_object (&manager);
  }
}

static void
test_config_kde_manual (void)
{
  int idx;

  for (idx = 0; idx < G_N_ELEMENTS (config_kde_manual_test_set); idx++) {
    g_autoptr (PxManager) manager = NULL;
    g_autoptr (GError) error = NULL;
    g_autoptr (GUri) uri = NULL;
    g_auto (GStrv) config = NULL;
    ConfigKdeTest test = config_kde_manual_test_set[idx];
    g_autofree char *path = g_test_build_filename (G_TEST_DIST, "data", "sample-kde-proxy-manual", NULL);

    manager = px_test_manager_new ("config-kde", path);
    g_clear_error (&error);

    uri = g_uri_parse (test.url, G_URI_FLAGS_NONE, &error);
    if (!uri) {
      g_warning ("Could not parse url '%s': %s", test.url, error ? error->message : "");
      g_assert_not_reached ();
    }

    config = px_manager_get_configuration (manager, uri);
    if (test.success)
      g_assert_cmpstr (config[0], ==, test.proxy);
    else
      g_assert_cmpstr (config[0], !=, test.proxy);

    g_clear_object (&manager);
  }
}

static void
test_config_kde_wpad (void)
{
  int idx;

  for (idx = 0; idx < G_N_ELEMENTS (config_kde_wpad_test_set); idx++) {
    g_autoptr (PxManager) manager = NULL;
    g_autoptr (GError) error = NULL;
    g_autoptr (GUri) uri = NULL;
    g_auto (GStrv) config = NULL;
    ConfigKdeTest test = config_kde_wpad_test_set[idx];
    g_autofree char *path = g_test_build_filename (G_TEST_DIST, "data", "sample-kde-proxy-wpad", NULL);

    manager = px_test_manager_new ("config-kde", path);
    g_clear_error (&error);

    uri = g_uri_parse (test.url, G_URI_FLAGS_NONE, &error);
    if (!uri) {
      g_warning ("Could not parse url '%s': %s", test.url, error ? error->message : "");
      g_assert_not_reached ();
    }

    config = px_manager_get_configuration (manager, uri);
    if (test.success)
      g_assert_cmpstr (config[0], ==, "wpad://");
    else
      g_assert_cmpstr (config[0], !=, "wpad://");

    g_clear_object (&manager);
  }
}

static void
test_config_kde_pac (void)
{
  int idx;

  for (idx = 0; idx < G_N_ELEMENTS (config_kde_pac_test_set); idx++) {
    g_autoptr (PxManager) manager = NULL;
    g_autoptr (GError) error = NULL;
    g_autoptr (GUri) uri = NULL;
    g_auto (GStrv) config = NULL;
    ConfigKdeTest test = config_kde_pac_test_set[idx];
    g_autofree char *path = g_test_build_filename (G_TEST_DIST, "data", "sample-kde-proxy-pac", NULL);

    manager = px_test_manager_new ("config-kde", path);
    g_clear_error (&error);

    uri = g_uri_parse (test.url, G_URI_FLAGS_NONE, &error);
    if (!uri) {
      g_warning ("Could not parse url '%s': %s", test.url, error ? error->message : "");
      g_assert_not_reached ();
    }

    config = px_manager_get_configuration (manager, uri);
    if (test.success)
      g_assert_cmpstr (config[0], ==, "pac+http://127.0.0.1/px-manager-sample.pac");
    else
      g_assert_cmpstr (config[0], !=, "pac+http://127.0.0.1/px-manager-sample.pac");

    g_clear_object (&manager);
  }
}

static void
test_config_kde_fail (void)
{
  g_autoptr (PxManager) manager = NULL;
  g_autoptr (GError) error = NULL;
  g_autoptr (GUri) uri = NULL;
  g_auto (GStrv) config = NULL;
  g_autofree char *path = g_test_build_filename (G_TEST_DIST, "data", "sample-kde-proxy-pac", NULL);

  /* Disable KDE support */
  g_setenv ("XDG_CURRENT_DESKTOP", "SOMETHING_ELSE", TRUE);

  manager = px_test_manager_new ("config-kde", path);

  uri = g_uri_parse ("https://www.example.com", G_URI_FLAGS_NONE, &error);

  config = px_manager_get_configuration (manager, uri);
  g_print ("%s", config[0]);
  g_assert_null (config[0]);
}

int
main (int    argc,
      char **argv)
{
  g_test_init (&argc, &argv, NULL);

  g_setenv ("XDG_CURRENT_DESKTOP", "KDE", TRUE);

  g_test_add_func ("/config/kde/disabled", test_config_kde_disabled);
  g_test_add_func ("/config/kde/manual", test_config_kde_manual);
  g_test_add_func ("/config/kde/wpad", test_config_kde_wpad);
  g_test_add_func ("/config/kde/pac", test_config_kde_pac);
  g_test_add_func ("/config/kde/fail", test_config_kde_fail);

  return g_test_run ();
}
