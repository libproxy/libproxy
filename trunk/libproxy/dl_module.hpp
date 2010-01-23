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

#ifndef DLOBJECT_HPP_
#define DLOBJECT_HPP_

#include <string>
#include <stdexcept>

#include "config.hpp"

namespace com {
namespace googlecode {
namespace libproxy {

using namespace std;

class dl_error : public runtime_error {
public:
	dl_error(const string& __arg): runtime_error(__arg) {}
};

class dl_module {
	public:
		     ~dl_module();
		      dl_module(string filename="")             throw (dl_error);
		void* get_symbol(string symbolname)       const throw (dl_error);
		bool  operator==(const dl_module& module) const;

	private:
		void* dlobject;
};

}
}
}

#endif /* DLOBJECT_HPP_ */
