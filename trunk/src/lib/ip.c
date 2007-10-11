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

#include <stdint.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <stdio.h>

#include "misc.h"
#include "ip.h"

/**
 * pxIP object. All fields are private.
 */
struct _IP {
	pxIPVersion version;
	int         length;
	char        data[16];
};

/**
 * Opens an IP connection to the IP address at the specified port and connection type.
 * @conntype The type of connection to create (TCP or UDP)
 * @port The port to connect to
 * @return The open socket from the connection
 */
int
px_ip_connect(pxIP *self, pxIPConnectionType conntype, int port)
{
	struct sockaddr *dest_addr;
	int addr_len = 0, domain = 0;
	
	if (self->version == pxIPv4) // Create IPv4 dest_addr
	{
		struct sockaddr_in *tmp = px_malloc0(sizeof(struct sockaddr_in));
		tmp->sin_family = AF_INET;
		memcpy(&(tmp->sin_addr.s_addr), self->data, self->length);
		tmp->sin_port   = htons(port);
		dest_addr       = (struct sockaddr *) tmp;
		addr_len        = sizeof(struct sockaddr_in);
		domain          = PF_INET;
	}	
	else if (self->version == pxIPv6) // Create IPv6 dest_addr
	{
		struct sockaddr_in6 *tmp = px_malloc0(sizeof(struct sockaddr_in6));
		tmp->sin6_family = AF_INET6;
		memcpy(&(tmp->sin6_addr.in6_u), self->data, self->length);
		tmp->sin6_port   = htons(port);
		dest_addr        = (struct sockaddr *) tmp;
		addr_len         = sizeof(struct sockaddr_in6);
		domain           = PF_INET6;
	} 
	else
		return -1;
	
	// Create socket
	int sock = -1;
	if      (conntype == TCP) sock = socket(domain, SOCK_STREAM, 0);
	else if (conntype == UDP) sock = socket(domain, SOCK_DGRAM,  0);
	
	// Make sure we have a valid socket
	if (sock < 0) { px_free(dest_addr); return sock; }
	
	// Connect
	if (connect(sock, dest_addr, addr_len))
	{
		close(sock);
		px_free(dest_addr);
		return -1;
	}
	
	px_free(dest_addr);
	return sock;
}

/**
 * Frees an pxIP object
 */
void
px_ip_free(pxIP *self)
{
	px_free(self);
}

/**
 * Returns the version of the IP address (pxIPv4 or pxIPv6)
 * @return pxIPv4 or pxIPv6
 */
pxIPVersion
px_ip_get_version(pxIP *self)
{
	return self->version;
}

/**
 * Creates a new pxIP from raw data.
 * @version The version of the IP address (pxIPv4 or pxIPv6)
 * @data The raw data to use (correct byte order is assumed)
 * @return New pxIP from the raw data
 */
pxIP *
px_ip_new_from_data(pxIPVersion version, char *data)
{
	if (!data) return NULL;
	
	// Create the pxIP object
	pxIP *self = px_malloc0(sizeof(pxIP));
	
	// Set the version and length
	self->version = version;
	if      (version == pxIPv4) self->length = 4;
	else if (version == pxIPv6) self->length = 16;
	else { px_free(self); return NULL; }
	
	// Copy the byte data
	memcpy(self->data, data, self->length);
	return self;
}

/**
 * Creates a new pxIP from a hostent struct
 * @host The hostent struct to use
 * @return The first pxIP from the hostent struct
 */
pxIP *
px_ip_new_from_hostent(const struct hostent *host)
{
	if (!host) return NULL;
	
	if      (host->h_addrtype == AF_INET)
		return px_ip_new_from_data(pxIPv4, host->h_addr);
	else if (host->h_addrtype == AF_INET6)
		return px_ip_new_from_data(pxIPv6, host->h_addr);

	return NULL;
}

/**
 * Creates all possible new pxIPs from a hostent struct
 * @host The hostent struct to use
 * @return NULL-terminated array of all possible pxIPs
 */
pxIP **
px_ip_new_from_hostent_all(const struct hostent *host)
{
	int version, count;
	if (!host) return NULL;
	
	// Get the version
	if      (host->h_addrtype == AF_INET)  version = pxIPv4;
	else if (host->h_addrtype == AF_INET6) version = pxIPv6;
	else                                   return NULL;
	
	// Count the total ip addresses
	for (count = 0 ; host->h_addr_list[count] ; count++);
	
	// Allocate
	pxIP **ips = px_malloc0(sizeof(pxIP *) * (count + 1));
	
	// Create
	for (count = 0 ; host->h_addr_list[count] ; count++)
	{
		ips[count] = px_ip_new_from_data(version, host->h_addr_list[count]);
		if (!ips[count]) goto error;
	}
		
	return ips;
	
	error:
		for (count = 0 ; host->h_addr_list[count] ; count++)
			px_free(ips[count]);
		px_free(ips);
		return NULL;
}

/**
 * Creates a string representation of the IP address.
 * @return Newly allocated string representing the IP address
 */
char *
px_ip_to_string(pxIP *self)
{
	if (self->version != pxIPv4 && self->version != pxIPv6) return NULL;
	
	char *string = px_malloc0(40);
	if (self->version == pxIPv4)
		sprintf(string, "%hhu.%hhu.%hhu.%hhu", self->data[0], 
		                                       self->data[1],
		                                       self->data[2],
		                                       self->data[3]);
	else if (self->version == pxIPv6)
		sprintf(string, "%hhx%hhx:%hhx%hhx:%hhx%hhx:%hhx%hhx:%hhx%hhx:%hhx%hhx:%hhx%hhx:%hhx%hhx",
		        self->data[0], 
		        self->data[1],
		        self->data[2],
		        self->data[3],
		        self->data[4],
		        self->data[5],
		        self->data[6],
		        self->data[7],
		        self->data[8],
		        self->data[9],
		        self->data[10],
		        self->data[11],
		        self->data[12],
		        self->data[13],
		        self->data[14],
		        self->data[15]);
	
	return string;
}


