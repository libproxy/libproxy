/*******************************************************************************
 * libproxy - A library for proxy configuration
 * Copyright (C) 2006 Nathaniel McCallum <nathaniel@natemccallum.com>
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
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
#include <config_file.h>

pxConfig *
kde_config_cb(pxProxyFactory *self)
{
	char *url = NULL, *ignore = NULL, *tmp = getenv("HOME");
	pxConfig *config;
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
	}
	
	// Anything else?  Fall back to WPAD...
	
	// Cleanup
	px_free(tmp);
	px_config_file_free(cf);
			
	out:
		config         = px_malloc0(sizeof(pxConfig));
		config->url    = url    ? url    : px_strdup("wpad://");
		config->ignore = ignore ? ignore : px_strdup("");
		return config;
}

void
kde_on_get_proxy(pxProxyFactory *self)
{
	// If we are running in KDE, then make sure this plugin is registered.
	// Otherwise, make sure this plugin is NOT registered.
	if (!system("xlsclients 2>/dev/null | grep -q '[\t ]kicker$'"))
		px_proxy_factory_config_add(self, "kde", PX_CONFIG_CATEGORY_SESSION, 
									(pxProxyFactoryPtrCallback) kde_config_cb);
	else
		px_proxy_factory_config_del(self, "kde");
	
	return;
}

bool
on_proxy_factory_instantiate(pxProxyFactory *self)
{
	// Note that we instantiate like this because SESSION config plugins
	// are only suppossed to remain registered while the application
	// is actually IN that session.  So for instance, if the app
	// was run in GNU screen and is taken out of the GNOME sesion
	// it was started in, we should handle that gracefully.
	px_proxy_factory_on_get_proxy_add(self, kde_on_get_proxy);
	return true;
}

void
on_proxy_factory_destantiate(pxProxyFactory *self)
{
	px_proxy_factory_on_get_proxy_del(self, kde_on_get_proxy);
	px_proxy_factory_config_del(self, "kde");
	px_config_file_free(px_proxy_factory_misc_get(self, "kde"));
	px_proxy_factory_misc_set(self, "kde", NULL);
}
