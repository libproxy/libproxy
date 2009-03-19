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
#include <plugin_manager.h>
#include <plugin_wpad.h>
#include <pac.h>

typedef struct _pxDNSWPADPlugin {
	PX_PLUGIN_SUBCLASS(pxWPADPlugin);
	bool  rewound;
} pxDNSWPADPlugin;

static pxPAC *
_next(pxWPADPlugin *self)
{
	if (((pxDNSWPADPlugin *) self)->rewound)
	{
		pxPAC *pac = px_pac_new_from_string("http://wpad/wpad.dat");
		self->found = pac != NULL;
		return pac;
	}
	else
		return NULL;
}

static void
_rewind(pxWPADPlugin *self)
{
	((pxDNSWPADPlugin *) self)->rewound = true;
}

static bool
_constructor(pxPlugin *self)
{
	((pxDNSWPADPlugin *) self)->rewound = true;
	((pxWPADPlugin *) self)->found      = false;
	((pxWPADPlugin *) self)->next       = _next;
	((pxWPADPlugin *) self)->rewind     = _rewind;

	return true;
}

bool
px_module_load(pxPluginManager *self)
{
	return px_plugin_manager_constructor_add_subtype(self, "wpad_dns", pxWPADPlugin, pxDNSWPADPlugin, _constructor);
}
