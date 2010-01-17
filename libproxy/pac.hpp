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


#ifndef PAC_H_
#define PAC_H_

#include "url.hpp"

/**
 * pxPAC object.  All fields are private.
 */
typedef struct _pxPAC pxPAC;

/**
 * Frees memory used by the ProxyAutoConfig.
 */
void px_pac_free(pxPAC *self);

/**
 * Get the URL which the pxPAC uses.
 * @return The URL that the pxPAC came from
 */
__attribute__ ((visibility("default")))
const pxURL *px_pac_get_url(pxPAC *self);

/**
 * Create a new ProxyAutoConfig.
 * @url The URL where the PAC file is found
 * @return A new ProxyAutoConfig or NULL on error
 */
pxPAC *px_pac_new(pxURL *url);

/**
 * Create a new ProxyAutoConfig (from a string for convenience).
 * @url The url (string) where the PAC file is found
 * @return A new ProxyAutoConfig or NULL on error
 */
__attribute__ ((visibility("default")))
pxPAC *px_pac_new_from_string(char *url);

/**
 * Returns the Javascript code which the pxPAC uses.
 * @return The Javascript code used by the pxPAC
 */
__attribute__ ((visibility("default")))
const char *px_pac_to_string(pxPAC *self);

/**
 * Download the latest version of the pxPAC file
 * @return Whether the download was successful or not
 */
__attribute__ ((visibility("default")))
bool px_pac_reload(pxPAC *self);

#endif /*PAC_H_*/
