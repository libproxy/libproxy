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

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>

#ifdef _WIN32
#define _WIN32_WINNT 0x0501
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#endif

#include "misc.h"
#include "url.h"

/**
 * pxURL object. All fields are private.
 */
struct _pxURL {
	char             *url;
	char             *scheme;
	char             *username;
	char             *password;
	char             *host;
	int               port;
	char             *path;
	struct sockaddr **ips;
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
			px_free(self->ips[i]);
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
px_url_get(pxURL *self, const char **headers)
{
	char *request = NULL;
	char *joined_headers = NULL;
	int sock = -1;

	/* in case of a file:// url we open the file and return a hande to it */
	if (!strcmp(self->scheme,"file"))
		return open(self->path, O_RDONLY);

	/* DNS lookup of host */
	if (!px_url_get_ips(self, true)) goto error;

	/* Iterate through each pxIP trying to make a connection */
	for (int i = 0 ;  self->ips && self->ips[i] && sock < 0 ; i++)
	{
		sock = socket(self->ips[i]->sa_family, SOCK_STREAM, 0);
		if (sock < 0) continue;

		if (self->ips[i]->sa_family == AF_INET &&
			!connect(sock, self->ips[i], sizeof(struct sockaddr_in)))
			break;
		else if (self->ips[i]->sa_family == AF_INET6 &&
			!connect(sock, self->ips[i], sizeof(struct sockaddr_in6)))
			break;

		close(sock);
		sock = -1;
	}
	if (sock < 0) goto error;

	/* Merge optional headers */
	if (headers)
	{
		joined_headers = px_strjoin(headers, "\r\n");
		if (!joined_headers) goto error;
	}
	else
		joined_headers = px_strdup("");

	/* Create request header */
	request = px_strcat("GET ", px_url_get_path(self),
						" HTTP/1.1\r\nHost: ", px_url_get_host(self),
						"\r\n", joined_headers, "\r\n\r\n", NULL);
	px_free(joined_headers);

	/* Send HTTP request */
	if (send(sock, request, strlen(request), 0) != strlen(request)) goto error;
	px_free(request); request = NULL;

	/* Return the socket, which is ready for reading the response */
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
 * Get the IP addresses of the hostname in this pxURL.
 * @usedns Should we look up hostnames in DNS?
 * @return IP addresses of the host in the pxURL.
 */
const struct sockaddr **
px_url_get_ips(pxURL *self, bool usedns)
{
	if (!self) return NULL;

	/* Check the cache */
	if (self->ips) return (const struct sockaddr **) self->ips;

	/* Check without DNS first */
	if (usedns && px_url_get_ips(self, false)) return (const struct sockaddr **) self->ips;

	/* Check DNS for IPs */
	struct addrinfo *info;
	struct addrinfo flags;
	flags.ai_family   = AF_UNSPEC;
	flags.ai_socktype = 0;
	flags.ai_protocol = 0;
	flags.ai_flags    = AI_NUMERICHOST;
	if (!getaddrinfo(px_url_get_host(self), NULL, usedns ? NULL : &flags, &info))
	{
		struct addrinfo *first = info;
		int count;

		/* Count how many IPs we got back */
		for (count=0 ; info ; info = info->ai_next)
			count++;

		/* Copy the sockaddr's into self->ips */
		info = first;
		self->ips = px_malloc0(sizeof(struct sockaddr *) * ++count);
		for (int i=0 ; info ; info = info->ai_next)
		{
			if (info->ai_addr->sa_family == AF_INET)
			{
				self->ips[i] = px_malloc0(sizeof(struct sockaddr_in));
				memcpy(self->ips[i], info->ai_addr, sizeof(struct sockaddr_in));
				((struct sockaddr_in *) self->ips[i++])->sin_port = htons(self->port);
			}
			else if (info->ai_addr->sa_family == AF_INET6)
			{
				self->ips[i] = px_malloc0(sizeof(struct sockaddr_in6));
				memcpy(self->ips[i], info->ai_addr, sizeof(struct sockaddr_in6));
				((struct sockaddr_in6 *) self->ips[i++])->sin6_port = htons(self->port);
			}
		}

		freeaddrinfo(first);
		return (const struct sockaddr **) self->ips;
	}

	/* No addresses found */
	return NULL;
}

/**
 * @return Password portion of the pxURL
 */
const char *
px_url_get_password(pxURL *self)
{
	if (!self) return NULL;
	return self->password;
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
 * @return Username portion of the pxURL
 */
const char *
px_url_get_username(pxURL *self)
{
	if (!self) return NULL;
	return self->username;
}

/**
 * @url String used to create the new pxURL object
 * @return New pxURL object
 */
pxURL *
px_url_new(const char *url)
{
	if (!url) return NULL;
	const char *start = url;

	/* Allocate pxURL */
	pxURL *self  = px_malloc0(sizeof(pxURL));

	/* Get scheme */
	if (!strstr(start, "://")) goto error;
	self->scheme = px_strndup(start, strstr(start, "://") - start);
	start += strlen(self->scheme) + 3;

	/* If we have a username and password */
	if (strchr(start, '@') && (strchr(start, '/') > strchr(start, '@') || strchr(start, '/') == NULL))
	{
		if (!strchr(start, ':')) goto error; // Can't find user/pass delimiter
		self->username = px_strndup(start, strchr(start, ':') - start);
		start += strlen(self->username) + 1;
		self->password = px_strndup(start, strchr(start, '@') - start);
		start += strlen(self->password) + 1;
	}

	/* Get host */
	self->host   = px_strdup(start);

	/* Get path */
	self->path   = px_strdup(strchr(self->host, '/'));
	if (self->path)
		self->host[strlen(self->host) - strlen(self->path)] = 0;
	else
		self->path = px_strdup("");

	/* Get the port */
	bool port_specified = false;
	if (strchr(self->host, ':')) {
		if (!atoi(strchr(self->host, ':')+1)) goto error;
		self->port = atoi(strchr(self->host, ':')+1);
		*(strchr(self->host, ':')) = 0;
		port_specified = true;
	}
	else
		self->port = px_url_get_default_port(self);

	/* Make sure we have a real host */
	if (!strcmp(self->host, "") && strcmp(self->scheme, "file")) goto error;

	/* Verify by re-assembly */
	self->url = px_malloc0(strlen(url) + 1);
	if (self->username && self->password)
		snprintf(self->url, strlen(url) + 1, "%s://%s:%s@%s", self->scheme, self->username, self->password, self->host);
	else
		snprintf(self->url, strlen(url) + 1, "%s://%s", self->scheme, self->host);
	if (port_specified)
		snprintf(self->url + strlen(self->url), strlen(url) + 1 - strlen(self->url), ":%d%s", self->port, self->path);
	else
		snprintf(self->url + strlen(self->url), strlen(url) + 1 - strlen(self->url), "%s", self->path);
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
