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

#include <kstandarddirs.h> // For KStandardDirs
#include "xhasclient.cpp"  // For xhasclient(...)

#include "../config_file.hpp"
#include "../module_types.hpp"
using namespace com::googlecode::libproxy;

class kde_config_module : public config_module {
public:
	PX_MODULE_ID(NULL);
	PX_MODULE_CONFIG_CATEGORY(config_module::CATEGORY_SESSION);

	url get_config(url) throw (runtime_error) {
		// Open the config file
		if (this->cf.is_stale())
		{
			QString localdir = KStandardDirs().localkdedir();
			QByteArray ba = localdir.toLatin1();
			if (!cf.load(string(ba.data()) + "/share/config/kioslaverc"))
				throw runtime_error("Unable to load kde config file!");
		}

		try {
			// Read the config file to find out what type of proxy to use
			string ptype = this->cf.get_value("Proxy Settings", "ProxyType");

			// Use a manual proxy
			if (ptype == "1")
				return com::googlecode::libproxy::url(this->cf.get_value("Proxy Settings", "httpProxy"));

			// Use a manual PAC
			else if (ptype == "2")
			{
				string tmp = "";
				try { tmp = this->cf.get_value("Proxy Settings", "Proxy Config Script"); }
				catch (key_error&) {}

				if (tmp != "") return com::googlecode::libproxy::url(string("pac+") + tmp);
				else           return com::googlecode::libproxy::url("wpad://");
			}

			// Use WPAD
			else if (ptype == "3")
				return com::googlecode::libproxy::url("wpad://");

			// Use envvar
			else if (ptype == "4")
				throw runtime_error("User config_envvar"); // We'll bypass this config plugin and let the envvar plugin work
		}
		catch (key_error&) { }

		// Don't use any proxy
		return com::googlecode::libproxy::url("direct://");
	}

private:
	config_file cf;
};

PX_MODULE_LOAD(config, kde, xhasclient("kicker", NULL));
