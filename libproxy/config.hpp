/*******************************************************************************
 * libproxy - A library for proxy configuration
 * Copyright (C) 2010 Nathaniel McCallum <nathaniel@natemccallum.com>
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

#ifndef CONFIG_HPP_
#define CONFIG_HPP_

#ifdef _WIN32
#pragma warning(disable: 4251)
#pragma warning(disable: 4290)
#pragma warning(disable: 4275)
#define DLL_PUBLIC __declspec(dllexport)
#define PATHSEP '\\'
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <stdint.h>
typedef unsigned short int sa_family_t;
#else
#define DLL_PUBLIC __attribute__ ((visibility("default")))
#define PATHSEP '/'
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#endif

#endif /* CONFIG_HPP_ */
