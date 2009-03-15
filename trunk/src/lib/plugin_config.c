/*******************************************************************************
 * libproxy - A library for proxy configuration
 * Copyright (C) 2009 Nathaniel McCallum <nathaniel@natemccallum.com>
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

#include "misc.h"
#include "plugin_config.h"

#define DEFAULT_CONFIG_ORDER "USER,SESSION,SYSTEM,config_envvar,config_wpad,config_direct"

static int
_findpos(pxConfigPluginCategory category, const char *name)
{
	int pos = 0;

    /* Attempt to get config order */
    char *fileorder = getenv("_PX_CONFIG_ORDER");
    char *envorder  = getenv("PX_CONFIG_ORDER");

    /* Create the config order */
    char *order     = px_strcat(fileorder ? fileorder : "", ",", envorder ? envorder : "", ",", DEFAULT_CONFIG_ORDER, NULL);

    /* Create the config plugin order vector */
    char **orderv   = px_strsplit(order, ",");
    px_free(order);

    /* Get the config by searching the config order */
    for (pos=0 ; orderv[pos] ; pos++)
    {
            if ((!strcmp(orderv[pos], "USER")    && PX_CONFIG_PLUGIN_CATEGORY_USER    == category) ||
            	(!strcmp(orderv[pos], "SESSION") && PX_CONFIG_PLUGIN_CATEGORY_SESSION == category) ||
            	(!strcmp(orderv[pos], "SYSTEM")  && PX_CONFIG_PLUGIN_CATEGORY_SYSTEM  == category) ||
            	(!strcmp(orderv[pos], name)))
            	break;
    }
    px_strfreev(orderv);
    return pos;
}

int
px_config_plugin_compare(pxPlugin *self, pxPlugin *other)
{
	if (!self || !other) return 0;
	int s = _findpos(((pxConfigPlugin *)  self)->category, self->name);
	int o = _findpos(((pxConfigPlugin *) other)->category, other->name);
	return s - o;
}
