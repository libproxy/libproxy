/*******************************************************************************
 * libproxy - A library for proxy configuration
 * Copyright (C) 2006 Nathaniel McCallum <nathaniel@natemccallum.com>
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

#ifndef PROXY_H_
#define PROXY_H_

enum _pxConfigBackend {
	PX_CONFIG_BACKEND_AUTO    = 0,
	PX_CONFIG_BACKEND_NONE    = 0,
	PX_CONFIG_BACKEND_WIN32   = (1 << 0),
	PX_CONFIG_BACKEND_KCONFIG = (1 << 1),
	PX_CONFIG_BACKEND_GCONF   = (1 << 2),
	PX_CONFIG_BACKEND_USER    = (1 << 3),
	PX_CONFIG_BACKEND_SYSTEM  = (1 << 4),
	PX_CONFIG_BACKEND_ENVVAR  = (1 << 5),
	PX_CONFIG_BACKEND__LAST   = PX_CONFIG_BACKEND_ENVVAR
};

typedef struct _pxProxyFactory  pxProxyFactory;
typedef enum   _pxConfigBackend pxConfigBackend;

pxProxyFactory *px_proxy_factory_new      (pxConfigBackend config);
char           *px_proxy_factory_get_proxy(pxProxyFactory *self, char *url);
void            px_proxy_factory_free     (pxProxyFactory *self);

#endif /*PROXY_H_*/
