/* libproxy-test.c
 *
 * Copyright 2023 The Libproxy Team
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

#include "proxy.h"

#include <gio/gio.h>

typedef struct {
  pxProxyFactory *pf;
} Fixture;

static void
fixture_setup (Fixture       *self,
               gconstpointer  data)
{
  self->pf = px_proxy_factory_new ();
}

static void
fixture_teardown (Fixture       *fixture,
                  gconstpointer  data)
{
  px_proxy_factory_free (fixture->pf);
}

static void
test_libproxy_setup (Fixture    *self,
                     const void *user_data)
{
  char **proxies = NULL;

  proxies = px_proxy_factory_get_proxies (self->pf, "https://www.example.com");
  g_assert_nonnull (proxies);
  g_assert_nonnull (proxies[0]);
  px_proxy_factory_free_proxies (proxies);

  return;

  /* FIXME: Fails on Windows */
  /* g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_WARNING, "Could not query proxy: URI is not absolute, and no base URI was provided"); */
  /* proxies = px_proxy_factory_get_proxies (self->pf, "http_unknown://www.example.com"); */
  /* g_assert_nonnull (proxies); */
  /* g_assert_nonnull (proxies[0]); */
  /* px_proxy_factory_free_proxies (proxies); */
}

int
main (int    argc,
      char **argv)
{
  g_test_init (&argc, &argv, NULL);

  g_test_add ("/libproxy/setup", Fixture, NULL, fixture_setup, test_libproxy_setup, fixture_teardown);

  return g_test_run ();
}
