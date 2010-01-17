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

#include <cstring> // ?

#include "../module_types.hpp"
using namespace com::googlecode::libproxy;

// Work around a mozjs include bug
#ifndef JS_HAS_FILE_OBJECT
#define JS_HAS_FILE_OBJECT 0
#endif
#include <jsapi.h>
#include "pacutils.h"

#ifndef INET_ADDRSTRLEN
#define INET_ADDRSTRLEN 16
#endif

#ifndef INET6_ADDRSTRLEN
#define INET6_ADDRSTRLEN 46
#endif

static JSBool dnsResolve(JSContext *cx, JSObject */*obj*/, uintN /*argc*/, jsval *argv, jsval *rval) {
	// Get hostname argument
	char *tmp = JS_strdup(cx, JS_GetStringBytes(JS_ValueToString(cx, argv[0])));

	// Set the default return value
	*rval = JSVAL_NULL;

	// Look it up
	struct addrinfo *info = NULL;
	if (getaddrinfo(tmp, NULL, NULL, &info))
		goto out;

	// Allocate the IP address
	JS_free(cx, tmp);
	tmp = (char *) JS_malloc(cx, INET6_ADDRSTRLEN+1);
	memset(tmp, 0, INET6_ADDRSTRLEN+1);

	// Try for IPv4 and IPv6
	if (!inet_ntop(info->ai_family,
					&((struct sockaddr_in *) info->ai_addr)->sin_addr,
					tmp, INET_ADDRSTRLEN+1) > 0)
		if (!inet_ntop(info->ai_family,
						&((struct sockaddr_in6 *) info->ai_addr)->sin6_addr,
						tmp, INET6_ADDRSTRLEN+1) > 0)
			goto out;

	// We succeeded
	*rval = STRING_TO_JSVAL(JS_NewString(cx, tmp, strlen(tmp)));
	tmp = NULL;

	out:
		if (info) freeaddrinfo(info);
		JS_free(cx, tmp);
		return true;
}

static JSBool myIpAddress(JSContext *cx, JSObject *obj, uintN /*argc*/, jsval */*argv*/, jsval *rval) {
	char *hostname = (char *) JS_malloc(cx, 1024);
	if (!gethostname(hostname, 1023)) {
		JSString *myhost = JS_NewString(cx, hostname, strlen(hostname));
		jsval arg = STRING_TO_JSVAL(myhost);
		return dnsResolve(cx, obj, 1, &arg, rval);
	}
	JS_free(cx, hostname);
	*rval = JSVAL_NULL;
	return true;
}

class mozjs_pacrunner : public pacrunner {
public:
	mozjs_pacrunner(const pac pac) throw (bad_alloc) {
		jsval     rval;

		// Set defaults
		this->jsrun = NULL;
		this->jsctx = NULL;

		// Setup Javascript global class
		JSClass cls = {
				"global", JSCLASS_GLOBAL_FLAGS,
				JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
				JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
				NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
		};

		// Initialize Javascript runtime environment
		if (!(this->jsrun = JS_NewRuntime(1024 * 1024)))                  goto error;
		if (!(this->jsctx = JS_NewContext(this->jsrun, 1024 * 1024)))     goto error;
	    //JS_SetOptions(this->jsctx, JSOPTION_VAROBJFIX);
	    //JS_SetVersion(this->jsctx, JSVERSION_LATEST);
	    //JS_SetErrorReporter(cx, reportError);
		if (!(this->jsglb = JS_NewObject(this->jsctx, &cls, NULL, NULL))) goto error;
		if (!JS_InitStandardClasses(this->jsctx, this->jsglb))            goto error;

		// Define Javascript functions
		JS_DefineFunction(this->jsctx, this->jsglb, "dnsResolve", dnsResolve, 1, 0);
		JS_DefineFunction(this->jsctx, this->jsglb, "myIpAddress", myIpAddress, 0, 0);
		JS_EvaluateScript(this->jsctx, this->jsglb, JAVASCRIPT_ROUTINES,
				        strlen(JAVASCRIPT_ROUTINES), "pacutils.js", 0, &rval);

		// Add PAC to the environment
		JS_EvaluateScript(this->jsctx, this->jsglb, pac.to_string().c_str(),
							strlen(pac.to_string().c_str()),
							pac.get_url().to_string().c_str(), 0, &rval);
		return;

		error:
			if (this->jsctx) JS_DestroyContext(this->jsctx);
			if (this->jsrun) JS_DestroyRuntime(this->jsrun);
			throw bad_alloc();
	}

	~mozjs_pacrunner() {
		if (this->jsctx) JS_DestroyContext(this->jsctx);
		if (this->jsrun) JS_DestroyRuntime(this->jsrun);
		// JS_ShutDown()?
	}

	string run(const url url) throw (bad_alloc) {
		// Build arguments to the FindProxyForURL() function
		char *tmpurl  = JS_strdup(this->jsctx, url.to_string().c_str());
		char *tmphost = JS_strdup(this->jsctx, url.get_host().c_str());
		if (!tmpurl || !tmphost) {
			if (tmpurl) JS_free(this->jsctx, tmpurl);
			if (tmphost) JS_free(this->jsctx, tmphost);
			throw bad_alloc();
		}
		jsval args[2] = {
			STRING_TO_JSVAL(JS_NewString(this->jsctx, tmpurl, strlen(tmpurl))),
			STRING_TO_JSVAL(JS_NewString(this->jsctx, tmphost, strlen(tmphost)))
		};

		// Find the proxy (call FindProxyForURL())
		jsval rval;
		JSBool result = JS_CallFunctionName(this->jsctx, this->jsglb, "FindProxyForURL", 2, args, &rval);
		if (!result) return "";
		string answer = string(JS_GetStringBytes(JS_ValueToString(this->jsctx, rval)));
		if (answer == "undefined") return "";
		return answer;
	}

private:
	JSRuntime *jsrun;
	JSContext *jsctx;
	JSObject  *jsglb;
};

PX_DEFINE_PACRUNNER_MODULE(mozjs, true);
