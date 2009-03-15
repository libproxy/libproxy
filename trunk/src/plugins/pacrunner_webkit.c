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
#include <plugin_manager.h>
#include <plugin_pacrunner.h>

#include <JavaScriptCore/JavaScript.h>
#include "pacutils.h"

typedef struct {
	JSGlobalContextRef ctx;
	char              *pac;
} ctxStore;

typedef struct _pxWebKitPACRunnerPlugin {
	PX_PLUGIN_SUBCLASS(pxPACRunnerPlugin);
	void    (*old_destructor)(pxPlugin *);
	ctxStore *ctxs;
} pxWebKitPACRunnerPlugin;

static char *jstr2str(JSStringRef str, bool release)
{
	char *tmp = px_malloc0(JSStringGetMaximumUTF8CStringSize(str)+1);
	JSStringGetUTF8CString(str, tmp, JSStringGetMaximumUTF8CStringSize(str)+1);
	if (release) JSStringRelease(str);
	return tmp;
}

static JSValueRef dnsResolve(JSContextRef ctx, JSObjectRef func, JSObjectRef self, size_t argc, const JSValueRef argv[], JSValueRef* exception)
{
	if (argc != 1)                         return NULL;
	if (!JSValueIsString(ctx, argv[0]))    return NULL;

	// Get hostname argument
	char *tmp = jstr2str(JSValueToStringCopy(ctx, argv[0], NULL), true);

	// Look it up
	struct addrinfo *info;
	if (getaddrinfo(tmp, NULL, NULL, &info))
		return NULL;
	px_free(tmp);

	// Try for IPv4
	tmp = px_malloc0(INET6_ADDRSTRLEN+1);
	if (!inet_ntop(info->ai_family,
					&((struct sockaddr_in *) info->ai_addr)->sin_addr,
					tmp, INET_ADDRSTRLEN+1) > 0)
		// Try for IPv6
		if (!inet_ntop(info->ai_family,
						&((struct sockaddr_in6 *) info->ai_addr)->sin6_addr,
						tmp, INET6_ADDRSTRLEN+1) > 0)
		{
			freeaddrinfo(info);
			px_free(tmp);
			return NULL;
		}
	freeaddrinfo(info);

	// Create the return value
	JSStringRef str = JSStringCreateWithUTF8CString(tmp);
	JSValueRef  ret = JSValueMakeString(ctx, str);
	JSStringRelease(str);
	px_free(tmp);

	return ret;
}

static JSValueRef myIpAddress(JSContextRef ctx, JSObjectRef func, JSObjectRef self, size_t argc, const JSValueRef argv[], JSValueRef* exception)
{
	char hostname[1024];
	if (!gethostname(hostname, 1023)) {
		JSStringRef str = JSStringCreateWithUTF8CString(hostname);
		JSValueRef  val = JSValueMakeString(ctx, str);
		JSStringRelease(str);
		JSValueRef ip = dnsResolve(ctx, func, self, 1, &val, NULL);
		return ip;
	}
	return NULL;
}

static void ctxs_free(ctxStore *self)
{
	if (!self) return;
	JSGlobalContextRelease(self->ctx);
	JSGarbageCollect(self->ctx);
	px_free(self->pac);
	px_free(self);
}

static ctxStore *ctxs_new(pxPAC *pac)
{
	JSStringRef str  = NULL;
	JSObjectRef func = NULL;

	// Create the basic context
	ctxStore *self = px_malloc0(sizeof(ctxStore));
	self->pac = px_strdup(px_pac_to_string(pac));
	if (!(self->ctx = JSGlobalContextCreate(NULL))) goto error;

	// Add dnsResolve into the context
	str = JSStringCreateWithUTF8CString("dnsResolve");
	func = JSObjectMakeFunctionWithCallback(self->ctx, str, dnsResolve);
	JSObjectSetProperty(self->ctx, JSContextGetGlobalObject(self->ctx), str, func, kJSPropertyAttributeNone, NULL);
	JSStringRelease(str);

	// Add myIpAddress into the context
	str = JSStringCreateWithUTF8CString("myIpAddress");
	func = JSObjectMakeFunctionWithCallback(self->ctx, str, myIpAddress);
	JSObjectSetProperty(self->ctx, JSContextGetGlobalObject(self->ctx), str, func, kJSPropertyAttributeNone, NULL);
	JSStringRelease(str);

	// Add all other routines into the context
	str = JSStringCreateWithUTF8CString(JAVASCRIPT_ROUTINES);
	if (!JSCheckScriptSyntax(self->ctx, str, NULL, 0, NULL)) goto error;
	JSEvaluateScript(self->ctx, str, NULL, NULL, 1, NULL);
	JSStringRelease(str);

	// Add the PAC into the context
	str = JSStringCreateWithUTF8CString(self->pac);
	if (!JSCheckScriptSyntax(self->ctx, str, NULL, 0, NULL)) goto error;
	JSEvaluateScript(self->ctx, str, NULL, NULL, 1, NULL);
	JSStringRelease(str);

	return self;

error:
	if (str) JSStringRelease(str);
	ctxs_free(self);
	return NULL;
}

static void
_destructor(pxPlugin *self)
{
	ctxs_free(((pxWebKitPACRunnerPlugin *) self)->ctxs);
	((pxWebKitPACRunnerPlugin *) self)->old_destructor(self);
}

static char *
_run(pxPACRunnerPlugin *self, pxPAC *pac, pxURL *url)
{
	JSStringRef str = NULL;
	JSValueRef  val = NULL;
	char       *tmp = NULL;

	// Make sure we have the pac file and url
	if (!pac) return NULL;
	if (!url) return NULL;
	if (!px_pac_to_string(pac) && !px_pac_reload(pac)) return NULL;

	// Get the cached context
	ctxStore *ctxs = ((pxWebKitPACRunnerPlugin *) self)->ctxs;

	// If there is a cached context, make sure it is the same pac
	if (ctxs && strcmp(ctxs->pac, px_pac_to_string(pac)))
	{
		ctxs_free(ctxs);
		ctxs = NULL;
	}

	// If no context exists (or if the pac was changed), create one
	if (!ctxs)
	{
		if ((ctxs = ctxs_new(pac)))
			((pxWebKitPACRunnerPlugin *) self)->ctxs = ctxs;
		else
			return NULL;
	}

	// Run the PAC
	tmp = px_strcat("FindProxyForURL(\"", px_url_to_string(url), "\", \"", px_url_get_host(url), "\");", NULL);
	str = JSStringCreateWithUTF8CString(tmp);
	px_free(tmp); tmp = NULL;
	if (!JSCheckScriptSyntax(ctxs->ctx, str, NULL, 0, NULL))            goto error;
	if (!(val = JSEvaluateScript(ctxs->ctx, str, NULL, NULL, 1, NULL))) goto error;
	if (!JSValueIsString(ctxs->ctx, val))                               goto error;
	JSStringRelease(str);

	// Convert the return value to a string
	return jstr2str(JSValueToStringCopy(ctxs->ctx, val, NULL), true);

error:
	if (str) JSStringRelease(str);
	ctxs_free(ctxs);
	return tmp;
}

static bool
_constructor(pxPlugin *self)
{
	((pxWebKitPACRunnerPlugin *) self)->old_destructor = self->destructor;
	self->destructor                                    = _destructor;
	return true;
}

bool
px_module_load(pxPluginManager *self)
{
	return px_plugin_manager_constructor_add_subtype(self, "pacrunner_webkit", pxPACRunnerPlugin, pxWebKitPACRunnerPlugin, _constructor);
}
