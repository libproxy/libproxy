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

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#include "misc.h"
#include "config_file.h"

struct _pxConfigFileSection {
	char *name;
	char **keys;
	char **vals;
};
typedef struct _pxConfigFileSection pxConfigFileSection;

struct _pxConfigFile {
	char                 *filename;
	time_t                mtime;
	pxConfigFileSection **sections;
};

pxConfigFile *
px_config_file_new(char *filename)
{
	// Open the file and stat it
	struct stat st;
	int fd = open(filename, O_RDONLY);
	if (fd < 0) return NULL;
	fstat(fd, &st);

	// Allocate our structure; get mtime and filename
	pxConfigFile *self      = px_malloc0(sizeof(pxConfigFile));
	self->mtime             = st.st_mtime;
	self->filename          = px_strdup(filename);
	
	// Add one section (PX_CONFIG_FILE_DEFAULT_SECTION)
	self->sections               = px_malloc0(sizeof(pxConfigFileSection *) * 2);
	self->sections[0]            = px_malloc0(sizeof(pxConfigFileSection));
	self->sections[0]->name      = px_strdup(PX_CONFIG_FILE_DEFAULT_SECTION);
	pxConfigFileSection *current = self->sections[0];
	
	// Parse our file
	for (char *line=NULL ; (line = px_readline(fd)) ; px_free(line))
	{
		// Strip
		char *tmp = px_strstrip(line);
		px_free(line); line = tmp;
		
		// Check for comment and/or empty line
		if (*line == '#' || !strcmp(line, "")) continue;
		
		// If we have a new section
		if (*line == '[' || line[strlen(line)-1] == ']')
		{
			// Get just the section name
			memmove(line, line+1, strlen(line)-1);
			line[strlen(line)-2] = '\0';
			
			// Check for each section...
			for (int i=0 ; self->sections[i] ; i++)
			{
				// If we found the section already, set it as current and move on
				if (!strcmp(self->sections[i]->name, line))
				{
					current = self->sections[i];
					break;
				}
				
				// If the section wasn't found, add a new section
				if (!self->sections[i+1])
				{
					// Create new section
					current       = px_malloc0(sizeof(pxConfigFileSection));
					current->name = px_strdup(line);
					
					// Add section to the end
					pxConfigFileSection **sections = self->sections;
					self->sections = px_malloc0(sizeof(pxConfigFileSection *) * (i+3));
					memcpy(self->sections, sections, sizeof(pxConfigFileSection) * (i+1));
					self->sections[i+1] = current;
					px_free(sections);
					
					break;
				}
			}
			continue;
		}

		// If this is a key/val line, get the key/val.
		if ((tmp = strchr(line, '=')) && tmp[1])
		{
			// If this is our first key/val, create a new array
			if (!current->keys || !current->vals)
			{
				// Add key
				current->keys    = px_malloc0(sizeof(char *) * 2);
				current->keys[0] = px_strndup(line, tmp - line);
				current->keys[1] = NULL;
				
				// Add val
				current->vals    = px_malloc0(sizeof(char *) * 2);
				current->vals[0] = px_strdup(tmp+1);
				current->vals[1] = NULL;
			}
			
			// If this is not our first key/val, tack it on the end
			else
			{
				for (int i=0 ; current->keys[i] ; i++)
				{
					if (!current->keys[i+1])
					{
						// Add val
						char **vals = px_malloc0(sizeof(char *) * (i+3));
						memcpy(vals, current->vals, sizeof(char *) * (i+1));
						vals[i+1] = px_strstrip(tmp+1);
						px_free(current->vals); current->vals = vals;
						
						// Add key
						*tmp = '\0';
						char **keys = px_malloc0(sizeof(char *) * (i+3));
						memcpy(keys, current->keys, sizeof(char *) * (i+1));
						keys[i+1] = px_strstrip(line);
						px_free(current->keys); current->keys = keys;
						
						break;
					}
				}
			}
			continue;
		}
	}
	
	close(fd);
	return self;
}

bool
px_config_file_is_stale(pxConfigFile *self)
{
	struct stat st;
	return (!stat(self->filename, &st) && st.st_mtime > self->mtime);
}

char **
px_config_file_get_sections(pxConfigFile *self)
{
	int count;
	for (count=0 ; self->sections[count] ; count++);
	char **output = px_malloc0(sizeof(char *) * ++count);
	for (count=0 ; self->sections[count] ; count++)
		output[count] = px_strdup(self->sections[count]->name);
	return output;
}

char **
px_config_file_get_keys(pxConfigFile *self, char *section)
{
	for (int i=0 ; self->sections[i] ; i++)
	{
		if (!strcmp(self->sections[i]->name, section))
			return px_strdupv((const char **) self->sections[i]->keys);
	}
	
	return NULL;
}

char *
px_config_file_get_value(pxConfigFile *self, char *section, char *key)
{
	for (int i=0 ; self->sections[i] ; i++)
		if (!strcmp(self->sections[i]->name, section))
			for (int j=0 ; self->sections[i]->keys && self->sections[i]->keys[j] ; j++)
				if (!strcmp(self->sections[i]->keys[j], key))
					return px_strdup(self->sections[i]->vals[j]);
	
	return NULL;
}

void
px_config_file_free(pxConfigFile *self)
{
	for (int i=0 ; self->sections[i] ; i++)
	{
		px_free(self->sections[i]->name);
		px_strfreev(self->sections[i]->keys);
		px_strfreev(self->sections[i]->vals);
		px_free(self->sections[i]);
	}
	px_free(self);
}


