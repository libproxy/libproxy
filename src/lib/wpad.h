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

#ifndef WPAD_H_
#define WPAD_H_

#include "pac.h"
#include "dhcp.h"
#include "dns.h"
#include "slp.h"

/**
 * WPAD object. All fields are private.
 */
typedef struct _pxWPAD pxWPAD;

/**
 * Create a new WPAD object (for detecting PAC locations)
 * @return A New WPAD object
 */
pxWPAD *px_wpad_new();

/**
 * Creates a new WPAD object with more options.
 * For all options, ownership is stolen by the new WPAD object.
 * In other words, don't pass a detector to this function and then use it 
 * later (or free it).
 * @dhcp The DHCP detector to use (or NULL for the default detector)
 * @dns The DNS detector to use (or NULL for the default detector)
 * @slp The SLP detector to use (or NULL for the default detector)
 * @return New DNS PAD detector
 */
pxWPAD *px_wpad_new_full(pxDHCP *dhcp, pxDNS *dns, pxSLP *slp);

/**
 * Detect the next PAC in the chain.
 * @return Detected PAC or NULL if none is found
 */
pxPAC *px_wpad_next(pxWPAD *self);

/**
 * Restarts the detection chain at the beginning.
 */
void px_wpad_rewind(pxWPAD *self);

/**
 * Frees a WPAD object.
 */
void px_wpad_free(pxWPAD *self);

/**
 * Returns whether or not a PAC was found during this detection cycle.
 * Always returns false if called directly after wpad_wpad_rewind().
 * @return Whether or not a PAC was found
 */
bool px_wpad_pac_found(pxWPAD *self);

#endif /*WPAD_H_*/
