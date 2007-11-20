/*******************************************************************************
 * libproxy - A library for proxy configuration
 * Copyright (C) 2006 Nathaniel McCallum <nathaniel@natemccallum.com>
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
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

#ifndef CONFIG_FILE_H_
#define CONFIG_FILE_H_

#include <stdbool.h>

#define PX_CONFIG_FILE_DEFAULT_SECTION "__DEFAULT__"

typedef struct _pxConfigFile pxConfigFile;

pxConfigFile *px_config_file_new         (char *filename);
bool          px_config_file_is_stale    (pxConfigFile *self);
char        **px_config_file_get_sections(pxConfigFile *self);
char        **px_config_file_get_keys    (pxConfigFile *self, char *section);
char         *px_config_file_get_value   (pxConfigFile *self, char *section, char *key);
void          px_config_file_free        (pxConfigFile *self);

#endif /*CONFIG_FILE_H_*/
