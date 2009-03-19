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
#include <netdb.h>
#define __USE_BSD
#include <unistd.h>

#include <misc.h>
#include <array.h>
#include <plugin_manager.h>
#include <plugin_wpad.h>
#include <pac.h>

/* The top-level domain blacklist */
/* TODO: Make this not suck */
static char *tld[] = {
	/* General top-level domains */
	"arpa", "root", "aero", "biz", "cat", "com", "coop", "edu", "gov", "info",
	"int", "jobs", "mil", "mobi", "museum", "name", "net", "org", "pro", "travel",

	/* Country codes */
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

	/* Other domains to blacklist */
	"co.uk", "com.au",

	/* Terminator */
	NULL
};

typedef struct _pxDNSDevolutionWPADPlugin {
	PX_PLUGIN_SUBCLASS(pxWPADPlugin);
	pxArray *urls;
	int      next;
} pxDNSDevolutionWPADPlugin;

static char *
_get_domain_name()
{
	/* Get the hostname */
	char *hostname = px_malloc0(128);
	for (int i = 0 ; gethostname(hostname, (i + 1) * 128) && errno == ENAMETOOLONG ; )
		hostname = px_malloc0((++i + 1) * 128);

	/* Lookup the hostname */
	/* TODO: Make this whole process not suck */
	struct hostent *host_info = gethostbyname(hostname);
	if (host_info)
	{
		px_free(hostname);
		hostname = px_strdup(host_info->h_name);
	}

	/* Get domain portion */
	if (!strchr(hostname, '.')) return NULL;
	if (!strcmp(".", strchr(hostname, '.'))) return NULL;
	char *tmp = px_strdup(strchr(hostname, '.') + 1);
	px_free(hostname);
	return tmp;
}

static pxArray *
_get_urls()
{
	char *domain = _get_domain_name();
	if (!domain) return NULL;

	/* Split up the domain */
	char **domainv = px_strsplit(domain, ".");
	px_free(domain);
	if (!domainv) return NULL;

	pxArray *urls = px_array_new(NULL, px_free, false, false);
	for (int i=1 ; domainv[i] ; i++)
	{
		/* Check the domain against the blacklist */
		domain = px_strjoin((const char **) (domainv + i), ".");
		for (int k=0; tld[k] ; k++)
			if (!strcmp(domain, tld[k])) { px_free(domain); domain = NULL; break; }
		if (!domain) continue;

		/* Create a URL */
		char *url = px_strcat("http://wpad.", domain, "/wpad.dat", NULL);
		px_array_add(urls, url);
		px_free(domain);
	}

	return urls;
}

static pxPAC *
_next(pxWPADPlugin *self)
{
	pxDNSDevolutionWPADPlugin *dnsd = (pxDNSDevolutionWPADPlugin *) self;
	if (!dnsd->urls)
		dnsd->urls = _get_urls();
	if (!dnsd->urls)
		return NULL;

	pxPAC *pac = px_pac_new_from_string((char *) px_array_get(dnsd->urls, dnsd->next++));
	if (pac)
		self->found = true;
	return pac;
}

static void
_rewind(pxWPADPlugin *self)
{
	px_array_free(((pxDNSDevolutionWPADPlugin *) self)->urls);
	((pxDNSDevolutionWPADPlugin *) self)->urls = NULL;
	((pxDNSDevolutionWPADPlugin *) self)->next = 0;
	self->found = false;
}

static bool
_constructor(pxPlugin *self)
{
	((pxWPADPlugin *) self)->next   = _next;
	((pxWPADPlugin *) self)->rewind = _rewind;
    _rewind((pxWPADPlugin *) self);
	return true;
}

bool
px_module_load(pxPluginManager *self)
{
	return px_plugin_manager_constructor_add_subtype(self, "wpad_dnsdevolution", pxWPADPlugin, pxDNSDevolutionWPADPlugin, _constructor);
}
