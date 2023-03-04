/* proxy.h
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


#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <gio/gio.h>

typedef struct _pxProxyFactory pxProxyFactory;

#define PX_TYPE_PROXY_FACTORY (px_proxy_factory_get_type ())

pxProxyFactory *px_proxy_factory_new (void);
GType           px_proxy_factory_get_type (void) G_GNUC_CONST;

char **px_proxy_factory_get_proxies (pxProxyFactory *self, const char *url);
void px_proxy_factory_free_proxies (char **proxies);
void px_proxy_factory_free (pxProxyFactory *self);

#ifdef __cplusplus
}
#endif
