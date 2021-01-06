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

#include <vector>
#include <cstring>   // For strdup()
#include <iostream>  // For cerr
#include <stdexcept> // For exception
#include <typeinfo>  // Only for debug messages.
#include "config.hpp"
#include <stdlib.h>

#include <gio/gio.h>

namespace libproxy {
using namespace std;

class proxy_factory {
public:
	proxy_factory();
	~proxy_factory();
	vector<string> get_proxies(const string &url);

private:
	bool   debug;
        GDBusProxy *proxy;
        GError **error = NULL;
};

proxy_factory::proxy_factory() {

  this->proxy = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SESSION,
                                               G_DBUS_PROXY_FLAGS_NONE,
                                               NULL, /* GDBusInterfaceInfo */
                                               "org.libproxy.proxy",
                                               "/org/libproxy/proxy",
                                               "org.libproxy.proxy",
                                               NULL, /* GCancellable */
                                               this->error);
}

proxy_factory::~proxy_factory() {

}


vector<string> proxy_factory::get_proxies(const string &realurl) {

	vector<string>             response;
	GVariant* result;
	result = g_dbus_proxy_call_sync (proxy,
					 "query",
					 g_variant_new("(s)", realurl.c_str()),
					 G_DBUS_CALL_FLAGS_NONE,
					 -1,
					 NULL,
					 this->error);

	GVariantIter *iter;
	gchar *str;

	g_variant_get (result, "(as)", &iter);
	while (g_variant_iter_loop (iter, "s", &str))
		response.push_back(str);
	g_variant_iter_free (iter);

	do_return:
		if (response.size() == 0)
			response.push_back("direct://");
		return response;
}

struct pxProxyFactory_ {
	libproxy::proxy_factory pf;
};

extern "C" DLL_PUBLIC struct pxProxyFactory_ *px_proxy_factory_new(void) {
	return new struct pxProxyFactory_;
}

extern "C" DLL_PUBLIC char** px_proxy_factory_get_proxies(struct pxProxyFactory_ *self, const char *url) {
	std::vector<std::string> proxies;
	char** retval;

	// Call the main method
	try {
		proxies = self->pf.get_proxies(url);
		retval  = (char**) malloc(sizeof(char*) * (proxies.size() + 1));
		if (!retval) return NULL;
	} catch (std::exception&) {
		// Return NULL on any exception
		return NULL;
	}

	// Copy the results into an array
	// Return NULL on memory allocation failure
	retval[proxies.size()] = NULL;
	for (size_t i=0 ; i < proxies.size() ; i++) {
		retval[i] = strdup(proxies[i].c_str());
		if (retval[i] == NULL) {
			for (int j=i-1 ; j >= 0 ; j--)
				free(retval[j]);
			free(retval);
			return NULL;
		}
	}
	return retval;
}

extern "C" DLL_PUBLIC void px_proxy_factory_free_proxies(char ** const proxies) {
	if (!proxies)
		return;

	for (size_t i = 0; proxies[i]; ++i)
		free(proxies[i]);

	free(proxies);
}

extern "C" DLL_PUBLIC void px_proxy_factory_free(struct pxProxyFactory_ *self) {
	delete self;
}
}
