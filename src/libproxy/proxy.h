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

#include <glib-object.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * SECTION:px-proxy
 * @short_description: A convient helper for using proxy servers
 */

typedef struct _pxProxyFactory pxProxyFactory;

#define PX_TYPE_PROXY_FACTORY (px_proxy_factory_get_type ())

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
pxProxyFactory *px_proxy_factory_new (void);

GType           px_proxy_factory_get_type (void) G_GNUC_CONST;

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
char **px_proxy_factory_get_proxies (pxProxyFactory *self, const char *url);

/**
 * px_proxy_factory_free_proxies
 * @proxies: (array zero-terminated=1): a %NULL-terminated array of proxies
 *
 * Frees the proxy array returned by @px_proxy_factory_get_proxies when no
 * longer used.
 *
 * @since 0.4.16
 */
void px_proxy_factory_free_proxies (char **proxies);

/**
 * px_proxy_factory_free:
 * @self: a #pxProxyFactory
 *
 * Frees the `pxProxyFactory`.
 */
void px_proxy_factory_free (pxProxyFactory *self);

#ifdef __cplusplus
}
#endif
