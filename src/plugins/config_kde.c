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
#include <plugin_manager.h>
#include <plugin_config.h>
#include <config_file.h>

// From xhasclient.c
bool x_has_client(char *prog, ...);

typedef struct _pxKConfigConfigPlugin {
	PX_PLUGIN_SUBCLASS(pxConfigPlugin);
	pxConfigFile  *cf;
	void         (*old_destructor)(pxPlugin *);
} pxKConfigConfigPlugin;

static void
_destructor(pxPlugin *self)
{
	px_config_file_free(((pxKConfigConfigPlugin *) self)->cf);
	((pxKConfigConfigPlugin *) self)->old_destructor(self);
}

static char *
_get_config(pxConfigPlugin *self, pxURL *url)
{
	// TODO: make ignores work w/ KDE
	char *curl = NULL, *tmp = getenv("HOME");
	if (!tmp) return NULL;

	// Open the config file
	pxConfigFile *cf = ((pxKConfigConfigPlugin *) self)->cf;
	if (!cf || px_config_file_is_stale(cf))
	{
		if (cf) px_config_file_free(cf);
		tmp = px_strcat(getenv("HOME"), "/.kde/share/config/kioslaverc", NULL);
		cf = px_config_file_new(tmp);
		px_free(tmp);
		((pxKConfigConfigPlugin *) self)->cf = cf;
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
_get_ignore(pxConfigPlugin *self, pxURL *url)
{
	return px_strdup("");
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
_constructor(pxPlugin *self)
{
	PX_CONFIG_PLUGIN_BUILD(self, PX_CONFIG_PLUGIN_CATEGORY_SESSION, _get_config, _get_ignore, _get_credentials, _set_credentials);
	((pxKConfigConfigPlugin *) self)->old_destructor = self->destructor;
	self->destructor                                 = _destructor;
	return true;
}

bool
px_module_load(pxPluginManager *self)
{
	// If we are running in KDE, then make sure this plugin is registered.
	if (x_has_client("kicker", NULL))
		return px_plugin_manager_constructor_add_subtype(self, "config_kde", pxConfigPlugin, pxKConfigConfigPlugin, _constructor);
	return false;
}
