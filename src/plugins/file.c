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


pxConfig *get_config_from_file(char *filename)
{
	return NULL;
}

pxConfig *system_get_config_cb(pxProxyFactory *self)
{
	return get_config_from_file("/etc/proxy.conf");
}

pxConfig *user_get_config_cb(pxProxyFactory *self)
{
	return get_config_from_file("~/.proxy.conf");
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
}
