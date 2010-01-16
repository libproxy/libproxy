/*******************************************************************************
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

#include <windows.h>

#include <stdlib.h>
#include <string.h>

#include "../misc.h"
#include "../modules.h"

#define W32REG_OFFSET_PAC  (1 << 2)
#define W32REG_OFFSET_WPAD (1 << 3)
#define W32REG_BASEKEY "Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings"

static bool
_get_registry(const char *key, const char *name, uchar **sval, uint32_t *slen, uint32_t *ival)
{
	HKEY  hkey;
	LONG  result;
	DWORD type;
	DWORD buflen = 1024;
	BYTE  buffer[buflen];

	// Don't allow the caller to specify both sval and ival
	if (sval && ival)
		return false;
	
	// Open the key
	if (RegOpenKeyExA(HKEY_CURRENT_USER, key, 0, KEY_READ, &hkey) != ERROR_SUCCESS)
		return false;

	// Read the value
	result = RegQueryValueExA(hkey, name, NULL, &type, buffer, &buflen);

	// Close the key
	RegCloseKey(hkey);

	// Evaluate
	if (result != ERROR_SUCCESS)
		return false;
	switch (type)
	{
		case REG_BINARY:
		case REG_EXPAND_SZ:
		case REG_SZ:
			if (!sval) return false;
			if (slen) *slen = buflen;
			*sval = px_malloc(buflen);
			return !memcpy_s(*sval, buflen, buffer, buflen);
		case REG_DWORD:
			if (ival) return !memcpy_s(ival, sizeof(uint32_t), buffer, buflen);
	}
	return false;
}


static bool
_is_enabled(uint8_t type)
{
	uchar    *data   = NULL;
	uint32_t  dlen   = 0;
	bool      result = false;

	// Get the binary value DefaultConnectionSettings
	if (!_get_registry(W32REG_BASEKEY "\\Connections", "DefaultConnectionSettings", &data, &dlen, NULL))
		return false;

	// WPAD and PAC are contained in the 9th value
	if (dlen >= 9)
		result = data[8] & type == type; // Check to see if the bit is set

	px_free(data);
	return result;
}

static char *
_get_pac()
{
	char *pac = NULL;
	char *url = NULL;

	if (!_is_enabled(W32REG_OFFSET_PAC)) return NULL;
	if (!_get_registry(W32REG_BASEKEY, "AutoConfigURL", &pac, NULL, NULL)) return NULL;

	url = px_strcat("pac+", pac, NULL);
	px_free(pac);
	return url;
}

static char *
_get_manual(pxURL *url)
{
	char      *val     = NULL;
	char      *url     = NULL;
	char     **vals    = NULL;
	uint32_t   enabled = 0;

	// Check to see if we are enabled
	if (!_get_registry(W32REG_BASEKEY, "ProxyEnable", NULL, NULL, &enabled) || !enabled) return NULL;

	// Get the value of ProxyServer
	if (!_get_registry(W32REG_BASEKEY, "ProxyServer", &val, NULL, NULL)) return NULL;

	// ProxyServer comes in two formats:
	//   1.2.3.4:8080 or ftp=1.2.3.4:8080;https=1.2.3.4:8080...
	// If we have the first format, just prepend "http://" and we're done
	if (!strchr(val, ';'))
	{
		url = px_strcat("http://", val, NULL);
		px_free(val);
		return url;
	}

	// Ok, now we have the more difficult format...
	// ... so split up all the various proxies
	vals = px_strsplit(val, ";");
	px_free(val);

	// First we look for an exact match
	for (int i=0 ; !url && vals[i] ; i++)
	{
		char *tmp = px_strcat(px_url_get_scheme(url), "=", NULL);

		// If we've found an exact match, use it
		if (!strncmp(tmp, vals[i], strlen(tmp)))
			url = px_strcat(px_url_get_scheme(url), "://", vals[i]+strlen(tmp), NULL);

		px_free(tmp);
	}

	// Second we look for http=
	for (int i=0 ; !url && vals[i] ; i++)
		if (!strncmp("http=", vals[i], strlen("http=")))
			url = px_strcat("http://", vals[i]+strlen("http="), NULL);

	// Last we look for socks=
	for (int i=0 ; !url && vals[i] ; i++)
		if (!strncmp("socks=", vals[i], strlen("socks=")))
			url = px_strcat("socks://", vals[i]+strlen("socks="), NULL);

	return url;
}

static char *
_get_config(pxConfigModule *self, pxURL *url)
{
	char *config = NULL;

	// WPAD
	if (_is_enabled(W32REG_OFFSET_WPAD))
		return px_strdup("wpad://");

	// PAC
	if ((config = _get_pac()))
		return config;

	// Manual proxy
	if ((config = _get_manual(url)))
		return config;

	// Direct
	return px_strdup("direct://");
}

static char *
_get_ignore(pxConfigModule *self, pxURL *url)
{
	return px_strdup("");
}

static bool
_get_credentials(pxConfigModule *self, pxURL *url, char **username, char **password)
{
	return false;
}

static bool
_set_credentials(pxConfigModule *self, pxURL *url, const char *username, const char *password)
{
	return false;
}

static void *
_constructor()
{
	pxConfigModule *self = px_malloc0(sizeof(pxConfigModule));
	PX_CONFIG_MODULE_BUILD(self, PX_CONFIG_MODULE_CATEGORY_USER, _get_config, _get_ignore, _get_credentials, _set_credentials);
	return self;
}

bool
px_module_load(pxModuleManager *self)
{
	return px_module_manager_register_module(self, pxConfigModule, _constructor, px_free);
}
