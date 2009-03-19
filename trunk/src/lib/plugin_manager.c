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
#include <dlfcn.h>
#include <string.h>
#include <dirent.h>
#include <math.h>

#include "plugin_manager.h"
#include "misc.h"
#include "array.h"
#include "strdict.h"

struct _pxInfo {
	const char *name;
	size_t      size;
	bool      (*constructor)(pxPlugin *);
	pxPlugin   *prototype;
};

struct _pxPluginManager {
	pxArray   *modules;
	pxStrDict *ptypes;
	pxStrDict *constructors;
};

static pxPlugin *
copyprototype(const pxPlugin *prototype, size_t allocsize, size_t copysize)
{
	if (!prototype) return NULL;
	if (copysize > allocsize) return NULL;
	pxPlugin *proto = px_malloc0(allocsize);
	if (prototype)
		memcpy(proto, prototype, copysize);
	return proto;
}

static struct _pxInfo *
makeinfo(const char *name, size_t size, bool (*constructor)(pxPlugin *), const pxPlugin *prototype)
{
	struct _pxInfo *info = px_malloc0(sizeof(struct _pxInfo));
	info->name           = name;
	info->size           = size;
	info->constructor    = constructor;
	info->prototype      = copyprototype(prototype, size, size);
	return info;
}

static void
freeinfo(struct _pxInfo *info)
{
	if (info && info->prototype)
		info->prototype->destructor(info->prototype);
	px_free(info);
}

static void
freeplugin(pxPlugin *self)
{
	self->destructor(self);
}

static char *
makekey(const char *name, unsigned int version)
{
	/* Combine name and version into a single field */
	if (version == 0)
		return px_strdup(name);

	char *key = px_malloc0(strlen(name) + (int) log10(version) + 4);
	sprintf(key, "%s__%u", name, version);
	return key;
}

static int
plugincmp(const void *a, const void *b)
{
	pxPlugin *pa = *((pxPlugin **) a);
	pxPlugin *pb = *((pxPlugin **) b);

	if (!pa->compare) return 0;
	return pa->compare(pa, pb);
}

pxPluginManager *
px_plugin_manager_new()
{
	pxPluginManager *self = px_malloc0(sizeof(pxPluginManager));
	self->modules         = px_array_new(NULL, (void *) dlclose, true, false);
	self->ptypes          = px_strdict_new((void *) freeinfo);
	self->constructors    = px_strdict_new((void *) px_array_free);
	return self;
}

void
px_plugin_manager_free(pxPluginManager *self)
{
	for (int i=0 ; i < px_array_length(self->modules) ; i++)
	{
		pxModuleUnload unload = dlsym((void *) px_array_get(self->modules, i), "px_module_unload");
		if (unload) unload(self);
	}
	px_strdict_free(self->constructors);
	px_strdict_free(self->ptypes);
	px_array_free(self->modules);
	px_free(self);
}


bool
px_plugin_manager_load(pxPluginManager *self, char *filename)
{
	if (!self)     return false;
	if (!filename) return false;

	/* Load the module */
	void *module = dlopen(filename, RTLD_NOW | RTLD_LOCAL);
	if (!module) return false;

	/* Make sure this plugin is unique */
	if (px_array_find(self->modules, module) >= 0) goto error;

	/* Call the px_module_load() function */
	pxModuleLoad load = dlsym(module, "px_module_load");
	if (!load || !load(self)) goto error;

	px_array_add(self->modules, module);
	return true;

	error:
		dlclose(module);
		return false;
}

bool
px_plugin_manager_load_dir(pxPluginManager *self, char *dirname)
{
	if (!self)    return false;
	if (!dirname) return false;

	/* Open the plugin dir */
	DIR *plugindir = opendir(dirname);
	if (!plugindir) return false;

	/* For each plugin... */
	struct dirent *ent;
	bool           loaded = false;
	for (int i=0 ; (ent = readdir(plugindir)) ; i++)
	{
		/* Load the plugin */
		char *tmp = px_strcat(dirname, "/", ent->d_name, NULL);
		loaded = px_plugin_manager_load(self, tmp) || loaded;
		px_free(tmp);
	}
	closedir(plugindir);
	return loaded;
}

