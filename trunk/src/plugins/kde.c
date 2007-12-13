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

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include <misc.h>
#include <proxy_factory.h>
#include <config_file.h>

#include <X11/Xlib.h>
#include <X11/Xmu/WinUtil.h>

// This function is shamelessly stolen from xlsclient... :)
/*
 *
 * 
Copyright 1989, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.
 * *
 * Author:  Jim Fulton, MIT X Consortium
 */
bool
x_has_client(char *prog, ...)
{
	va_list ap;
	
	// Open display
	Display *display = XOpenDisplay(NULL);
	if (!display)
		return false;
	
	// For each screen...
	for (int i=0; i < ScreenCount(display); i++)
	{
		Window dummy, *children = NULL;
		unsigned int nchildren = 0;
		
		// Get the root window's children
		if (!XQueryTree(display, RootWindow(display, i), &dummy, &dummy, &children, &nchildren))
			continue;
		
		// For each child on the screen...
	    for (int j=0; j < nchildren; j++)
	    {
	    	// If we can get their client info...
	    	Window client;
	    	if ((client = XmuClientWindow(display, children[j])) != None)
	    	{
	    		int argc;
	    		char **argv;
	    		
	    		// ... and if we can find out their command ...
	    		if (!XGetCommand (display, client, &argv, &argc) || argc == 0)
	    			continue;
	    		
	    		// ... check the commands against our list
	    		va_start(ap, prog);
	    		for (char *s = prog ; s ; s = va_arg(ap, char *))
	    		{
	    			// We've found a match, return...
	    			if (!strcmp(argv[0], s))
	    			{
	    				va_end(ap);
		    			XCloseDisplay(display);
		    			return true;
	    			}
	    		}
	    		va_end(ap);
	    	}
	    }
	}
	
	// Close the display
	XCloseDisplay(display);
	return false;
}

pxConfig *
kde_config_cb(pxProxyFactory *self)
{
	// TODO: make ignores work w/ KDE
	char *url = NULL, *ignore = NULL, *tmp = getenv("HOME");
	if (!tmp) return NULL;
	
	// Open the config file
	pxConfigFile *cf = px_proxy_factory_misc_get(self, "kde");
	if (!cf || px_config_file_is_stale(cf))
	{
		if (cf) px_config_file_free(cf);
		tmp = px_strcat(getenv("HOME"), "/.kde/share/config/kioslaverc", NULL);
		cf = px_config_file_new(tmp);
		px_free(tmp);
		px_proxy_factory_misc_set(self, "kde", cf);
	}
	if (!cf)  goto out;
	
	// Read the config file to find out what type of proxy to use
	tmp = px_config_file_get_value(cf, "Proxy Settings", "ProxyType");
	if (!tmp) { px_config_file_free(cf); goto out; }
	
	// Don't use any proxy
	if (!strcmp(tmp, "0"))
		url = px_strdup("direct://");
		
	// Use a manual proxy
	else if (!strcmp(tmp, "1"))
		url = px_config_file_get_value(cf, "Proxy Settings", "httpProxy");
		
	// Use a manual PAC
	else if (!strcmp(tmp, "2"))
	{
		px_free(tmp);
		tmp = px_config_file_get_value(cf, "Proxy Settings", "Proxy Config Script");
		if (tmp) url = px_strcat("pac+", tmp);
		else     url = px_strdup("wpad://");
	}
	
	// Use WPAD
	else if (!strcmp(tmp, "3"))
		url = px_strdup("wpad://");
	
	// Use envvar
	else if (!strcmp(tmp, "4"))
		url = NULL; // We'll bypass this config plugin and let the envvar plugin work
	
	// Cleanup
	px_free(tmp);
	px_config_file_free(cf);
			
	out:
		return px_config_create(url, ignore);
}

bool
on_proxy_factory_instantiate(pxProxyFactory *self)
{
	// If we are running in KDE, then make sure this plugin is registered.
	if (x_has_client("kicker", NULL))
		return px_proxy_factory_config_add(self, "kde", PX_CONFIG_CATEGORY_SESSION, 
											(pxProxyFactoryPtrCallback) kde_config_cb);
	return false;
}

void
on_proxy_factory_destantiate(pxProxyFactory *self)
{
	px_proxy_factory_config_del(self, "kde");
	px_config_file_free(px_proxy_factory_misc_get(self, "kde"));
	px_proxy_factory_misc_set(self, "kde", NULL);
}
