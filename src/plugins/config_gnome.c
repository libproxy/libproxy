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

#include <glib.h>
#include <gconf/gconf-client.h>

// From xhasclient.c
bool x_has_client(char *prog, ...);

typedef struct _pxGConfConfigPlugin {
	PX_PLUGIN_SUBCLASS(pxConfigPlugin);
	GConfClient  *client;
#ifdef G_THREADS_ENABLED
	GMutex       *mutex;
#endif
	char         *mode;
	char         *autoconfig_url;
	char         *http_host;
	int           http_port;
	char         *https_host;
	int           https_port;
	char         *ftp_host;
	int           ftp_port;
	char         *socks_host;
	int           socks_port;
	char         *ignore_hosts;
	void        (*old_destructor)(pxPlugin *);
} pxGConfConfigPlugin;

static const char *_all_keys[] = {
        "/system/proxy/mode",       "/system/proxy/autoconfig_url",
        "/system/http_proxy/host",  "/system/http_proxy/port",
        "/system/https_proxy/host", "/system/https_proxy/port",
        "/system/ftp_proxy/host",   "/system/ftp_proxy/port",
        "/system/socks_proxy/host", "/system/socks_proxy/port",
        "/system/http_proxy/ignore_hosts", NULL
      };

static void
_on_key_change(GConfClient *client, guint cnxn_id, GConfEntry *entry, gpointer user_data)
{
	pxGConfConfigPlugin *self = (pxGConfConfigPlugin *) user_data;

#ifdef G_THREADS_ENABLED
	if (self->mutex)
		g_mutex_lock(self->mutex);
#endif

	if (!strcmp(entry->key, "/system/proxy/mode")) {
		g_free(self->mode);
		self->mode = entry->value ? g_strdup(gconf_value_get_string(entry->value)) : NULL;
	}
	else if (!strcmp(entry->key, "/system/proxy/autoconfig_url")) {
		g_free(self->autoconfig_url);
		self->autoconfig_url = entry->value ? g_strdup(gconf_value_get_string(entry->value)) : NULL;
	}
	else if (!strcmp(entry->key, "/system/http_proxy/host")) {
		g_free(self->http_host);
		self->http_host = entry->value ? g_strdup(gconf_value_get_string(entry->value)) : NULL;
	}
	else if (!strcmp(entry->key, "/system/http_proxy/port"))
		self->http_port = entry->value ? gconf_value_get_int(entry->value) : 0;
	else if (!strcmp(entry->key, "/system/https_proxy/host")) {
		g_free(self->https_host);
		self->https_host = entry->value ? g_strdup(gconf_value_get_string(entry->value)) : NULL;
	}
	else if (!strcmp(entry->key, "/system/https_proxy/port"))
		self->https_port = entry->value ? gconf_value_get_int(entry->value) : 0;
	else if (!strcmp(entry->key, "/system/ftp_proxy/host")) {
		g_free(self->ftp_host);
		self->ftp_host = entry->value ? g_strdup(gconf_value_get_string(entry->value)) : NULL;
	}
	else if (!strcmp(entry->key, "/system/ftp_proxy/port"))
		self->ftp_port = entry->value ? gconf_value_get_int(entry->value) : 0;
	else if (!strcmp(entry->key, "/system/socks_proxy/host")) {
		g_free(self->socks_host);
		self->socks_host = entry->value ? g_strdup(gconf_value_get_string(entry->value)) : NULL;
	}
	else if (!strcmp(entry->key, "/system/socks_proxy/port"))
		self->socks_port = entry->value ? gconf_value_get_int(entry->value) : 0;
	else if (!strcmp(entry->key, "/system/http_proxy/ignore_hosts"))
	{
		g_free(self->ignore_hosts);
		GSList *ignores = gconf_value_get_list(entry->value);
		if (!ignores || g_slist_length(ignores) == 0)
			self->ignore_hosts = px_strdup("");
		else
		{
			GString *ignore_str = g_string_new(NULL);
			GSList  *start      = ignores;
			for ( ; ignores ; ignores = ignores->next)
			{
				if (ignore_str->len)
					g_string_append(ignore_str, ",");
				g_string_append(ignore_str, ignores->data);
				g_free(ignores->data);
			}

			self->ignore_hosts = g_string_free(ignore_str, FALSE);
		}
	}

#ifdef G_THREADS_ENABLED
	if (self->mutex)
		g_mutex_unlock(self->mutex);
#endif
}

static void
_destructor(pxPlugin *self)
{
#ifdef G_THREADS_ENABLED
	if (((pxGConfConfigPlugin *) self)->mutex)
	{
		g_mutex_lock(((pxGConfConfigPlugin *) self)->mutex);
		g_mutex_free(((pxGConfConfigPlugin *) self)->mutex);
	}
#endif
	g_object_unref(((pxGConfConfigPlugin *) self)->client);
	g_free(((pxGConfConfigPlugin *) self)->mode);
	g_free(((pxGConfConfigPlugin *) self)->autoconfig_url);
	g_free(((pxGConfConfigPlugin *) self)->http_host);
	g_free(((pxGConfConfigPlugin *) self)->https_host);
	g_free(((pxGConfConfigPlugin *) self)->ftp_host);
	g_free(((pxGConfConfigPlugin *) self)->socks_host);
	g_free(((pxGConfConfigPlugin *) self)->ignore_hosts);
	((pxGConfConfigPlugin *) self)->old_destructor(self);
}

