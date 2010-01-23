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

#ifndef URL_HPP_
#define URL_HPP_

#include <map>
#include <stdexcept>
#include <string>
#include <vector>

#include "config.hpp"

namespace com {
namespace googlecode {
namespace libproxy {

using namespace std;

class DLL_PUBLIC parse_error : public runtime_error {
public:
	parse_error(const string& __arg): runtime_error(__arg) {}
};

class DLL_PUBLIC url {
public:
	static bool is_valid(const string url);

	~url();
	url(const url& url);
	url(string url) throw (parse_error, logic_error);
	bool operator==(const url& url) const;
	url& operator=(const url& url);
	url& operator=(string url) throw (parse_error);

	string   get_host()     const;
	sockaddr const* const* get_ips(bool usedns);
	string   get_password() const;
	string   get_path()     const;
	uint16_t get_port()     const;
	string   get_scheme()   const;
	string   get_username() const;
	string   to_string()    const;
	char*    get_pac(); // Allocated, must free.  NULL on error.

private:
	string     host;
	sockaddr** ips;
	string     pass;
	string     path;
	uint16_t   port;
	string     scheme;
	string     orig;
	string     user;
};

}
}
}

#endif /*URL_HPP_*/
