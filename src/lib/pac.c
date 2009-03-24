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

#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>

#include "url.h"
#include "misc.h"
#include "pac.h"

#define PAC_MIME_TYPE "application/x-ns-proxy-autoconfig"

/**
 * ProxyAutoConfig object.  All fields are private.
 */
struct _pxPAC {
	pxURL     *url;
	char      *cache;
	time_t     expires;
};

/**
 * Frees memory used by the ProxyAutoConfig.
 */
void
px_pac_free(pxPAC *self)
{
	if (!self) return;
	px_url_free(self->url);
	px_free(self->cache);
	px_free(self);
}

/**
 * Get the URL which the pxPAC uses.
 * @return The URL that the pxPAC came from
 */
const pxURL *
px_pac_get_url(pxPAC *self)
{
	return self->url;
}

/**
 * Create a new ProxyAutoConfig.
 * @url The URL where the PAC file is found
 * @return A new ProxyAutoConfig or NULL on error
 */
pxPAC *
px_pac_new(pxURL *url)
{
	if (!url) return NULL;

	/* Allocate the object */
	pxPAC *self = px_malloc0(sizeof(pxPAC));

	/* Copy the given URL */
	self->url = px_url_new(px_url_to_string(url)); /* Always returns valid value */

	/* Make sure we have a real pxPAC */
	if (!px_pac_reload(self)) { px_pac_free(self); return NULL; }

	return self;
}

/**
 * Create a new ProxyAutoConfig (from a string for convenience).
 * @url The url (string) where the PAC file is found
 * @return A new ProxyAutoConfig or NULL on error
 */
pxPAC *
px_pac_new_from_string(char *url)
{
	/* Create temporary URL */
	pxURL *tmpurl = px_url_new(url);
	if (!tmpurl) return NULL;

	/* Create pac */
	pxPAC *self = px_pac_new(tmpurl);
	px_url_free(tmpurl); /* Free no longer used URL */
	if (!self) return NULL;
	return self;
}

/**
 * Returns the Javascript code which the pxPAC uses.
 * @return The Javascript code used by the pxPAC
 */
const char *
px_pac_to_string(pxPAC *self)
{
	return self->cache;
}

/**
 * Download the latest version of the pxPAC file
 * @return Whether the download was successful or not
 */
bool
px_pac_reload(pxPAC *self)
{
	int sock;
	char *line = NULL;
	const char *headers[3] = { "Accept: " PAC_MIME_TYPE, "Connection: close", NULL };
	bool correct_mime_type;
	unsigned long int content_length = 0;

	/* Get the pxPAC */
	sock = px_url_get(self->url, headers);
	if (sock < 0) return false;

	if (strcmp(px_url_get_scheme(self->url),"file"))
	{
		/* Verify status line */
		line = px_readline(sock, NULL, 0);
		if (!line)                                                    goto error;
		if (strncmp(line, "HTTP", strlen("HTTP")))                    goto error; /* Check valid HTTP response */
		if (!strchr(line, ' ') || atoi(strchr(line, ' ') + 1) != 200) goto error; /* Check status code */

		/* Check for correct mime type and content length */
		while (strcmp(line, "\r")) {
			/* Check for content type */
			if (strstr(line, "Content-Type: ") == line && strstr(line, PAC_MIME_TYPE))
				correct_mime_type = true;

			/* Check for content length */
			else if (strstr(line, "Content-Length: ") == line)
				content_length = atoi(line + strlen("Content-Length: "));

			/* Get new line */
			px_free(line);
			line = px_readline(sock, NULL, 0);
			if (!line) goto error;
		}

		/* Get content */
		if (!content_length || !correct_mime_type) goto error;
		px_free(line); line = NULL;
		px_free(self->cache);
		self->cache = px_malloc0(content_length+1);
		for (int recvd=0 ; recvd != content_length ; )
			recvd += recv(sock, self->cache + recvd, content_length - recvd, 0);
	}
	else
	{ /* file:// url */
		struct stat buffer;
		int status;

		status = fstat(sock, &buffer);
		self->cache = px_malloc0(buffer.st_blksize * buffer.st_blocks);
		status = read(sock, self->cache, buffer.st_blksize * buffer.st_blocks);
	}

	/* Clean up */
	close(sock);
	return true;

	error:
		px_free(self->cache); self->cache = NULL;
		if (sock >= 0) close(sock);
		px_free(line);
		return false;
}
