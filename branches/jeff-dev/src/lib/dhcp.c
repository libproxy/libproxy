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

#include "pac.h"
#include "dhcp.h"

struct _pxDHCP { };

/**
 * Creates a new pxDHCP PAC detector.
 * @return New pxDHCP PAD detector
 */
pxDHCP *
px_dhcp_new()
{
	return NULL;
}

/**
 * Detect the next PAC in the chain.
 * @return Detected PAC or NULL if none is found
 */
pxPAC *
px_dhcp_next(pxDHCP *self)
{
	return NULL;
}

/**
 * Restarts the detection chain at the beginning.
 */
void
px_dhcp_rewind(pxDHCP *self)
{
	return;
}

/**
 * Frees a pxDHCP object.
 */
void
px_dhcp_free(pxDHCP *self)
{
	return;
}
