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
#include <config_file.h>


pxConfig *get_config_from_file(pxProxyFactory *self, char *misc, char *filename)
{
	pxConfigFile *cf = px_proxy_factory_misc_get(self, misc);
	if (!cf || px_config_file_is_stale(cf))
	{
		if (cf) px_config_file_free(cf);
		cf = px_config_file_new(filename);
		px_proxy_factory_misc_set(self, misc, cf);
	}
	
	if (!cf) return NULL;
	return px_config_create(
				px_config_file_get_value(cf, PX_CONFIG_FILE_DEFAULT_SECTION, "proxy"),
				px_config_file_get_value(cf, PX_CONFIG_FILE_DEFAULT_SECTION, "ignore"));
}

pxConfig *system_get_config_cb(pxProxyFactory *self)
{
	return get_config_from_file(self, "file_system", SYSCONFDIR "/proxy.conf");
}

pxConfig *user_get_config_cb(pxProxyFactory *self)
{
	char *tmp = getenv("HOME");
	if (!tmp) return NULL;
	char *filename = px_strcat(tmp, "/", ".proxy.conf", NULL);
	pxConfig *config = get_config_from_file(self, "file_user", filename);
	px_free(filename);
	return config;
}

bool on_proxy_factory_instantiate(pxProxyFactory *self)
{
	bool a, b;
	a = px_proxy_factory_config_add(self, "file_system", PX_CONFIG_CATEGORY_SYSTEM, 
										(pxProxyFactoryPtrCallback) system_get_config_cb);
	b = px_proxy_factory_config_add(self, "file_user", PX_CONFIG_CATEGORY_USER, 
										(pxProxyFactoryPtrCallback) user_get_config_cb);
	return (a || b);
}

void on_proxy_factory_destantiate(pxProxyFactory *self)
{
	px_proxy_factory_config_del(self, "file_system");
	px_proxy_factory_config_del(self, "file_user");
	px_config_file_free(px_proxy_factory_misc_get(self, "file_system"));
	px_config_file_free(px_proxy_factory_misc_get(self, "file_user"));
	px_proxy_factory_misc_set(self, "file_system", NULL);
	px_proxy_factory_misc_set(self, "file_user", NULL);
}
