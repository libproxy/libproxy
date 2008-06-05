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

/*
 * Concept and some basic code shamelessly stolen from xlsclients... :)
 * Reworked by Nathaniel McCallum, 2007
 */

#include <stdbool.h>
#include <stdarg.h>
#include <string.h>

#include <X11/Xlib.h>
#include <X11/Xmu/WinUtil.h>

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
