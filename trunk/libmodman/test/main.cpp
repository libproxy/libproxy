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

#include <iostream>
#include <vector>
#include <cstring>

#include "main.hpp"

int main(int argc, const char** argv) {
	module_manager mm;

	if (argc < 2) {
		cerr << "Usage: " << argv[0] << " MODULEDIR MODNAME ..." << endl;
		return 1;
	}

	if (!mm.register_type<EXTTYPE>()) {
		cerr << "Unable to register type!" << endl;
		return 2;
	}

	if (!mm.load_dir(argv[1]) && argc > 2) {
		if (!mm.load_dir(argv[1], false)) {
			cerr << "Unable to load modules!" << endl;
			return 3;
		}
	}

	vector<EXTTYPE*> exts = mm.get_extensions<EXTTYPE>();
	if (exts.size() != (unsigned int) argc - 2) {
		cerr << "Wrong number of extensions found!" << endl;
		return 4;
	}

	for (unsigned int i=0 ; i < exts.size() ; i++) {
		if (!strstr(typeid(*(exts[i])).name(), argv[i+2])) {
			cerr << "Unable to find extension! Here's the list:" << endl;
			for (unsigned int j=0 ; j < exts.size() ; j++)
				cerr << "\t" << typeid(*(exts[j])).name() << endl;
			return 5;
		}
	}

	return 0;
}
