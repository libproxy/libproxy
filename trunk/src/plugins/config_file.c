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

#include <stdlib.h>
#include <string.h>

#include <misc.h>
#include <plugin_manager.h>
#include <plugin_config.h>
#include <config_file.h>

typedef struct _pxFileConfigPlugin {
	PX_PLUGIN_SUBCLASS(pxConfigPlugin);
	char          *filename;
	pxConfigFile  *cf;
	void         (*old_destructor)(pxPlugin *);
} pxFileConfigPlugin;

static void
_destructor(pxPlugin *self)
{
	px_free(((pxFileConfigPlugin *) self)->filename);
	px_config_file_free(((pxFileConfigPlugin *) self)->cf);
	((pxFileConfigPlugin *) self)->old_destructor(self);
}

static char *
_get_config(pxConfigPlugin *self, pxURL *url)
{
	if (!((pxFileConfigPlugin *) self)->cf)
		((pxFileConfigPlugin *) self)->cf = px_config_file_new(((pxFileConfigPlugin *) self)->filename);
	if (!((pxFileConfigPlugin *) self)->cf)
		return NULL;
	return px_config_file_get_value(((pxFileConfigPlugin *) self)->cf, PX_CONFIG_FILE_DEFAULT_SECTION, "proxy");
}

static char *
_get_ignore(pxConfigPlugin *self, pxURL *url)
{
	if (!((pxFileConfigPlugin *) self)->cf)
		((pxFileConfigPlugin *) self)->cf = px_config_file_new(((pxFileConfigPlugin *) self)->filename);
	if (!((pxFileConfigPlugin *) self)->cf)
		return NULL;
	return px_config_file_get_value(((pxFileConfigPlugin *) self)->cf, PX_CONFIG_FILE_DEFAULT_SECTION, "ignore");
}

static bool
_get_credentials(pxConfigPlugin *self, pxURL *proxy, char **username, char **password)
{
	return false;
}

static bool
_set_credentials(pxConfigPlugin *self, pxURL *proxy, const char *username, const char *password)
{
	return false;
}

static bool
_system_constructor(pxPlugin *self)
{
	PX_CONFIG_PLUGIN_BUILD(self, PX_CONFIG_PLUGIN_CATEGORY_SYSTEM, _get_config, _get_ignore, _get_credentials, _set_credentials);
	((pxFileConfigPlugin *) self)->old_destructor = self->destructor;
	self->destructor                              = _destructor;

	((pxFileConfigPlugin *) self)->filename = px_strdup(SYSCONFDIR "/proxy.conf");
	return true;
}

static bool
_user_constructor(pxPlugin *self)
{
	PX_CONFIG_PLUGIN_BUILD(self, PX_CONFIG_PLUGIN_CATEGORY_USER, _get_config, _get_ignore, _get_credentials, _set_credentials);
	((pxFileConfigPlugin *) self)->old_destructor = self->destructor;
	self->destructor                              = _destructor;

	((pxFileConfigPlugin *) self)->filename = px_strcat(getenv("HOME"), "/", ".proxy.conf", NULL);
	if (!((pxFileConfigPlugin *) self)->filename || !strcmp(((pxFileConfigPlugin *) self)->filename, ""))
		return false;
	return true;
}

bool
px_module_load(pxPluginManager *self)
{
	bool a = px_plugin_manager_constructor_add_subtype(self, "config_file_system", pxConfigPlugin, pxFileConfigPlugin, _system_constructor);
	bool b = px_plugin_manager_constructor_add_subtype(self, "config_file_user",   pxConfigPlugin, pxFileConfigPlugin, _user_constructor);
	return (a || b);
}
