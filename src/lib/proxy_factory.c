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
#define _BSD_SOURCE

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdio.h>
#include <dlfcn.h>
#include <math.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>


#include "misc.h"
#include "proxy.h"
#include "wpad.h"
#include "config_file.h"
#include "array.h"
#include "strdict.h"
#include "plugin_manager.h"
#include "plugin_config.h"
#include "plugin_pacrunner.h"
#include "plugin_network.h"
#include "plugin_ignore.h"

struct _pxProxyFactory {
	pthread_mutex_t  mutex;
	pxPluginManager *pm;
	pxArray         *configs;
	pxArray         *pacrunners;
	pxArray         *networks;
	pxArray         *ignores;
	pxPAC           *pac;
	pxWPAD          *wpad;
	//pxConfigFile    *cf;
};

/* Convert the PAC formatted response into our proxy URL array response */
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
			char *hostport = px_strstrip(tmp + 5);
			if (!strncmp(tmp, "PROXY", 5))
				chain[i] = px_strcat("http://", hostport, NULL);
			else
				chain[i] = px_strcat("socks://", hostport, NULL);
			px_free(hostport);
		}
		else
			chain[i] = px_strdup("direct://");

		px_free(tmp);
	}

	return chain;
}

/**
 * Creates a new pxProxyFactory instance. This instance should be kept
 * around as long as possible as it contains cached data to increase
 * performance.  Memory usage should be minimal (cache is small) and the
 * cache lifespan is handled automatically.
 *
 * @return A new pxProxyFactory instance or NULL on error
 */
pxProxyFactory *
px_proxy_factory_new ()
{
	pxProxyFactory *self = px_malloc0(sizeof(pxProxyFactory));
	self->pm             = px_plugin_manager_new();

	/* Register the ConfigPlugin type */
	pxConfigPlugin cp;
	memset(&cp, 0, sizeof(pxConfigPlugin));
	((pxPlugin *) &cp)->compare = &px_config_plugin_compare;
	if (!px_plugin_manager_register_type(self->pm, pxConfigPlugin, (pxPlugin *) &cp)) goto error;

	/* Register the PACRunnerPlugin type */
	pxPACRunnerPlugin prp;
	memset(&prp, 0, sizeof(pxPACRunnerPlugin));
	((pxPlugin *) &prp)->constructor = &px_pac_runner_plugin_constructor;
	if (!px_plugin_manager_register_type(self->pm, pxPACRunnerPlugin, (pxPlugin *) &prp)) goto error;

	/* Register the NetworkPlugin type */
	pxNetworkPlugin np;
	memset(&np, 0, sizeof(pxNetworkPlugin));
	if (!px_plugin_manager_register_type(self->pm, pxNetworkPlugin, (pxPlugin *) &np)) goto error;

	/* Register the IgnorePlugin type */
	pxIgnorePlugin ip;
	memset(&ip, 0, sizeof(pxIgnorePlugin));
	if (!px_plugin_manager_register_type(self->pm, pxIgnorePlugin, (pxPlugin *) &ip)) goto error;

    /* If we have a config file, load the config order from it */
	pxConfigFile *cf = px_config_file_new(SYSCONFDIR "/proxy.conf");
    if (cf)
    {
    	char *tmp = px_config_file_get_value(cf, PX_CONFIG_FILE_DEFAULT_SECTION, "config_order");
    	px_config_file_free(cf);
    	if (tmp && setenv("_PX_CONFIG_ORDER", tmp, 1))
    	{
    		px_free(tmp);
    		goto error;
    	}
    }

	/* Load all modules */
	if (!px_plugin_manager_load_dir(self->pm, PLUGINDIR)) goto error;

	/* Instantiate all our plugins */
	self->configs    = px_plugin_manager_instantiate_plugins(self->pm, pxConfigPlugin);
	self->pacrunners = px_plugin_manager_instantiate_plugins(self->pm, pxPACRunnerPlugin);
	self->networks   = px_plugin_manager_instantiate_plugins(self->pm, pxNetworkPlugin);
	self->ignores    = px_plugin_manager_instantiate_plugins(self->pm, pxIgnorePlugin);

	pthread_mutex_init(&self->mutex, NULL);
	return self;

	error:
		px_proxy_factory_free(self);
		return NULL;
}

