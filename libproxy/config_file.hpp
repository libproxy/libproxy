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

#ifndef CONFIG_FILE_HPP_
#define CONFIG_FILE_HPP_

#include <stdexcept>
#include <map>
using namespace std;

namespace com {
namespace googlecode {
namespace libproxy {

class key_error : public runtime_error {
public:
	key_error(const string& __arg): runtime_error(__arg) {}
};

class config_file {
public:
	string get_value(const string key) throw (key_error);
	string get_value(const string section, const string key) throw (key_error);
	bool   is_stale();
	bool   load(string filename);

private:
	string                            filename;
	time_t                            mtime;
	map<string, map<string, string> > sections;
};

}
}
}

#endif /*CONFIG_FILE_HPP_*/
