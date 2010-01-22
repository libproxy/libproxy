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

#ifndef MODULE_TYPES_HPP_
#define MODULE_TYPES_HPP_

#include <stdexcept>

#include "module_manager.hpp"
#include "url.hpp"

namespace com {
namespace googlecode {
namespace libproxy {
using namespace std;

// Config module
class config_module : public module {
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

#define PX_MODULE_CONFIG_CATEGORY(cat) virtual category get_category() const { return cat; }

// Ignore module
class ignore_module : public module {
public:
	// Abstract methods
	virtual bool ignore(url& dst, string ignorestr)=0;
};

// Network module
class network_module : public module {
public:
	// Abstract methods
	virtual bool changed()=0;
};

// PACRunner module
class pacrunner {
public:
	pacrunner(string pac, string pacurl);
	virtual ~pacrunner() {};
	virtual string run(string url, string host) throw (bad_alloc)=0;
};

class pacrunner_module : public module {
public:
	// Virtual methods
	virtual pacrunner* get(string pac, string pacurl) throw (bad_alloc);
	virtual ~pacrunner_module();

	// Final methods
	pacrunner_module();

protected:
	// Abstract methods
	virtual pacrunner* create(string pac, string pacurl) throw (bad_alloc)=0;

private:
	pacrunner* pr;
	string     last;
};

#define PX_DEFINE_PACRUNNER_MODULE(name, cond) \
	class name ## _pacrunner_module : public pacrunner_module { \
	public: \
		PX_MODULE_ID(NULL); \
	protected: \
		virtual pacrunner* create(string pac, string pacurl) throw (bad_alloc) { \
			return new name ## _pacrunner(pac, pacurl); \
		} \
	}; \
	PX_MODULE_LOAD(pacrunner, name, cond)

// WPAD module
class wpad_module : public module {
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

#endif /* MODULE_TYPES_HPP_ */