bool
px_plugin_manager_constructor_add_full(pxPluginManager *self,
                                       const char *name,
                                       const char *type,
                                       unsigned int version,
                                       size_t size,
                                       bool (*constructor)(pxPlugin *))
{
	if (!self)        return false;
	if (!type)        return false;
	if (!constructor) return false;
	if (!name)        return false;

    /* Check for plugins to blacklist */
	char **blacklist = px_strsplit(getenv("PX_PLUGIN_BLACKLIST"), ",");
	for (int i=0 ; blacklist && blacklist[i] ; i++)
	{
		if (!strcmp(name, blacklist[i]))
		{
			px_strfreev(blacklist);
			return false;
		}
	}
	px_strfreev(blacklist);

	/* Get an array of all of the constructors for a particular name/version
	 * as well as information about the type.
	 */
	char    *key     = makekey(type, version);
	pxArray *constrs = (pxArray *) px_strdict_get(self->constructors, key);

	/* If no constructors exist, create an empty array */
	if (!constrs)
	{
		constrs = px_array_new(NULL, (void *) freeinfo, true, true);
		px_strdict_set(self->constructors, key, constrs);
	}
	px_free(key);

	/* Add the constructor */
	struct _pxInfo *info   = makeinfo(name, size, constructor, NULL);
	bool            result = px_array_add(constrs, info);
	if (!result)
		freeinfo(info);
	return result;
}

pxArray *
px_plugin_manager_instantiate_plugins_full(pxPluginManager *self,
                                           const char *type,
                                           unsigned int version)
{
	pxArray *plugins = px_array_new(NULL, (void *) freeplugin, true, false);

	if (!self) return plugins;
	if (!type) return plugins;

	/* Lookup all constructors and a possible prototype for this name/version */
	char           *key     = makekey(type, version);
	pxArray        *constrs = (pxArray  *)       px_strdict_get(self->constructors, key);
	struct _pxInfo *ptype   = (struct _pxInfo *) px_strdict_get(self->ptypes, key);
	px_free(key);
	if (!constrs) return plugins;

	/* Build an array of all the instantiated plugins */
	for (int i=0 ; i < px_array_length(constrs) ; i++)
	{
		/* Get the info for this constructor */
		const struct _pxInfo *info = px_array_get(constrs, i);

		/* Allocate the memory for the prototype */
		pxPlugin *plugin = copyprototype(ptype ? ptype->prototype : NULL, info->size, ptype ? ptype->size : info->size);
		if (!plugin) continue;

		/* Add the plugin to the plugin array */
		px_array_add(plugins, plugin);

		/* Set some defaults */
		plugin->name        = info->name;
		plugin->type        = px_strdup(type);
		plugin->version     = version;
		plugin->constructor = info->constructor;

		/* Call the prototype constructor */
		if (!ptype || !ptype->prototype || !ptype->prototype->constructor || ptype->prototype->constructor(plugin))
			/* Call the instance constructor */
			if (plugin->constructor(plugin))
				continue;

		/* If we arrived here the constructor failed, so remove the plugin */
		px_array_del(plugins, plugin);
	}

	/* Sort them */
	px_array_sort(plugins, plugincmp);
	return plugins;
}

bool
px_plugin_manager_register_type_full(pxPluginManager *self,
                                     const char *type,
                                     unsigned int version,
                                     size_t size,
                                     const pxPlugin *prototype)
{
	if (!self)                   return false;
	if (!type)                   return false;
	if (!prototype)              return false;
	if (size < sizeof(pxPlugin)) return false;

	char           *key    = makekey(type, version);
	struct _pxInfo *info   = makeinfo(NULL, size, NULL, prototype);
	bool            result = px_strdict_set(self->ptypes, key, info);
	px_free(key);
	if (!info->prototype->destructor)
		info->prototype->destructor = &px_plugin_destructor;
	if (!result)
		freeinfo(info);
	return result;
}
