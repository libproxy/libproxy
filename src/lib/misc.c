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
#include <stdarg.h>

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
 * Duplicates a string vector
 * @sv String vector to duplicate
 * @return Newly allocated string vector (free w/ px_strfreev())
 */
char **
px_strdupv(const char **sv)
{
	int count;
	
	if (!sv) return NULL;
	for (count=0 ; sv[count] ; count++); 
	
	char **output = px_malloc0(sizeof(char *) * ++count);
	for (int i=0 ; sv[i] ; i++)
		output[i] = px_strdup(sv[i]);
		
	return output;
}

/**
 * Concatenates two or more strings into a newly allocated string
 * @s The first string to concatenate.
 * @... Subsequent strings.  The last argument must be NULL.
 * @return Newly allocated string
 */
char *
px_strcat(const char *s, ...)
{
	va_list args;
	
	/* Count the number of characters to concatentate */
	va_start(args, s);
	int count = strlen(s);
	for (char *tmp = NULL ; (tmp = va_arg(args, char *)) ; count += strlen(tmp));
	va_end(args);
	
	/* Build our output string */
	char *output = px_malloc0(count + 1);
	strcat(output, s);
	va_start(args, s);
	for (char *tmp = NULL ; (tmp = va_arg(args, char *)) ; )
		strcat(output, tmp);
	va_end(args);
	
	return output;
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
	
	/* Count up the length we need */
	size_t length = 0;
	for (int i=0 ; strv[i]; i++)
		length += strlen(strv[i]) + strlen(delimiter);
	if (!length) return NULL;
	
	/* Do the join */
	char *str = px_malloc0(length);
	for (int i=0 ; strv[i]; i++)
	{
		strcat(str, strv[i]);
		if (strv[i+1]) strcat(str, delimiter);
	}
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
	/* Count how many times the delimiter appears */
	int count = 1;
	for (const char *tmp = string ; (tmp = strstr(tmp, delimiter)) ; tmp += strlen(delimiter))
		count++;
		
	/* Allocate the vector */
	char **strv = px_malloc0(sizeof(char *) * (count + 1));
	
	/* Fill the vector */
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
 * @buffer The buffer to write to (usually NULL)
 * @bufsize The size of the buffer (usually 0)
 * @return Newly allocated string containing one line only
 */
char *
px_readline(int fd, char *buffer, size_t bufsize)
{
	char c = '\0';

	/* Verify we have an open socket */
	if (fd < 0) return NULL;

	/* Read a character.  If we don't get a character, return the buffer. */
	if (read(fd, &c, 1) != 1) return buffer;

	/* If we are at the end of the line, return. */
	if (c == '\n') return buffer ? buffer : px_strdup("");

	/* We have a character, make sure we have a buffer. */
	if (!buffer)
	{
		buffer = px_strdup("");
		bufsize = 0;
	}

	/* If our buffer is full, add more to the buffer. */
	if (bufsize <= strlen(buffer))
	{
		char *tmp = px_malloc0(1024 + strlen(buffer) + 1);
		strcpy(tmp, buffer);
		free(buffer);
		buffer = tmp;
		bufsize = strlen(buffer) + 1024;
	}

	strncat(buffer, &c, 1);
	return px_readline(fd, buffer, bufsize);
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
	tmp[1] = '\0';
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
