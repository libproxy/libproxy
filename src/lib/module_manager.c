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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <math.h>

#ifdef _WIN32
#include <windows.h>
#define pdlmtype HMODULE
#define pdlopen(filename) LoadLibrary(filename)
#define pdlsym GetProcAddress
#define pdlclose FreeLibrary
#else
#include <dlfcn.h>
#define pdlmtype void *
#define pdlopen(filename) dlopen(filename, RTLD_NOW | RTLD_LOCAL)
#define pdlsym dlsym
#define pdlclose dlclose
#endif

#include "module_manager.h"
#include "misc.h"
#include "array.h"
#include "strdict.h"

struct _pxModuleManager {
	pxArray   *dlmodules;
	pxStrDict *registrations;
	pxStrDict *types;
};

static void
regfree(pxModuleRegistration *self)
{
	px_free(self->name);
	if (self->instance)
		self->free(self->instance);
	px_free(self);
}

static bool
regeq(pxModuleRegistration *self, pxModuleRegistration *other)
{
	return !strcmp(self->name, other->name);
}

pxModuleManager *
px_module_manager_new()
{
	pxModuleManager *self = px_malloc0(sizeof(pxModuleManager));
	self->dlmodules     = px_array_new(NULL, (void *) pdlclose, true, false);
	self->registrations = px_strdict_new((void *) px_array_free);
	self->types         = px_strdict_new(NULL);
	return self;
}

void
px_module_manager_free(pxModuleManager *self)
{
	px_strdict_free(self->types);
	px_strdict_free(self->registrations);
	px_array_free(self->dlmodules);
	px_free(self);
}


bool
px_module_manager_load(pxModuleManager *self, char *filename)
{
	if (!self)     return false;
	if (!filename) return false;

	/* Load the module */
	pdlmtype module = pdlopen(filename);
	if (!module) goto error;

	/* Make sure this module is unique */
	if (px_array_find(self->dlmodules, module) >= 0) goto error;

	/* Call the px_module_load() function */
	pxModuleLoadFunction load = (pxModuleLoadFunction) pdlsym(module, "px_module_load");
	if (!load || !load(self)) goto error;

	if (!px_array_add(self->dlmodules, module)) goto error;
	return true;

	error:
		if (module) pdlclose(module);
		return false;
}

bool
px_module_manager_load_dir(pxModuleManager *self, char *dirname)
{
	if (!self)    return false;
	if (!dirname) return false;

	/* Open the module dir */
	DIR *moduledir = opendir(dirname);
	if (!moduledir) return false;

	/* For each module... */
	struct dirent *ent;
	bool           loaded = false;
	for (int i=0 ; (ent = readdir(moduledir)) ; i++)
	{
		/* Load the module */
		char *tmp = px_strcat(dirname, "/", ent->d_name, NULL);
		loaded = px_module_manager_load(self, tmp) || loaded;
		px_free(tmp);
	}
	closedir(moduledir);
	return loaded;
}

bool
_px_module_manager_register_module_full(pxModuleManager *self,
                                        const char *id,
                                        const char *name,
                                        pxModuleConstructor new,
                                        pxModuleDestructor free)
{
	if (!self) return false;
	if (!id)   return false;
	if (!name) return false;
	if (!new)  return false;

	pxModuleRegistration *reg = px_malloc0(sizeof(pxModuleRegistration));
	reg->name = px_strdup(name);
	reg->new  = new;
	reg->free = free;

	// Create a new empty array if there is no registrations for this id
	if (!px_strdict_get(self->registrations, id))
		px_strdict_set(self->registrations, id, px_array_new((void *) regeq, (void *) regfree, true, true));

	// Add the module to the registrations for this id
	pxArray *registrations = (pxArray *) px_strdict_get(self->registrations, id);
	return px_array_add(registrations, reg);
}

void **
_px_module_manager_instantiate_type_full(pxModuleManager *self,
                                         const char *id)
{
	if (!self) return NULL;
	if (!id)   return NULL;

	// Find the array of registrations for this id
	pxArray *regs = (pxArray *) px_strdict_get(self->registrations, id);
	if (!regs || px_array_length(regs) < 1)  return NULL;

	// Make sure we have an instance for each of our registrations
	for (int i=0 ; i < px_array_length(regs) ; i++)
	{
		pxModuleRegistration *reg = (pxModuleRegistration *) px_array_get(regs, i);
		if (!reg->instance)
		{
			puts(reg->name);
			reg->instance = reg->new();
		}
	}

	// Sort the instances
	if (px_strdict_get(self->types, id))
		px_array_sort(regs, px_strdict_get(self->types, id));

	// Allocate our instances array
	void **instances = px_malloc0(sizeof(void *) * (px_array_length(regs) + 1));
	for (int i=0 ; i < px_array_length(regs) ; i++)
		instances[i] = ((pxModuleRegistration *) px_array_get(regs, i))->instance;
	return instances;
}

bool
_px_module_manager_register_type_full(pxModuleManager *self,
                                      const char *id,
                                      pxModuleRegistrationComparison cmp)
{
	if (!self) return false;
	if (!id)   return false;

	return px_strdict_set(self->types, id, cmp);
}
