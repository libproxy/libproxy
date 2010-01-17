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


#ifndef PAC_HPP_
#define PAC_HPP_

#include <stdexcept>

#include "url.hpp"

namespace com {
namespace googlecode {
namespace libproxy {
using namespace std;

class io_error : public runtime_error {
public:
	io_error(const string& __arg): runtime_error(__arg) {}
};

class pac {
public:
	      ~pac();
	       pac(const url& url) throw (io_error);
	       pac(const pac& pac);
	url    get_url()     const;
	string to_string()   const;

private:
	url*   pacurl;
	string cache;
};

}
}
}

#endif /*PAC_HPP_*/
