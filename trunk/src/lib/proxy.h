/*******************************************************************************
 * libproxy - A library for proxy configuration
 * Copyright (C) 2006 Nathaniel McCallum <nathaniel@natemccallum.com>
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
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

#ifndef PROXY_H_
#define PROXY_H_

typedef struct _pxProxyFactory  pxProxyFactory;

/**
 * Creates a new pxProxyFactory instance.  This instance
 * and all its methods are NOT thread safe, so please take
 * care in their use.
 * 
 * @return A new pxProxyFactory instance or NULL on error
 */
pxProxyFactory  *px_proxy_factory_new      ();

/**
 * Get which proxies to use for the specified URL.
 * 
 * A NULL-terminated array of proxy strings is returned.
 * If the first proxy fails, the second should be tried, etc...
 * Don't forget to free the strings/array when you are done.
 * 
 * The format of the returned proxy strings are as follows:
 *   - http://proxy:port
 *   - socks://proxy:port
 *   - direct://
 * @url The URL we are trying to reach
 * @return A NULL-terminated array of proxy strings to use
 */
char           **px_proxy_factory_get_proxies(pxProxyFactory *self, char *url);

/**
 * Frees the pxProxyFactory instance when no longer used.
 */
void             px_proxy_factory_free     (pxProxyFactory *self);

#endif /*PROXY_H_*/
