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
#include "pac.h"

enum _pxConfigCategory {
	PX_CONFIG_CATEGORY_AUTO    = 0,
	PX_CONFIG_CATEGORY_NONE    = 0,
	PX_CONFIG_CATEGORY_SYSTEM  = 1,
	PX_CONFIG_CATEGORY_SESSION = 2,
	PX_CONFIG_CATEGORY_USER    = 3,
	PX_CONFIG_CATEGORY__LAST   = PX_CONFIG_CATEGORY_USER
};
typedef enum  _pxConfigCategory pxConfigCategory;

// URLs look like this:
//   http://host:port
//   socks://host:port
//   pac+http://pac_host:port/path/to/pac
//   wpad://
//   direct://
// TODO: ignore syntax TBD
struct _pxConfig {
	char *url;
	char *ignore;
};
typedef struct _pxConfig pxConfig;

/**
 * Utility function to create pxConfig objects. Steals ownership of the parameters.
 * @url The proxy config url.  If NULL, no pxConfig will be created.
 * @ignore Ignore patterns.  If NULL, a pxConfig will still be created.
 * @return pxConfig instance or NULL if url is NULL.
 */
pxConfig *px_config_create(char *url, char *ignore);

typedef void     (*pxProxyFactoryVoidCallback)    (pxProxyFactory *self);
typedef bool     (*pxProxyFactoryBoolCallback)    (pxProxyFactory *self);
typedef void    *(*pxProxyFactoryPtrCallback)     (pxProxyFactory *self);
typedef char    *(*pxPACRunnerCallback)           (pxProxyFactory *self, pxPAC *pac, pxURL *url);

bool             px_proxy_factory_config_add      (pxProxyFactory *self, const char *name, pxConfigCategory category, pxProxyFactoryPtrCallback callback);
bool             px_proxy_factory_config_del      (pxProxyFactory *self, const char *name);
bool             px_proxy_factory_misc_set        (pxProxyFactory *self, const char *key, const void *value);
void            *px_proxy_factory_misc_get        (pxProxyFactory *self, const char *key);
void             px_proxy_factory_network_changed (pxProxyFactory *self);
bool             px_proxy_factory_on_get_proxies_add(pxProxyFactory *self, pxProxyFactoryVoidCallback callback);
bool             px_proxy_factory_on_get_proxies_del(pxProxyFactory *self, pxProxyFactoryVoidCallback callback);
bool             px_proxy_factory_pac_runner_set  (pxProxyFactory *self, pxPACRunnerCallback callback);

#endif /*PROXY_FACTORY_H_*/

