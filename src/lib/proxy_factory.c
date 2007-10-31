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
#include <assert.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdio.h>
#include <dlfcn.h>
#include <math.h>

#include "misc.h"
#include "proxy_factory.h"
#include "wpad.h"

#define DEFAULT_CONFIG_ORDER "USER,SESSION,SYSTEM"

struct _pxProxyFactoryConfig {
	pxConfigCategory           category;
	char                      *name;
	pxProxyFactoryPtrCallback  callback;
};
typedef struct _pxProxyFactoryConfig pxProxyFactoryConfig;

struct _pxProxyFactory {
	void                      **plugins;
	pxProxyFactoryConfig      **configs;
	pxProxyFactoryVoidCallback *on_get_proxy;
	pxPACRunnerCallback         pac_runner;
	pxPAC                      *pac;
	pxWPAD                     *wpad;
};

// Convert the PAC formatted response into our proxy URL array response
static char **
_format_pac_response(char *response)
{
	char **chain, *tmp;
	
	if (!response) return px_strsplit("direct://", ";");
	chain = px_strsplit(response, ";");
	px_free(response);
	
	for (int i=0 ; chain[i] ; i++)
	{
		tmp = px_strstrip(chain[i]);
		px_free(chain[i]);
		
		if (!strncmp(tmp, "PROXY", 5) || !strncmp(tmp, "SOCKS", 5))
		{
			char *tmpa = px_strstrip(tmp + 5);
			chain[i] = px_malloc0(strlen(tmpa) + 9);
			if (!strncmp(tmp, "PROXY", 5))
				strcpy(chain[i], "http://");
			else
				strcpy(chain[i], "socks://");
			strcat(chain[i], tmpa);
			px_free(tmpa);
		}
		else
			chain[i] = px_strdup("direct://");
		
		px_free(tmp);
	}
	
	return chain;
}

/**
 * Creates a new pxProxyFactory instance.  This instance
 * and all its methods are NOT thread safe, so please take
 * care in their use.
 * 
 * @return A new pxProxyFactory instance or NULL on error
 */
pxProxyFactory *
px_proxy_factory_new ()
{
	pxProxyFactory *self = px_malloc0(sizeof(pxProxyFactory));
	unsigned int i;
	
	// Open the plugin dir
	DIR *plugindir = opendir(PLUGINDIR);
	if (!plugindir) return self;
	
	// Count the number of plugins
	for (i=0 ; readdir(plugindir) ; i++);
	self->plugins = (void **) px_malloc0(sizeof(void *) * (i + 1));
	rewinddir(plugindir);
	
	// For each plugin...
	struct dirent *ent;
	for (i=0 ; (ent = readdir(plugindir)) ; i++)
	{
		// Load the plugin
		char *tmp = px_malloc0(strlen(PLUGINDIR) + strlen(ent->d_name) + 2);
		sprintf(tmp, PLUGINDIR "/%s", ent->d_name);
		self->plugins[i] = dlopen(tmp, RTLD_LOCAL);
		px_free(tmp);
		if (!(self->plugins[i]))
		{
			i--;
			continue;
		}
		
		// Call the instantiation hook
		pxProxyFactoryBoolCallback instantiate;
		instantiate = dlsym(self->plugins[i], "on_proxy_factory_instantiate");
		if (instantiate && !instantiate(self))
		{
			dlclose(self->plugins[i]);
			self->plugins[i--] = NULL;
			continue;
		}
	}
	closedir(plugindir);
	
	return self;
}

