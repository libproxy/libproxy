/*******************************************************************************
 * libmodman - A library for extending applications
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

#include <typeinfo>
#include <cstdlib>

#ifdef WIN32
#define DLL_PUBLIC __declspec(dllexport)
#define __MOD_DEF_PREFIX extern "C"
#else
#define DLL_PUBLIC __attribute__ ((visibility("default")))
#define __MOD_DEF_PREFIX
#endif

#define MM_MODULE_VERSION 1
#define MM_MODULE_NAME __module
#define MM_MODULE_DEFINE __MOD_DEF_PREFIX struct libmodman::module DLL_PUBLIC MM_MODULE_NAME[]

#define MM_MODULE_LAST { MM_MODULE_VERSION, NULL, NULL, NULL, NULL }
#define MM_MODULE_RECORD(type, init, test, symb) \
	{ MM_MODULE_VERSION, type::base_type(), init, test, symb }

#define MM_MODULE_EZ(clsname, cond, symb) \
	static bool clsname ## _test() { \
		return (cond); \
	} \
	static libmodman::base_extension** clsname ## _init() { \
		libmodman::base_extension** retval = new libmodman::base_extension*[2]; \
		retval[0] = new clsname(); \
		retval[1] = NULL; \
		return retval; \
	} \
	MM_MODULE_DEFINE = { \
		MM_MODULE_RECORD(clsname, clsname ## _init, clsname ## _test, symb), \
		MM_MODULE_LAST, \
	};

namespace libmodman {

class DLL_PUBLIC base_extension {
public:
	static const char* base_type() { return typeid(base_extension).name(); }
	static bool        singleton();

	virtual ~base_extension();
	virtual bool operator<(const base_extension&) const;
};

template <class T>
class DLL_PUBLIC extension : public base_extension {
public:
	static const char* base_type() { return typeid(T).name(); }
};

struct module {
	const unsigned int  vers;
	const char* const   type;
	base_extension**  (*init)();
	bool              (*test)();
	const char* const   symb;
};

}

#endif /* MODULE_HPP_ */
