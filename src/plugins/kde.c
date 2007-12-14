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
#include <proxy_factory.h>
#include <config_file.h>

#include <X11/Xlib.h>
#include <X11/Xmu/WinUtil.h>

// From xhasclient.c
bool x_has_client(char *prog, ...);

pxConfig *
kde_config_cb(pxProxyFactory *self)
{
	// TODO: make ignores work w/ KDE
	char *url = NULL, *ignore = NULL, *tmp = getenv("HOME");
	if (!tmp) return NULL;
	
	// Open the config file
	pxConfigFile *cf = px_proxy_factory_misc_get(self, "kde");
	if (!cf || px_config_file_is_stale(cf))
	{
		if (cf) px_config_file_free(cf);
		tmp = px_strcat(getenv("HOME"), "/.kde/share/config/kioslaverc", NULL);
		cf = px_config_file_new(tmp);
		px_free(tmp);
		px_proxy_factory_misc_set(self, "kde", cf);
	}
	if (!cf)  goto out;
	
	// Read the config file to find out what type of proxy to use
	tmp = px_config_file_get_value(cf, "Proxy Settings", "ProxyType");
	if (!tmp) { px_config_file_free(cf); goto out; }
	
	// Don't use any proxy
	if (!strcmp(tmp, "0"))
		url = px_strdup("direct://");
		
	// Use a manual proxy
	else if (!strcmp(tmp, "1"))
		url = px_config_file_get_value(cf, "Proxy Settings", "httpProxy");
		
	// Use a manual PAC
	else if (!strcmp(tmp, "2"))
	{
		px_free(tmp);
		tmp = px_config_file_get_value(cf, "Proxy Settings", "Proxy Config Script");
		if (tmp) url = px_strcat("pac+", tmp);
		else     url = px_strdup("wpad://");
	}
	
	// Use WPAD
	else if (!strcmp(tmp, "3"))
		url = px_strdup("wpad://");
	
	// Use envvar
	else if (!strcmp(tmp, "4"))
		url = NULL; // We'll bypass this config plugin and let the envvar plugin work
	
	// Cleanup
	px_free(tmp);
	px_config_file_free(cf);
			
	out:
		return px_config_create(url, ignore);
}

bool
on_proxy_factory_instantiate(pxProxyFactory *self)
{
	// If we are running in KDE, then make sure this plugin is registered.
	if (x_has_client("kicker", NULL))
		return px_proxy_factory_config_add(self, "kde", PX_CONFIG_CATEGORY_SESSION, 
											(pxProxyFactoryPtrCallback) kde_config_cb);
	return false;
}

void
on_proxy_factory_destantiate(pxProxyFactory *self)
{
	px_proxy_factory_config_del(self, "kde");
	px_config_file_free(px_proxy_factory_misc_get(self, "kde"));
	px_proxy_factory_misc_set(self, "kde", NULL);
}
