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
#include <stdarg.h>

#include <misc.h>
#include <modules.h>
#include <config_file.h>

typedef struct _pxKConfigConfigModule {
	PX_MODULE_SUBCLASS(pxConfigModule);
	pxConfigFile  *cf;
} pxKConfigConfigModule;

static void
_destructor(void *s)
{
	pxKConfigConfigModule *self = (pxKConfigConfigModule *) s;

	px_config_file_free(self->cf);
	px_free(self);
}

static char *
_get_config(pxConfigModule *s, pxURL *url)
{
	pxKConfigConfigModule *self = (pxKConfigConfigModule *) s;

	// TODO: make ignores work w/ KDE
	char *curl = NULL, *tmp = getenv("HOME");
	if (!tmp) return NULL;

	// Open the config file
	pxConfigFile *cf = self->cf;
	if (!cf || px_config_file_is_stale(cf))
	{
		if (cf) px_config_file_free(cf);
		tmp = px_strcat(getenv("HOME"), "/.kde/share/config/kioslaverc", NULL);
		cf = px_config_file_new(tmp);
		px_free(tmp);
		self->cf = cf;
	}
	if (!cf)  goto out;

	// Read the config file to find out what type of proxy to use
	tmp = px_config_file_get_value(cf, "Proxy Settings", "ProxyType");
	if (!tmp) goto out;

	// Don't use any proxy
	if (!strcmp(tmp, "0"))
		curl = px_strdup("direct://");

	// Use a manual proxy
	else if (!strcmp(tmp, "1"))
		curl = px_config_file_get_value(cf, "Proxy Settings", "httpProxy");

	// Use a manual PAC
	else if (!strcmp(tmp, "2"))
	{
		px_free(tmp);
		tmp = px_config_file_get_value(cf, "Proxy Settings", "Proxy Config Script");
		if (tmp) curl = px_strcat("pac+", tmp);
		else     curl = px_strdup("wpad://");
	}

	// Use WPAD
	else if (!strcmp(tmp, "3"))
		curl = px_strdup("wpad://");

	// Use envvar
	else if (!strcmp(tmp, "4"))
		curl = NULL; // We'll bypass this config plugin and let the envvar plugin work

	// Cleanup
	px_free(tmp);

	out:
		return curl;
}

static char *
_get_ignore(pxConfigModule *self, pxURL *url)
{
	return px_strdup("");
}

static bool
_get_credentials(pxConfigModule *self, pxURL *proxy, char **username, char **password)
{
	return false;
}

static bool
_set_credentials(pxConfigModule *self, pxURL *proxy, const char *username, const char *password)
{
	return false;
}

static void *
_constructor()
{
	pxKConfigConfigModule *self = px_malloc0(sizeof(pxKConfigConfigModule));
	PX_CONFIG_MODULE_BUILD(self, PX_CONFIG_MODULE_CATEGORY_SESSION, _get_config, _get_ignore, _get_credentials, _set_credentials);
	return self;
}

bool
px_module_load(pxModuleManager *self)
{
	// If we are running in KDE, then make sure this plugin is registered.
	char *tmp = getenv("KDE_FULL_SESSION");
	if (strcmp(tmp, "true")) {
		return false;
	}
	return px_module_manager_register_module(self, pxConfigModule, _constructor, _destructor);
}
