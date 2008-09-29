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
#define _GNU_SOURCE

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
 * @buffer The buffer to write to (usually NULL)
 * @bufsize The size of the buffer (usually 0)
 * @return Newly allocated string containing one line only
 */
static char *
readline(int fd, char *buffer, size_t bufsize)
{
	char c = '\0';

	/* Verify we have an open socket */
	if (fd < 0) return NULL;

	/* Read a character.  If we don't get a character, return the buffer. */
	if (read(fd, &c, 1) != 1) return buffer;

	/* If we are at the end of the line, return. */
	if (c == '\n') return buffer ? buffer : strdup("");

	/* We have a character, make sure we have a buffer. */
	if (!buffer)
	{
		assert((buffer = strdup("")));
		bufsize = 0;
	}

	/* If our buffer is full, add more to the buffer. */
	if (bufsize <= strlen(buffer))
	{
		char *tmp = NULL;
		assert((tmp = malloc(1024 + strlen(buffer) + 1)));
		strcpy(tmp, buffer);
		free(buffer);
		buffer = tmp;
		bufsize = strlen(buffer) + 1024;
	}

	strncat(buffer, &c, 1);
	return readline(fd, buffer, bufsize);
}

/**
 * Prints an array of proxie. Proxies are space separated.
 * @proxies an array containing the proxies returned by libproxy.
 */
void
print_proxies(char **proxies)
{
	for (int j = 0; proxies[j] ; j++)
	{
		printf(proxies[j]);
		if (proxies[j+1])
			printf(" ");
		else
			printf("\n");
		free(proxies[j]);
	}
	free(proxies);
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
	/* User entered some arguments on startup. skip interactive */
	if (argc > 1) 
	{ 
		for(int i = 1; i < argc ; i++) 
		{
			/*
			 * Get an array of proxies to use. These should be used
			 * in the order returned. Only move on to the next proxy
			 * if the first one fails (etc).
			 */
			print_proxies(px_proxy_factory_get_proxies(pf, argv[i]));
		}	
	}
	/* Interactive mode */
	else
	{ 
		/* For each URL we read on STDIN, get the proxies to use */
		for (char *url = NULL ; url = readline(STDIN, NULL, 0) ; free(url))
		{
			/*
			 * Get an array of proxies to use. These should be used
			 * in the order returned. Only move on to the next proxy
			 * if the first one fails (etc).
			 */
			print_proxies(px_proxy_factory_get_proxies(pf, url));
		}
	}
	/* Destantiate the proxy factory object */
	px_proxy_factory_free(pf);
	return 0;
}
