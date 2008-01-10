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

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

/* Import libproxy API */
#include <proxy.h>

#define STDIN fileno(stdin)

/**
 * Reads a single line of text from the specified file descriptor
 * @fd File descriptor to read from
 * @return Newly allocated string containing one line only
 */
static char *
readline(int fd)
{
	/* Verify we have an open socket */
	if (fd < 0) return NULL;
	
	/* For each character received add it to the buffer unless it is a newline */
	char *buffer = NULL;
	for (int i=1; i > 0 ; i++)
	{
		char c;
		
		/* Receive a single character, check for newline or EOF */
		if (read(fd, &c, 1) != 1) return buffer;
		if (c == '\n')            return buffer ? buffer : strdup("");

		/* Allocate new buffer if we need */
		if (i % 1024 == 1)
		{
			char *tmp = buffer;
			buffer = malloc(1024 * i + 1);
			assert(buffer != NULL);
			if (tmp) { strcpy(buffer, tmp); free(tmp); }
		}

		/* Add new character */
		buffer[i-1] = c;
	}
	return buffer;
}

int
main(int argc, char **argv)
{
	/* Create the proxy factory object */
	pxProxyFactory *pf = px_proxy_factory_new();
	if (!pf)
	{
		fprintf(stderr, "An unknown error occurred!\n");
		return 1;
	}
	
	/* For each URL we read on STDIN, get the proxies to use */
	for (char *url = NULL ; url = readline(STDIN) ; free(url))
	{
		/*
		 * Get an array of proxies to use. These should be used
		 * in the order returned. Only move on to the next proxy
		 * if the first one fails (etc).
		 */
		char **proxies = px_proxy_factory_get_proxies(pf, url);
		for (int i = 0 ; proxies[i] ; i++)
		{
			printf(proxies[i]);
			if (proxies[i+1])
				printf(" ");
			else
				printf("\n");
			free(proxies[i]);
		}
		free(proxies);
	}
	
	/* Destantiate the proxy factory object */
	px_proxy_factory_free(pf);
	return 0;
}
