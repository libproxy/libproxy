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

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#define __USE_BSD
#include <unistd.h>

#include <misc.h>
#include <modules.h>

#include <jsapi.h>
#include "pacutils.h"

typedef struct {
	JSRuntime *run;
	JSContext *ctx;
	JSClass   *cls;
	char      *pac;
} ctxStore;

typedef struct _pxMozillaPACRunnerModule {
	PX_MODULE_SUBCLASS(pxPACRunnerModule);
	ctxStore *ctxs;
} pxMozillaPACRunnerModule;

static JSBool dnsResolve(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
	// Get hostname argument
	char *tmp = px_strdup(JS_GetStringBytes(JS_ValueToString(cx, argv[0])));

	// Set the default return value
	*rval = JSVAL_NULL;

	// Look it up
	struct addrinfo *info = NULL;
	if (getaddrinfo(tmp, NULL, NULL, &info))
		goto out;

	// Allocate the IP address
	px_free(tmp);
	tmp = px_malloc0(INET6_ADDRSTRLEN+1);

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
		px_free(tmp);
		return true;
}

static JSBool myIpAddress(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
	char *hostname = JS_malloc(cx, 1024);
	if (!gethostname(hostname, 1023)) {
		JSString *myhost = JS_NewString(cx, hostname, strlen(hostname));
		jsval arg = STRING_TO_JSVAL(myhost);
		return dnsResolve(cx, obj, 1, &arg, rval);
	}
	px_free(hostname);
	*rval = JSVAL_NULL;
	return true;
}

static void ctxs_free(ctxStore *self)
{
	if (!self) return;
	if (self->ctx) JS_DestroyContext(self->ctx);
	if (self->run) JS_DestroyRuntime(self->run);
	if (self->cls) px_free(self->cls);
	px_free(self->pac);
	px_free(self);
}

static ctxStore *ctxs_new(pxPAC *pac)
{
	JSObject *global = NULL;
	jsval     rval;

	// Create the basic context
	ctxStore *self = px_malloc0(sizeof(ctxStore));

	// Setup Javascript global class
	self->cls     = px_malloc0(sizeof(JSClass));
	self->cls->name        = "global";
	self->cls->flags       = 0;
	self->cls->addProperty = JS_PropertyStub;
	self->cls->delProperty = JS_PropertyStub;
	self->cls->getProperty = JS_PropertyStub;
	self->cls->setProperty = JS_PropertyStub;
	self->cls->enumerate   = JS_EnumerateStub;
	self->cls->resolve     = JS_ResolveStub;
	self->cls->convert     = JS_ConvertStub;
	self->cls->finalize    = JS_FinalizeStub;

	// Initialize Javascript runtime environment
	if (!(self->run = JS_NewRuntime(1024 * 1024)))                   goto error;
	if (!(self->ctx = JS_NewContext(self->run, 1024 * 1024)))        goto error;
	if (!(global  = JS_NewObject(self->ctx, self->cls, NULL, NULL))) goto error;
	JS_InitStandardClasses(self->ctx, global);

	// Define Javascript functions
	JS_DefineFunction(self->ctx, global, "dnsResolve", dnsResolve, 1, 0);
	JS_DefineFunction(self->ctx, global, "myIpAddress", myIpAddress, 0, 0);
	JS_EvaluateScript(self->ctx, global, JAVASCRIPT_ROUTINES,
			        strlen(JAVASCRIPT_ROUTINES), "pacutils.js", 0, &rval);

	// Add PAC to the environment
	JS_EvaluateScript(self->ctx, global, px_pac_to_string(pac),
                       strlen(px_pac_to_string(pac)),
                       px_url_to_string((pxURL *) px_pac_get_url(pac)),
                       0, &rval);

	// Save the pac
	self->pac = px_strdup(px_pac_to_string(pac));
	return self;

error:
	ctxs_free(self);
	return NULL;
}

static void
_destructor(void *s)
{
	pxMozillaPACRunnerModule * self = (pxMozillaPACRunnerModule *) s;

	ctxs_free(self->ctxs);
	px_free(self);
}

static char *
_run(pxPACRunnerModule *self, pxPAC *pac, pxURL *url)
{
	// Make sure we have the pac file and url
	if (!pac) return NULL;
	if (!url) return NULL;
	if (!px_pac_to_string(pac) && !px_pac_reload(pac)) return NULL;

	// Get the cached context
	ctxStore *ctxs = ((pxMozillaPACRunnerModule *) self)->ctxs;

	// If there is a cached context, make sure it is the same pac
	if (ctxs && strcmp(ctxs->pac, px_pac_to_string(pac)))
	{
		ctxs_free(ctxs);
		ctxs = NULL;
		((pxMozillaPACRunnerModule *) self)->ctxs = NULL;
	}

	// If no context exists (or if the pac was changed), create one
	if (!ctxs)
	{
		if ((ctxs = ctxs_new(pac)))
			((pxMozillaPACRunnerModule *) self)->ctxs = ctxs;
		else
			return NULL;
	}

	// Build arguments to the FindProxyForURL() function
	char *tmpurl  = px_strdup(px_url_to_string(url));
	char *tmphost = px_strdup(px_url_get_host(url));
	jsval args[2] = {
		STRING_TO_JSVAL(JS_NewString(ctxs->ctx, tmpurl, strlen(tmpurl))),
		STRING_TO_JSVAL(JS_NewString(ctxs->ctx, tmphost, strlen(tmphost)))
	};

	// Find the proxy (call FindProxyForURL())
	jsval rval;
	JSBool result = JS_CallFunctionName(ctxs->ctx, JS_GetGlobalObject(ctxs->ctx), "FindProxyForURL", 2, args, &rval);
	if (!result) return NULL;
	char *answer = px_strdup(JS_GetStringBytes(JS_ValueToString(ctxs->ctx, rval)));
	if (!answer || !strcmp(answer, "undefined")) {
		px_free(answer);
		return NULL;
	}
	return answer;
}

static void *
_constructor()
{
	pxMozillaPACRunnerModule *self = px_malloc0(sizeof(pxMozillaPACRunnerModule));
	self->__parent__.run = _run;
	return self;
}

bool
px_module_load(pxModuleManager *self)
{
	return px_module_manager_register_module(self, pxPACRunnerModule, "pacrunner_mozjs", _constructor, _destructor);
}