static char *
_get_config(pxConfigPlugin *self, pxURL *url)
{
	/* We are not in a main loop */
	if (g_main_depth() <= 0)
	{
		for (int i=0 ; _all_keys[i] ; i++)
		{
			/* Get the defaults */
			GConfEntry ignores;
			ignores.key = (char *) _all_keys[i];
			ignores.value = gconf_client_get(((pxGConfConfigPlugin *) self)->client, ignores.key, NULL);
			_on_key_change(((pxGConfConfigPlugin *) self)->client, 0, &ignores, self);
			gconf_value_free(ignores.value);
		}
	}

#ifdef G_THREADS_ENABLED
	if (((pxGConfConfigPlugin *) self)->mutex)
		g_mutex_lock(((pxGConfConfigPlugin *) self)->mutex);
#endif
	GConfClient *client = ((pxGConfConfigPlugin *) self)->client;

	// Make sure we have a mode
	if (!((pxGConfConfigPlugin *) self)->mode)
		return NULL;

	char *curl = NULL;

	// Mode is direct://
	if (!strcmp(((pxGConfConfigPlugin *) self)->mode, "none"))
		curl = px_strdup("direct://");

	// Mode is wpad:// or pac+http://...
	else if (!strcmp(((pxGConfConfigPlugin *) self)->mode, "auto"))
	{
		if (px_url_is_valid(((pxGConfConfigPlugin *) self)->autoconfig_url))
			curl = g_strdup_printf("pac+%s", ((pxGConfConfigPlugin *) self)->autoconfig_url);
		else
			curl = g_strdup("wpad://");
	}

	// Mode is http://... or socks://...
	else if (!strcmp(((pxGConfConfigPlugin *) self)->mode, "manual"))
	{
		char *type = g_strdup("http");
		char *host = NULL;
		int   port = 0;

		// Get the per-scheme proxy settings
		if (!strcmp(px_url_get_scheme(url), "https"))
		{
			host = ((pxGConfConfigPlugin *) self)->https_host;
			port = ((pxGConfConfigPlugin *) self)->https_port;
		}
		else if (!strcmp(px_url_get_scheme(url), "ftp"))
		{
			host = ((pxGConfConfigPlugin *) self)->ftp_host;
			port = ((pxGConfConfigPlugin *) self)->ftp_port;
		}
		if (!host || !strcmp(host, "") || !port)
		{
			if (host) g_free(host);

			host = ((pxGConfConfigPlugin *) self)->http_host;
			port = ((pxGConfConfigPlugin *) self)->http_port;
		}
		if (port < 0 || port > 65535) port = 0;

		// If http(s)/ftp proxy is not set, try socks
		if (!host || !strcmp(host, "") || !port)
		{
			if (type) g_free(type);
			if (host) g_free(host);

			type = g_strdup("socks");
			host = ((pxGConfConfigPlugin *) self)->socks_host;
			port = ((pxGConfConfigPlugin *) self)->socks_port;
			if (port < 0 || port > 65535) port = 0;
		}

		// If host and port were found, build config url
		if (host && strcmp(host, "") && port)
			curl = g_strdup_printf("%s://%s:%d", type, host, port);

		g_free(type);
		g_free(host);
	}

	char *tmp = px_strdup(curl);
	if (curl)
		g_free(curl);

#ifdef G_THREADS_ENABLED
		if (((pxGConfConfigPlugin *) self)->mutex)
			g_mutex_unlock(((pxGConfConfigPlugin *) self)->mutex);
#endif

	return tmp;
}

static char *
_get_ignore(pxConfigPlugin *self, pxURL *url)
{
	/* We are not in a main loop */
	if (g_main_depth() <= 0)
	{
		/* Get the default ignore list */
		GConfEntry ignores;
		ignores.key = (char *) "/system/http_proxy/ignore_hosts";
		ignores.value = gconf_client_get(((pxGConfConfigPlugin *) self)->client, ignores.key, NULL);
		_on_key_change(((pxGConfConfigPlugin *) self)->client, 0, &ignores, self);
		gconf_value_free(ignores.value);
	}

#ifdef G_THREADS_ENABLED
	if (((pxGConfConfigPlugin *) self)->mutex)
		g_mutex_lock(((pxGConfConfigPlugin *) self)->mutex);
#endif

	char *ignores = px_strdup(((pxGConfConfigPlugin *) self)->ignore_hosts);
	if (!ignores)
		ignores = px_strdup("");

#ifdef G_THREADS_ENABLED
		if (((pxGConfConfigPlugin *) self)->mutex)
			g_mutex_unlock(((pxGConfConfigPlugin *) self)->mutex);
#endif
	return ignores;
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

	/* Get the default gconf client */
	((pxGConfConfigPlugin *) self)->client = gconf_client_get_default();
	if (!((pxGConfConfigPlugin *) self)->client)
		return false;

    /* Enable our mutex */
#ifdef G_THREADS_ENABLED
	if (g_thread_get_initialized())
		((pxGConfConfigPlugin *) self)->mutex = g_mutex_new();
#endif

	/* Get our default values */
	for (int i=0 ; _all_keys[i] ; i++)
	{
		/* Setup our notifications for change */
		char *dir = g_strndup(_all_keys[i], strrchr(_all_keys[i], '/') - _all_keys[i]);
		gconf_client_add_dir(((pxGConfConfigPlugin *) self)->client, dir, GCONF_CLIENT_PRELOAD_NONE, NULL);
		g_free(dir);
		gconf_client_notify_add(((pxGConfConfigPlugin *) self)->client, _all_keys[i], _on_key_change, self, NULL, NULL);

		/* Do a first read of the values */
		gconf_client_notify(((pxGConfConfigPlugin *) self)->client, _all_keys[i]);
	}

	((pxGConfConfigPlugin *) self)->old_destructor = self->destructor;
	self->destructor                               = _destructor;

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
