/* config-osx-test.c
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

static void
test_config_osx (void)
{
  g_autoptr (GError) error = NULL;
  g_autoptr (PxManager) manager = NULL;
  g_autoptr (GUri) uri = NULL;
  g_auto (GStrv) config = NULL;

  manager = px_test_manager_new ("config-osx", NULL);
  g_clear_error (&error);

  uri = g_uri_parse ("https://www.example.com", G_URI_FLAGS_PARSE_RELAXED, &error);
  config = px_manager_get_configuration (manager, uri, &error);
  g_assert_nonnull (config);
  g_assert_null (config[0]);
}

int
main (int    argc,
      char **argv)
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/config/osx", test_config_osx);

  return g_test_run ();
}
