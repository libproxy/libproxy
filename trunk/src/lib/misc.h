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

#ifndef MISC_H_
#define MISC_H_

#include <stdlib.h> // For type size_t

/**
 * Allocates memory and always returns valid memory.
 * @size Amount of memory to allocate in bytes
 * @return Pointer to the allocated memory
 */
void *px_malloc0(size_t size);

/**
 * Frees memory and doesn't crash if that memory is NULL
 * @mem Memory to free or NULL
 */ 
void px_free(void *mem);

/**
 * Duplicates the first n characters of the string s
 * @s String to duplicate
 * @n Number of characters of the string to duplicate
 * @return Newly allocated string
 */
char *px_strndup(const char *s, size_t n);

/**
 * Duplicates a string
 * @s String to duplicate
 * @return Newly allocated string
 */
char *px_strdup(const char *s);

/**
 * Joins NULL terminated array of strings into one string separated by delimiter
 * @strv NULL terminated array of string to join
 * @delimiter The string to use in between each string in the array
 * @return Newly allocated string
 */
char *px_strjoin(const char **strv, const char *delimiter);

/**
 * Splits a string into a NULL terminated array based on delimiter
 * @string The string to split
 * @delimiter The delimiter to split on
 * @return The NULL terminated array (free with px_strfreev())
 */
char **px_strsplit(const char *string, const char *delimiter);

/**
 * Frees the memory used by a NULL terminated string array
 * @strv The NULL terminated string array
 */
void px_strfreev(char **strv);

/**
 * Reads a single line of text from the specified file descriptor
 * @fd File descriptor to read from
 * @return Newly allocated string containing one line only
 */
char *px_readline(int fd);

/**
 * Trims off all the leading whitespace characters
 * @string The string to strip
 * @return A newly allocated copy of string without all the leading whitespace
 */
char *px_lstrip(char *string);

#endif /*MISC_H_*/
