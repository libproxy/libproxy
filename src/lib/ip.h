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

#ifndef IP_H_
#define IP_H_

#include <netdb.h> // For struct hostent

/**
 * Version of the pxIP protocol to use
 */
enum _pxIPVersion {
	pxIPv4 = 4,
	pxIPv6 = 6,
};
typedef enum _pxIPVersion pxIPVersion;

/**
 * Types of pxIP connections
 */
enum _pxIPConnectionType {
	TCP = 0,
	UDP = 1,
};
typedef enum _pxIPConnectionType pxIPConnectionType;

/**
 * pxIP object. All fields are private.
 */
typedef struct _IP pxIP;

/**
 * Opens an IP connection to the IP address at the specified port and connection type.
 * @conntype The type of connection to create (TCP or UDP)
 * @port The port to connect to
 * @return The open socket from the connection
 */
int px_ip_connect(pxIP *self, pxIPConnectionType conntype, int port);

/**
 * Frees an pxIP object
 */
void px_ip_free(pxIP *self);

/**
 * Returns the version of the IP address (pxIPv4 or pxIPv6)
 * @return pxIPv4 or pxIPv6
 */
pxIPVersion px_ip_get_version(pxIP *self);

/**
 * Creates a new pxIP from raw data.
 * @version The version of the IP address (pxIPv4 or pxIPv6)
 * @data The raw data to use (correct byte order is assumed)
 * @return New pxIP from the raw data
 */
pxIP *px_ip_new_from_data(pxIPVersion version, char *data);

/**
 * Creates a new pxIP from a hostent struct
 * @host The hostent struct to use
 * @return The first pxIP from the hostent struct
 */
pxIP *px_ip_new_from_hostent(const struct hostent *host);

/**
 * Creates all possible new pxIPs from a hostent struct
 * @host The hostent struct to use
 * @return NULL-terminated array of all possible pxIPs
 */
pxIP **px_ip_new_from_hostent_all(const struct hostent *host);

/**
 * Creates a string representation of the IP address.
 * @return Newly allocated string representing the IP address
 */
char *px_ip_to_string(pxIP *self);

#endif /*IP_H_*/
