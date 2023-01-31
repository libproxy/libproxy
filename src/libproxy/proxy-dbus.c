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

#include "proxy.h"

/**
 * SECTION:px-proxy
 * @short_description: A convient helper for using proxy servers
 */

struct px_proxy_factory {
  GDBusProxy *proxy;
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
  g_autoptr (GError) error = NULL;
  struct px_proxy_factory *self = g_malloc0 (sizeof (struct px_proxy_factory));

  self->cancellable = g_cancellable_new ();
  self->proxy = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SESSION,
                                               G_DBUS_PROXY_FLAGS_NONE,
                                               NULL, /* GDBusInterfaceInfo */
                                               "org.libproxy.proxy",
                                               "/org/libproxy/proxy",
                                               "org.libproxy.proxy",
                                               self->cancellable, /* GCancellable */
                                               &error);
  if (!self->proxy)
    g_warning ("Could not create libproxy dbus proxy: %s", error->message);

  return self;
}

char **
px_proxy_factory_get_proxies (struct px_proxy_factory *self,
                              const char              *url)
{
  g_autoptr (GVariant) result = NULL;
  g_autoptr (GError) error = NULL;
  g_autoptr (GVariantIter) iter = NULL;
  g_autoptr (GList) list = NULL;
  GList *tmp;
  char *str;
  char **retval;
  gsize len;
  gsize idx;

  if (!self->proxy)
    return NULL;

  result = g_dbus_proxy_call_sync (self->proxy,
                                   "query",
                                   g_variant_new ("(s)", url),
                                   G_DBUS_CALL_FLAGS_NONE,
                                   -1,
                                   self->cancellable,
                                   &error);
  if (!result) {
    g_warning ("Could not query proxy dbus: %s", error->message);
    return NULL;
  }

  g_variant_get (result, "(as)", &iter);

  while (g_variant_iter_loop (iter, "&s", &str)) {
    list = g_list_prepend (list, str);
  }

  len = g_list_length (list);
  if (len == 0) {
    retval = g_malloc0 (sizeof (char *) * 2);
    retval[0] = g_strdup ("direct://");

    return retval;
  }

  retval = g_malloc0_n (len + 1, sizeof (char *));
  for (tmp = list, idx = 0; tmp && tmp->data; tmp = tmp->next, idx++) {
    char *value = tmp->data;
    retval[idx] = g_strdup (value);
  }

  return retval;
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
  g_clear_object (&self->proxy);
  g_clear_pointer (&self, g_free);
}
