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
#define __USE_BSD
#include <unistd.h>

#include <misc.h>
#include <proxy_factory.h>

#include <jsapi.h>
#include "pacutils.h"

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

char *mozjs_pacrunner(pxProxyFactory *self, pxPAC *pac, pxURL *url)
{
	JSRuntime *runtime = NULL;
	JSContext *context = NULL;
	JSObject  *global  = NULL;
	jsval rval;
	char *answer = NULL;
	
	// Make sure we have the pac file and url
	if (!pac) return NULL;
	if (!url) return NULL;
	if (!px_pac_to_string(pac) && !px_pac_reload(pac)) return NULL;
		
	// Setup Javascript global class
	JSClass *global_class     = px_malloc0(sizeof(JSClass));
	global_class->name        = "global";
	global_class->flags       = 0;
	global_class->addProperty = JS_PropertyStub;
	global_class->delProperty = JS_PropertyStub;
	global_class->getProperty = JS_PropertyStub;
	global_class->setProperty = JS_PropertyStub;
	global_class->enumerate   = JS_EnumerateStub;
	global_class->resolve     = JS_ResolveStub;
	global_class->convert     = JS_ConvertStub;
	global_class->finalize    = JS_FinalizeStub;
	
	// Initialize Javascript runtime environment
	if (!(runtime = JS_NewRuntime(1024 * 1024))) goto out;
	if (!(context = JS_NewContext(runtime, 1024 * 1024))) goto out;
	if (!(global  = JS_NewObject(context, global_class, NULL, NULL))) goto out;
	JS_InitStandardClasses(context, global);
	
	// Define Javascript functions
	JS_DefineFunction(context, global, "dnsResolve", dnsResolve, 1, 0);
	JS_DefineFunction(context, global, "myIpAddress", myIpAddress, 0, 0);
	JS_EvaluateScript(context, global, JAVASCRIPT_ROUTINES, 
	                  strlen(JAVASCRIPT_ROUTINES), "pacutils.js", 0, &rval);
	                  
	// Add PAC to the environment
	JS_EvaluateScript(context, JS_GetGlobalObject(context),
                      px_pac_to_string(pac), strlen(px_pac_to_string(pac)), 
	                  px_url_to_string((pxURL *) px_pac_get_url(pac)), 0, &rval);

	// Build arguments to the FindProxyForURL() function
	char *tmpurl  = px_strdup(px_url_to_string(url));
	char *tmphost = px_strdup(px_url_get_host(url));
	jsval val[2] = {
		STRING_TO_JSVAL(JS_NewString(context, tmpurl, strlen(tmpurl))),
		STRING_TO_JSVAL(JS_NewString(context, tmphost, strlen(tmphost)))
	};
	
	// Find the proxy (call FindProxyForURL())
	JSBool result = JS_CallFunctionName(context, JS_GetGlobalObject(context), 
	                                    "FindProxyForURL", 2, val, &rval);
	if (!result) goto out;
	answer = px_strdup(JS_GetStringBytes(JS_ValueToString(context, rval)));
	if (!answer || !strcmp(answer, "undefined")) {
		px_free(answer); 
		answer = NULL;
	}
	
	out:
		if (context) JS_DestroyContext(context);
		if (runtime) JS_DestroyRuntime(runtime);
		px_free(global_class);
		return answer;
}

bool on_proxy_factory_instantiate(pxProxyFactory *self)
{
	return px_proxy_factory_pac_runner_set(self, mozjs_pacrunner);
}

void on_proxy_factory_destantiate(pxProxyFactory *self)
{
	px_proxy_factory_pac_runner_set(self, NULL);
}
