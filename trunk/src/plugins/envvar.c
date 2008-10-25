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
#include <proxy_factory.h>

pxConfig *get_config_cb(pxProxyFactory *self, pxURL *url)
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

	return px_config_create(px_strdup(proxy), px_strdup(getenv("no_proxy")));
}

bool on_proxy_factory_instantiate(pxProxyFactory *self)
{
	return px_proxy_factory_config_add(self, "envvar", PX_CONFIG_CATEGORY_NONE, get_config_cb);
}

void on_proxy_factory_destantiate(pxProxyFactory *self)
{
	px_proxy_factory_config_del(self, "envvar");
}
