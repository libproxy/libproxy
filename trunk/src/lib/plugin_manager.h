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

#ifndef PLUGIN_MANAGER_H_
#define PLUGIN_MANAGER_H_
#include "plugin.h"
#include "array.h"

typedef struct _pxPluginManager pxPluginManager;

typedef bool (*pxModuleLoad)  (pxPluginManager *self);
typedef void (*pxModuleUnload)(pxPluginManager *self);

pxPluginManager *px_plugin_manager_new       ();
void             px_plugin_manager_free      (pxPluginManager *self);

bool             px_plugin_manager_load      (pxPluginManager *self, char *filename);
bool             px_plugin_manager_load_dir  (pxPluginManager *self, char *dirname);

bool             px_plugin_manager_constructor_add_full(pxPluginManager *self,
                                                        const char *name,
                                                        const char *type,
                                                        unsigned int version,
                                                        size_t size,
                                                        bool (*constructor)(pxPlugin *));
#define          px_plugin_manager_constructor_add(self, name, type, constructor) \
						px_plugin_manager_constructor_add_full(self, name, #type, type ## Version, sizeof(type), constructor)
#define          px_plugin_manager_constructor_add_subtype(self, name, type, subtype, constructor) \
						px_plugin_manager_constructor_add_full(self, name, #type, type ## Version, sizeof(subtype), constructor)

pxArray         *px_plugin_manager_instantiate_plugins_full(pxPluginManager *self,
                                                            const char *type,
                                                            unsigned int version);
#define          px_plugin_manager_instantiate_plugins(self, type) \
						px_plugin_manager_instantiate_plugins_full(self, #type, type ## Version)

/*
 * NOTE: the prototype MUST have a destructor.
 */
bool             px_plugin_manager_register_type_full(pxPluginManager *self,
                                                      const char *type,
                                                      unsigned int version,
                                                      size_t size,
                                                      const pxPlugin *prototype);
#define          px_plugin_manager_register_type(self, type, prototype) \
						px_plugin_manager_register_type_full(self, #type, type ## Version, sizeof(type), prototype)

#endif /* PLUGIN_MANAGER_H_ */
