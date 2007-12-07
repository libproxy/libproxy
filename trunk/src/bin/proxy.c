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

#include <stdio.h>

// External libproxy API
#include <proxy.h>

// Internal libproxy API
#include <proxy_factory.h>
#include <misc.h>

#define STDIN fileno(stdin)

int
main(int argc, char **argv)
{
	pxProxyFactory *pf = px_proxy_factory_new();
	if (!pf)
	{
		fprintf(stderr, "An unknown error occurred!\n");
		return 1;
	}
	
	for (char *line = NULL ; line = px_readline(STDIN) ; px_free(line))
	{
		char **proxy = px_proxy_factory_get_proxies(pf, line);
		for (int i = 0 ; proxy[i] ; i++)
		{
			printf(proxy[i]);
			if (proxy[i+1])
				printf(" ");
			else
				printf("\n");
		}
		px_strfreev(proxy);
	}
	
	px_proxy_factory_free(pf);
	return 0;
}
