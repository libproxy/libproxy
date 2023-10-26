/* proxy.c
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

#include <gio/gio.h>

#include "px-manager.h"
#include "proxy.h"

struct _pxProxyFactory {
  PxManager *manager;
};

pxProxyFactory *px_proxy_factory_copy (pxProxyFactory *self);

G_DEFINE_BOXED_TYPE (pxProxyFactory,
                     px_proxy_factory,
                     (GBoxedCopyFunc)px_proxy_factory_copy,
                     (GFreeFunc)px_proxy_factory_new);

pxProxyFactory *
px_proxy_factory_new (void)
{
  pxProxyFactory *self = g_new0 (pxProxyFactory, 1);

  self->manager = px_manager_new ();

  return self;
}

pxProxyFactory *
px_proxy_factory_copy (pxProxyFactory *self)
{
  return g_memdup2 (self, sizeof (pxProxyFactory));
}

char **
px_proxy_factory_get_proxies (pxProxyFactory *self,
                              const char     *url)
{
  g_auto (GStrv) result = NULL;

  result = px_manager_get_proxies_sync (self->manager, url);
  return g_steal_pointer (&result);
}

void
px_proxy_factory_free_proxies (char **proxies)
{
  g_clear_pointer (&proxies, g_strfreev);
}

void
px_proxy_factory_free (pxProxyFactory *self)
{
  g_clear_object (&self->manager);
  g_clear_pointer (&self, g_free);
}