bool
px_proxy_factory_config_add(pxProxyFactory *self, char *name, pxConfigCategory category, pxProxyFactoryPtrCallback callback)
{
	int count;
	pxProxyFactoryConfig **tmp;
	
	// Verify some basic stuff
	if (!self)                      return false;
	if (!callback)                  return false;
	if (!name || !strcmp(name, "")) return false;
	
	// Allocate an empty config array if there is none
	if (!self->configs) self->configs = px_malloc0(sizeof(pxProxyFactoryConfig *));
	
	// Make sure that 'name' is unique
	// Also, get a count of how many configs we have
	for (count=0 ; self->configs[count] ; count++)
		if (!strcmp(self->configs[count]->name, name))
			return false;
	
	// Allocate new array, copy old values into it and free old array
	tmp = px_malloc0(sizeof(pxProxyFactoryConfig *) * (count + 2));
	memcpy(tmp, self->configs, sizeof(pxProxyFactoryConfig *) * count);
	px_free(self->configs);
	self->configs = tmp;
	
	// Add the new callback to the end
	self->configs[count]           = px_malloc0(sizeof(pxProxyFactoryConfig));
	self->configs[count]->category = category;
	self->configs[count]->name     = px_strdup(name);
	self->configs[count]->callback = callback;
	
	return true;
}

bool
px_proxy_factory_config_del(pxProxyFactory *self, char *name)
{
	int i,j;
	
	// Verify some basic stuff
	if (!self)                      return false;
	if (!name || !strcmp(name, "")) return false;
	if (!self->configs)             return false;
	
	// Remove and shift all configs down (if found)
	for (i=0,j=0 ; self->configs[j]; i++,j++)
	{
		if (i != j)
			self->configs[j] = self->configs[i];
		else if (!strcmp(self->configs[i]->name, name))
		{
			px_free(self->configs[i]->name);
			px_free(self->configs[j--]);
		}
	}
	
	// If we have an empty array, free it
	if (!self->configs[0])
	{
		px_free(self->configs);
		self->configs = NULL;
	}
	
	return i != j ? true : false;
}

/**
 * Get which proxies to use for the specified URL.
 * 
 * A NULL-terminated array of proxy strings is returned.
 * If the first proxy fails, the second should be tried, etc...
 * 
 * The format of the returned proxy strings are as follows:
 *   - http://proxy:port
 *   - socks://proxy:port
 *   - direct://
 * @url The URL we are trying to reach
 * @return A NULL-terminated array of proxy strings to use
 */
