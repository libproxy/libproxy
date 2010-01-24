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

#ifndef MODULE_PACRUNNER_HPP_
#define MODULE_PACRUNNER_HPP_

#include "module_manager.hpp"
#include "url.hpp"

#define PX_DEFINE_PACRUNNER_MODULE(name, cond) \
	class name ## _pacrunner_module : public pacrunner_module { \
	public: \
		PX_MODULE_ID(NULL); \
	protected: \
		virtual pacrunner* create(string pac, const url& pacurl) throw (bad_alloc) { \
			return new name ## _pacrunner(pac, pacurl); \
		} \
	}; \
	PX_MODULE_LOAD(pacrunner, name, cond)

namespace com {
namespace googlecode {
namespace libproxy {

// PACRunner module
class DLL_PUBLIC pacrunner {
public:
	pacrunner(string pac, const url& pacurl);
	virtual ~pacrunner() {};
	virtual string run(const url& url) throw (bad_alloc)=0;
};

class DLL_PUBLIC pacrunner_module : public module {
public:
	// Virtual methods
	virtual pacrunner* get(string pac, const url& pacurl) throw (bad_alloc);
	virtual ~pacrunner_module();

	// Final methods
	pacrunner_module();

protected:
	// Abstract methods
	virtual pacrunner* create(string pac, const url& pacurl) throw (bad_alloc)=0;

private:
	pacrunner* pr;
	string     last;
};

}
}
}


#endif /* MODULE_PACRUNNER_HPP_ */
