/*******************************************************************************
 * libproxy - A library for proxy configuration
 * Copyright (C) 2006 Nathaniel McCallum <nathaniel@natemccallum.com>
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

#include "../module_wpad.hpp"
using namespace com::googlecode::libproxy;

class dns_wpad_module : public wpad_module {
public:
	PX_MODULE_ID(NULL);

	dns_wpad_module() { rewind(); }
	bool found()      { return lastpac != NULL; }
	void rewind()     { lasturl = NULL; lastpac = NULL; }

	url* next(char** pac) {
		if (lasturl) return false;

		lasturl = new url("http://wpad/wpad.dat");
		lastpac = *pac = lasturl->get_pac();
		if (!lastpac) {
			delete lasturl;
			return NULL;
		}

		return lasturl;
	}

private:
	url*  lasturl;
	char* lastpac;
};

PX_MODULE_LOAD(wpad, dns, true);