char **
px_proxy_factory_get_proxy (pxProxyFactory *self, char *url)
{
	pxURL    *realurl  = px_url_new(url);
	pxConfig *config   = NULL;
	char    **response = px_strsplit("direct://", ";");
	
	// Verify some basic stuff
	if (!self)                    goto do_return;
	if (!url || !strcmp(url, "")) goto do_return;
	if (!realurl)                 goto do_return;
	
	// Call the events
	for (int i=0 ; self->on_get_proxy && self->on_get_proxy[i] ; i++)
		self->on_get_proxy[i](self);
	
	// Get the configuration order
	char *tmp = NULL, **order = NULL;
	if (getenv("PX_CONFIG_ORDER"))
	{
		tmp = px_malloc0(strlen(getenv("PX_CONFIG_ORDER")) + strlen(DEFAULT_CONFIG_ORDER) + 2);
		strcpy(tmp, getenv("PX_CONFIG_ORDER"));
		strcat(tmp, ",");
	}
	else
		tmp = px_malloc0(strlen(DEFAULT_CONFIG_ORDER) + 1);
	strcat(tmp, DEFAULT_CONFIG_ORDER);
	order = px_strsplit(tmp, ",");
	px_free(tmp);
	
	// Get the config by searching the config order
	for (int i=0 ; order[i] && !config ; i++)
	{
		// Get the category (if applicable)
		pxConfigCategory category;
		if (!strcmp(order[i], "USER"))
			category = PX_CONFIG_CATEGORY_USER;
		else if (!strcmp(order[i], "SESSION"))
			category = PX_CONFIG_CATEGORY_SESSION;
		else if (!strcmp(order[i], "SYSTEM"))
			category = PX_CONFIG_CATEGORY_SYSTEM;
		else
			category = PX_CONFIG_CATEGORY_NONE;
		
		for (int j=0 ; self->configs && self->configs[j] && !config ; j++)
		{
			if (category != PX_CONFIG_CATEGORY_NONE && self->configs[j]->category == category)
				config = self->configs[j]->callback(self);
			else if (category == PX_CONFIG_CATEGORY_NONE && !strcmp(self->configs[j]->name, order[i]))
				config = self->configs[j]->callback(self);
		}
	}
	px_strfreev(order);
	
	// No config was found via search order, call all plugins
	for (int i=0 ; self->configs && self->configs[i] && !config ; i++)
		config = self->configs[i]->callback(self);
	
	// No plugin returned a valid config, fall back to 'wpad://'
	if (!config)
	{
		fprintf(stderr, "*** Unable to locate valid config! Falling back to auto-detection...\n");
		config         = px_malloc0(sizeof(pxConfig));
		config->url    = px_strdup("wpad://");
		config->ignore = px_strdup("");
	}
	
	// If the config plugin returned an invalid config type or malformed URL, fall back to 'wpad://'
	if (!(!strncmp(config->url, "http://", 7) || 
		  !strncmp(config->url, "socks://", 8) ||
		  !strncmp(config->url, "pac+", 4) ||
		  !strcmp (config->url, "wpad://") ||
		  !strcmp (config->url, "direct://")))
	{
		fprintf(stderr, "*** Config plugin returned invalid URL type! Falling back to auto-detection...\n");
		px_free(config->url);
		config->url = px_strdup("wpad://");
	}
	else if (!strncmp(config->url, "pac+", 4) && !px_url_is_valid(config->url + 4))
	{
		fprintf(stderr, "*** Config plugin returned malformed URL! Falling back to auto-detection...\n");
		px_free(config->url);
		config->url = px_strdup("wpad://");
	}
	else if ((!strncmp(config->url, "http://", 7) || !strncmp(config->url, "socks://", 8)) && 
			  !px_url_is_valid(config->url))
	{
		fprintf(stderr, "*** Config plugin returned malformed URL! Falling back to auto-detection...\n");
		px_free(config->url);
		config->url = px_strdup("wpad://");
	}
	
	// TODO: Ignores
	
	// If we have a wpad config
	if (!strcmp(config->url, "wpad://"))
	{
		// Get the WPAD object if needed
		if (!self->wpad)
		{
			if (self->pac) px_pac_free(self->pac);
			self->pac  = NULL;
			self->wpad = px_wpad_new();
			if (!self->wpad)
			{
				fprintf(stderr, "*** Unable to create WPAD! Falling back to direct...\n");
				goto do_return;
			}
		}
		
		// If we have no PAC, get one
		// If getting the PAC fails, but the WPAD cycle worked, restart the cycle
		if (!self->pac && !(self->pac = px_wpad_next(self->wpad)) && px_wpad_pac_found(self->wpad))
		{
			px_wpad_rewind(self->wpad);
			self->pac = px_wpad_next(self->wpad);
		}
		
		// If the WPAD cycle failed, fall back to direct
		if (!self->pac)
		{
			fprintf(stderr, "*** Unable to locate PAC! Falling back to direct...\n");
			goto do_return;
		}
		
		// Run the PAC
		if (self->pac_runner)
		{
			px_strfreev(response);
			response = _format_pac_response(self->pac_runner(self, self->pac, realurl));
		}
		
		// No PAC runner found, fall back to direct
		else
			fprintf(stderr, "*** PAC found, but no active PAC runner! Falling back to direct...\n");
	}
	
	// If we have a PAC config
	else if (!strncmp(config->url, "pac+", 4))
	{
		// Clear WPAD to indicate that this is a non-WPAD PAC
		if (self->wpad)
		{
			px_wpad_free(self->wpad);
			self->wpad = NULL;
		}
		
		// If a PAC alread exists, but comes from a different URL than the one
		// specified, remove it
		if (self->pac)
		{
			pxURL *urltmp = px_url_new(config->url + 4);
			if (!urltmp)
			{
				fprintf(stderr, "*** Invalid PAC URL! Falling back to direct...\n");
				goto do_return;
			}
			if (!px_url_equals(urltmp, px_pac_get_url(self->pac)))
			{
				px_pac_free(self->pac);
				self->pac = NULL;
			}
			px_url_free(urltmp);
		}
		
		// Try to load the PAC if it is not already loaded
		if (!self->pac && !(self->pac = px_pac_new_from_string(config->url + 4)))
		{
			fprintf(stderr, "*** Invalid PAC URL! Falling back to direct...\n");
			goto do_return;
		}

		// Run the PAC
		if (self->pac_runner)
		{
			px_strfreev(response);
			response = _format_pac_response(self->pac_runner(self, self->pac, realurl));
		}
		else
			fprintf(stderr, "*** PAC found, but no active PAC runner! Falling back to direct...\n");
	}
	
	// If we have a manual config (http://..., socks://...)
	else if (!strncmp(config->url, "http://", 7) || !strncmp(config->url, "socks://", 8))
	{
		if (self->wpad) { px_wpad_free(self->wpad); self->wpad = NULL; }
		if (self->pac)  { px_pac_free(self->pac);   self->pac = NULL; }
		px_strfreev(response);
		response = px_strsplit(config->url, ";");
	}
	
	// Actually return, freeing misc stuff
	do_return:
		if (config)  { px_free(config->url); px_free(config->ignore); px_free(config); }
		if (realurl) px_url_free(realurl);
		return response;
}

