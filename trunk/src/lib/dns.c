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

#include <string.h>
#include <errno.h>
#include <stdio.h>
#define __USE_BSD
#include <unistd.h>

#include "misc.h"
#include "pac.h"
#include "dns.h"

struct _pxDNS {
	pxURL **urls;
	int     next;
	char   *domain;
};

// The top-level domain blacklist
static char *tld[] = {
	// General top-level domains
	"arpa", "root", "aero", "biz", "cat", "com", "coop", "edu", "gov", "info",
	"int", "jobs", "mil", "mobi", "museum", "name", "net", "org", "pro", "travel",
	
	// Country codes
	"ac", "ad", "ae", "af", "ag", "ai", "al", "am", "an", "ao", "aq", "ar", 
	"as", "at", "au", "aw", "ax", "az", "ba", "bb", "bd", "be", "bf", "bg", 
	"bh", "bi", "bj", "bm", "bn", "bo", "br", "bs", "bt", "bv", "bw", "by", 
	"bz", "ca", "cc", "cd", "cf", "cg", "ch", "ci", "ck", "cl", "cm", "cn", 
	"co", "cr", "cu", "cv", "cx", "cy", "cz", "de", "dj", "dk", "dm", "do", 
	"dz", "ec", "ee", "eg", "er", "es", "et", "eu", "fi", "fj", "fk", "fm", 
	"fo", "fr", "ga", "gb", "gd", "ge", "gf", "gg", "gh", "gi", "gl", "gm", 
	"gn", "gp", "gq", "gr", "gs", "gt", "gu", "gw", "gy", "hk", "hm", "hn", 
	"hr", "ht", "hu", "id", "ie", "il", "im", "in", "io", "iq", "ir", "is", 
	"it", "je", "jm", "jo", "jp", "ke", "kg", "kh", "ki", "km", "kn", "kr", 
	"kw", "ky", "kz", "la", "lb", "lc", "li", "lk", "lr", "ls", "lt", "lu", 
	"lv", "ly", "ma", "mc", "md", "mg", "mh", "mk", "ml", "mm", "mn", "mo", 
	"mp", "mq", "mr", "ms", "mt", "mu", "mv", "mw", "mx", "my", "mz", "na", 
	"nc", "ne", "nf", "ng", "ni", "nl", "no", "np", "nr", "nu", "nz", "om", 
	"pa", "pe", "pf", "pg", "ph", "pk", "pl", "pm", "pn", "pr", "ps", "pt", 
	"pw", "py", "qa", "re", "ro", "ru", "rw", "sa", "sb", "sc", "sd", "se", 
	"sg", "sh", "si", "sj", "sk", "sl", "sm", "sn", "sr", "st", "su", "sv", 
	"sy", "sz", "tc", "td", "tf", "tg", "th", "tj", "tk", "tl", "tm", "tn", 
	"to", "tp", "tr", "tt", "tv", "tw", "tz", "ua", "ug", "uk", "um", "us", 
	"uy", "uz", "va", "vc", "ve", "vg", "vi", "vn", "vu", "wf", "ws", "ye", 
	"yt", "yu", "za", "zm", "zw",
	
	// Other domains to blacklist
	"co.uk", "com.au", 
	
	// Terminator
	NULL
};

static char *
get_domain_name()
{
	// Get the hostname
	char *tmp = px_malloc0(128);
	for (int i = 0 ; gethostname(tmp, (i + 1) * 128) && errno == ENAMETOOLONG ; )
		tmp = px_malloc0((++i + 1) * 128);
		
	// Lookup the hostname
	struct hostent *host_info = gethostbyname(tmp);
	if (host_info)
	{
		px_free(tmp);
		tmp = px_strdup(host_info->h_name);
	}
	
	// Get domain portion
	if (!strchr(tmp, '.')) return NULL;
	if (!strcmp(".", strchr(tmp, '.'))) return NULL;
	return px_strdup(strchr(tmp, '.') + 1);
}

static pxURL **
get_urls(const char *domain)
{
	// Split up the domain
	char **domainv = px_strsplit(domain, ".");
	if (!domainv) return NULL;
	
	// Count the number of domain blocks
	int count = 0;
	for (int i=0 ; *(domainv + i) ; i++)
		count++;
	
	// Allocate our URL array
	pxURL **urls = px_malloc0(sizeof(pxURL *) * count);
	
	// Create the URLs
	char *url  = px_malloc0(strlen("http://wpad./wpad.dat") + strlen(domain) + 1);
	for (int i=0, j=0 ; domainv[i] ; i++) {
		// Check the domain against the blacklist
		char *tmp = px_strjoin((const char **) (domainv + i), ".");
		for (int k=0; tld[k] ; k++)
			if (!strcmp(tmp, tld[k])) { px_free(tmp); tmp = NULL; break; }
		if (!tmp) continue;
		
		// Create the URL
		sprintf(url, "http://wpad.%s/wpad.dat", tmp);
		px_free(tmp); tmp = NULL;
		urls[j] = px_url_new(url);
		if (urls[j]) j++;
	}
	px_free(url);
	
	return urls;
}

/**
 * Creates a new pxDNS PAC detector.
 * @return New pxDNS PAD detector
 */
pxDNS *
px_dns_new()
{
	return px_dns_new_full(NULL);
}

/**
 * Creates a new pxDNS PAC detector with more options.
 * @domain The domain name to use when detecting or NULL to use the default
 * @return New pxDNS PAD detector
 */
pxDNS *
px_dns_new_full(const char *domain)
{
	pxDNS *self = px_malloc0(sizeof(pxDNS));
	self->domain = px_strdup(domain);
	return self;
}

/**
 * Detect the next PAC in the chain.
 * @return Detected PAC or NULL if none is found
 */
pxPAC *
px_dns_next(pxDNS *self)
{
	if (!self->urls) {
		char *domain;
		
		// Reset the counter
		self->next = 0;
		
		// Get the domain name
		if (self->domain)
			domain = px_strdup(self->domain);
		else 
			domain = get_domain_name();
		if (!domain) return NULL;

		// Get the URLs
		self->urls = get_urls(domain);
		px_free(domain);
		
		// Make sure we have more than one URL
		if (!self->urls || !self->urls[0])
			return NULL;
	}
	
	// Try to find a PAC at each URL
	for (pxPAC *pac = NULL ; self->urls[self->next] ; )
		if ((pac = px_pac_new(self->urls[self->next++])))
			return pac;
	
	return NULL;
}

/**
 * Restarts the detection chain at the beginning.
 */
void
px_dns_rewind(pxDNS *self)
{
	if (!self) return;
	
	if (self->urls) {
		for (int i = 0 ; self->urls[i] ; i++)
			px_url_free(self->urls[i]);
		px_free(self->urls); 
		self->urls = NULL;
	}
	self->next = 0;
	return;
}

/**
 * Frees a pxDNS object.
 */
void
px_dns_free(pxDNS *self)
{
	if (!self) return;
	
	if (self->urls) {
		for (int i = 0 ; self->urls[i] ; i++)
			px_url_free(self->urls[i]);
		px_free(self->urls); self->urls = NULL;
	}
	px_free(self->domain);
	px_free(self);
}
