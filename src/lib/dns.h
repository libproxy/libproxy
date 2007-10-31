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

#ifndef DNS_H_
#define DNS_H_

#include "pac.h"

/**
 * A pxDNS PAC detector. All fields are private.
 */
typedef struct _pxDNS pxDNS;

/**
 * Creates a new pxDNS PAC detector.
 * @return New pxDNS PAD detector
 */
pxDNS *px_dns_new();

/**
 * Creates a new pxDNS PAC detector with more options.
 * @domain The domain name to use when detecting or NULL to use the default
 * @return New pxDNS PAD detector
 */
pxDNS *px_dns_new_full(const char *domain);

/**
 * Detect the next PAC in the chain.
 * @return Detected PAC or NULL if none is found
 */
pxPAC *px_dns_next(pxDNS *self);

/**
 * Restarts the detection chain at the beginning.
 */
void px_dns_rewind(pxDNS *self);

/**
 * Frees a pxDNS object.
 */
void px_dns_free(pxDNS *self);

#endif /*DNS_H_*/
