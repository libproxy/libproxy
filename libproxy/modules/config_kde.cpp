/*******************************************************************************
 * libproxy - A library for proxy configuration
 * Copyright (C) 2006 Nathaniel McCallum <nathaniel@natemccallum.com>
 * Copyright (C) 2016 Fabian Vogt <fvogt@suse.com>
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

#include <algorithm>
#include <cstdlib>
#include <cstdio>

#include "../extension_config.hpp"
using namespace libproxy;

class kde_config_extension : public config_extension {
public:
    kde_config_extension()
    {
        try {
            // Try the KF5 one first
            command = "kreadconfig5";
            kde_config_val("proxyType", "-1");
            return; // Worked
        }
        catch(...) {}

        try {
            // The KDE4 one next
            command = "kreadconfig";
            kde_config_val("proxyType", "-1");
            return; // Worked
        }
        catch(...) {}

        // Neither worked, so throw in get_config
        command = "";
    }

    vector<url> get_config(const url &dst) throw (runtime_error) {
        // See constructor
        if(command.empty())
            throw runtime_error("Unable to read configuration");

        vector<url> response;

        string tmp, proxyType = kde_config_val("ProxyType", "-1");

        // Just switch on the first byte, either a digit, '-' ("-1") or '\0'
        switch(proxyType.c_str()[0])
        {
        case '1':
            tmp = kde_config_val(dst.get_scheme() + "Proxy", "");
            if(tmp.empty()) {
                tmp = kde_config_val("httpProxy", "");
                if(tmp.empty()) {
                    tmp = kde_config_val("socksProxy", "");
                    if(tmp.empty())
                        tmp = "direct://";
                }
            }

            // KDE uses "http://127.0.0.1 8080" instead of "http://127.0.0.1:8080"
            replace(tmp.begin(), tmp.end(), ' ', ':');

            response.push_back(url(tmp));
            break;

        case '2':
            tmp = "pac+" + kde_config_val("Proxy Config Script", "");
            if (url::is_valid(tmp))
            {
                response.push_back(url(tmp));
                break;
            }
            // else fallthrough

        case '3':
            response.push_back(url(string("wpad://")));
            break;

        case '4':
            throw runtime_error("User config_envvar"); // We'll bypass this config plugin and let the envvar plugin wor

        case '0':
        default: // Not set or unknown/illegal
            response.push_back(url("direct://"));
            break;
        }

        return response;
	}

	string get_ignore(const url&) {
        // See constructor
        if(command.empty())
            return "";

        string proxyType = kde_config_val("ProxyType", "-1");
        if(proxyType.c_str()[0] != '1')
            return ""; // Not manual config

        string prefix = kde_config_val("ReversedException", "false") != "false" ? "-" : "";
        return prefix + kde_config_val("NoProxyFor", ""); // Already in the right format
	}

private:
    // Neither key nor def must contain '
    string kde_config_val(const string &key, const string &def) throw (runtime_error) {
        string cmdline =
                command + " --file kioslaverc --group 'Proxy Settings' --key '" + key + "' --default '" + def + "'";

        FILE *pipe = popen(cmdline.c_str(), "r");
        if (!pipe)
            throw runtime_error("Unable to run command");

        char buffer[128];
        string result = "";
        while (!feof(pipe)) {
         if (fgets(buffer, 128, pipe) != NULL)
             result += buffer; // TODO: If this throws bad_alloc, pipe is leaked
        }

        pclose(pipe);

        // Trim newlines and whitespace at end
        result.erase(result.begin() + (result.find_last_not_of(" \n\t")+1), result.end());
        return result;
    }

    // Whether to use kreadconfig or kreadconfig5
    string command;
};

MM_MODULE_INIT_EZ(kde_config_extension, getenv("KDE_FULL_SESSION"), NULL, NULL);
