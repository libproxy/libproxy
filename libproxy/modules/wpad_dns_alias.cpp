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

#include "../extension_wpad.hpp"
#ifdef _WIN32
#define HOST_NAME_MAX 256
#else
#include <limits.h> // HOST_NAME_MAX
#endif // _WIN32

#include <list>

using namespace libproxy;

class dns_alias_wpad_extension : public wpad_extension {
public:
	dns_alias_wpad_extension() : lasturl(NULL), lastpac(NULL) {
        // According to RFC https://tools.ietf.org/html/draft-cooper-webi-wpad-00#page-11
        // We should check for 
        char hostbuffer[HOST_NAME_MAX] = {NULL};
        char tmp[HOST_NAME_MAX] = { NULL };
        char* ptr = NULL;

        // To retrieve hostname 
        gethostname(hostbuffer, sizeof(hostbuffer));
        std::string hostname(hostbuffer);
        possible_pac_urls.push_back(new url("http://wpad/wpad.dat"));

        ptr = &hostbuffer[0];
        for (int i = 0; i < std::count(hostname.begin(), hostname.end(), '.') -1; i++) {
            // get next subdomain
            ptr = strchr(ptr+1, '.');
            snprintf(tmp, sizeof(tmp), "http://wpad%s/wpad.dat", ptr);
            possible_pac_urls.push_back(new url(tmp));
        }
    }
	bool found() { return lastpac != NULL; }

	void rewind() {
		if (lasturl) delete lasturl;
		if (lastpac) delete lastpac;
		lasturl = NULL;
		lastpac = NULL;
	}

	url* next(char** pac) {
        for (auto possible_url : possible_pac_urls) {
            lastpac = *pac = possible_url->get_pac();
            if (!lastpac) {
                delete possible_url;
                possible_url = nullptr;
                lasturl = NULL;
            }
            else {
                return possible_url;
            } 
        }
        return NULL;
	}

private:
	url*  lasturl;
	char* lastpac;
    std::list<url*> possible_pac_urls;
};

MM_MODULE_INIT_EZ(dns_alias_wpad_extension, true, NULL, NULL);
