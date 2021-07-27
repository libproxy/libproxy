/*******************************************************************************
 * libproxy - A library for proxy configuration
 * Copyright (C) 2006 Nathaniel McCallum <nathaniel@natemccallum.com>
 * Copyright (C) 2021 Zhaofeng Li <hello@zhaofeng.li>
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

#include "../extension_pacrunner.hpp"
#include <unistd.h> // gethostname
using namespace libproxy;

#include <duktape.h>
#include "pacutils.h"

static duk_ret_t dnsResolve(duk_context *ctx) {
	if (duk_get_top(ctx) != 1) {
		// Invalid number of arguments
		return 0;
	}

	// We do not need to free the string - It's managed by Duktape.
	const char *hostname = duk_get_string(ctx, 0);
	if (!hostname) {
		return 0;
	}

	// Look it up
	struct addrinfo *info;
	if (getaddrinfo(hostname, NULL, NULL, &info)) {
		return 0;
	}

	// Try for IPv4
	char tmp[INET6_ADDRSTRLEN+1];
	if (getnameinfo(info->ai_addr, info->ai_addrlen,
					tmp, INET6_ADDRSTRLEN+1,
					NULL, 0,
					NI_NUMERICHOST)) {
		freeaddrinfo(info);
		duk_push_null(ctx);
		return 1;
	}
	freeaddrinfo(info);

	// Create the return value
	duk_push_string(ctx, tmp);
	return 1;
}

static duk_ret_t myIpAddress(duk_context *ctx) {
	char hostname[1024];
	hostname[sizeof(hostname) - 1] = '\0';

	if (!gethostname(hostname, sizeof(hostname) - 1)) {
		duk_push_string(ctx, hostname);
		return dnsResolve(ctx);
	}

	return duk_error(ctx, DUK_ERR_ERROR, "Unable to find hostname!");
}

class duktape_pacrunner : public pacrunner {
public:
	duktape_pacrunner(string pac, const url& pacurl) : pacrunner(pac, pacurl) {
		this->ctx = duk_create_heap_default();
		if (!this->ctx) goto error;
		duk_push_c_function(this->ctx, dnsResolve, 1);
		duk_put_global_string(this->ctx, "dnsResolve");

		duk_push_c_function(this->ctx, myIpAddress, 1);
		duk_put_global_string(this->ctx, "myIpAddress");

		// Add other routines
		duk_push_string(this->ctx, JAVASCRIPT_ROUTINES);
		if (duk_peval_noresult(this->ctx)) {
			goto error;
		}

		// Add the PAC into the context
		duk_push_string(this->ctx, pac.c_str());
		if (duk_peval_noresult(this->ctx)) {
			goto error;
		}

		return;
	error:
		duk_destroy_heap(this->ctx);
		throw bad_alloc();
	}

	~duktape_pacrunner() {
		duk_destroy_heap(this->ctx);
	}

	string run(const url& url_) override {
		string url = url_.to_string();
		string host = url_.get_host();

		duk_get_global_string(this->ctx, "FindProxyForURL");
		duk_push_string(this->ctx, url.c_str());
		duk_push_string(this->ctx, host.c_str());
		duk_int_t result = duk_pcall(this->ctx, 2);

		if (result == 0) {
			// Success
			const char *proxy = duk_get_string(this->ctx, 0);
			if (!proxy) {
				duk_pop(this->ctx);
				return "";
			}
			string proxyString = string(proxy);
			duk_pop(this->ctx);
			return proxyString;
		} else {
			// Something happened. The top of the stack is an error.
			duk_pop(this->ctx);
			return "";
		}
	}

private:
	duk_context *ctx;
};

PX_PACRUNNER_MODULE_EZ(duktape, "duk_create_heap_default", "duktape");
