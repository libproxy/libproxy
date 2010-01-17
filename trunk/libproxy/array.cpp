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

#include "misc.hpp"
#include "array.hpp"

struct _pxArray {
	pxArrayItemsEqual   equals;
	pxArrayItemCallback free;
	bool                unique;
	bool                replace;
	unsigned int        length;
	void              **data;
};

static bool
identity(void *a, void *b)
{
	return (a == b);
}

static void
nothing(void *a)
{
	return;
}

pxArray *
px_array_new(pxArrayItemsEqual equals, pxArrayItemCallback free, bool unique, bool replace)
{
	pxArray *self = (pxArray *) px_malloc0(sizeof(pxArray));
	self->equals  = equals ? equals : &identity;
	self->free    = free   ? free   : &nothing;
	self->unique  = unique;
	self->replace = replace;
	self->length  = 0;
	self->data    = NULL;
	return self;
}

bool
px_array_add(pxArray *self, void *item)
{
	/* Verify some basic stuff */
	if (!self || !item) return false;

	/* If we are a unique array and a dup is found, either bail or replace the item */
	if (self->unique && px_array_find(self, item) >= 0)
	{
		if (!self->replace)
			return false;

		self->free(self->data[px_array_find(self, item)]);
		self->data[px_array_find(self, item)] = item;
		return true;
	}

	void **data = (void **) px_malloc0(sizeof(void *) * (self->length + 1));
	memcpy(data, self->data, sizeof(void *) * self->length);
	data[self->length++] = item;
	px_free(self->data);
	self->data = data;
	return true;
}

bool
px_array_del(pxArray *self, const void *item)
{
	int index = px_array_find(self, item);
	if (index < 0) return false;

	/* Free the old one and shift elements down */
	self->free(self->data[index]);
	memmove(self->data+index,
			self->data+index+1,
			sizeof(void *) * (--self->length - index));

	return true;
}

void
px_array_foreach(pxArray *self, pxArrayItemCallbackWithArg cb, void *arg)
{
	for (int i=0 ; i < self->length ; i++)
		cb(self->data[i], arg);
}

int
px_array_find(pxArray *self, const void *item)
{
	if (!self || !item) return -1;

	for (int i=0 ; i < self->length ; i++)
		if (self->equals(self->data[i], (void *) item))
			return i;

	return -1;
}

const void *
px_array_get(pxArray *self, int index)
{
	if (!self)                 return NULL;
	if (index < 0) index = self->length + index;
	if (index < 0)             return NULL;
	if (index >= self->length) return NULL;

	return self->data[index];
}

void
px_array_free(pxArray *self)
{
	if (!self) return;

	// NOTE: We free in the reverse order of allocation
	// This fixes an odd bug where dlopen()'d modules need
	// to be dlclose()'d in the reverse order of their opening
	for (int i=self->length-1 ; i >= 0 ; i--)
		self->free(self->data[i]);
	px_free(self->data);
	px_free(self);
}

int
px_array_length(pxArray *self)
{
	if (!self) return -1;

	return self->length;
}

void
px_array_sort(pxArray *self, int (*compare)(const void *, const void *))
{
	qsort(self->data, self->length, sizeof(void *), compare);
}

