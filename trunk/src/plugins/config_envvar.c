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

static char *
_get_config(pxConfigPlugin *self, pxURL *url)
{
	char *proxy = NULL;

	// If the URL is an ftp url, try to read the ftp proxy
	if (!strcmp(px_url_get_scheme(url), "ftp"))
		proxy = getenv("ftp_proxy");

	// If the URL is an https url, try to read the https proxy
	else if (!strcmp(px_url_get_scheme(url), "https"))
		proxy = getenv("https_proxy");

	// If the URL is not ftp or no ftp_proxy was found, get the http_proxy
	if (!proxy)
		proxy = getenv("http_proxy");

	return px_strdup(proxy);
}

static char *
_get_ignore(pxConfigPlugin *self, pxURL *url)
{
	return px_strdup(getenv("no_proxy"));
}

static bool
_get_credentials(pxConfigPlugin *self, pxURL *url, char **username, char **password)
{
	return false;
}

static bool
_set_credentials(pxConfigPlugin *self, pxURL *url, const char *username, const char *password)
{
	return false;
}

static bool
_constructor(pxPlugin *plugin)
{
	PX_CONFIG_PLUGIN_BUILD(plugin, PX_CONFIG_PLUGIN_CATEGORY_NONE, _get_config, _get_ignore, _get_credentials, _set_credentials);
	return true;
}

bool
px_module_load(pxPluginManager *self)
{
	return px_plugin_manager_constructor_add(self, "config_envvar", pxConfigPlugin, _constructor);
}
