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

#include <cstdlib>

#include "../module_types.hpp"
using namespace com::googlecode::libproxy;

class envvar_config_module : public config_module {
public:
	PX_MODULE_ID(NULL);
	PX_MODULE_CONFIG_CATEGORY(config_module::CATEGORY_NONE);

	url get_config(url url) throw (runtime_error) {
		char *proxy = NULL;

		// If the URL is an ftp url, try to read the ftp proxy
		if (url.get_scheme() == "ftp") {
			if (!(proxy = getenv("ftp_proxy")))
				proxy = getenv("FTP_PROXY");
		}

		// If the URL is an https url, try to read the https proxy
		if (url.get_scheme() == "https") {
			if (!(proxy = getenv("https_proxy")))
				proxy = getenv("HTTPS_PROXY");
		}

		// If the URL is not ftp or no ftp_proxy was found, get the http_proxy
		if (!proxy) {
			if (!(proxy = getenv("http_proxy")))
				proxy = getenv("HTTP_PROXY");
		}

		if (!proxy)
			throw runtime_error("Unable to read configuration");
		return com::googlecode::libproxy::url(proxy);
	}

	string get_ignore(url dst) {
		char *ignore = getenv("no_proxy");
		      ignore = ignore ? ignore : getenv("NO_PROXY");
		return string(ignore ? ignore : "");
	}
};

PX_MODULE_LOAD(config_module, envvar, true);
