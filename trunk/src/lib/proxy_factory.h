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

#ifndef PROXY_FACTORY_H_
#define PROXY_FACTORY_H_

#include <stdbool.h>

#include "proxy.h"

// URLs look like this:
//   http://host:port
//   socks://host:port
//   pac+http://pac_host:port/path/to/pac
//   wpad://
//   direct://
// TODO: ignore syntax TBD
struct _PXConfig {
	char *url;
	char *ignore;
};
typedef struct _PXConfig PXConfig;

typedef void      (*PXProxyFactoryVoidCallback)     (pxProxyFactory *self);
typedef bool      (*PXProxyFactoryBoolCallback)     (pxProxyFactory *self);
typedef void     *(*PXProxyFactoryPtrCallback)      (pxProxyFactory *self);

typedef char     *(*PXPACRunnerCallback)            (pxProxyFactory *self, const char *pac, const char *url, const char *hostname);

bool              px_proxy_factory_config_set       (pxProxyFactory *self, pxConfigBackend config, PXProxyFactoryPtrCallback callback);
pxConfigBackend   px_proxy_factory_config_get_active(pxProxyFactory *self);
void              px_proxy_factory_wpad_restart     (pxProxyFactory *self);
void              px_proxy_factory_on_get_proxy_add (pxProxyFactory *self, PXProxyFactoryVoidCallback callback);
void              px_proxy_factory_on_get_proxy_del (pxProxyFactory *self, PXProxyFactoryVoidCallback callback);
bool              px_proxy_factory_pac_runner_set   (pxProxyFactory *self, PXPACRunnerCallback callback);

#endif /*PROXY_FACTORY_H_*/

