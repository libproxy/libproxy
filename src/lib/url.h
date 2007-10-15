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

#ifndef URL_H_
#define URL_H_

#include "ip.h"      // For class pxURL
#include "stdbool.h" // For type bool

/**
 * WPAD object. All fields are private.
 */
typedef struct _pxURL pxURL;

/**
 * @return Frees the pxURL
 */
void px_url_free(pxURL *self);

/**
 * Tells whether or not a pxURL string has valid syntax
 * @return true if the pxURL is valid, otherwise false
 */
bool px_url_is_valid(const char *url);

/**
 * Sends a get request for the pxURL.
 * @headers A list of headers to be included in the request.
 * @return Socket to read the response on.
 */
int px_url_open(pxURL *self, const char **headers);

/**
 * @return Host portion of the pxURL
 */
const char *px_url_get_host(pxURL *self);

/**
 * @return pxIP address of the host in the pxURL
 */
const pxIP **px_url_get_ips(pxURL *self);

/**
 * @return Path portion of the pxURL
 */
const char *px_url_get_path(pxURL *self);

/**
 * @return Port portion of the pxURL
 */
int px_url_get_port(pxURL *self);

/**
 * @return Scheme portion of the pxURL
 */
const char *px_url_get_scheme(pxURL *self);

/**
 * @url String used to create the new pxURL object
 * @return New pxURL object
 */
pxURL *px_url_new(const char *url);

/**
 * @return String representation of the pxURL
 */
const char *px_url_to_string(pxURL *self);

/**
 * @return true if the URLs are the same, else false
 */
bool px_url_equals(pxURL *self, const pxURL *other);

#endif /*URL_H_*/
