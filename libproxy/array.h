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

#ifndef ARRAY_H_
#define ARRAY_H_
#include <stdbool.h>

typedef bool (*pxArrayItemsEqual)(void *, void *);
typedef void (*pxArrayItemCallback)(void *);
typedef void (*pxArrayItemCallbackWithArg)(void *, void *);
typedef struct _pxArray pxArray;

pxArray *px_array_new(pxArrayItemsEqual equals, pxArrayItemCallback free, bool unique, bool replace);

bool px_array_add(pxArray *self, void *item);

bool px_array_del(pxArray *self, const void *item);

void px_array_foreach(pxArray *self, pxArrayItemCallbackWithArg cb, void *arg);

int px_array_find(pxArray *self, const void *item);

const void *px_array_get(pxArray *self, int index);

void px_array_free(pxArray *self);

int px_array_length(pxArray *self);

void px_array_sort(pxArray *self, int (*compare)(const void *, const void *));

#endif /* ARRAY_H_ */
