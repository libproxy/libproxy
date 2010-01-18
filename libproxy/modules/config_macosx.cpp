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

#include <sstream>

#include "../module_types.hpp"
using namespace com::googlecode::libproxy;

#include <SystemConfiguration/SystemConfiguration.h>

static bool getint(CFDictionaryRef settings, string key, int64_t& answer) {
	CFStringRef k = CFStringCreateWithCString(NULL, key.c_str(), kCFStringEncodingMacRoman);
	if (!k) return false;
	CFNumberRef n = (CFNumberRef) CFDictionaryGetValue(settings, k);
	CFRelease(k);
	if (!n) return false;
	if (!CFNumberGetValue(n, kCFNumberSInt64Type, &answer))
		return false;
	return true;
}

static bool getbool(CFDictionaryRef settings, string key, bool dflt=false) {
	int64_t i;
	if (!getint(settings, key, i)) return dflt;
	return i != 0;
}

static bool getstr(CFDictionaryRef settings, string key, string& str) {
	CFStringRef k = CFStringCreateWithCString(NULL, key.c_str(), kCFStringEncodingMacRoman);
	if (!k) return false;
	CFStringRef s = (CFStringRef) CFDictionaryGetValue(settings, k);
	CFRelease(k);
	if (!s) return false;
	const char* tmp = CFStringGetCStringPtr(s, kCFStringEncodingMacRoman);
	if (!tmp) return false;

	str = string(tmp);
	return true;
}

static bool protocol_url(CFDictionaryRef settings, string protocol, string& config) {
        int64_t port;
	string  host;

	// Check ProtocolEnabled
	if (!getbool(settings, protocol + "Enable"))
		return false;

	// Get ProtocolPort
	if (!getint(settings, protocol + "Port", port))
		return false;

	// Get ProtocolProxy
	if (!getstr(settings, protocol + "Proxy", host))
		return false;

	stringstream ss;
	if (protocol == "HTTP" || protocol == "HTTPS" || protocol == "FTP")
		ss << "http://";
	else if (protocol == "Gopher")
		ss << "gopher://"; // Is this correct?
	else if (protocol == "RTSP")
		ss << "rtsp://";   // Is this correct?
	else if (protocol == "SOCKS")
		ss << "socks://";
	else
		return false;
	ss << host;
	ss << ":";
	ss << port;

	config = ss.str();
	return true;
}

static string toupper(string str) { return str; }

class macosx_config_module : public config_module {
public:
	PX_MODULE_ID(NULL);
	PX_MODULE_CONFIG_CATEGORY(config_module::CATEGORY_NONE);

	url get_config(url url) throw (runtime_error) {
		string tmp;
		CFDictionaryRef proxies;

		proxies = SCDynamicStoreCopyProxies(NULL);
		if (!proxies) throw runtime_error("Unable to fetch proxy configuration");

		// wpad://
		if (getbool(proxies, "ProxyAutoDiscoveryEnable"))
			return com::googlecode::libproxy::url(string("wpad://"));

		// pac+http://...
		if (getbool(proxies, "ProxyAutoConfigEnable") &&
	            getstr(proxies, "ProxyAutoConfigURLString", tmp) &&
        	    url::is_valid(tmp))
			return com::googlecode::libproxy::url(string("pac+") + tmp);

		// http:// or socks://
		if (protocol_url(proxies, toupper(url.get_scheme()), tmp) ||
	            protocol_url(proxies, toupper("socks"), tmp))
			return com::googlecode::libproxy::url(tmp);

		// direct://
		return com::googlecode::libproxy::url(string("direct://"));
	}

/*	string get_ignore(url) {
		char *ignore = getenv("no_proxy");
		      ignore = ignore ? ignore : getenv("NO_PROXY");
		return string(ignore ? ignore : "");
	}*/
};

PX_MODULE_LOAD(config, macosx, true);

