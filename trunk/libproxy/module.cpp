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

#include "module.hpp"
using namespace com::googlecode::libproxy;

string module::make_name(string filename) {
	// Basename
	if (filename.find_last_of(PATHSEP) != string::npos)
		filename = filename.substr(filename.find_last_of(PATHSEP)+1);

	// Noext
	if (filename.rfind('.') != string::npos)
		return filename.substr(0, filename.rfind('.'));
	return filename;
}

module::~module() {}

bool module::operator<(const module&) const { return false; }
