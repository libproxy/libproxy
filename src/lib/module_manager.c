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
#include <libgen.h>
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

typedef struct _pxModuleTypeRegistration {
	pxModuleRegistrationComparison cmp;
	bool                           sngl;
} pxModuleTypeRegistration;

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

static char *
basename_noext(const char *filename)
{
	char *tmp = px_strdup(filename);
	filename = px_strdup(basename(tmp));
	px_free(tmp);
	if (strrchr(filename, '.'))
		strrchr(filename, '.')[0] = '\0';
	return (char *) filename;
}

static bool
globmatch(const char *glob, const char *string)
{
	if (!glob)   return false;
	if (!string) return false;

	char **segments = px_strsplit(glob, "*");
	for (int i=0 ; segments[i] ; i++)
	{
		// Search for this segment in this string
		char *offset = strstr(string, segments[i]);

		// If the segment isn't found at all, its not a match
		if (!offset)
			goto nomatch;

		// Handle when the glob does not start with '*'
		// (insist the segment match the start of the string)
		if (i == 0 && strcmp(segments[i], "") && offset != string)
			goto nomatch;

		// Increment further into the string
		string = offset + strlen(segments[i]);

		// Handle when the glob does not end with '*'
		// (insist the segment match the end of the string)
		if (!segments[i+1] && strcmp(segments[i], "") && string[0])
			goto nomatch;
	}
	px_strfreev(segments);
	return true;

	nomatch:
		px_strfreev(segments);
		return false;
}

pxModuleManager *
px_module_manager_new()
{
	pxModuleManager *self = px_malloc0(sizeof(pxModuleManager));
	self->dlmodules       = px_array_new(NULL, (pxArrayItemCallback) pdlclose, true, false);
	self->registrations   = px_strdict_new((pxStrDictItemCallback) px_array_free);
	self->types           = px_strdict_new((pxStrDictItemCallback) px_free);
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

	// Prepare for blacklist check
	char **blacklist = px_strsplit(getenv("PX_MODULE_BLACKLIST"), ",");
	char **whitelist = px_strsplit(getenv("PX_MODULE_WHITELIST"), ",");
	char  *modname   = basename_noext(filename);
	bool   doload    = true;

	// Check our whitelist/blacklist to see if we should load this module
	for (int i=0 ; blacklist && blacklist[i]; i++)
		if (globmatch(blacklist[i], modname))
			doload = false;
	for (int i=0 ; whitelist && whitelist[i]; i++)
		if (globmatch(whitelist[i], modname))
			doload = true;

	// Cleanup
	px_strfreev(blacklist);
	px_strfreev(whitelist);
	px_free(modname);

	if (!doload)
		return false;

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
                                        size_t namelen,
                                        pxModuleConstructor new,
                                        pxModuleDestructor free)
{
	if (!self) return false;
	if (!id)   return false;
	if (!name) return false;
	if (!new)  return false;

	// Ensure only a single registration in the case of a singleton
	pxModuleTypeRegistration *tr = (pxModuleTypeRegistration *) px_strdict_get(self->types, id);
	if (tr && tr->sngl && px_array_length((pxArray *) px_strdict_get(self->registrations, id)) > 0)
		return false;

	pxModuleRegistration *reg = px_malloc0(sizeof(pxModuleRegistration));
	reg->name = px_strndup(name, namelen);
	reg->pxnew  = new;
	reg->free = free;

	// Create a new empty array if there is no registrations for this id
	if (!px_strdict_get(self->registrations, id))
		px_strdict_set(self->registrations, id, px_array_new((pxArrayItemsEqual) regeq, (pxArrayItemCallback) regfree, true, true));

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
			reg->instance = reg->pxnew();
	}

	// Sort the instances
	if (px_strdict_get(self->types, id))
		px_array_sort(regs, ((pxModuleTypeRegistration *) px_strdict_get(self->types, id))->cmp);

	// Allocate our instances array
	void **instances = px_malloc0(sizeof(void *) * (px_array_length(regs) + 1));
	for (int i=0 ; i < px_array_length(regs) ; i++)
		instances[i] = ((pxModuleRegistration *) px_array_get(regs, i))->instance;
	return instances;
}

bool
_px_module_manager_register_type_full(pxModuleManager *self,
                                      const char *id,
                                      pxModuleRegistrationComparison cmp,
                                      bool singleton)
{
	if (!self) return false;
	if (!id)   return false;
	if (!cmp && !singleton) return true;

	pxModuleTypeRegistration *tr = px_malloc0(sizeof(pxModuleTypeRegistration));
	tr->cmp  = cmp;
	tr->sngl = singleton;
	return px_strdict_set(self->types, id, tr);
}
