/*******************************************************************************
 * libproxy - A library for proxy configuration
 * Copyright (C) 2006 Nathaniel McCallum <nathaniel@natemccallum.com>
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

#include <gio/gio.h>

#include "px-manager.h"
#include "proxy.h"

/**
 * SECTION:px-proxy
 * @short_description: A convient helper for using proxy servers
 *
 * Test 123
 */

struct px_proxy_factory {
  PxManager *manager;
  GCancellable *cancellable;
};

/**
 * px_proxy_factory_new:
 * Creates a new proxy factory.
 *
 * Returns: pointer to #px_proxy_factory
 */
struct px_proxy_factory *
px_proxy_factory_new (void)
{
  struct px_proxy_factory *self = g_malloc0 (sizeof (struct px_proxy_factory));

  self->cancellable = g_cancellable_new ();
  self->manager = px_manager_new ();

  return self;
}

char **
px_proxy_factory_get_proxies (struct px_proxy_factory *self,
                              const char              *url)
{
  g_auto (GStrv) result = NULL;
  g_autoptr (GError) error = NULL;

  result = px_manager_get_proxies_sync (self->manager, url, &error);
  if (!result) {
    g_warning ("Could not query proxy: %s", error ? error->message : "");
    return NULL;
  }

  return g_steal_pointer (&result);
}

void
px_proxy_factory_free_proxies (char **proxies)
{
  g_clear_pointer (&proxies, g_strfreev);
}

/**
 * px_proxy_factory_free:
 * @self: a px_proxy_factory
 *
 * Free px_proxy_factory
 */
void
px_proxy_factory_free (struct px_proxy_factory *self)
{
  g_cancellable_cancel (self->cancellable);
  g_clear_object (&self->cancellable);
  g_clear_object (&self->manager);
  g_clear_pointer (&self, g_free);
}