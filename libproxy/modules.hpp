/*******************************************************************************
 * libproxy - A library for proxy configuration
 * Copyright (C) 2009 Nathaniel McCallum <nathaniel@natemccallum.com>
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

#ifndef MODULE_H_
#define MODULE_H_
#include "module_manager.hpp"
#include "pac.hpp"

/*
 * Config module
 */
enum _pxConfigModuleCategory {
	PX_CONFIG_MODULE_CATEGORY_AUTO    = 0,
	PX_CONFIG_MODULE_CATEGORY_NONE    = 0,
	PX_CONFIG_MODULE_CATEGORY_SYSTEM  = 1,
	PX_CONFIG_MODULE_CATEGORY_SESSION = 2,
	PX_CONFIG_MODULE_CATEGORY_USER    = 3,
	PX_CONFIG_MODULE_CATEGORY__LAST   = PX_CONFIG_MODULE_CATEGORY_USER
};
typedef enum  _pxConfigModuleCategory pxConfigModuleCategory;

typedef struct _pxConfigModule {
	pxConfigModuleCategory category;
	bool                   valid;
	char                *(*get_config)     (struct _pxConfigModule *self, pxURL *url);
	char                *(*get_ignore)     (struct _pxConfigModule *self, pxURL *url);
	bool                 (*get_credentials)(struct _pxConfigModule *self, pxURL *proxy, char **username, char **password);
	bool                 (*set_credentials)(struct _pxConfigModule *self, pxURL *proxy, const char *username, const char *password);
} pxConfigModule;
#define pxConfigModuleVersion 0

#define PX_CONFIG_MODULE_BUILD(self, cat, getconf, getign, getcred, setcred) \
	((pxConfigModule *) self)->category        = cat;     \
	((pxConfigModule *) self)->get_config      = getconf; \
	((pxConfigModule *) self)->get_ignore      = getign;  \
	((pxConfigModule *) self)->get_credentials = getcred; \
	((pxConfigModule *) self)->set_credentials = setcred;

/*
 * Ignore module
 */
typedef struct _pxIgnoreModule {
	bool   (*ignore)(struct _pxIgnoreModule *self, pxURL *url, const char *ignorestr);
} pxIgnoreModule;
#define pxIgnoreModuleVersion 0

/*
 * Network module
 */
typedef struct _pxNetworkModule {
	bool (*changed)(struct _pxNetworkModule *self);
} pxNetworkModule;
#define pxNetworkModuleVersion 0

/*
 * PACRunner module
 */
typedef struct _pxPACRunnerModule {
	char *(*run)(struct _pxPACRunnerModule *self, pxPAC *pac, pxURL *url);
} pxPACRunnerModule;
#define pxPACRunnerModuleVersion 0

/*
 * WPAD module
 */
typedef struct _pxWPADModule {
	bool     found;
	pxPAC *(*next)  (struct _pxWPADModule *self);
	void   (*rewind)(struct _pxWPADModule *self);
} pxWPADModule;
#define pxWPADModuleVersion 0

#endif /* MODULE_H_ */
