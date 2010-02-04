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
#include "module.hpp"

#define PX_MODULE_LOAD_NAME px_module_load
#define PX_MODULE_LOAD(type, name, cond) \
	extern "C" DLL_PUBLIC bool PX_MODULE_LOAD_NAME(module_manager& mm) { \
		if (cond) return mm.register_module<type ## _module>(new name ## _ ## type ## _module); \
		return false; \
	}

namespace com {
namespace googlecode {
namespace libproxy {
using namespace std;

class DLL_PUBLIC module_manager {
public:
	typedef bool (*LOAD_TYPE)(module_manager&);

	~module_manager();

	template <class T> vector<T*> get_modules() const {
		map<string, vector<module*> >::const_iterator it = this->modules.find(typeid(T).name());
		vector<T*>      retlist;

		if (it != this->modules.end()) {
			vector<module*> modlist = it->second;

			for (size_t i=0 ; i < modlist.size() ; i++)
				retlist.push_back(dynamic_cast<T*>(modlist[i]));
		}

		return retlist;
	}

	template <class T> bool register_module(T* module)  {
		struct pcmp {
			static bool cmp(T* x, T* y) { return *x < *y; }
		};

		// If the class for this module is a singleton...
		if (this->singletons.find(typeid(T).name()) != this->singletons.end()) {
			// ... and we already have an instance of this class ...
			if (this->modules[typeid(T).name()].size() > 0) {
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
		this->modules[typeid(T).name()].clear();
		for (size_t i=0 ; i < modlist.size() ; i++)
			this->modules[typeid(T).name()].push_back(modlist[i]);

		return true;
	}

	template <class T> bool set_singleton(bool singleton) {
		if (singleton)
			return this->singletons.insert(typeid(T).name()).second;
		this->singletons.erase(typeid(T).name());
		return true;
	}

	bool load_file(const string filename, const string condsym="");
	bool load_dir(const string dirname);

private:
	set<dl_module*>               dl_modules;
	map<string, vector<module*> > modules;
	set<string>                   singletons;

	bool _load(const string filename, const string initsym);
};

}
}
}

#endif /* MODULEMANAGER_HPP_ */
