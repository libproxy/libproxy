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
#include <modules.h>
#include <pac.h>

typedef struct _pxDNSWPADModule {
	PX_MODULE_SUBCLASS(pxWPADModule);
	bool rewound;
} pxDNSWPADModule;

static pxPAC *
_next(pxWPADModule *self)
{
	if (((pxDNSWPADModule *) self)->rewound)
	{
		pxPAC *pac = px_pac_new_from_string("http://wpad/wpad.dat");
		self->found = pac != NULL;
		return pac;
	}
	else
		return NULL;
}

static void
_rewind(pxWPADModule *self)
{
	((pxDNSWPADModule *) self)->rewound = true;
}

static void *
_constructor()
{
	pxDNSWPADModule *self = px_malloc0(sizeof(pxDNSWPADModule));
	self->rewound           = true;
	self->__parent__.found  = false;
	self->__parent__.next   = _next;
	self->__parent__.rewind = _rewind;
	return self;
}

bool
px_module_load(pxModuleManager *self)
{
	return px_module_manager_register_module(self, pxWPADModule, "wpad_dns", _constructor, px_free);
}
