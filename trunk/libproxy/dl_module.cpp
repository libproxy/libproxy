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

#include "dl_module.hpp"

#ifdef _WIN32
#include <windows.h>
#define pdlmtype HMODULE
#define pdlopen(filename) LoadLibrary(filename)
#define pdlsym GetProcAddress
#define pdlclose FreeLibrary
static std::string pdlerror() {
	std::string e;
	LPTSTR msg;

	FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER |FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			GetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR) &msg,
			0,
			NULL);
	e = std::string(msg);
    LocalFree(msg);
    return e;
}

#else
#include <dlfcn.h>
#define pdlmtype void*
#define pdlopen(filename) dlopen(filename, RTLD_NOW | RTLD_LOCAL)
#define pdlsym dlsym
#define pdlclose dlclose
static std::string pdlerror() { return std::string(dlerror()); }
#endif

namespace com {
namespace googlecode {
namespace libproxy {
using namespace std;

dl_module::~dl_module() {
	pdlclose((pdlmtype) this->dlobject);
}

dl_module::dl_module(const string filename) throw (dl_error) {
	if (filename == "")
		this->dlobject = pdlopen(NULL);
	else
		this->dlobject = pdlopen(filename.c_str());
	if (!this->dlobject)
		throw dl_error(pdlerror());
}

bool dl_module::operator==(const dl_module& module) const {
	return (this->dlobject == module.dlobject);
}

void* dl_module::getsym(const string symbolname) const throw (dl_error) {
	void*sym = pdlsym((pdlmtype) this->dlobject, symbolname.c_str());
	if (!sym)
		throw dl_error("Symbol not found: " + symbolname);
	return sym;
}

}
}
}
