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

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#include "misc.h"
#include "strdict.h"
#include "config_file.h"

struct _pxConfigFile {
	char      *filename;
	time_t     mtime;
	pxStrDict *sections;
};

pxConfigFile *
px_config_file_new(char *filename)
{
	/* Open the file and stat it */
	struct stat st;
	int fd = open(filename, O_RDONLY);
	if (fd < 0) return NULL;
	fstat(fd, &st);

	/* Allocate our structure; get mtime and filename */
	pxConfigFile *self      = px_malloc0(sizeof(pxConfigFile));
	self->filename          = px_strdup(filename);
	self->mtime             = st.st_mtime;
	self->sections          = px_strdict_new((pxStrDictItemCallback) px_strdict_free);

	/* Add one section (PX_CONFIG_FILE_DEFAULT_SECTION) */
	px_strdict_set(self->sections, PX_CONFIG_FILE_DEFAULT_SECTION, px_strdict_new(free));
	pxStrDict *current = (pxStrDict *) px_strdict_get(self->sections, PX_CONFIG_FILE_DEFAULT_SECTION);

	/* Parse our file */
	for (char *line=NULL ; (line = px_readline(fd, NULL, 0)) ; px_free(line))
	{
		/* Strip */
		char *tmp = px_strstrip(line);
		px_free(line); line = tmp;

		/* Check for comment and/or empty line */
		if (*line == '#' || !strcmp(line, "")) continue;

		/* If we have a new section */
		if (*line == '[' || line[strlen(line)-1] == ']')
		{
			/* Get just the section name */
			memmove(line, line+1, strlen(line)-1);
			line[strlen(line)-2] = '\0';

			if (px_strdict_get(self->sections, line))
				current = (pxStrDict *) px_strdict_get(self->sections, line);
			else
				px_strdict_set(self->sections, line, px_strdict_new(free));
		}

		/* If this is a key/val line, get the key/val. */
		else if ((tmp = strchr(line, '=')) && tmp[1])
		{
			*tmp = '\0';
			char *key = px_strstrip(line);
			px_strdict_set(current, key, px_strstrip(tmp+1));
			px_free(key);
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

char *
px_config_file_get_value(pxConfigFile *self, char *section, char *key)
{
	return px_strdup(px_strdict_get((pxStrDict *) px_strdict_get(self->sections, section), key));
}

void
px_config_file_free(pxConfigFile *self)
{
	if (!self) return;

	px_strdict_free(self->sections);
	px_free(self->filename);
	px_free(self);
}


