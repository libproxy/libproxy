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
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

#include "../misc.hpp"
#include "../modules.hpp"
#include "../strdict.hpp"
#include "xhasclient.cpp"

#define BUFFERSIZE 10240
#define CACHETIME 5

typedef struct _pxGConfConfigModule {
	PX_MODULE_SUBCLASS(pxConfigModule);
	FILE      *pipe;
	pxStrDict *data;
	time_t     last;
} pxGConfConfigModule;

static const char *_all_keys[] = {
	"/system/proxy/mode",       "/system/proxy/autoconfig_url",
	"/system/http_proxy/host",  "/system/http_proxy/port",
	"/system/proxy/secure_host", "/system/proxy/secure_port",
	"/system/proxy/ftp_host",   "/system/proxy/ftp_port",
	"/system/proxy/socks_host", "/system/proxy/socks_port",
	"/system/http_proxy/ignore_hosts",
	"/system/http_proxy/use_authentication",
	"/system/http_proxy/authentication_user",
	"/system/http_proxy/authentication_password", NULL
};

static FILE *
_start_get_config()
{
	char buffer[BUFFERSIZE] = "";

	// Build our command
	if (strlen(GCONFTOOLBIN " -g") + 1 > BUFFERSIZE)
		return NULL;
	strcpy(buffer, GCONFTOOLBIN " -g");
	for (int i=0 ; _all_keys[i] ; i++)
	{
		if (strlen(buffer) + strlen(_all_keys[i]) + 2 > BUFFERSIZE)
			return NULL;
		strcat(buffer, " ");
		strcat(buffer, _all_keys[i]);
	}
	if (strlen(buffer) + strlen(" 2>&1") + 1 > BUFFERSIZE)
		return NULL;
	strcat(buffer, " 2>&1");

	// Open our pipe
	return popen(buffer, "r");
}

static pxStrDict *
_finish_get_config(FILE *pipe)
{
	char buffer[BUFFERSIZE] = "";
	char **values = NULL;
	pxStrDict *kv = NULL;

	if (!pipe) return NULL;

	// Read the output and split it into its separate values (one per line)
	if (fread(buffer, sizeof(char), BUFFERSIZE, pipe) == 0) goto error;
	if (!(values = px_strsplit(buffer, "\n")))              goto error;

	// Build up our dictionary with the values
	kv = px_strdict_new((pxStrDictItemCallback) px_free);
	for (int i=0 ; _all_keys[i] ; i++)
	{
		if (!values[i])
			goto error;
		if (strchr(values[i], ' '))
			strcpy(values[i], "");
		if (!px_strdict_set(kv, _all_keys[i], px_strdup(values[i])))
			goto error;
	}

	// Cleanup
	px_strfreev(values);
	if (pclose(pipe) < 0)
	{
		px_strdict_free(kv);
		return NULL;
	}
	return kv;

	error:
		pclose(pipe);
		px_strfreev(values);
		px_strdict_free(kv);
		return NULL;
}

static void
_destructor(void *s)
{
	pxGConfConfigModule *self = (pxGConfConfigModule *) s;

	if (self->pipe) pclose(self->pipe);
	px_strdict_free(self->data);
	px_free(self);
}

