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

#include <netdb.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "misc.h"
#include "url.h"

/**
 * pxURL object. All fields are private.
 */
struct _pxURL {
	char  *url;
	char  *scheme;
	char  *host;
	int    port;
	char  *path;
	pxIP **ips;
};

/**
 * @return Frees the pxURL
 */
void
px_url_free(pxURL *self)
{
	if (!self) return;
	px_free(self->url);
	px_free(self->scheme);
	px_free(self->host);
	px_free(self->path);
	if (self->ips)
	{
		for (int i=0 ; self->ips[i] ; i++)
			px_ip_free(self->ips[i]);
		px_free(self->ips);
	}
	px_free(self);
}

/**
 * Tells whether or not a pxURL string has valid syntax
 * @return true if the pxURL is valid, otherwise false
 */
bool
px_url_is_valid(const char *url)
{
	pxURL *tmp = px_url_new(url);
	if (!tmp) return false;
	px_url_free(tmp);
	return true;
}

/**
 * Sends a get request for the pxURL.
 * @headers A list of headers to be included in the request.
 * @return Socket to read the response on.
 */
int
px_url_open(pxURL *self, const char **headers)
{
	char *request = NULL;
	char *joined_headers = NULL;
	int sock = -1;
	
	// DNS lookup of host
	if (!px_url_get_ips(self)) goto error;
	
	// Iterate through each pxIP trying to make a connection
	px_url_get_ips(self); // Do lookup
	for (int i = 0 ;  self->ips && self->ips[i] && sock < 0 ; i++)
		sock = px_ip_connect(self->ips[i], TCP, px_url_get_port(self));
	if (sock < 0) goto error;
	
	// Merge optional headers
	if (headers)
	{
		joined_headers = px_strjoin(headers, "\r\n");
		if (!joined_headers) goto error;
	}
	else
		joined_headers = px_strdup("");

	// Create request header
	request = px_strcat("GET ", px_url_get_path(self), 
						" HTTP/1.1\r\nHost: ", px_url_get_host(self),
						"\r\n", joined_headers, "\r\n\r\n", NULL);
	px_free(joined_headers);
			
	// Send HTTP request
	if (send(sock, request, strlen(request), 0) != strlen(request)) goto error;
	px_free(request); request = NULL;
	
	// Return the socket, which is ready for reading the response
	return sock;
	
	error:
		if (sock >= 0) close(sock);
		px_free(request);
		return -1;
}

/**
 * @return The default port for this type of pxURL
 */
static int
px_url_get_default_port(pxURL *self)
{
	struct servent *serv;
	if ((serv = getservbyname(self->scheme, NULL))) return ntohs(serv->s_port);
	return 0;
}

/**
 * @return Host portion of the pxURL
 */
const char *
px_url_get_host(pxURL *self)
{
	if (!self) return NULL;
	return self->host;
}

/**
 * @return pxIP address of the host in the pxURL
 */
const pxIP **
px_url_get_ips(pxURL *self)
{
	if (!self) return NULL;
	
	// Check the cache
	if (self->ips) return (const pxIP **) self->ips;
	
	self->ips = px_ip_new_from_hostent_all(gethostbyname(self->host));
	return (const pxIP **) self->ips;
}

/**
 * @return Path portion of the pxURL
 */
const char *
px_url_get_path(pxURL *self)
{
	if (!self) return NULL;
	return self->path;
}

/**
 * @return Port portion of the pxURL
 */
int
px_url_get_port(pxURL *self)
{
	if (!self) return 0;
	return self->port;
}

/**
 * @return Scheme portion of the pxURL
 */
const char *
px_url_get_scheme(pxURL *self)
{
	if (!self) return NULL;
	return self->scheme;
}

/**
 * @url String used to create the new pxURL object
 * @return New pxURL object
 */
pxURL *
px_url_new(const char *url)
{
	// Allocate pxURL
	pxURL *self  = px_malloc0(sizeof(pxURL));
	
	// Get scheme
	if (!strstr(url, "://")) goto error;
	self->scheme = px_strndup(url, strstr(url, "://") - url);
	
	// Get host
	self->host   = px_strdup(strstr(url, "://") + strlen("://"));
	
	// Get path
	self->path   = px_strdup(strchr(self->host, '/'));
	if (self->path)
		self->host[strlen(self->host) - strlen(self->path)] = 0;
	else
		self->path = px_strdup("");
		
	// Get the port
	bool port_specified = false;
	if (strchr(self->host, ':')) {
		if (!atoi(strchr(self->host, ':')+1)) goto error;
		self->port = atoi(strchr(self->host, ':')+1);
		*(strchr(self->host, ':')) = 0;
		port_specified = true;
	}
	else
		self->port = px_url_get_default_port(self);
		
	// Make sure we have a real host
	if (!strcmp(self->host, "")) goto error;
	
	// Verify by re-assembly
	self->url = px_malloc0(strlen(url) + 1);
	if (!port_specified)
		snprintf(self->url, strlen(url) + 1, "%s://%s%s", self->scheme, self->host, self->path);
	else
		snprintf(self->url, strlen(url) + 1, "%s://%s:%d%s", self->scheme, self->host, self->port, self->path);
	if (strcmp(self->url, url)) goto error;
	
	return self;
	
	error:
		px_url_free(self);
		return NULL;
}

/**
 * @return String representation of the pxURL
 */
const char *
px_url_to_string(pxURL *self)
{
	if (!self) return NULL;
	return self->url;
}

/**
 * @return true if the URLs are the same, else false
 */
bool
px_url_equals(pxURL *self, const pxURL *other)
{
	return !strcmp(px_url_to_string(self), px_url_to_string((pxURL*) other));
}
