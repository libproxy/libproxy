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
#define __MM_DLL_EXPORT __declspec(dllexport)
#define __MM_FUNC_DEF_PREFIX extern "C" __MM_DLL_EXPORT
#define __MM_SCLR_DEF_PREFIX extern "C" __MM_DLL_EXPORT
#else
#define __MM_DLL_EXPORT __attribute__ ((visibility("default")))
#define __MM_FUNC_DEF_PREFIX            __MM_DLL_EXPORT
#define __MM_SCLR_DEF_PREFIX extern "C" __MM_DLL_EXPORT
#endif

#define MM_MODULE_VERSION 1

#define MM_MODULE_VARNAME(name) __mm_ ## name
#define MM_MODULE_INIT(mtype, minit) \
	template <class T> static const char* mtype ## _type() { return T::base_type(); } \
	__MM_SCLR_DEF_PREFIX const unsigned int  MM_MODULE_VARNAME(vers)    = MM_MODULE_VERSION; \
	__MM_FUNC_DEF_PREFIX const char*       (*MM_MODULE_VARNAME(type))() = mtype ## _type<mtype>; \
	__MM_FUNC_DEF_PREFIX base_extension**  (*MM_MODULE_VARNAME(init))() = minit;
#define MM_MODULE_TEST(mtest) \
	__MM_FUNC_DEF_PREFIX bool              (*MM_MODULE_VARNAME(test))() = mtest;
#define MM_MODULE_SYMB(msymb, msmod) \
	__MM_SCLR_DEF_PREFIX const char* const   MM_MODULE_VARNAME(symb)    = msymb; \
	__MM_SCLR_DEF_PREFIX const char* const   MM_MODULE_VARNAME(smod)    = msmod;

#define MM_MODULE_INIT_EZ(clsname) \
	static libmodman::base_extension** clsname ## _init() { \
		libmodman::base_extension** retval = new libmodman::base_extension*[2]; \
		retval[0] = new clsname(); \
		retval[1] = NULL; \
		return retval; \
	} \
	MM_MODULE_INIT(clsname, clsname ## _init)
#define MM_MODULE_TEST_EZ(clsname, mtest) \
	static bool clsname ## _test() { return mtest; } \
	MM_MODULE_TEST(clsname ## _test)

namespace libmodman {

class __MM_DLL_EXPORT base_extension {
public:
	static const char* base_type() { return typeid(base_extension).name(); }
	static bool        singleton();

	virtual ~base_extension();
	virtual bool operator<(const base_extension&) const;
};

template <class T>
class __MM_DLL_EXPORT extension : public base_extension {
public:
	static const char* base_type() { return typeid(T).name(); }
};

}

#endif /* MODULE_HPP_ */
