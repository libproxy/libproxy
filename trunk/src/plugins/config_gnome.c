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

#include <gconf/gconf-client.h>

// From xhasclient.c
bool x_has_client(char *prog, ...);

typedef struct _pxGConfConfigPlugin {
	PX_PLUGIN_SUBCLASS(pxConfigPlugin);
	GConfClient *client;
	void       (*old_destructor)(pxPlugin *);
} pxGConfConfigPlugin;

static void
_destructor(pxPlugin *self)
{
	g_object_unref(((pxGConfConfigPlugin *) self)->client);
	((pxGConfConfigPlugin *) self)->old_destructor(self);
}

static char *
_get_config(pxConfigPlugin *self, pxURL *url)
{
	GConfClient *client = ((pxGConfConfigPlugin *) self)->client;

	// Get the mode
	char *mode = gconf_client_get_string(client, "/system/proxy/mode", NULL);
	if (!mode)
		return NULL;

	char *curl = NULL;

	// Mode is direct://
	if (!strcmp(mode, "none"))
		curl = px_strdup("direct://");

	// Mode is wpad:// or pac+http://...
	else if (!strcmp(mode, "auto"))
	{
		char *tmp = gconf_client_get_string(client, "/system/proxy/autoconfig_url", NULL);
		if (px_url_is_valid(tmp))
			curl = g_strdup_printf("pac+%s", tmp);
		else
			curl = g_strdup("wpad://");
		g_free(tmp);
	}

	// Mode is http://... or socks://...
	else if (!strcmp(mode, "manual"))
	{
		char *type = g_strdup("http");
		char *host = NULL;
		int   port = 0;

		// Get the per-scheme proxy settings
		if (!strcmp(px_url_get_scheme(url), "https"))
		{
			host = gconf_client_get_string(client, "/system/https_proxy/host", NULL);
			port = gconf_client_get_int   (client, "/system/https_proxy/port", NULL);
		}
		else if (!strcmp(px_url_get_scheme(url), "ftp"))
		{
			host = gconf_client_get_string(client, "/system/ftp_proxy/host", NULL);
			port = gconf_client_get_int   (client, "/system/ftp_proxy/port", NULL);
		}
		if (!host || !strcmp(host, "") || !port)
		{
			if (host) g_free(host);

			host = gconf_client_get_string(client, "/system/http_proxy/host", NULL);
			port = gconf_client_get_int   (client, "/system/http_proxy/port", NULL);
		}
		if (port < 0 || port > 65535) port = 0;

		// If http(s)/ftp proxy is not set, try socks
		if (!host || !strcmp(host, "") || !port)
		{
			if (type) g_free(type);
			if (host) g_free(host);

			type = g_strdup("socks");
			host = gconf_client_get_string(client, "/system/proxy/socks_host", NULL);
			port = gconf_client_get_int   (client, "/system/proxy/socks_port", NULL);
			if (port < 0 || port > 65535) port = 0;
		}

		// If host and port were found, build config url
		if (host && strcmp(host, "") && port)
			curl = g_strdup_printf("%s://%s:%d", type, host, port);

		if (type) g_free(type);
		if (host) g_free(host);
	}
	g_free(mode);

	char *tmp = px_strdup(curl);
	if (curl)
		g_free(curl);
	return tmp;
}

static char *
_get_ignore(pxConfigPlugin *self, pxURL *url)
{
	GSList *ignores = gconf_client_get_list(((pxGConfConfigPlugin *) self)->client,
                                            "/system/http_proxy/ignore_hosts",
											GCONF_VALUE_STRING, NULL);
	if (!ignores)
		return px_strdup("");

	GString *ignore_str = g_string_new(NULL);
	GSList  *start      = ignores;
	for ( ; ignores ; ignores = ignores->next)
	{
		if (ignore_str->len)
			g_string_append(ignore_str, ",");
		g_string_append(ignore_str, ignores->data);
		g_free(ignores->data);
	}
	g_slist_free(start);

	char *tmp = g_string_free(ignore_str, FALSE);
	char *ignore = px_strdup(tmp);
	g_free(tmp);
	return ignore;
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
	((pxGConfConfigPlugin *) self)->old_destructor = self->destructor;
	self->destructor                               = _destructor;

	((pxGConfConfigPlugin *) self)->client         = gconf_client_get_default();
	if (!((pxGConfConfigPlugin *) self)->client)
		return false;
	return true;
}

bool
px_module_load(pxPluginManager *self)
{
	// If we are running in GNOME, then make sure this plugin is registered.
	if (x_has_client("gnome-session", "gnome-settings-daemon", "gnome-panel", NULL))
	{
		g_type_init();
		return px_plugin_manager_constructor_add_subtype(self, "config_gnome", pxConfigPlugin, pxGConfConfigPlugin, _constructor);
	}
	return false;
}
