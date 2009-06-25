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
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <pthread.h>

#ifdef _WIN32
#include <windows.h>
#define setenv(name, value, overwrite) SetEnvironmentVariable(name, value)
#endif

#include "misc.h"
#include "proxy.h"
#include "config_file.h"
#include "array.h"
#include "strdict.h"
#include "module_manager.h"
#include "modules.h"

struct _pxProxyFactory {
	pthread_mutex_t     mutex;
	pxModuleManager    *mm;
	pxPAC              *pac;
	bool                wpad;
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

static inline int
_px_wpad_module_findpos(const char *name)
{
	int pos = 0;

	char **orderv = px_strsplit("wpad_dhcp,wpad_slp,wpad_dns,wpad_dnsdevolution", ",");
	for (pos = 0 ; orderv[pos] ; pos++)
		if (!strcmp(name, orderv[pos]))
			goto do_return;

	do_return:
		px_strfreev(orderv);
		return pos;
}

static int
_px_wpad_module_cmp(pxModuleRegistration **a, pxModuleRegistration **b)
{
	if (!a || !*a) return 0;
	if (!b || !*b) return 0;
	return _px_wpad_module_findpos((*a)->name) - _px_wpad_module_findpos((*b)->name);
}

static inline int
_px_config_module_findpos(pxConfigModuleCategory category, const char *name)
{
	int pos = 0;

    /* Attempt to get config order */
    char *fileorder = getenv("_PX_CONFIG_ORDER");
    char *envorder  = getenv("PX_CONFIG_ORDER");

    /* Create the config order */
    char *order     = px_strcat(fileorder ? fileorder : "", ",",
                                envorder ? envorder : "", ",",
                                "USER,SESSION,SYSTEM,config_envvar,config_wpad,config_direct",
                                NULL);

    /* Create the config plugin order vector */
    char **orderv   = px_strsplit(order, ",");
    px_free(order);

    /* Get the config by searching the config order */
    for (pos=0 ; orderv[pos] ; pos++)
    {
            if ((!strcmp(orderv[pos], "USER")    && PX_CONFIG_MODULE_CATEGORY_USER    == category) ||
            	(!strcmp(orderv[pos], "SESSION") && PX_CONFIG_MODULE_CATEGORY_SESSION == category) ||
            	(!strcmp(orderv[pos], "SYSTEM")  && PX_CONFIG_MODULE_CATEGORY_SYSTEM  == category) ||
            	(!strcmp(orderv[pos], name)))
            	break;
    }
    px_strfreev(orderv);
    return pos;
}

static int
_px_config_module_cmp(pxModuleRegistration **a, pxModuleRegistration **b)
{
	if (!a || !*a) return 0;
	if (!b || !*b) return 0;

	return _px_config_module_findpos(((pxConfigModule *) (*a)->instance)->category, (*a)->name) -
	       _px_config_module_findpos(((pxConfigModule *) (*b)->instance)->category, (*b)->name);
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
	self->mm             = px_module_manager_new();

	/* Register our module types */
	if (!px_module_manager_register_type(self->mm, pxConfigModule,    _px_config_module_cmp, false)) goto error;
	if (!px_module_manager_register_type(self->mm, pxPACRunnerModule, NULL,                   true)) goto error;
	if (!px_module_manager_register_type(self->mm, pxWPADModule,      _px_wpad_module_cmp,   false)) goto error;

    /* If we have a config file, load the config order from it */
    setenv("_PX_CONFIG_ORDER", "", 1);
	pxConfigFile *cf = px_config_file_new(SYSCONFDIR "proxy.conf");
    if (cf)
    {
    	char *tmp = px_config_file_get_value(cf, PX_CONFIG_FILE_DEFAULT_SECTION, "config_order");
    	px_config_file_free(cf);
    	if (tmp && setenv("_PX_CONFIG_ORDER", tmp, 1))
    	{
    		px_free(tmp);
    		goto error;
    	}
    	px_free(tmp);
    }

	/* Load all modules */
	if (!px_module_manager_load_dir(self->mm, MODULEDIR)) goto error;

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
 *   - http://[username:password@]proxy:port
 *   - socks://[username:password@]proxy:port
 *   - direct://
 * Please note that the username and password in the above URLs are optional
 * and should be use to authenticate the connection if present.
 *
 * @url The URL we are trying to reach
 * @return A NULL-terminated array of proxy strings to use
 */
char **
px_proxy_factory_get_proxies (pxProxyFactory *self, char *url)
{
	char          **response = px_strsplit("direct://", ";");
	char           *confurl  = NULL;
	char           *confign  = NULL;
	pxConfigModule *config   = NULL;

	/* Verify some basic stuff */
	if (!self) return response;
	pthread_mutex_lock(&self->mutex);

	/* Convert the URL */
	pxURL *realurl = px_url_new(url);
	if (!realurl) goto do_return;

	/* Check to see if our network has changed */
	pxNetworkModule **networks = px_module_manager_instantiate_type(self->mm, pxNetworkModule);
	for (int i=0 ; networks && networks[i] ; i++)
	{
		if (networks[i]->changed(networks[i]))
		{
			pxWPADModule **wpads = px_module_manager_instantiate_type(self->mm, pxWPADModule);
			for (int j=0 ; wpads[j] ; j++)
			{
				wpads[j]->rewind(wpads[j]);
			}
			px_free(wpads);
			px_pac_free(self->pac);
			self->pac  = NULL;
			break;
		}
	}
	px_free(networks);

	/* Attempt to load a valid config */
	pxConfigModule **configs = px_module_manager_instantiate_type(self->mm, pxConfigModule);
	for (int i=0 ; configs && configs[i] ; i++)
	{
		config  = configs[i];
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
	px_free(configs);
	if (!config) goto do_return;

	/* Check our ignore patterns */
	pxIgnoreModule **ignores = px_module_manager_instantiate_type(self->mm, pxIgnoreModule);
	char **ignore_split    = px_strsplit(confign, ",");
	px_free(confign); confign = NULL;
	for (int i=0 ; ignore_split && ignore_split[i] ; i++)
	{
		for (int j=0 ; ignores && ignores[j] ; j++)
		{
			if (ignores[j]->ignore(ignores[j], realurl, ignore_split[i]))
			{
				px_free(ignores);
				px_strfreev(ignore_split);
				goto do_return;
			}
		}
	}
	px_free(ignores);
	px_strfreev(ignore_split);

	/* If we have a wpad config */
	if (!strcmp(confurl, "wpad://"))
	{
		/* If the config has just changed from PAC to WPAD, clear the PAC */
		if (!self->wpad)
		{
			px_pac_free(self->pac);
			self->pac  = NULL;
			self->wpad = true;
		}

		/* If we have no PAC, get one */
		if (!self->pac)
		{
			pxWPADModule **wpads = px_module_manager_instantiate_type(self->mm, pxWPADModule);
			for (int i=0 ; wpads[i] ; i++)
				if ((self->pac = wpads[i]->next(wpads[i])))
					break;

			/* If getting the PAC fails, but the WPAD cycle worked, restart the cycle */
			if (!self->pac)
			{
				for (int i=0 ; wpads[i] ; i++)
				{
					if (wpads[i]->found)
					{
						/* Since a PAC was found at some point,
						 * rewind all the WPADS to start from the beginning */
						/* Yes, the reuse of 'i' here is intentional...
						 * This is because if *any* of the wpads have found a pac,
						 * we need to reset them all to keep consistent state. */
						for (i=0 ; wpads[i] ; i++)
							wpads[i]->rewind(wpads[i]);

						/* Attempt to find a PAC */
						for (i=0 ; wpads[i] ; i++)
							if ((self->pac = wpads[i]->next(wpads[i])))
								break;
						break;
					}
				}
			}
			px_free(wpads);
		}
	}
	/* If we have a PAC config */
	else if (!strncmp(confurl, "pac+", 4))
	{
		/* Save the PAC config */
		if (self->wpad)
			self->wpad = false;

		/* If a PAC already exists, but came from a different URL than the one specified, remove it */
		if (self->pac)
		{
			pxURL *urltmp = px_url_new(confurl + 4);
			if (!urltmp)
				goto do_return;
			if (!px_url_equals(urltmp, px_pac_get_url(self->pac)))
			{
				px_pac_free(self->pac);
				self->pac = NULL;
			}
			px_url_free(urltmp);
		}

		/* Try to load the PAC if it is not already loaded */
		if (!self->pac && !(self->pac = px_pac_new_from_string(confurl + 4)))
			goto do_return;
	}

	/* In case of either PAC or WPAD, we'll run the PAC */
	if (!strcmp(confurl, "wpad://") || !strncmp(confurl, "pac+", 4))
	{
		pxPACRunnerModule **pacrunners = px_module_manager_instantiate_type(self->mm, pxPACRunnerModule);

		/* No PAC runner found, fall back to direct */
		if (!pacrunners)
		{
			px_free(pacrunners);
			goto do_return;
		}

		/* Run the PAC, but only try one PACRunner */
		px_strfreev(response);
		response = _format_pac_response(pacrunners[0]->run(pacrunners[0], self->pac, realurl));
		px_free(pacrunners);
	}

	/* If we have a manual config (http://..., socks://...) */
	else if (!strncmp(confurl, "http://", 7) || !strncmp(confurl, "socks://", 8))
	{
		self->wpad = false;
		if (self->pac)  { px_pac_free(self->pac); self->pac = NULL; }
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

	/* Free the module manager */
	px_module_manager_free(self->mm);

	/* Free everything else */
	px_pac_free(self->pac);
	pthread_mutex_unlock(&self->mutex);
	pthread_mutex_destroy(&self->mutex);
	px_free(self);
}
