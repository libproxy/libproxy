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

#ifndef PLUGIN_PACRUNNER_H_
#define PLUGIN_PACRUNNER_H_
#include "plugin.h"
#include "url.h"
#include "pac.h"

typedef struct _pxPACRunnerPlugin {
	PX_PLUGIN_SUBCLASS(pxPlugin);
	char *(*run)(struct _pxPACRunnerPlugin *self, pxPAC *pac, pxURL *url);
} pxPACRunnerPlugin;
#define pxPACRunnerPluginVersion 0

bool px_pac_runner_plugin_constructor(pxPlugin *self);

#endif /* PLUGIN_PACRUNNER_H_ */
