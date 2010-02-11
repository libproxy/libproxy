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
#include <sys/select.h>   // For select()
#include <fcntl.h>        // For fcntl()
#include <errno.h>        // For errno stuff
#include <unistd.h>       // For pipe(), close(), vfork(), dup(), execl(), _exit()
#include <signal.h>       // For kill()
#include "xhasclient.cpp" // For xhasclient()

#include "../extension_config.hpp"
using namespace libproxy;

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

static int popen2(const char *program, int* read, int* write, pid_t* pid) {
	if (!read || !write || !pid || !program || !*program)
		return EINVAL;
	*read  = NULL;
	*write = NULL;
	*pid   = 0;

	// Open the pipes
	int rpipe[2];
	int wpipe[2];
	if (pipe(rpipe) < 0)
		return errno;
	if (pipe(wpipe) < 0) {
		close(rpipe[0]);
		close(rpipe[1]);
		return errno;
	}

	switch (*pid = vfork()) {
	case -1: // Error
		close(rpipe[0]);
		close(rpipe[1]);
		close(wpipe[0]);
		close(wpipe[1]);
		return errno;

	case 0: // Child
		close(STDIN_FILENO);  // Close stdin
		close(STDOUT_FILENO); // Close stdout

		dup(wpipe[0]); // Dup the read end of the write pipe to stdin
		dup(rpipe[1]); // Dup the write end of the read pipe to stdout

		// Close unneeded fds
		close(rpipe[0]);
		close(rpipe[1]);
		close(wpipe[0]);
		close(wpipe[1]);

		// Exec
		execl("/bin/sh", "sh", "-c", program, (char*) NULL);
		_exit(127);  // Whatever we do, don't return

	default: // Parent
		close(rpipe[1]);
		close(wpipe[0]);
		*read  = rpipe[0];
		*write = wpipe[1];
		return 0;
	}
}

class gnome_config_extension : public config_extension {
public:
	gnome_config_extension() {
		// Build the command
		int count;
		string cmd = LIBEXECDIR "pxgconf";
		for (count=0 ; _all_keys[count] ; count++)
			cmd += string(" ", 1) + _all_keys[count];

		// Get our pipes
		if (popen2(cmd.c_str(), &this->read, &this->write, &this->pid) != 0)
			throw runtime_error("Unable to open gconf helper!");

		// Set the read pipe to non-blocking
		if (fcntl(this->read, F_SETFL, FNONBLOCK) == -1) {
			close(this->read);
			close(this->write);
			kill(this->pid, SIGTERM);
			throw runtime_error("Unable to set pipe to non-blocking!");
		}

		// Read in the first print-out of all our keys
		this->update_data(count);
	}

	~gnome_config_extension() {
		close(this->read);
		close(this->write);
		kill(this->pid, SIGTERM);
	}

	url get_config(url dest) throw (runtime_error) {
		// Check for changes in the config
		this->update_data();

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

	string get_ignore(url) {
		return this->data["/system/http_proxy/ignore_hosts"];
	}

	bool set_creds(url /*proxy*/, string username, string password) {
		string auth = "/system/http_proxy/use_authentication\ttrue\n";
		string user = string("/system/http_proxy/authentication_user\t") + username + "\n";
		string pass = string("/system/http_proxy/authentication_password\t") + password + "\n";

		return (::write(this->write, auth.c_str(), auth.size()) == (ssize_t) auth.size() &&
				::write(this->write, user.c_str(), user.size()) == (ssize_t) user.size() &&
				::write(this->write, pass.c_str(), pass.size()) == (ssize_t) pass.size());
	}

private:
	int   read;
	int   write;
	pid_t pid;
	map<string, string> data;

	string readline(string buffer="") {
		char c;

		// If the read() call would block, an error occurred or
		// we are at the end of the line, we're done
		if (::read(this->read, &c, 1) != 1 || c == '\n')
			return buffer;

		// Process the next character
		return this->readline(buffer + string(&c, 1));
	}

	// This method attempts to update data
	// If called with no arguments, it will check for new data (sleeping for <=10
	// useconds) and returning true or false depending on if at least one line of
	// data was found.
	// However, if req > 0, we will keep checking for new lines (at 10 usec ivals)
	// until enough lines are found.  This allows us to wait for *all* the initial
	// values to be read in before we start processing gconf requests.
	bool update_data(int req=0, int found=0) {
		// If we have collected the correct number of lines, return true
		if (req > 0 && found >= req)
			return true;

		// We need the pipe to be open
		if (!this->read) return false;

		fd_set rfds;
		struct timeval timeout = { 0, 10 }; // select() for 1/1000th of a second
		FD_ZERO(&rfds);
		FD_SET(this->read, &rfds);
		if (select(this->read+1, &rfds, NULL, NULL, &timeout) < 1)
			return req > 0 ? this->update_data(req, found) : false; // If we still haven't met
			                                                        // our req quota, try again

		bool retval = false;
		for (string line = this->readline() ; line != "" ; line = this->readline()) {
			string key = line.substr(0, line.find("\t"));
			string val = line.substr(line.find("\t")+1);
			this->data[key] = val;
			retval = ++found >= req;
		}

		return (this->update_data(req, found) || retval);
	}
};

// Only attempt to load this module if we are in a gnome session
static bool gnome_config_extension_test() {
	return xhasclient("gnome-session", "gnome-settings-daemon", "gnome-panel", NULL);
}

static base_extension** gnome_config_extension_init() {
	base_extension** retval = new base_extension*[2];
	retval[1] = NULL;
	try {
		retval[0] = new gnome_config_extension();
		return retval;
	}
	catch (runtime_error) {
		delete retval;
		return NULL;
	}
}

MM_MODULE_DEFINE = {
		MM_MODULE_RECORD(gnome_config_extension, gnome_config_extension_init, gnome_config_extension_test, NULL, NULL),
		MM_MODULE_LAST,
};
