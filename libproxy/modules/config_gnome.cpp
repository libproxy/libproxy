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

#include <cstdio>         // For fileno(), fread(), pclose(), popen(), sscanf()
#include <sys/select.h>   // For select(...)
#include <fcntl.h>        // For fcntl(...)
#include "xhasclient.cpp" // For xhasclient(...)

/*
int popen2(const char *program, FILE **read, FILE **write) {
	int wpipe[2];

	if (!read || !write || !program || !*program)
		return EINVAL;

	*read  = NULL;
	*write = NULL;

	if (pipe(wpipe) < 0)
		return errno;

	switch (pid = vfork()) {
	case -1: // Error
		close(wpipe[0]);
		close(wpipe[1]);
		return ASOIMWE;
	case 0:  // Child
		close(wpipe[1]);
		dup2(wpipe[0], STDIN_FILENO);
		close(wpipe[0]);



		execl(_PATH_BSHELL, "sh", "-c", program, (char *)NULL);
			_exit(127);
			// NOTREACHED
		}
	}

	// Parent; assume fdopen can't fail.
	if (*type == 'r') {
			iop = fdopen(pdes[0], type);
			(void)close(pdes[1]);
	} else {
			iop = fdopen(pdes[1], type);
			(void)close(pdes[0]);
	}

	// Link into list of file descriptors.
	cur->fp = iop;
	cur->pid =  pid;
	cur->next = pidlist;
	pidlist = cur;

	return (iop);
}*/

#include "../module_types.hpp"
using namespace com::googlecode::libproxy;

static const char *_all_keys[] = {
	"/system/proxy/mode",        "/system/proxy/autoconfig_url",
	"/system/http_proxy/host",   "/system/http_proxy/port",
	"/system/proxy/secure_host", "/system/proxy/secure_port",
	"/system/proxy/ftp_host",    "/system/proxy/ftp_port",
	"/system/proxy/socks_host",  "/system/proxy/socks_port",
	"/system/http_proxy/ignore_hosts",
	"/system/http_proxy/use_authentication",
	"/system/http_proxy/authentication_user",
	"/system/http_proxy/authentication_password",
	NULL
};

class gnome_config_module : public config_module {
public:
	PX_MODULE_ID(NULL);
	PX_MODULE_CONFIG_CATEGORY(config_module::CATEGORY_SESSION);

	gnome_config_module() {
		int count;
		string cmd = LIBEXECDIR "pxgconf";
		for (count=0 ; _all_keys[count] ; count++)
			cmd += string(" ", 1) + _all_keys[count];

		this->pipe = popen(cmd.c_str(), "r");
		if (!this->pipe)
			throw io_error("Unable to open gconf helper!");
		if (fcntl(fileno(this->pipe), F_SETFL, FNONBLOCK) == -1) {
			pclose(this->pipe);
			throw io_error("Unable to set pipe to non-blocking!");
		}

		this->update_data(count);
	}

	~gnome_config_module() {
		if (this->pipe)
			pclose(this->pipe);
	}

	url get_config(url dest) throw (runtime_error) {
		// Check for changes in the config
		if (this->pipe) this->update_data();

		// Mode is wpad:// or pac+http://...
		if (this->data["/system/proxy/mode"] == "auto") {
			string pac = this->data["/system/proxy/autoconfig_url"];
			return url::is_valid(pac) ? url(string("pac+") + pac) : url("wpad://");
		}

		// Mode is http://... or socks://...
		else if (this->data["/system/proxy/mode"] == "manual") {
			string type = "http", host, port;
			bool   auth     = this->data["/system/http_proxy/use_authentication"] == "true";
			string username = this->data["/system/http_proxy/authentication_user"];
			string password = this->data["/system/http_proxy/authentication_password"];
			uint16_t p = 0;

			// Get the per-scheme proxy settings
			if (dest.get_scheme() == "https") {
				host = this->data["/system/proxy/secure_host"];
				port = this->data["/system/proxy/secure_port"];
				if (sscanf(port.c_str(), "%hu", &p) != 1) p = 0;
			}
			else if (dest.get_scheme() == "ftp") {
				host = this->data["/system/proxy/ftp_host"];
				port = this->data["/system/proxy/ftp_port"];
				if (sscanf(port.c_str(), "%hu", &p) != 1) p = 0;
			}
			if (host == "" || p == 0)
			{
				host = this->data["/system/http_proxy/host"];
				port = this->data["/system/http_proxy/port"];
				if (sscanf(port.c_str(), "%hu", &p) != 1) p = 0;
			}

			// If http(s)/ftp proxy is not set, try socks
			if (host == "" || p == 0)
			{
				host = this->data["/system/proxy/socks_host"];
				port = this->data["/system/proxy/socks_port"];
				if (sscanf(port.c_str(), "%hu", &p) != 1) p = 0;
			}

			// If host and port were found, build config url
			if (host != "" && p != 0) {
				string tmp = type + "://";
				if (auth)
					tmp += username + ":" + password + "@";
				tmp += host + ":" + port;
				return url(tmp);
			}
		}

		// Mode is direct://
		return url("direct://");
	}

	string get_ignore(url url) {
		return this->data["/system/http_proxy/ignore_hosts"];
	}

private:
	FILE               *pipe;
	map<string, string> data;

	string readline(string buffer="") {
		char c;

		// If the fread() call would block, an error occurred or
		// we are at the end of the line, we're done
		if (fread(&c, sizeof(char), 1, this->pipe) != 1 || c == '\n')
			return buffer;

		// Process the next character
		return this->readline(buffer + string(&c, 1));
	}

	// This method attempts to update data
	// If called with no arguments, it will check for new data (sleeping for <=1000
	// useconds) and returning true or false depending on if at least one line of
	// data was found.
	// However, if req > 0, we will keep checking for new lines (at 1000 usec ivals)
	// until enough lines are found.  This allows us to wait for *all* the initial
	// values to be read in before we start processing gconf requests.
	bool update_data(int req=0, int found=0) {
		// If we have collected the correct number of lines, return true
		if (req > 0 && found >= req)
			return true;

		// We need the pipe to be open
		if (!this->pipe) return false;

		fd_set rfds;
		struct timeval timeout = { 0, 1000 };
		FD_ZERO(&rfds);
		FD_SET(fileno(this->pipe), &rfds);
		if (select(fileno(this->pipe)+1, &rfds, NULL, NULL, &timeout) < 1)
			return req > 0 ? this->update_data(req, found) : false; // If we still haven't met
			                                                        // our req quota, try again

		bool retval = false;
		for (string line = this->readline() ; line != "" ; line = this->readline()) {
			string key = line.substr(0, line.find("\t"));
			string val = line.substr(line.find("\t")+1);
			this->data[key] = val;
			retval = ++found > req;
		}

		return (this->update_data(req, found) || retval);
	}
};

// If we are running in GNOME, then make sure this plugin is registered.
extern "C" bool px_module_load(module_manager& mm) {
	if (xhasclient("gnome-session", "gnome-settings-daemon", "gnome-panel", NULL)) {
		try { return mm.register_module<config_module>(new gnome_config_module); }
		catch (io_error) { return false; }
	}
}
