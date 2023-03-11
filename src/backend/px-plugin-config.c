/* px-plugin-config.c
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

#include "px-plugin-config.h"

G_DEFINE_INTERFACE (PxConfig, px_config, G_TYPE_OBJECT)

static void
px_config_default_init (PxConfigInterface *iface)
{
  g_object_interface_install_property (iface,
                                       g_param_spec_string ("config-option",
                                                            NULL,
                                                            NULL,
                                                            NULL,
                                                            G_PARAM_READWRITE |
                                                            G_PARAM_CONSTRUCT_ONLY |
                                                            G_PARAM_STATIC_STRINGS));
}
