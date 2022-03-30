/*******************************************************************************
 * libproxy - A library for proxy configuration
 * Copyright (C) 2006 Nathaniel McCallum <nathaniel@natemccallum.com>
 * Copyright (C) 2021 Fabian Vogt <fvogt@suse.com>
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

#include <sys/stat.h>
#include <pwd.h>

#include <algorithm>
#include <cstdlib>
#include <cstdio>
#include <sstream>

#ifdef WIN32
#define popen _popen
#define pclose _pclose
#else
#include <unistd.h>
#endif

#include "../extension_config.hpp"
using namespace libproxy;

class kde_config_extension : public config_extension {
public:
    kde_config_extension()
        : cache_time(0)
    {
        try {
            // Try the KF5 one first
            command = "kreadconfig5";
            command_output("kreadconfig5 --key nonexistant");

            try {
                use_xdg_config_dirs();
            }
            catch(...) {}

            return; // Worked
        }
        catch(...) {}

        try {
            // The KDE4 one next
            command = "kreadconfig";
            command_output(command);

            try {
                parse_dir_list(command_output("kde4-config --path config"));
            }
            catch(...) {}

            return; // Worked
        }
        catch(...) {}

        // Neither worked, so throw in get_config
        command = "";
    }

    vector<url> get_config(const url &dst) {
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
    string command_output(const string &cmdline) {
        // Capture stderr as well
        const string command = "(" + cmdline + ")2>&1";
        FILE *pipe = popen(command.c_str(), "r");
        if (!pipe)
            throw runtime_error("Unable to run command");

        char buffer[128];
        string result = "";
        while (!feof(pipe)) {
            if (fgets(buffer, 128, pipe) != NULL)
                result += buffer; // TODO: If this throws bad_alloc, pipe is leaked
        }

        if(pclose(pipe) != 0)
            throw runtime_error("Command failed");

        // Trim newlines and whitespace at end
        result.erase(result.begin() + (result.find_last_not_of(" \n\t")+1), result.end());

        return result;
    }

    // Neither key nor def must contain '
    const string &kde_config_val(const string &key, const string &def) {
        if (cache_needs_refresh())
            cache.clear();
        else {
            // Already in cache?
            map<string, string>::iterator it = cache.find(key);
            if(it != cache.end())
                return it->second;
        }

        // Although all values are specified internally and/or validated,
        // checking is better than trusting.
        if(key.find('\'') != string::npos || def.find('\'') != string::npos)
            return def;

        // Add result to cache and return it
        return cache[key] = command_output(
                command + " --file kioslaverc --group 'Proxy Settings' --key '" + key + "' --default '" + def + "'");
    }

    // Used for cache invalidation
    struct configfile {
        string path;
        time_t mtime; // 0 means either not refreshed or doesn't exist
    };

    // Parses colon-delimited lists of paths to fill config_locs
    void parse_dir_list(const string &dirs) {
        string config_path;
        stringstream config_paths_stream(dirs);

        // Try each of the listed folders, seperated by ':'
        while (getline(config_paths_stream, config_path, ':')) {
            configfile config_loc;
            config_loc.path = config_path + "/kioslaverc";
            config_loc.mtime = 0;
            config_locs.push_back(config_loc);
        }
    }

    // Add XDG configuration locations to the configuration paths
    void use_xdg_config_dirs() {
        // Return environment value as std::string if set, otherwise def
        auto getenv_default = [](const char *name, const std::string &def) {
            const char *ret = getenv(name);
            return std::string(ret ? ret : def);
        };

        std::string homedir = getenv_default("HOME", "");
        if (homedir.empty()) {
            size_t bufsize = sysconf(_SC_GETPW_R_SIZE_MAX);
            if (bufsize == -1)
                bufsize = 16384;

            std::vector<char> buf(bufsize);
            struct passwd pwd, *result;
            getpwuid_r(getuid(), &pwd, buf.data(), buf.size(), &result);
            if (result)
                homedir = pwd.pw_dir;
        }

        if (homedir.empty())
            throw std::runtime_error("Failed to get home directory");

        parse_dir_list(getenv_default("XDG_CONFIG_HOME", homedir + "/.config"));
        parse_dir_list(getenv_default("XDG_CONFIG_DIRS", "/etc/xdg"));
    }

    // If any of the locations in config_locs changed (different mtime),
    // update config_locs and return true.
    bool cache_needs_refresh() {
        // Play safe here, if we can't determine the location,
        // don't cache at all.
        bool needs_refresh = config_locs.empty();
        struct stat config_info;

        for (unsigned int i = 0; i < config_locs.size(); ++i) {
            configfile &config = config_locs[i];
            time_t current_mtime = stat(config.path.c_str(), &config_info) == 0 ? config_info.st_mtime : 0;
            if (config.mtime != current_mtime) {
                config.mtime = current_mtime;
                needs_refresh = true;
            }
        }

        return needs_refresh;
    }

    // Whether to use kreadconfig or kreadconfig5
    string command;
    // When the cache was flushed last
    time_t cache_time;
    // Cache for config values
    map<string, string> cache;
    // State of the config files at the time of the last cache flush
    vector<configfile> config_locs;
};

MM_MODULE_INIT_EZ(kde_config_extension, getenv("KDE_FULL_SESSION"), NULL, NULL);
