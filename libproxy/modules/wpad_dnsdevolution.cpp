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

#include <cstring>

#include "../module_types.hpp"
using namespace com::googlecode::libproxy;

/* The domain blacklist */
/* TODO: Make this not suck */
/* Is there a list of 'public' domains somewhere? */
static const char *blacklist[] = {
	"co.uk", "com.au",

	/* Terminator */
	NULL
};

static string
_get_fqdn()
{
#define BUFLEN 512
	char hostname[BUFLEN];

	// Zero out the buffer
	memset(hostname, 0, BUFLEN);

	// Get the hostname
	if (gethostname(hostname, BUFLEN)) return "";

	/* Lookup the hostname */
	/* TODO: Make this whole process not suck */
	struct hostent *host_info = gethostbyname(hostname);
	if (host_info)
		strncpy(hostname, host_info->h_name, BUFLEN-1);

	try { return string(hostname).substr(string(hostname).find(".")).substr(1); }
	catch (out_of_range& e) { return ""; }
}

class dnsdevolution_wpad_module : public wpad_module {
public:
	PX_MODULE_ID(NULL);

	dnsdevolution_wpad_module() {
		this->rewind();
	}

	bool found() {
		return this->lpac != NULL;
	}

	pac* next() {
		// If we have rewound start the new count
		if (this->last == "")
			this->last = _get_fqdn();

		// Get the 'next' segment
		if (this->last.find(".") == string::npos) return NULL;
		this->last = this->last.substr(this->last.find(".")+1);

		// Don't do TLD's
		if (this->last.find(".") == string::npos) return NULL;

		// Process blacklist
		for (int i=0 ; blacklist[i] ; i++)
			if (this->last == blacklist[i])
				return NULL;

		// Try to load
		try { this->lpac = new pac(url(string("http://wpad.") + this->last + "/wpad.dat")); }
		catch (parse_error& e) { }
		catch (io_error& e) { }

		return this->lpac;
	}

	void rewind() {
		this->last = "";
	}


private:
	string last;
	pac*   lpac;
};

PX_MODULE_LOAD(wpad_module, dnsdevolution, true);
