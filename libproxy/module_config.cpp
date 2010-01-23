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

#include <cstdlib> // For getenv()

#include "module_config.hpp"
using namespace com::googlecode::libproxy;

static const char *DEFAULT_CONFIG_ORDER[] = {
	"USER",
	"SESSION",
	"SYSTEM",
	"config_envvar",
	"config_wpad",
	NULL
};

static inline int _config_module_findpos(config_module::category category, string id, string order) {
	if (order == "")
		return 0;

	size_t next    = order.find(",");
	string segment = order.substr(0, next);

    if ((segment == "USER"    && config_module::CATEGORY_USER    == category) ||
    	(segment == "SESSION" && config_module::CATEGORY_SESSION == category) ||
    	(segment == "SYSTEM"  && config_module::CATEGORY_SYSTEM  == category) ||
    	(segment == id))
    	return 0;

    return 1 + _config_module_findpos(category, id, order.substr(next == string::npos ? string::npos : next+1));
}

string config_module::get_ignore(url /*url*/) {
	return "";
}

bool config_module::set_creds(url /*proxy*/, string /*username*/, string /*password*/) {
	return false;
}

bool config_module::operator<(const config_module& module) const {
    // Attempt to get config order
    const char* fileorder = getenv("_PX_CONFIG_ORDER");
    const char* envorder  = getenv("PX_CONFIG_ORDER");

    // Create the config order
    string order     = string(fileorder ? fileorder : "") +
					   string((fileorder && envorder) ? "," : "") +
			           string( envorder ?  envorder : "");
    for (int i=0 ; DEFAULT_CONFIG_ORDER[i] ; i++)
    	order += string(",") + DEFAULT_CONFIG_ORDER[i];

    return (_config_module_findpos(this->get_category(), this->get_id(), order) -
    		_config_module_findpos(module.get_category(), module.get_id(), order) < 0);
}

bool config_module::get_valid() {
	return this->valid;
}

void config_module::set_valid(bool valid) {
	this->valid = valid;
}
