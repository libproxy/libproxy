/*******************************************************************************
 * libproxy - A library for proxy configuration
 * Copyright (C) 2009 Nathaniel McCallum <nathaniel@natemccallum.com>
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

#ifndef PLUGIN_H_
#define PLUGIN_H_
#include <stdbool.h>

/*
 * This defines the prototype for a plugin.
 *
 * destructor() MUST ALWAYS BE SET!!!  Do it or
 * goats will eat one sock from every pair out of
 * your sock drawer... BE WARNED!
 *
 * 'name' will be automatically set by the plugin manager.
 * Same is true for 'type' and 'version'.
 */
typedef struct _pxPlugin {
	const char   *name;
	char         *type;
	unsigned int  version;
	bool        (*constructor)(struct _pxPlugin *);
	void        (*destructor) (struct _pxPlugin *self);
	int         (*compare)    (struct _pxPlugin *self, struct _pxPlugin *other);
} pxPlugin;

#define PX_PLUGIN_SUBCLASS(type) type __head__

/*
 * The default constructor which MUST be called.
 */
void px_plugin_destructor(pxPlugin *self);

#endif /* PLUGIN_H_ */
