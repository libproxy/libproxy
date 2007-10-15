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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

#include "misc.h"

/**
 * Allocates memory and always returns valid memory.
 * @size Amount of memory to allocate in bytes
 * @return Pointer to the allocated memory
 */
void *
px_malloc0(size_t size)
{
	void *mem = malloc(size);
	assert(mem != NULL);
	memset(mem, 0, size);
	return mem;
}

/**
 * Frees memory and doesn't crash if that memory is NULL
 * @mem Memory to free or NULL
 */ 
void
px_free(void *mem)
{
	if (!mem) return;
	free(mem);	
}

/**
 * Duplicates the first n characters of the string s
 * @s String to duplicate
 * @n Number of characters of the string to duplicate
 * @return Newly allocated string
 */
char *
px_strndup(const char *s, size_t n)
{
	if (!s) return NULL;
	char *tmp = px_malloc0(n+1);
	strncpy(tmp, s, n);
	return tmp;	
}

/**
 * Duplicates a string
 * @s String to duplicate
 * @return Newly allocated string
 */
char *
px_strdup(const char *s)
{
	if (!s) return NULL;
	return px_strndup(s, strlen(s));
}

/**
 * Joins NULL terminated array of strings into one string separated by delimiter
 * @strv NULL terminated array of string to join
 * @delimiter The string to use in between each string in the array
 * @return Newly allocated string
 */
char *
px_strjoin(const char **strv, const char *delimiter)
{
	if (!strv) return NULL;
	if (!delimiter) return NULL;
	
	// Count up the length we need
	size_t length = 0;
	for (int i=0 ; strv[i]; i++)
		length += strlen(strv[i]) + strlen(delimiter);
	if (!length) return NULL;
	
	// Do the join
	char *str = px_malloc0(length);
	for (int i=0 ; strv[i]; i++)
		sprintf(str, "%s%s%s", str, strv[i], delimiter);
	str[strlen(str) - strlen(delimiter)] = 0;
	return str;
}

/**
 * Splits a string into a NULL terminated array based on delimiter
 * @string The string to split
 * @delimiter The delimiter to split on
 * @return The NULL terminated array (free with px_strfreev())
 */
char **
px_strsplit(const char *string, const char *delimiter)
{
	// Count how many times the delimiter appears
	int count = 1;
	for (const char *tmp = string ; (tmp = strstr(tmp, delimiter)) ; tmp += strlen(delimiter))
		count++;
		
	// Allocate the vector
	char **strv = px_malloc0(sizeof(char *) * (count + 1));
	
	// Fill the vector
	const char *last = string;
	for (int i=0 ; i < count ; i++)
	{
		char *tmp = strstr(last, delimiter);
		if (!tmp)
			strv[i] = px_strdup(last);
		else
		{
			strv[i] = px_strndup(last, tmp - last);
			last = tmp + strlen(delimiter);
		} 
	}

	return strv;
}

/**
 * Frees the memory used by a NULL terminated string array
 * @strv The NULL terminated string array
 */
void
px_strfreev(char **strv)
{
	if (!strv) return;
	for (int i=0 ; *(strv + i) ; i++)
		px_free(*(strv + i));
	px_free(strv);
}

/**
 * Reads a single line of text from the specified file descriptor
 * @fd File descriptor to read from
 * @return Newly allocated string containing one line only
 */
char *
px_readline(int fd)
{
	// Verify we have an open socket
	if (fd < 0) return NULL;
	
	// For each character received add it to the buffer unless it is a newline
	char *buffer = NULL;
	for (int i=1; i > 0 ; i++)
	{
		char c;
		
		// Receive a single character, check for newline or EOF
		if (read(fd, &c, 1) != 1 || c == '\n') break;

		// Allocate new buffer if we need
		if (i % 1024 == 1)
		{
			char *tmp = buffer;
			buffer = px_malloc0(1024+i);
			if (tmp) { strcpy(buffer, tmp); px_free(tmp); }
		}

		// Add new character
		buffer[i-1] = c;
	}
	return buffer;
}

/**
 * Trims off all the leading whitespace characters
 * @string The string to strip
 * @return A newly allocated copy of string without all the leading whitespace
 */
char *
px_strlstrip(char *string)
{
	for (int i=0 ; string[i] ; i++)
		if (!isspace(string[i]))
			return px_strdup(string + i);
	return px_strdup("");
}

/**
 * Trims off all the trailing whitespace characters
 * @string The string to strip
 * @return A newly allocated copy of string without all the trailing whitespace
 */
char *
px_strrstrip(char *string)
{
	char *tmp = string = px_strdup(string);
	
	for (int i=0 ; string[i] ; i++)
		if (!isspace(string[i]))
			tmp = string + i;
	*tmp = '\0';
	return string;
}

/**
 * Trims off all the leading and trailing whitespace characters
 * @string The string to strip
 * @return A newly allocated copy of string without all the leading and trailing whitespace
 */
char *
px_strstrip(char *string)
{
	char *tmp = px_strrstrip(string);
	string    = px_strlstrip(tmp);
	px_free(tmp);
	return string;
}









