/*******************************************************************************
 * libproxy - A library for proxy configuration
 * Copyright (C) 2009 Nathaniel McCallum <nathaniel@natemccallum.com>
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

#ifndef MODULE_HPP_
#define MODULE_HPP_

#include <string>
#include <set>
#include <map>
#include <vector>
#include <typeinfo>
#include <algorithm>

#include "config.hpp"

#define PX_MODULE_ID(name) virtual string get_id() const { return module::make_name(name ? name : __FILE__); }

namespace com {
namespace googlecode {
namespace libproxy {
using namespace std;

class DLL_PUBLIC module {
public:
	static string make_name(string filename);

	virtual ~module();
	virtual bool operator<(const module&) const;
	virtual string get_id() const=0;
};

}
}
}

#endif /* MODULE_HPP_ */
