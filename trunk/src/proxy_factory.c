#include "proxy_plugin.h"

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdio.h>
#include <dlfcn.h>
#include <math.h>

#ifdef WIN32
#  define PROXY_IMPORT __declspec(dllimport)
#  define PROXY_EXPORT __declspec(dllexport)
#else
#  define PROXY_IMPORT __attribute__((visibility("default")))
#  define PROXY_EXPORT __attribute__((visibility("default")))
#endif

#define PLUGIN_DIR "/usr/lib/proxy/" VERSION "/plugins"

struct _PXProxyFactory {
	PXConfigBackend          mask;
	void                   **plugins;
	PXProxyFactoryCallback  *configs;
	PXProxyFactoryCallback  *on_get_proxy;
	PXPACRunnerCallback     *pac_runners;
};

static void *proxy_malloc (size_t size)
{
	void *tmp = malloc(size);
	assert(tmp != NULL);
	memset(tmp, 0, size);
	return tmp;
}

static void proxy_free (void *mem)
{
	if (!mem) return;
	free(mem);
}

PROXY_EXPORT PXProxyFactory *px_proxy_factory_new (PXConfigBackend config)
{
	PXProxyFactory *self = proxy_malloc(sizeof(PXProxyFactory));
	self->configs        = proxy_malloc(sizeof(PXProxyFactoryCallback) * (((int) log2(PX_CONFIG_BACKEND__LAST)) + 1));
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
	self->plugins = (void **) proxy_malloc(sizeof(void *) * (i + 1));
	rewinddir(plugindir);
	
	// For each plugin...
	struct dirent *ent;
	for (i=0 ; ent = readdir(plugindir) ; i++)
	{
		// Load the plugin
		char *tmp = proxy_malloc(strlen(PLUGIN_DIR) + strlen(ent->d_name) + 2);
		sprintf(tmp, PLUGIN_DIR "/%s", ent->d_name);
		self->plugins[i] = dlopen(tmp, RTLD_LOCAL);
		proxy_free(tmp);
		if (!(self->plugins[i]))
		{
			i--;
			continue;
		}
		
		// Call the instantiation hook
		PXProxyFactoryCallback instantiate;
		instantiate = dlsym(self->plugins[i], "on_proxyfactory_instantiate");
		if (instantiate)
			instantiate(self);
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

PROXY_EXPORT void px_proxy_factory_config_set (PXProxyFactory *self, PXConfigBackend config, PXProxyFactoryCallback callback)
{
	self->configs[((int) log2(config))] = callback;
}

PROXY_EXPORT PXConfigBackend px_proxy_factory_config_get_active (PXProxyFactory *self)
{
	// Find the first provided config
	for (unsigned int i=0 ; i <= ((int) log2(PX_CONFIG_BACKEND__LAST)) ; i++)
		if (self->configs[i])
			return (1 << i);
			
	// No valid config found!
	return PX_CONFIG_BACKEND_NONE;
}

PROXY_EXPORT char *px_proxy_factory_get_proxy (PXProxyFactory *self, char *url)
{
	return NULL;
}

PROXY_EXPORT void px_proxy_factory_on_get_proxy_add (PXProxyFactory *self, PXProxyFactoryCallback callback)
{
	unsigned int length = 0;
	PXProxyFactoryCallback *tmp = NULL;

	// Backup our current callbacks (NULL if none)
	tmp = self->on_get_proxy;
	
	// If we have any callbacks, count them
	// If user is trying to add an already existing callback, just ignore it
	if (self->on_get_proxy)
		for (length=0 ; self->on_get_proxy[length] ; length++)
			if (callback == self->on_get_proxy[length])
				return;
	
	// Allocate enough space for the old + new callbacks
	self->on_get_proxy = proxy_malloc(sizeof(PXProxyFactoryCallback) * (length + 2));
	
	// Copy old callbacks into new memory (if any)
	if (tmp)
		memcpy(self->on_get_proxy, tmp, length);
	
	// Free old memory (if any)
	proxy_free(tmp);
}

PROXY_EXPORT void px_proxy_factory_on_get_proxy_del (PXProxyFactory *self, PXProxyFactoryCallback callback)
{
	unsigned int length = 0, count = 0;
	PXProxyFactoryCallback *tmp = NULL;
	
	// Sanity checks, get length and count
	if (!self->on_get_proxy) return;
	for (length=0,count=0 ; self->on_get_proxy[length] ; length++)
		if (self->on_get_proxy[length] == callback)
			count++;
	if (length < 1 || count < 1) return;
	
	// Backup current callbacks
	tmp = self->on_get_proxy;
	
	// Allocate new array
	self->on_get_proxy = proxy_malloc(sizeof(PXProxyFactoryCallback) * (length - count + 1));
	
	// Copy in new values
	int i,j;
	for (i=0,j=0 ; tmp[i] ; i++)
		if (tmp[i] != callback)
			self->on_get_proxy[j++] = tmp[i];
	
	// Free old memory
	proxy_free(tmp);
}

PROXY_EXPORT void px_proxy_factory_pac_runner_add (PXProxyFactory *self, PXPACRunnerCallback callback)
{
	unsigned int length = 0;
	PXPACRunnerCallback *tmp = NULL;

	// Backup our current callbacks (NULL if none)
	tmp = self->pac_runners;
	
	// If we have any callbacks, count them
	// If user is trying to add an already existing callback, just ignore it
	if (self->pac_runners)
		for (length=0 ; self->pac_runners[length] ; length++)
			if (callback == self->pac_runners[length])
				return;
	
	// Allocate enough space for the old + new callbacks
	self->pac_runners = proxy_malloc(sizeof(PXPACRunnerCallback) * (length + 2));
	
	// Copy old callbacks into new memory (if any)
	if (tmp)
		memcpy(self->pac_runners, tmp, length);
	
	// Free old memory (if any)
	proxy_free(tmp);
}

PROXY_EXPORT void px_proxy_factory_pac_runner_del (PXProxyFactory *self, PXPACRunnerCallback callback)
{
	unsigned int length = 0, count = 0;
	PXPACRunnerCallback *tmp = NULL;
	
	// Sanity checks, get length and count
	if (!self->pac_runners) return;
	for (length=0,count=0 ; self->pac_runners[length] ; length++)
		if (self->pac_runners[length] == callback)
			count++;
	if (length < 1 || count < 1) return;
	
	// Backup current callbacks
	tmp = self->pac_runners;
	
	// Allocate new array
	self->pac_runners = proxy_malloc(sizeof(PXPACRunnerCallback) * (length - count + 1));
	
	// Copy in new values
	int i,j;
	for (i=0,j=0 ; tmp[i] ; i++)
		if (tmp[i] != callback)
			self->pac_runners[j++] = tmp[i];
	
	// Free old memory
	proxy_free(tmp);
}

PROXY_EXPORT void px_proxy_factory_free (PXProxyFactory *self)
{
	unsigned int i;
	
	if (!self) return;
	
	// Free the plugins
	if (self->plugins)
	{
		for (i=0 ; self->plugins[i] ; i++)
		{
			// Call the destantiation hook
			PXProxyFactoryCallback destantiate;
			destantiate = dlsym(self->plugins[i], "on_proxyfactory_destantiate");
			if (destantiate)
				destantiate(self);
			
			// Unload the plugin
			dlclose(self->plugins[i]);
			self->plugins[i] = NULL;
		}
		proxy_free(self->plugins);
	}
	
	// Free everything else
	if (self->configs)      proxy_free(self->configs);
	if (self->on_get_proxy) proxy_free(self->on_get_proxy);
	if (self->pac_runners)  proxy_free(self->pac_runners);
	proxy_free(self);
}