/**
 * Get which proxies to use for the specified URL.
 *
 * A NULL-terminated array of proxy strings is returned.
 * If the first proxy fails, the second should be tried, etc...
 * Don't forget to free the strings/array when you are done.
 * In all cases, at least one entry in the array will be returned.
 * There are no error conditions.
 *
 * Regarding performance: this method always blocks and may be called
 * in a separate thread (is thread-safe).  In most cases, the time
 * required to complete this function call is simply the time required
 * to read the configuration (i.e. from gconf, kconfig, etc).
 *
 * In the case of PAC, if no valid PAC is found in the cache (i.e.
 * configuration has changed, cache is invalid, etc), the PAC file is
 * downloaded and inserted into the cache. This is the most expensive
 * operation as the PAC is retrieved over the network. Once a PAC exists
 * in the cache, it is merely a javascript invocation to evaluate the PAC.
 * One should note that DNS can be called from within a PAC during
 * javascript invocation.
 *
 * In the case of WPAD, WPAD is used to automatically locate a PAC on the
 * network.  Currently, we only use DNS for this, but other methods may
 * be implemented in the future.  Once the PAC is located, normal PAC
 * performance (described above) applies.
 *
 * The format of the returned proxy strings are as follows:
 *   - http://proxy:port
 *   - socks://proxy:port
 *   - direct://
 * @url The URL we are trying to reach
 * @return A NULL-terminated array of proxy strings to use
 */
