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

#define DEFAULT_WPAD_ORDER "wpad_dhcp,wpad_slp,wpad_dns,wpad_dnsdevolution"

static int
_findpos(const char *name)
{
	int pos = 0;

	char **orderv = px_strsplit(DEFAULT_WPAD_ORDER, ",");
	for (pos = 0 ; orderv[pos] ; pos++)
		if (!strcmp(name, orderv[pos]))
			goto do_return;

	do_return:
		px_strfreev(orderv);
		return pos;
}

int
px_wpad_plugin_compare(pxPlugin *self, pxPlugin *other)
{
	if (!self || !other) return 0;
	int s = _findpos(self->name);
	int o = _findpos(other->name);
	return s - o;
}
