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

#ifndef MODULE_MANAGER_H_
#define MODULE_MANAGER_H_

#include <stdbool.h>
#include <assert.h>

/*
 * Define the pxModuleManager object
 */
typedef struct _pxModuleManager pxModuleManager;
typedef void *(*pxModuleConstructor)();
typedef void  (*pxModuleDestructor) (void *module);

typedef bool (*pxModuleLoadFunction)(pxModuleManager *);
typedef void (*pxModuleFreeFunction)(pxModuleManager *);


/*
 * Define the pxModuleRegistration object
 */
struct _pxModuleRegistration {
	char               *name;
	void               *instance;
	pxModuleConstructor new;
	pxModuleDestructor  free;
};
typedef struct _pxModuleRegistration pxModuleRegistration;
typedef int   (*pxModuleRegistrationComparison)(pxModuleRegistration **self, pxModuleRegistration **other);

pxModuleManager *px_module_manager_new       ();
void             px_module_manager_free      (pxModuleManager *self);

bool             px_module_manager_load      (pxModuleManager *self, char *filename);
bool             px_module_manager_load_dir  (pxModuleManager *self, char *dirname);

#define __str__(s) #s
#define __px_module_manager_get_id(type, version) #type "__" __str__(version)

bool   _px_module_manager_register_module_full(pxModuleManager *self, const char *id, const char *name, size_t namelen, pxModuleConstructor new, pxModuleDestructor free);
#define px_module_manager_register_module(self, type, new, free) \
	_px_module_manager_register_module_full(self, __px_module_manager_get_id(type, type ## Version), \
                                            __FILE__, strrchr(__FILE__, '.') ? strrchr(__FILE__, '.') - __FILE__ : strlen(__FILE__), \
                                            new, free)
#define px_module_manager_register_module_with_name(self, type, name, new, free) \
	_px_module_manager_register_module_full(self, __px_module_manager_get_id(type, type ## Version), name, strlen(name), new, free)

void **_px_module_manager_instantiate_type_full(pxModuleManager *self, const char *id);
#define px_module_manager_instantiate_type(self, type) \
	(type **) _px_module_manager_instantiate_type_full(self, __px_module_manager_get_id(type, type ## Version))

bool   _px_module_manager_register_type_full(pxModuleManager *self, const char *id, pxModuleRegistrationComparison cmp);
#define px_module_manager_register_type(self, type, cmp) \
	_px_module_manager_register_type_full(self, __px_module_manager_get_id(type, type ## Version), cmp)

#define PX_MODULE_SUBCLASS(type) type __parent__

#endif /* MODULE_MANAGER_H_ */