char **
px_proxy_factory_get_proxies (pxProxyFactory *self, char *url)
{
	char          **response = px_strsplit("direct://", ";");
	char           *confurl  = NULL;
	char           *confign  = NULL;
	pxConfigPlugin *config   = NULL;

	/* Verify some basic stuff */
	if (!self) return response;
	pthread_mutex_lock(&self->mutex);

	/* Convert the URL */
	pxURL *realurl = px_url_new(url);
	if (!realurl) goto do_return;

	/* Check to see if our network has changed */
	for (int i=0 ; i < px_array_length(self->networks) ; i++)
	{
		pxNetworkPlugin *np = (pxNetworkPlugin *) px_array_get(self->networks, i);
		if (np->changed(np))
		{
			px_wpad_free(self->wpad);
			px_pac_free(self->pac);
			self->wpad = NULL;
			self->pac  = NULL;
			break;
		}
	}

	/* Attempt to load a valid config */
	for (int i=0 ; i < px_array_length(self->configs) ; i++)
	{
		config  = (pxConfigPlugin *) px_array_get(self->configs, i);
		confurl = config->get_config(config, realurl);
		if (!confurl)
			goto invalid;
		confign = config->get_ignore(config, realurl);

		/* If the config plugin returned an invalid config type or malformed URL, mark as invalid */
		if (!(!strncmp(confurl, "http://", 7) ||
			  !strncmp(confurl, "socks://", 8) ||
			  !strncmp(confurl, "pac+", 4) ||
			  !strcmp (confurl, "wpad://") ||
			  !strcmp (confurl, "direct://")))
			goto invalid;
		else if (!strncmp(confurl, "pac+", 4) && !px_url_is_valid(confurl + 4))
			goto invalid;
		else if ((!strncmp(confurl, "http://", 7) || !strncmp(confurl, "socks://", 8)) &&
				  !px_url_is_valid(confurl))
			goto invalid;

		config->valid = true;
		break;

		invalid:
			px_free(confurl); confurl = NULL;
			px_free(confign); confign = NULL;
			config->valid = false;
			config        = NULL;
	}
	if (!config) goto do_return;

	/* Check our ignore patterns */
	char **ignores = px_strsplit(confign, ",");
	px_free(confign); confign = NULL;
	for (int i=0 ; ignores[i] ; i++)
	{
		for (int j=0 ; j < px_array_length(self->ignores) ; j++)
		{
			pxIgnorePlugin *ip = (pxIgnorePlugin *) px_array_get(self->ignores, j);
			if (ip->ignore(ip, realurl, ignores[i]))
			{
				px_strfreev(ignores);
				goto do_return;
			}
		}
	}
	px_strfreev(ignores);

	/* If we have a wpad config */
	if (!strcmp(confurl, "wpad://"))
	{
		/* Get the WPAD object if needed */
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

		/*
		 * If we have no PAC, get one
		 * If getting the PAC fails, but the WPAD cycle worked, restart the cycle
		 */
		if (!self->pac && !(self->pac = px_wpad_next(self->wpad)) && px_wpad_pac_found(self->wpad))
		{
			px_wpad_rewind(self->wpad);
			self->pac = px_wpad_next(self->wpad);
		}

		/* If the WPAD cycle failed, fall back to direct */
		if (!self->pac)
		{
			fprintf(stderr, "*** Unable to locate PAC! Falling back to direct...\n");
			goto do_return;
		}

		/* Run the PAC */
		for (int i = 0 ; i < px_array_length(self->pacrunners) ; i++)
		{
			px_strfreev(response);
			pxPACRunnerPlugin *prp = (pxPACRunnerPlugin *) px_array_get(self->pacrunners, i);
			response = _format_pac_response(prp->run(prp, self->pac, realurl));
			break; /* Only try one PAC Runner */
		}

		/* No PAC runner found, fall back to direct */
		if (px_array_length(self->pacrunners) < 1)
			fprintf(stderr, "*** PAC found, but no active PAC runner! Falling back to direct...\n");
	}

	/* If we have a PAC config */
	else if (!strncmp(confurl, "pac+", 4))
	{
		/* Clear WPAD to indicate that this is a non-WPAD PAC */
		if (self->wpad)
		{
			px_wpad_free(self->wpad);
			self->wpad = NULL;
		}

		/* If a PAC already exists, but came from a different URL than the one specified, remove it */
		if (self->pac)
		{
			pxURL *urltmp = px_url_new(confurl + 4);
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

		/* Try to load the PAC if it is not already loaded */
		if (!self->pac && !(self->pac = px_pac_new_from_string(confurl + 4)))
		{
			fprintf(stderr, "*** Invalid PAC URL! Falling back to direct...\n");
			goto do_return;
		}

		/* Run the PAC */
		for (int i = 0 ; i < px_array_length(self->pacrunners) ; i++)
		{
			px_strfreev(response);
			pxPACRunnerPlugin *prp = (pxPACRunnerPlugin *) px_array_get(self->pacrunners, i);
			response = _format_pac_response(prp->run(prp, self->pac, realurl));
			break; /* Only try one PAC Runner */
		}

		/* No PAC runner found, fall back to direct */
		if (px_array_length(self->pacrunners) < 1)
			fprintf(stderr, "*** PAC found, but no active PAC runner! Falling back to direct...\n");
	}

	/* If we have a manual config (http://..., socks://...) */
	else if (!strncmp(confurl, "http://", 7) || !strncmp(confurl, "socks://", 8))
	{
		if (self->wpad) { px_wpad_free(self->wpad); self->wpad = NULL; }
		if (self->pac)  { px_pac_free(self->pac);   self->pac = NULL; }
		px_strfreev(response);
		response = px_strsplit(confurl, ";");
	}

	/* Actually return, freeing misc stuff */
	do_return:
		px_free(confurl);
		px_url_free(realurl);
		pthread_mutex_unlock(&self->mutex);
		return response;
}

/**
 * Frees the pxProxyFactory instance when no longer used.
 */
void
px_proxy_factory_free (pxProxyFactory *self)
{
	if (!self) return;

	pthread_mutex_lock(&self->mutex);

	/* Free the plugin instances */
	px_array_free(self->configs);
	px_array_free(self->pacrunners);
	px_array_free(self->networks);
	px_array_free(self->ignores);

	/* Free the plugin manager */
	px_plugin_manager_free(self->pm);

	/* Free everything else */
	px_pac_free(self->pac);
	px_wpad_free(self->wpad);
	pthread_mutex_unlock(&self->mutex);
	pthread_mutex_destroy(&self->mutex);
	px_free(self);
}
