/* config-env-test.c
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
  const char *env;
  const char *proxy;
  const char *no_proxy;
  const char *url;
  gboolean config_is_proxy;
} ConfigEnvTest;

static const ConfigEnvTest config_env_test_set[] = {
  { "HTTP_PROXY", "http://127.0.0.1:8080", NULL, "https://www.example.com", TRUE},
  { "HTTP_PROXY", "http://127.0.0.1:8080", NULL, "http://www.example.com", TRUE},
  { "HTTP_PROXY", "http://127.0.0.1:8080", NULL, "ftp://www.example.com", TRUE},
  { "HTTP_PROXY", "http://127.0.0.1:8080", "www.example.com", "https://www.example.com", FALSE},
  { "HTTP_PROXY", "http://127.0.0.1:8080", "www.test.com", "https://www.example.com", TRUE},
  { "HTTP_PROXY", "http://127.0.0.1:8080", "*", "https://www.example.com", FALSE},
  { "http_proxy", "http://127.0.0.1:8080", NULL, "https://www.example.com", TRUE},
  { "http_proxy", "http://127.0.0.1:8080", "127.0.0.0/24", "http://127.0.0.1", FALSE},
  { "http_proxy", "http://127.0.0.1:8080", "127.0.0.0/24", "http://127.0.0.2", FALSE},
  { "http_proxy", "http://127.0.0.1:8080", "127.0.0.1", "http://127.0.0.255", TRUE},
  { "http_proxy", "http://127.0.0.1:8080", "::1", "http://[::1]/", FALSE},
  { "http_proxy", "http://127.0.0.1:8080", "::1", "http://[::1]:80/", FALSE},
  { "http_proxy", "http://127.0.0.1:8080", "::1", "http://[::1:1]/", TRUE},
  { "http_proxy", "http://127.0.0.1:8080", "::1", "http://[::1:1]:80/", TRUE},
  { "http_proxy", "http://127.0.0.1:8080", "::1", "http://[fe80::1]/", TRUE},
  { "http_proxy", "http://127.0.0.1:8080", "::1", "http://[fe80::1]:80/", TRUE},
  { "http_proxy", "http://127.0.0.1:8080", "::1", "http://[fec0::1]/", TRUE},
  { "HTTPS_PROXY", "http://127.0.0.1:8080", NULL, "https://www.example.com", TRUE},
  { "HTTPS_PROXY", "http://127.0.0.1:8080", NULL, "http://www.example.com", FALSE},
  { "HTTPS_PROXY", "http://127.0.0.1:8080", NULL, "ftp://www.example.com", FALSE},
  { "https_proxy", "http://127.0.0.1:8080", NULL, "ftp://www.example.com", FALSE},
  { "FTP_PROXY", "http://127.0.0.1:8080", NULL, "https://www.example.com", FALSE},
  { "FTP_PROXY", "http://127.0.0.1:8080", NULL, "http://www.example.com", FALSE},
  { "FTP_PROXY", "http://127.0.0.1:8080", NULL, "ftp://www.example.com", TRUE},
  { "ftp_proxy", "http://127.0.0.1:8080", NULL, "ftp://www.example.com", TRUE},
};

static void
test_config_env (void)
{
  int idx;

  for (idx = 0; idx < G_N_ELEMENTS (config_env_test_set); idx++) {
    g_autoptr (PxManager) manager = NULL;
    g_autoptr (GError) error = NULL;
    g_autoptr (GUri) uri = NULL;
    g_auto (GStrv) config = NULL;
    ConfigEnvTest test = config_env_test_set[idx];

    /* Set proxy environment variable. Must be done before px_test_manager_new()! */
    if (!g_setenv (test.env, test.proxy, TRUE)) {
      g_warning ("Could not set environment");
      continue;
    }

    if (test.no_proxy) {
      if (test.config_is_proxy)
        g_setenv ("NO_PROXY", test.no_proxy, TRUE);
      else
        g_setenv ("no_proxy", test.no_proxy, TRUE);
    }

    manager = px_test_manager_new ("config-env", NULL);
    g_clear_error (&error);

    uri = g_uri_parse (test.url, G_URI_FLAGS_PARSE_RELAXED, &error);
    config = px_manager_get_configuration (manager, uri, &error);
    if (test.config_is_proxy)
      g_assert_cmpstr (config[0], ==, test.proxy);
    else
      g_assert_cmpstr (config[0], !=, test.proxy);

    g_unsetenv (test.env);
    g_unsetenv ("NO_PROXY");
    g_unsetenv ("no_proxy");

    g_clear_object (&manager);
  }
}

int
main (int    argc,
      char **argv)
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/config/env", test_config_env);

  return g_test_run ();
}