static char *
_get_config(pxConfigModule *s, pxURL *url)
{
	pxGConfConfigModule *self = (pxGConfConfigModule *) s;

	// Update our config if possible
	if (self->pipe)
	{
		pxStrDict *tmp = _finish_get_config(self->pipe);
		self->pipe = NULL;
		if (tmp)
		{
			px_strdict_free(self->data);
			self->data = tmp;
			self->last = time(NULL);
		}
	}

	if (!px_strdict_get(self->data, "/system/proxy/mode"))
		return NULL;

	char *curl = NULL;

	// Mode is direct://
	if (!strcmp((const char *) px_strdict_get(self->data, "/system/proxy/mode"), "none"))
		curl = px_strdup("direct://");

	// Mode is wpad:// or pac+http://...
	else if (!strcmp((const char *) px_strdict_get(self->data, "/system/proxy/mode"), "auto"))
	{
		if (px_url_is_valid((const char *) px_strdict_get(self->data, "/system/proxy/autoconfig_url")))
			curl = px_strcat("pac+", px_strdict_get(self->data, "/system/proxy/autoconfig_url"), NULL);
		else
			curl = px_strdup("wpad://");
	}

	// Mode is http://... or socks://...
	else if (!strcmp((const char *) px_strdict_get(self->data, "/system/proxy/mode"), "manual"))
	{
		char *type = px_strdup("http");
		char *host = NULL;
		char *port = NULL;
		char *username = NULL;
		char *password = NULL;

		uint16_t p = 0;

		// Get the per-scheme proxy settings
		if (!strcmp(px_url_get_scheme(url), "https"))
		{
			host = px_strdup((const char *) px_strdict_get(self->data, "/system/proxy/secure_host"));
			port = px_strdup((const char *) px_strdict_get(self->data, "/system/proxy/secure_port"));
			if (!port || sscanf(port, "%hu", &p) != 1) p = 0;
		}
		else if (!strcmp(px_url_get_scheme(url), "ftp"))
		{
			host = px_strdup((const char *) px_strdict_get(self->data, "/system/proxy/ftp_host"));
			port = px_strdup((const char *) px_strdict_get(self->data, "/system/proxy/ftp_port"));
			if (!port || sscanf(port, "%hu", &p) != 1) p = 0;
		}
		if (!host || !strcmp(host, "") || !p)
		{
			px_free(host);
			px_free(port);

			host = px_strdup((const char *) px_strdict_get(self->data, "/system/http_proxy/host"));
			port = px_strdup((const char *) px_strdict_get(self->data, "/system/http_proxy/port"));
			if (!strcmp((const char *) px_strdict_get(self->data, "/system/http_proxy/use_authentication"), "true")) {
				username = px_strdup((const char *) px_strdict_get(self->data, "/system/http_proxy/authentication_user"));
				password = px_strdup((const char *) px_strdict_get(self->data, "/system/http_proxy/authentication_password"));
			}
			if (!port || sscanf(port, "%hu", &p) != 1) p = 0;
		}

		// If http(s)/ftp proxy is not set, try socks
		if (!host || !strcmp(host, "") || !p)
		{
			px_free(type);
			px_free(host);
			px_free(port);

			type = px_strdup("socks");
			host = px_strdup((const char *) px_strdict_get(self->data, "/system/proxy/socks_host"));
			port = px_strdup((const char *) px_strdict_get(self->data, "/system/proxy/socks_port"));
			if (!port || sscanf(port, "%hu", &p) != 1) p = 0;
		}

		// If host and port were found, build config url
		if (host && strcmp(host, "") && p)
			curl = px_strcat(type, "://", username && password ? px_strcat(username, ":", password, "@", NULL) : "", host, ":", port, NULL);

		px_free(type);
		px_free(host);
		px_free(port);
		px_free(username);
		px_free(password);
	}

	// Start a refresh in the background
	if (time(NULL) - self->last > CACHETIME)
		self->pipe = _start_get_config();

	return curl;
}

static char *
_get_ignore(pxConfigModule *s, pxURL *url)
{
	pxGConfConfigModule *self = (pxGConfConfigModule *) s;

	char *ignores = px_strdup((const char *) px_strdict_get(self->data, "/system/http_proxy/ignore_hosts"));
	if (ignores && ignores[strlen(ignores)-1] == ']' && ignores[0] == '[')
	{
		char *tmp = px_strndup(ignores+1, strlen(ignores+1)-1);
		px_free(ignores);
		ignores = tmp;
	}

	return ignores ? ignores : px_strdup("");
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
	pxGConfConfigModule *self = (pxGConfConfigModule *) px_malloc0(sizeof(pxGConfConfigModule));
	PX_CONFIG_MODULE_BUILD(self, PX_CONFIG_MODULE_CATEGORY_SESSION, _get_config, _get_ignore, _get_credentials, _set_credentials);
	self->pipe = _start_get_config();
	return self;
}

bool
px_module_load(pxModuleManager *self)
{
	// If we are running in GNOME, then make sure this plugin is registered.
	if (!x_has_client("gnome-session", "gnome-settings-daemon", "gnome-panel", NULL))
		return false;
	return px_module_manager_register_module(self, pxConfigModule, _constructor, _destructor);
}
