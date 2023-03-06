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

/**
 * SECTION:px-proxy
 * @short_description: A convient helper for using proxy servers
 */

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

/**
 * px_proxy_factory_new:
 *
 * Creates a new `pxProxyFactory` instance.
 *
 * This instance should be kept around as long as possible as it contains
 * cached data to increase performance.  Memory usage should be minimal
 * (cache is small) and the cache lifespan is handled automatically.
 *
 * Returns: The newly created `pxProxyFactory`
 */
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
  if (!self->proxy)
    g_warning ("Could not create libproxy dbus proxy: %s", error->message);

  return self;
}

/**
 * px_proxy_factory_get_proxies:
 * @self: a #pxProxyFactory
 * @url: Get proxxies for specificed URL
 *
 * Get which proxies to use for the specified @URL.
 *
 * A %NULL-terminated array of proxy strings is returned.
 * If the first proxy fails, the second should be tried, etc...
 * Don't forget to free the strings/array when you are done.
 * If an unrecoverable error occurs, this function returns %NULL.
 *
 * Regarding performance: this method always blocks and may be called
 * in a separate thread (is thread-safe).  In most cases, the time
 * required to complete this function call is simply the time required
 * to read the configuration (i.e. from gconf, kconfig, etc).
 *
 * In the case of PAC, if no valid PAC is found in the cache (i.e.
 * configuration has changed, cache is invalid, etc), the PAC file is
 * downloaded and inserted into the cache. This is the most expensive
 * operation as the PAC is retrieved over the network. Once a PAC exists
 * in the cache, it is merely a javascript invocation to evaluate the PAC.
 * One should note that DNS can be called from within a PAC during
 * javascript invocation.
 *
 * In the case of WPAD, WPAD is used to automatically locate a PAC on the
 * network.  Currently, we only use DNS for this, but other methods may
 * be implemented in the future.  Once the PAC is located, normal PAC
 * performance (described above) applies.
 *
 * The format of the returned proxy strings are as follows:
 *
 *   - http://[username:password@]proxy:port
 *
 *   - socks://[username:password@]proxy:port
 *
 *   - socks5://[username:password@]proxy:port
 *
 *   - socks4://[username:password@]proxy:port
 *
 *   - <procotol>://[username:password@]proxy:port
 *
 *   - direct://
 *
 * Please note that the username and password in the above URLs are optional
 * and should be use to authenticate the connection if present.
 *
 * For SOCKS proxies, when the protocol version is specified (socks4:// or
 * socks5://), it is expected that only this version is used. When only
 * socks:// is set, the client MUST try SOCKS version 5 protocol and, on
 * connection failure, fallback to SOCKS version 4.
 *
 * Other proxying protocols may exist. It is expected that the returned
 * configuration scheme shall match the network service name of the
 * proxy protocol or the service name of the protocol being proxied if the
 * previous does not exist. As an example, on Mac OS X you can configure a
 * RTSP streaming proxy. The expected returned configuration would be:
 *
 *   - rtsp://[username:password@]proxy:port
 *
 * To free the returned value, call @px_proxy_factory_free_proxies.
 *
 * Returns: (transfer full): a list of proxies
 */
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

/**
 * px_proxy_factory_free_proxies
 * @proxies: proxy array
 *
 * Frees the proxy array returned by @px_proxy_factory_get_proxies when no
 * longer used.
 *
 * @since 0.4.16
 */
void
px_proxy_factory_free_proxies (char **proxies)
{
  g_clear_pointer (&proxies, g_strfreev);
}

/**
 * px_proxy_factory_free:
 * @self: a #pxProxyFactory
 *
 * Frees the `pxProxyFactory`.
 */
void
px_proxy_factory_free (pxProxyFactory *self)
{
  g_cancellable_cancel (self->cancellable);
  g_clear_object (&self->cancellable);
  g_clear_object (&self->proxy);
  g_clear_pointer (&self, g_free);
}
