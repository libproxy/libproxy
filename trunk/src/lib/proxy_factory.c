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
#include <assert.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdio.h>
#include <dlfcn.h>
#include <math.h>

#include "misc.h"
#include "proxy_factory.h"

#define PLUGIN_DIR "/usr/lib/proxy/" VERSION "/plugins"

struct _pxProxyFactory {
	pxConfigBackend             mask;
	void                      **plugins;
	PXProxyFactoryPtrCallback  *configs;
	PXProxyFactoryVoidCallback *on_get_proxy;
	PXPACRunnerCallback         pac_runner;
};

pxProxyFactory *
px_proxy_factory_new (pxConfigBackend config)
{
	pxProxyFactory *self = px_malloc0(sizeof(pxProxyFactory));
	self->configs        = px_malloc0(sizeof(PXProxyFactoryPtrCallback) * (((int) log2(PX_CONFIG_BACKEND__LAST)) + 1));
	self->mask           = config;
	unsigned int i;
	
	// Open the plugin dir
	DIR *plugindir = opendir(PLUGIN_DIR);
	if (!plugindir)
	{
		px_proxy_factory_free(self);
		return NULL;
	}
	
	// Count the number of plugins
	for (i=0 ; readdir(plugindir) ; i++);
	self->plugins = (void **) px_malloc0(sizeof(void *) * (i + 1));
	rewinddir(plugindir);
	
	// For each plugin...
	struct dirent *ent;
	for (i=0 ; (ent = readdir(plugindir)) ; i++)
	{
		// Load the plugin
		char *tmp = px_malloc0(strlen(PLUGIN_DIR) + strlen(ent->d_name) + 2);
		sprintf(tmp, PLUGIN_DIR "/%s", ent->d_name);
		self->plugins[i] = dlopen(tmp, RTLD_LOCAL);
		px_free(tmp);
		if (!(self->plugins[i]))
		{
			i--;
			continue;
		}
		
		// Call the instantiation hook
		PXProxyFactoryBoolCallback instantiate;
		instantiate = dlsym(self->plugins[i], "on_proxy_factory_instantiate");
		if (instantiate && !instantiate(self))
		{
			dlclose(self->plugins[i]);
			self->plugins[i--] = NULL;
			continue;
		}
	}
	closedir(plugindir);
	
	// Make sure we have at least one config
	if (px_proxy_factory_config_get_active(self) == PX_CONFIG_BACKEND_NONE)
	{
		px_proxy_factory_free(self);
		return NULL;
	}
	
	return self;
}

bool
px_proxy_factory_config_set (pxProxyFactory *self, pxConfigBackend config, PXProxyFactoryPtrCallback callback)
{
	if ((config ^ self->mask) != config || self->mask == PX_CONFIG_BACKEND_AUTO) return false;
	if (self->configs[((int) log2(config))] && callback && self->configs[((int) log2(config))] != callback) return false;
	self->configs[((int) log2(config))] = callback;
	return true;
}

pxConfigBackend
px_proxy_factory_config_get_active (pxProxyFactory *self)
{
	// Find the first provided config
	for (unsigned int i=0 ; i <= ((int) log2(PX_CONFIG_BACKEND__LAST)) ; i++)
		if (self->configs[i])
			return (1 << i);
			
	// No valid config found!
	return PX_CONFIG_BACKEND_NONE;
}

char *
px_proxy_factory_get_proxy (pxProxyFactory *self, char *url)
{
	return NULL;
}

void
px_proxy_factory_on_get_proxy_add (pxProxyFactory *self, PXProxyFactoryVoidCallback callback)
{
	unsigned int length = 0;
	PXProxyFactoryVoidCallback *tmp = NULL;

	// Backup our current callbacks (NULL if none)
	tmp = self->on_get_proxy;
	
	// If we have any callbacks, count them
	// If user is trying to add an already existing callback, just ignore it
	if (self->on_get_proxy)
		for (length=0 ; self->on_get_proxy[length] ; length++)
			if (callback == self->on_get_proxy[length])
				return;
	
	// Allocate enough space for the old + new callbacks
	self->on_get_proxy = px_malloc0(sizeof(PXProxyFactoryVoidCallback) * (length + 2));
	
	// Copy old callbacks into new memory (if any)
	if (tmp)
		memcpy(self->on_get_proxy, tmp, length);
	
	// Add the new callback
	self->on_get_proxy[length] = callback;
	
	// Free old memory (if any)
	px_free(tmp);
}

void
px_proxy_factory_on_get_proxy_del (pxProxyFactory *self, PXProxyFactoryVoidCallback callback)
{
	unsigned int length = 0, count = 0;
	PXProxyFactoryVoidCallback *tmp = NULL;
	
	// Sanity checks, get length and count
	if (!self->on_get_proxy) return;
	for (length=0,count=0 ; self->on_get_proxy[length] ; length++)
		if (self->on_get_proxy[length] == callback)
			count++;
	if (length < 1 || count < 1) return;
	
	// Backup current callbacks
	tmp = self->on_get_proxy;
	
	// Allocate new array
	self->on_get_proxy = px_malloc0(sizeof(PXProxyFactoryVoidCallback) * (length - count + 1));
	
	// Copy in new values
	int i,j;
	for (i=0,j=0 ; tmp[i] ; i++)
		if (tmp[i] != callback)
			self->on_get_proxy[j++] = tmp[i];
	
	// Free old memory
	px_free(tmp);
}

bool
px_proxy_factory_pac_runner_set (pxProxyFactory *self, PXPACRunnerCallback callback)
{
	if (self->pac_runner && callback && self->pac_runner != callback) return false;
	self->pac_runner = callback;
	return true;
}

void
px_proxy_factory_free (pxProxyFactory *self)
{
	unsigned int i;
	
	if (!self) return;
	
	// Free the plugins
	if (self->plugins)
	{
		for (i=0 ; self->plugins[i] ; i++)
		{
			// Call the destantiation hook
			PXProxyFactoryVoidCallback destantiate;
			destantiate = dlsym(self->plugins[i], "on_proxy_factory_destantiate");
			if (destantiate)
				destantiate(self);
			
			// Unload the plugin
			dlclose(self->plugins[i]);
			self->plugins[i] = NULL;
		}
		px_free(self->plugins);
	}
	
	// Free everything else
	if (self->configs)      px_free(self->configs);
	if (self->on_get_proxy) px_free(self->on_get_proxy);
	px_free(self);
}

