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

#ifndef MODULE_WPAD_HPP_
#define MODULE_WPAD_HPP_

#include "module_manager.hpp"
#include "url.hpp"

namespace com {
namespace googlecode {
namespace libproxy {
using namespace std;

// WPAD module
class DLL_PUBLIC wpad_module : public module {
public:
	// Abstract methods
	virtual bool found()=0;
	virtual url* next(char** pac)=0;
	virtual void rewind()=0;

	// Virtual methods
	virtual bool operator<(const wpad_module& module) const;
	using module::operator<;
};

}
}
}

#endif /* MODULE_WPAD_HPP_ */
