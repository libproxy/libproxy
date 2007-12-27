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
#include <string.h>

#include "misc.h"
#include "array.h"
#include "strdict.h"

struct _pxStrDict {
	pxStrDictItemCallback free;
	pxArray              *data;
};

static bool
dict_equals(void *a, void *b)
{
	if (!a || !b) return false;
	void **aa = (void **) a;
	void **bb = (void **) b;
	return (!strcmp((char *) *aa, (char *) *bb));
}

static void
dict_free(void *item)
{
	char *key = (char *) ((void **) item)[0];
	void **realitem = ((void **) item)[1];
	pxStrDictItemCallback do_free = ((void **) item)[2];
	
	do_free(realitem);
	px_free(key);
	px_free(item);
}

static void
dict_foreach(void *item, void *misc)
{
	pxStrDictForeachCallback foreach = ((void **) misc)[0];
	char *key = ((void **) item)[0];
	void *val = ((void **) item)[1];
	void *arg = ((void **) misc)[1];
	foreach(key, val, arg);
}

static void
do_nothing(void *item)
{
}

pxStrDict *px_strdict_new(pxStrDictItemCallback free)
{
	pxStrDict *self = px_malloc0(sizeof(pxStrDict));
	self->free      = free ? free : do_nothing;
	self->data      = px_array_new(dict_equals, dict_free, true, false);
	return self;
}

bool
px_strdict_set(pxStrDict *self, const char *key, void *value)
{
	if (!self || !key) return false;
	
	// We are unseting the value
	if (!value)
	{
		void *item[3] = { (void *) key, value, self->free };
		return px_array_del(self->data, item);
	}
	
	void **item = px_malloc0(sizeof(void *) * 3);
	item[0] = px_strdup(key);
	item[1] = value;
	item[2] = self->free;
	
	if (px_array_add(self->data, item))
		return true;

	px_free(*item);
	px_free(item);
	return false;
}

const void *
px_strdict_get(pxStrDict *self, const char *key)
{
	void *v[3] = { (void *) key, NULL, NULL };
	int i = px_array_find(self->data, v);
	if (i < 0) return NULL;
	return px_array_get(self->data, i);
}

void
px_strdict_foreach(pxStrDict *self, pxStrDictForeachCallback *cb, void *arg)
{
	void *v[2] = { cb, arg }; 
	px_array_foreach(self->data, dict_foreach, v);
}

void
px_strdict_free(pxStrDict *self)
{
	px_array_free(self->data);
	px_free(self);
}
