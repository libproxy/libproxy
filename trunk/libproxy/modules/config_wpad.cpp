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

#include "../module_types.hpp"
using namespace com::googlecode::libproxy;

class wpad_config_module : public config_module {
public:
	PX_MODULE_ID(NULL);
	PX_MODULE_CONFIG_CATEGORY(config_module::CATEGORY_NONE);

	url get_config(url) throw (runtime_error) {
		return url("wpad://");
	}
};

PX_MODULE_LOAD(config_module, wpad, true);
