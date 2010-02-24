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

#include <cstdlib> // For NULL

#ifndef MM_MODULE_BUILTIN
#define MM_MODULE_BUILTIN
#endif

#ifdef WIN32
#define __MM_DLL_EXPORT __declspec(dllexport)
#define __MM_FUNC_DEF_PREFIX extern "C" __MM_DLL_EXPORT
#define __MM_SCLR_DEF_PREFIX extern "C" __MM_DLL_EXPORT
#else
#include <typeinfo>
#define __MM_DLL_EXPORT __attribute__ ((visibility("default")))
#define __MM_FUNC_DEF_PREFIX            __MM_DLL_EXPORT
#define __MM_SCLR_DEF_PREFIX extern "C" __MM_DLL_EXPORT
#endif

#define __MM_MODULE_VERSION 1
#define __MM_MODULE_VARNAME__(prefix, name) prefix ## __mm_ ## name
#define __MM_MODULE_VARNAME_(prefix, name) __MM_MODULE_VARNAME__(prefix, name)
#define __MM_MODULE_VARNAME(name) __MM_MODULE_VARNAME_(MM_MODULE_BUILTIN, name)
#define MM_MODULE_INIT(mtype, minit) \
	__MM_SCLR_DEF_PREFIX const unsigned int  __MM_MODULE_VARNAME(vers)    = __MM_MODULE_VERSION; \
	__MM_FUNC_DEF_PREFIX const char*       (*__MM_MODULE_VARNAME(type))() = mtype::base_type; \
	__MM_FUNC_DEF_PREFIX base_extension**  (*__MM_MODULE_VARNAME(init))() = minit;
#define MM_MODULE_TEST(mtest) \
	__MM_FUNC_DEF_PREFIX bool              (*__MM_MODULE_VARNAME(test))() = mtest;
#define MM_MODULE_SYMB(msymb, msmod) \
	__MM_SCLR_DEF_PREFIX const char* const   __MM_MODULE_VARNAME(symb)    = msymb; \
	__MM_SCLR_DEF_PREFIX const char* const   __MM_MODULE_VARNAME(smod)    = msmod;

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
	static  const char*     base_type() { return NULL; }
	static  bool            singleton() { return false; }
	virtual           ~base_extension() {}
	virtual const char* get_base_type() const =0;
	virtual bool            operator<(const base_extension&) const =0;
};

template <class basetype, bool sngl=false>
class __MM_DLL_EXPORT extension : public base_extension {
public:
#ifdef WIN32
	static  const char*     base_type() { return __FUNCSIG__; }
#else
	static  const char*     base_type() { return typeid(basetype).name(); }
#endif

	static  bool            singleton() { return sngl; }
	virtual const char* get_base_type() const { return basetype::base_type(); }
	virtual bool            operator<(const base_extension&) const { return false; };
};

}

#endif /* MODULE_HPP_ */
