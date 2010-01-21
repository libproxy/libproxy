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

#include "module_types.hpp"

namespace com {
namespace googlecode {
namespace libproxy {
using namespace std;

static const char *DEFAULT_CONFIG_ORDER[] = {
	"USER",
	"SESSION",
	"SYSTEM",
	"config_envvar",
	"config_wpad",
	NULL
};

static const char *DEFAULT_WPAD_ORDER[] = {
	"wpad_dhcp",
	"wpad_slp",
	"wpad_dns",
	"wpad_dnsdevolution",
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

pacrunner_module::pacrunner_module() {
	this->pr = NULL;
}

pacrunner_module::~pacrunner_module() {
	if (this->pr) delete this->pr;
}

string pacrunner_module::run(const char* pac, const url& dst) throw (bad_alloc) {
	if (!this->pr || this->last != pac) {
		if (this->pr) delete this->pr;
		this->last = pac;
		this->pr   = this->get_pacrunner(this->last);
	}

	return this->pr->run(dst);
}

bool wpad_module::operator<(const wpad_module& module) const {
	for (int i=0 ; DEFAULT_WPAD_ORDER[i] ; i++) {
		if (module.get_id() == DEFAULT_WPAD_ORDER[i])
			break;
		if (this->get_id() == DEFAULT_WPAD_ORDER[i])
			return true;
	}
	return false;
}


}
}
}
