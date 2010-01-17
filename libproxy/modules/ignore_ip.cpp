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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#ifdef _WIN32
#define _WIN32_WINNT 0x0501
#include <winsock2.h>
#include <ws2tcpip.h>
typedef unsigned short int sa_family_t;
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#endif

#include "../misc.hpp"
#include "../modules.hpp"

static bool
_sockaddr_equals(const struct sockaddr *ip_a, const struct sockaddr *ip_b, const struct sockaddr *nm)
{
        if (!ip_a || !ip_b) return false;
        if (ip_a->sa_family != ip_b->sa_family) return false;
        if (nm && ip_a->sa_family != nm->sa_family) return false;

        /* Setup the arrays */
        uint8_t bytes = 0, *a_data = NULL, *b_data = NULL, *nm_data = NULL;
        if (ip_a->sa_family == AF_INET)
        {
                bytes   = 32 / 8;
                a_data  = (uint8_t *) &((struct sockaddr_in *) ip_a)->sin_addr;
                b_data  = (uint8_t *) &((struct sockaddr_in *) ip_b)->sin_addr;
                nm_data = nm ? (uint8_t *) &((struct sockaddr_in *) nm)->sin_addr : NULL;
        }
        else if (ip_a->sa_family == AF_INET6)
        {
                bytes   = 128 / 8;
                a_data  = (uint8_t *) &((struct sockaddr_in6 *) ip_a)->sin6_addr;
                b_data  = (uint8_t *) &((struct sockaddr_in6 *) ip_b)->sin6_addr;
                nm_data = nm ? (uint8_t *) &((struct sockaddr_in6 *) nm)->sin6_addr : NULL;
        }
        else
                return false;

        for (int i=0 ; i < bytes ; i++)
        {
                if (nm && (a_data[i] & nm_data[i]) != (b_data[i] & nm_data[i]))
                        return false;
                else if (!nm && (a_data[i] != b_data[i]))
                        return false;
        }
        return true;
}

static struct sockaddr *
_sockaddr_from_string(const char *ip, int len)
{
        if (!ip) return NULL;
        struct sockaddr *result = NULL;

        /* Copy the string */
        if (len >= 0)
                ip = px_strndup(ip, len);
        else
                ip = px_strdup(ip);

        /* Try to parse */
        struct addrinfo *info = NULL;
        struct addrinfo flags;
        flags.ai_family = AF_UNSPEC;
        flags.ai_socktype = 0;
        flags.ai_protocol = 0;
        flags.ai_flags = AI_NUMERICHOST;
        if (getaddrinfo(ip, NULL, &flags, &info) != 0 || !info) goto out;

        /* Copy the results into our buffer */
        result = (struct sockaddr *) px_malloc0(info->ai_addrlen);
        memcpy(result, info->ai_addr, info->ai_addrlen);

        out:
                px_free((char *) ip);
                return result;
}

static struct sockaddr *
_sockaddr_from_cidr(sa_family_t af, uint8_t cidr)
{
        /* IPv4 */
        if (af == AF_INET)
        {
                struct sockaddr_in *mask = (sockaddr_in *) px_malloc0(sizeof(struct sockaddr_in));
                mask->sin_family = af;
                mask->sin_addr.s_addr = htonl(~0 << (32 - (cidr > 32 ? 32 : cidr)));

                return (struct sockaddr *) mask;
        }

        /* IPv6 */
        else if (af == AF_INET6)
        {
                struct sockaddr_in6 *mask = (sockaddr_in6 *) px_malloc0(sizeof(struct sockaddr_in6));
                mask->sin6_family = af;
                for (uint8_t i=0 ; i < sizeof(mask->sin6_addr) ; i++)
                        mask->sin6_addr.s6_addr[i] = ~0 << (8 - (8*i > cidr ? 0 : cidr-8*i < 8 ? cidr-8*i : 8) );

                return (struct sockaddr *) mask;
        }

        return NULL;
}

static bool
_ignore(struct _pxIgnoreModule *self, pxURL *url, const char *ignore)
{
    if (!url || !ignore) return false;


    bool result   = false;
    uint32_t port = 0;
    const struct sockaddr **dst_ips = px_url_get_ips(url, false);
    const struct sockaddr *dst_ip   = dst_ips && dst_ips[0] ? dst_ips[0] : NULL;
          struct sockaddr *ign_ip   = NULL, *net_ip = NULL;

    /*
     * IPv4
     * IPv6
     */
    if ((ign_ip = _sockaddr_from_string(ignore, -1)))
            goto out;

    /*
     * IPv4/CIDR
     * IPv4/IPv4
     * IPv6/CIDR
     * IPv6/IPv6
     */
    if (strchr(ignore, '/'))
    {
            ign_ip = _sockaddr_from_string(ignore, strchr(ignore, '/') - ignore);
            net_ip = _sockaddr_from_string(strchr(ignore, '/') + 1, -1);

            /* If CIDR notation was used, get the netmask */
            if (ign_ip && !net_ip)
            {
                    uint32_t cidr = 0;
                    if (sscanf(strchr(ignore, '/') + 1, "%d", &cidr) == 1)
                            net_ip = _sockaddr_from_cidr(ign_ip->sa_family, cidr);
            }

            if (ign_ip && net_ip && ign_ip->sa_family == net_ip->sa_family)
                    goto out;

            px_free(ign_ip);
            px_free(net_ip);
            ign_ip = NULL;
            net_ip = NULL;
    }

    /*
     * IPv4:port
     * [IPv6]:port
     */
    if (strrchr(ignore, ':') && sscanf(strrchr(ignore, ':'), ":%u", &port) == 1 && port > 0)
    {
            ign_ip = _sockaddr_from_string(ignore, strrchr(ignore, ':') - ignore);

            /* Make sure this really is just a port and not just an IPv6 address */
            if (ign_ip && (ign_ip->sa_family != AF_INET6 || ignore[0] == '['))
                    goto out;

            px_free(ign_ip);
            ign_ip = NULL;
            port   = 0;
    }

    out:
            result = _sockaddr_equals(dst_ip, ign_ip, net_ip);
            px_free(ign_ip);
            px_free(net_ip);
            return port != 0 ? (port == px_url_get_port(url) && result): result;
}

static void *
_constructor()
{
	pxIgnoreModule *self = (pxIgnoreModule*) px_malloc0(sizeof(pxIgnoreModule));
	self->ignore = _ignore;
	return self;
}

bool
px_module_load(pxModuleManager *self)
{
	return px_module_manager_register_module(self, pxIgnoreModule, _constructor, px_free);
}
