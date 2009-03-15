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

#include <misc.h>
#include <plugin_manager.h>
#include <plugin_ignore.h>

static inline bool
_endswith(char *string, char *suffix)
{
        int st_len = strlen(string);
        int su_len = strlen(suffix);

        return (st_len >= su_len && !strcmp(string + (st_len-su_len), suffix));
}

static bool
_ignore(struct _pxIgnorePlugin *self, pxURL *url, const char *ignore)
{
	if (!url || !ignore)
			return false;

	/* Get our URL's hostname and port */
	char *host = px_strdup(px_url_get_host(url));
	int   port = px_url_get_port(url);

	/* Get our ignore pattern's hostname and port */
	char *ihost = px_strdup(ignore);
	int   iport = 0;
	if (strchr(ihost, ':'))
	{
			char *tmp = strchr(ihost, ':');
			if (sscanf(tmp+1, "%d", &iport) == 1)
					*tmp  = '\0';
			else
					iport = 0;
	}

	/* Hostname match (domain.com or domain.com:80) */
	if (!strcmp(host, ihost))
			if (!iport || port == iport)
					goto match;

	/* Endswith (.domain.com or .domain.com:80) */
	if (ihost[0] == '.' && _endswith(host, ihost))
			if (!iport || port == iport)
					goto match;

	/* Glob (*.domain.com or *.domain.com:80) */
	if (ihost[0] == '*' && _endswith(host, ihost+1))
			if (!iport || port == iport)
					goto match;

	/* No match was found */
	px_free(host);
	px_free(ihost);
	return false;

	/* A match was found */
	match:
			px_free(host);
			px_free(ihost);
			return true;
}

static bool
_constructor(pxPlugin *self)
{
	((pxIgnorePlugin *) self)->ignore = _ignore;
	return true;
}

bool
px_module_load(pxPluginManager *self)
{
	return px_plugin_manager_constructor_add(self, "ignore_domain", pxIgnorePlugin, _constructor);
}
