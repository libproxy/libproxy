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

#ifndef MODULE_CONFIG_HPP_
#define MODULE_CONFIG_HPP_

#include "module_manager.hpp"
#include "url.hpp"

#define PX_MODULE_CONFIG_CATEGORY(cat) virtual category get_category() const { return cat; }

namespace com {
namespace googlecode {
namespace libproxy {
using namespace std;

// Config module
class DLL_PUBLIC config_module : public module {
public:
	typedef enum {
		CATEGORY_AUTO    = 0,
		CATEGORY_NONE    = 0,
		CATEGORY_SYSTEM  = 1,
		CATEGORY_SESSION = 2,
		CATEGORY_USER    = 3,
		CATEGORY__LAST   = CATEGORY_USER
	} category;

	// Abstract methods
	virtual category get_category() const=0;
	virtual url      get_config(url dst) throw (runtime_error)=0;

	// Virtual methods
	virtual string   get_ignore(url dst);
	virtual bool     set_creds(url proxy, string username, string password);

	// Final methods
	        bool     get_valid();
	        void     set_valid(bool valid);
	virtual bool     operator<(const config_module& module) const;
	using module::operator<;

private:
	bool valid;
};

}
}
}

#endif /* MODULE_CONFIG_HPP_ */
