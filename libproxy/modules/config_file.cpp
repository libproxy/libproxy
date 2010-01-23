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

#include "../config_file.hpp"
#include "../module_config.hpp"
using namespace com::googlecode::libproxy;

class system_file_config_module : public config_module {
public:
	PX_MODULE_ID("config_file_system");
	PX_MODULE_CONFIG_CATEGORY(config_module::CATEGORY_SYSTEM);

	system_file_config_module() {
		this->cf.load(this->get_filename());
	}

	url get_config(url) throw (runtime_error) {
		if (this->cf.is_stale())
			this->cf.load(this->get_filename());
		string tmp = "";
		this->cf.get_value("proxy", tmp);
		return tmp;
	}

	string get_ignore(url) {
		if (this->cf.is_stale())
			this->cf.load(this->get_filename());
		string tmp = "";
		this->cf.get_value("ignore", tmp);
		return tmp;
	}

protected:
	virtual string get_filename() { return SYSCONFDIR "proxy.conf"; }

private:
	config_file cf;
};

class user_file_config_module : public system_file_config_module {
public:
	PX_MODULE_ID("config_file_user");
	PX_MODULE_CONFIG_CATEGORY(config_module::CATEGORY_USER);

protected:
	virtual string get_filename() { return string(getenv("HOME")) + string("/.proxy.conf"); }
};

extern "C" bool PX_MODULE_LOAD_NAME(config, file)(module_manager& mm) {
	bool success = mm.register_module<config_module>(new user_file_config_module);
	return mm.register_module<config_module>(new system_file_config_module) || success;
}
