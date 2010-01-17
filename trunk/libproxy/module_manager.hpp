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

#ifndef MODULEMANAGER_HPP_
#define MODULEMANAGER_HPP_

#include <map>
#include <set>
#include <vector>
#include <typeinfo>
#include <algorithm>

#include "dl_module.hpp"

#ifdef BUILTIN
#define PX_MODULE_LOAD_NAME(type, name) type ## _ ## name ## _module_load
#else
#define PX_MODULE_LOAD_NAME(type, name) px_module_load
#endif
#define PX_MODULE_ID(name) virtual string get_id() const { return this->_bnne(name ? name : __FILE__); }
#define PX_MODULE_LOAD(type, name, cond) \
	extern "C" bool PX_MODULE_LOAD_NAME(type, name)(module_manager& mm) { \
		if (cond) return mm.register_module<type ## _module>(new name ## _ ## type ## _module); \
		return false; \
	}

namespace com {
namespace googlecode {
namespace libproxy {
using namespace std;

class module {
public:
	virtual ~module() {}
	virtual bool operator<(const module&) const { return false; }
	virtual string get_id() const=0;

protected:
	string _bnne(const string fn) const;
};

class module_manager {
public:
	typedef       bool (*INIT_TYPE)(module_manager&);
	static  const char*  INIT_NAME() { return "px_module_load"; }

	module_manager();
	~module_manager();

	template <class T> vector<T*> get_modules() const {
		vector<module*> modlist = this->modules.find(&typeid(T))->second;
		vector<T*>      retlist;

		for (size_t i=0 ; i < modlist.size() ; i++)
			retlist.push_back(dynamic_cast<T*>(modlist[i]));

		return retlist;
	}

	template <class T> bool register_module(T* module)  {
		  struct pcmp {
		    static bool cmp(T* x, T* y) { return *x < *y; }
		  };

		// If the class for this module is a singleton...
		if (this->singletons.find(&typeid(T)) != this->singletons.end()) {
			// ... and we already have an instance of this class ...
			if (this->modules[&typeid(T)].size() > 0) {
				// ... free the module and return
				delete module;
				return false;
			}
		}

		// Otherwise we just add the module and sort
		vector<T*> modlist = this->get_modules<T>();
		modlist.push_back(module);
		sort(modlist.begin(), modlist.end(), &pcmp::cmp);

		// Insert to our store
		this->modules[&typeid(T)].clear();
		for (size_t i=0 ; i < modlist.size() ; i++)
			this->modules[&typeid(T)].push_back(modlist[i]);

		return true;

	}

	template <class T> bool set_singleton(bool singleton) 	{
		if (singleton)
			return this->singletons.insert(&typeid(T)).second;
		this->singletons.erase(&typeid(T));
		return true;
	}

	bool load_file(const string filename);
	bool load_dir(const string dirname);

private:
	set<dl_module*>                         dl_modules;
	map<const type_info*, vector<module*> > modules;
	set<const type_info*>                   singletons;
};

}
}
}

#endif /* MODULEMANAGER_HPP_ */
