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

#ifndef PLUGIN_CONFIG_H_
#define PLUGIN_CONFIG_H_
#include "plugin.h"
#include "url.h"

enum _pxConfigPluginCategory {
	PX_CONFIG_PLUGIN_CATEGORY_AUTO    = 0,
	PX_CONFIG_PLUGIN_CATEGORY_NONE    = 0,
	PX_CONFIG_PLUGIN_CATEGORY_SYSTEM  = 1,
	PX_CONFIG_PLUGIN_CATEGORY_SESSION = 2,
	PX_CONFIG_PLUGIN_CATEGORY_USER    = 3,
	PX_CONFIG_PLUGIN_CATEGORY__LAST   = PX_CONFIG_PLUGIN_CATEGORY_USER
};
typedef enum  _pxConfigPluginCategory pxConfigPluginCategory;

typedef struct _pxConfigPlugin {
	PX_PLUGIN_SUBCLASS(pxPlugin);
	pxConfigPluginCategory category;
	bool                   valid;
	char                *(*get_config)     (struct _pxConfigPlugin *self, pxURL *url);
	char                *(*get_ignore)     (struct _pxConfigPlugin *self, pxURL *url);
	bool                 (*get_credentials)(struct _pxConfigPlugin *self, pxURL *proxy, char **username, char **password);
	bool                 (*set_credentials)(struct _pxConfigPlugin *self, pxURL *proxy, const char *username, const char *password);
} pxConfigPlugin;
#define pxConfigPluginVersion 0

#define PX_CONFIG_PLUGIN_BUILD(self, cat, getconf, getign, getcred, setcred) \
	((pxConfigPlugin *) self)->category        = cat;     \
	((pxConfigPlugin *) self)->get_config      = getconf; \
	((pxConfigPlugin *) self)->get_ignore      = getign;  \
	((pxConfigPlugin *) self)->get_credentials = getcred; \
	((pxConfigPlugin *) self)->set_credentials = setcred

int  px_config_plugin_compare(pxPlugin *self, pxPlugin *other);

#endif /* PLUGIN_CONFIG_H_ */
