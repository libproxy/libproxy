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
#include <unistd.h> // gethostname

#include "../extension_pacrunner.hpp"
using namespace libproxy;

// Work around a mozjs include bug
#ifndef JS_HAS_FILE_OBJECT
#define JS_HAS_FILE_OBJECT 0
#endif
#ifdef WIN32
#ifndef XP_WIN
#define XP_WIN
#endif
#endif
#pragma GCC diagnostic ignored "-Winvalid-offsetof"
#include <jsapi.h>
#pragma GCC diagnostic error "-Winvalid-offsetof"
#include <js/Initialization.h>
#include <js/CallArgs.h>
#include <js/CompilationAndEvaluation.h>
#include <js/MemoryFunctions.h>
#include <js/SourceText.h>

#include "pacutils.h"

#ifndef INET_ADDRSTRLEN
#define INET_ADDRSTRLEN 16
#endif

#ifndef INET6_ADDRSTRLEN
#define INET6_ADDRSTRLEN 46
#endif

static void dnsResolve_(JSContext *cx, JSString *hostname, JS::CallArgs *argv) {
	char *tmp;
	// Get hostname argument
	JS::RootedString str(cx, hostname);
	JS::UniqueChars chars = JS_EncodeStringToUTF8(cx, str);
	const char *val = chars.get();

	// Set the default return value
	argv->rval().setNull();

	// Look it up
	struct addrinfo *info = nullptr;
	if (getaddrinfo(val, NULL, NULL, &info))
		goto out;

	// Allocate the IP address
	tmp = (char *) JS_malloc(cx, INET6_ADDRSTRLEN+1);
	memset(tmp, 0, INET6_ADDRSTRLEN+1);

	// Try for IPv4 and IPv6
	if (getnameinfo(info->ai_addr, info->ai_addrlen,
					tmp, INET6_ADDRSTRLEN+1,
					NULL, 0,
					NI_NUMERICHOST)) goto out;

	// We succeeded
	argv->rval().setString(JS_NewStringCopyZ(cx, tmp));
	tmp = nullptr;

	out:
		if (info) freeaddrinfo(info);
}

static bool dnsResolve(JSContext *cx, unsigned argc, JS::Value *vp) {
	JS::CallArgs argv=JS::CallArgsFromVp(argc,vp);
	dnsResolve_(cx, argv[0].toString(), &argv);
	return true;
}

static bool myIpAddress(JSContext *cx, unsigned argc, JS::Value *vp) {
	JS::CallArgs argv=JS::CallArgsFromVp(argc,vp);
	char *hostname = (char *) JS_malloc(cx, 1024);
	hostname[1023] = '\0';

	if (!gethostname(hostname, 1023)) {
		JSString *myhost = JS_NewStringCopyN(cx, hostname, strlen(hostname));
		dnsResolve_(cx, myhost, &argv);
	} else {
		argv.rval().setNull();
	}

	JS_free(cx, hostname);
	return true;
}

// Setup Javascript global class
// This MUST be a static global
static JSClass cls = {
		"global", JSCLASS_GLOBAL_FLAGS,
};

class mozjs_pacrunner : public pacrunner {
public:
	mozjs_pacrunner(const string &pac, const url& pacurl) : pacrunner(pac, pacurl) {

		// Set defaults
		this->jsctx = nullptr;
		JS_Init();

		// Initialize Javascript context
		if (!(this->jsctx = JS_NewContext(1024 * 1024)))     goto error;
		{
			if (!JS::InitSelfHostedCode(this->jsctx)) goto error;

			JS::RootedValue  rval(this->jsctx);
			JS::RealmOptions realm_opts;

			this->jsglb = new JS::Heap<JSObject*>(JS_NewGlobalObject(
								  this->jsctx, &cls,
								  nullptr, JS::DontFireOnNewGlobalHook,
								  realm_opts));

			if (!(this->jsglb)) goto error;
			JS::RootedObject global(this->jsctx,this->jsglb->get());
			if (!(this->jsar = new JSAutoRealm(this->jsctx,  global))) goto error;

			// Define Javascript functions
			JS_DefineFunction(this->jsctx, global, "dnsResolve", dnsResolve, 1, 0);
			JS_DefineFunction(this->jsctx, global, "myIpAddress", myIpAddress, 0, 0);
			JS::CompileOptions options(this->jsctx);

			JS::SourceText<mozilla::Utf8Unit> routines, pac_source;
			if (!routines.init(this->jsctx,
					   JAVASCRIPT_ROUTINES,
					   strlen(JAVASCRIPT_ROUTINES),
					   JS::SourceOwnership::Borrowed))
				goto error;

			if (!pac_source.init(this->jsctx,
					     pac.c_str(),
					     pac.length(),
					     JS::SourceOwnership::Borrowed))
				goto error;


			JS::Evaluate(this->jsctx, options, routines, JS::MutableHandleValue(&rval));

			// Add PAC to the environment
			JS::Evaluate(this->jsctx, options, pac_source, JS::MutableHandleValue(&rval));
			return;
		}
		error:
			if (this->jsctx) JS_DestroyContext(this->jsctx);
			throw bad_alloc();
	}

	~mozjs_pacrunner() {
		if (this->jsar) delete this->jsar;
		if (this->jsglb) delete this->jsglb;
		if (this->jsctx) JS_DestroyContext(this->jsctx);
		JS_ShutDown();
	}

	string run(const url& url_) {
		// Build arguments to the FindProxyForURL() function
		string tmpurl(url_.to_string());
		string tmphost(url_.get_host());
		JS::AutoValueArray<2> args(this->jsctx);
		args[0].setString(JS_NewStringCopyZ(this->jsctx, tmpurl.c_str()));
		args[1].setString(JS_NewStringCopyZ(this->jsctx, tmphost.c_str()));

		// Find the proxy (call FindProxyForURL())
		JS::RootedValue rval(this->jsctx);
		JS::RootedObject global(this->jsctx,this->jsglb->get());
		bool result = JS_CallFunctionName(this->jsctx, global, "FindProxyForURL", args, &rval);
		if (!result) return "";
		if (!rval.isString())
			return "";

		JS::RootedString s(this->jsctx, rval.toString());
		JS::UniqueChars chars = JS_EncodeStringToUTF8(this->jsctx, s);
		const char *tmpanswer = chars.get();
		string answer = string(tmpanswer);

		if (answer == "undefined") return "";
		return answer;
	}

private:
	JSContext *jsctx;
	JS::Heap<JSObject*> *jsglb;
	JSAutoRealm *jsar;
};

PX_PACRUNNER_MODULE_EZ(mozjs, "JS_DefineFunction", "mozjs");
