/* px-manager-helper.c
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

PxManager *
px_test_manager_new (const char *config_plugin, const char *config_option)
{
  return px_manager_new_with_options ("config-plugin", config_plugin,
                                      "config-option", config_option,
                                      "force-online", TRUE,
                                      NULL);
}
