/* proxy-dbus.c
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

#include "proxy.h"

struct _pxProxyFactory {
  GDBusProxy *proxy;
  GCancellable *cancellable;
};

pxProxyFactory *px_proxy_factory_copy (pxProxyFactory *self);

G_DEFINE_BOXED_TYPE (pxProxyFactory,
                     px_proxy_factory,
                     (GBoxedCopyFunc)px_proxy_factory_copy,
                     (GFreeFunc)px_proxy_factory_new);

pxProxyFactory *
px_proxy_factory_copy (pxProxyFactory *self)
{
  return g_memdup2 (self, sizeof (pxProxyFactory));
}

pxProxyFactory *
px_proxy_factory_new (void)
{
  g_autoptr (GError) error = NULL;
  pxProxyFactory *self = g_new0 (pxProxyFactory, 1);

  self->cancellable = g_cancellable_new ();
  self->proxy = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SESSION,
                                               G_DBUS_PROXY_FLAGS_NONE,
                                               NULL, /* GDBusInterfaceInfo */
                                               "org.libproxy.proxy",
                                               "/org/libproxy/proxy",
                                               "org.libproxy.proxy",
                                               self->cancellable, /* GCancellable */
                                               &error);

  if (!self->proxy) {
    g_clear_error (&error);

    self->proxy = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SYSTEM,
                                                 G_DBUS_PROXY_FLAGS_NONE,
                                                 NULL, /* GDBusInterfaceInfo */
                                                 "org.libproxy.proxy",
                                                 "/org/libproxy/proxy",
                                                 "org.libproxy.proxy",
                                                 self->cancellable, /* GCancellable */
                                                 &error);
  }

  if (!self->proxy)
    g_warning ("Could not create libproxy dbus proxy: %s", error->message);

  return self;
}

char **
px_proxy_factory_get_proxies (pxProxyFactory *self,
                              const char     *url)
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

void
px_proxy_factory_free (pxProxyFactory *self)
{
  g_cancellable_cancel (self->cancellable);
  g_clear_object (&self->cancellable);
  g_clear_object (&self->proxy);
  g_clear_pointer (&self, g_free);
}
