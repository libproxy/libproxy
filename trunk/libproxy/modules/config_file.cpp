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

#include "../misc.hpp"
#include "../modules.hpp"
#include "../config_file.hpp"

typedef struct _pxFileConfigModule {
	PX_MODULE_SUBCLASS(pxConfigModule);
	char          *filename;
	pxConfigFile  *cf;
} pxFileConfigModule;

static void
_destructor(void *s)
{
	pxFileConfigModule *self = (pxFileConfigModule *) self;

	px_config_file_free(self->cf);
	px_free(self->filename);
	px_free(self);
}

static char *
_get_config(pxConfigModule *ss, pxURL *url)
{
	pxFileConfigModule *self = (pxFileConfigModule *) self;

	if (!self->cf)
		self->cf = px_config_file_new(self->filename);
	if (!self->cf)
		return NULL;
	return px_config_file_get_value(self->cf, PX_CONFIG_FILE_DEFAULT_SECTION, "proxy");
}

static char *
_get_ignore(pxConfigModule *s, pxURL *url)
{
	pxFileConfigModule *self = (pxFileConfigModule *) self;

	if (!self->cf)
		self->cf = px_config_file_new(self->filename);
	if (!self->cf)
		return NULL;
	return px_config_file_get_value(self->cf, PX_CONFIG_FILE_DEFAULT_SECTION, "ignore");
}

static bool
_get_credentials(pxConfigModule *s, pxURL *proxy, char **username, char **password)
{
	pxFileConfigModule *self = (pxFileConfigModule *) self;

	return false;
}

static bool
_set_credentials(pxConfigModule *s, pxURL *proxy, const char *username, const char *password)
{
	pxFileConfigModule *self = (pxFileConfigModule *) self;

	return false;
}

static void *
_system_constructor()
{
	pxFileConfigModule *self = (pxFileConfigModule *) px_malloc0(sizeof(pxFileConfigModule));
	PX_CONFIG_MODULE_BUILD(self, PX_CONFIG_MODULE_CATEGORY_SYSTEM, _get_config, _get_ignore, _get_credentials, _set_credentials);
	self->filename = px_strdup(SYSCONFDIR "proxy.conf");

	return self;
}

static void *
_user_constructor()
{
	pxFileConfigModule *self = (pxFileConfigModule *) px_malloc0(sizeof(pxFileConfigModule));
	PX_CONFIG_MODULE_BUILD(self, PX_CONFIG_MODULE_CATEGORY_USER, _get_config, _get_ignore, _get_credentials, _set_credentials);
	self->filename = px_strcat(getenv("HOME"), "/", ".proxy.conf", NULL);

	if (!self->filename || !strcmp(self->filename, ""))
	{
		_destructor((void *) self);
		return NULL;
	}
	return self;
}

bool
px_module_load(pxModuleManager *self)
{
	bool a = px_module_manager_register_module_with_name(self, pxConfigFile, "config_file_system", _system_constructor, _destructor);
	bool b = px_module_manager_register_module_with_name(self, pxConfigFile, "config_file_user",   _user_constructor,   _destructor);
	return (a || b);
}