bool
px_proxy_factory_on_get_proxy_add (pxProxyFactory *self, pxProxyFactoryVoidCallback callback)
{
	int count;
	pxProxyFactoryVoidCallback *tmp;
	
	// Verify some basic stuff
	if (!self)     return false;
	if (!callback) return false;
	
	// Allocate an empty config array if there is none
	if (!self->on_get_proxy) self->on_get_proxy = px_malloc0(sizeof(pxProxyFactoryVoidCallback));
	
	// Get a count of how many callbacks we have
	for (count=0 ; self->on_get_proxy[count] ; count++);
	
	// Allocate new array, copy old values into it and free old array
	tmp = px_malloc0(sizeof(pxProxyFactoryVoidCallback) * (count + 2));
	memcpy(tmp, self->on_get_proxy, sizeof(pxProxyFactoryVoidCallback) * count);
	px_free(self->on_get_proxy);
	self->on_get_proxy = tmp;
	
	// Add the new callback to the end
	self->on_get_proxy[count] = callback;
	
	return true;
}

bool
px_proxy_factory_on_get_proxy_del (pxProxyFactory *self, pxProxyFactoryVoidCallback callback)
{
	int i,j;
	
	// Verify some basic stuff
	if (!self)               return false;
	if (!callback)           return false;
	if (!self->on_get_proxy) return false;
	
	// Remove and shift all callbacks down (if found)
	for (i=0,j=0 ; self->on_get_proxy[j]; i++,j++)
	{
		if (i != j)
			self->on_get_proxy[j] = self->on_get_proxy[i];
		else if (self->on_get_proxy[i] == callback)
			self->on_get_proxy[j--] = NULL;
	}
	
	// If we have an empty array, free it
	if (!self->on_get_proxy[0])
	{
		px_free(self->on_get_proxy);
		self->on_get_proxy = NULL;
	}
	
	return i != j ? true : false;
}

bool
px_proxy_factory_pac_runner_set (pxProxyFactory *self, pxPACRunnerCallback callback)
{
	if (!self) return false;
	if (self->pac_runner && callback && self->pac_runner != callback) return false;
	self->pac_runner = callback;
	return true;
}

void
px_proxy_factory_network_changed(pxProxyFactory *self)
{
	if (self->wpad)
	{
		px_wpad_free(self->wpad);
		self->wpad = NULL;
	}
	
	if (self->pac)
	{
		px_pac_free(self->pac);
		self->pac = NULL;
	}
}

/**
 * Frees the pxProxyFactory instance when no longer used.
 */
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
			pxProxyFactoryVoidCallback destantiate;
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
	if (self->pac)  px_pac_free(self->pac);
	if (self->wpad) px_wpad_free(self->wpad);
	px_free(self);
}

