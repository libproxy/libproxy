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
  GDBusConnection *connection;
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

  self->connection = g_bus_get_sync (G_BUS_TYPE_SESSION, self->cancellable, &error);
  if (!self->connection) {
    g_clear_error (&error);
    self->connection = g_bus_get_sync (G_BUS_TYPE_SYSTEM, self->cancellable, &error);
  }

  if (!self->connection)
    g_warning ("Could not create dbus connection: %s", error->message);

  return self;
}

char **
px_proxy_factory_get_proxies (pxProxyFactory *self,
                              const char     *url)
{
  g_autoptr (GError) error = NULL;
  g_autoptr (GVariantIter) iter = NULL;
  g_autoptr (GList) list = NULL;
  g_autoptr (GDBusMessage) msg = NULL;
  g_autoptr (GDBusMessage) reply = NULL;
  GVariant *result;
  GList *tmp;
  char *str;
  char **retval;
  gsize len;
  gsize idx;

  if (!self->connection)
    return NULL;

  msg = g_dbus_message_new_method_call ("org.libproxy.proxy",
                                        "/org/libproxy/proxy",
                                        "org.libproxy.proxy",
                                        "GetProxiesFor");

  g_dbus_message_set_body (msg, g_variant_new ("(s)", url));

  reply = g_dbus_connection_send_message_with_reply_sync (self->connection, msg, G_DBUS_SEND_MESSAGE_FLAGS_NONE, -1, NULL, self->cancellable, &error);
  if (!reply) {
    g_warning ("Could not query proxy: %s", error->message);
    return NULL;
  }

  if (g_dbus_message_get_message_type (reply) != G_DBUS_MESSAGE_TYPE_METHOD_RETURN)
    return NULL;

  result = g_dbus_message_get_body (reply);
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
  g_clear_object (&self->connection);
  g_clear_pointer (&self, g_free);
}
