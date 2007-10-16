#include <stdlib.h>
#include <string.h>
#define __USE_BSD
#include <unistd.h>

#include <misc.h>
#include <proxy_factory.h>

#include <jsapi.h>
#include "pacutils.h"

static JSBool dnsResolve(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
	char *host, *tmp;
	
	// Get hostname argument
	host = px_strdup(JS_GetStringBytes(JS_ValueToString(cx, argv[0])));

	// Look it up
	struct hostent *host_info = gethostbyname(host);
	px_free(host);
	
	// If we found it...
	if (host_info && host_info->h_length == 4) {
		// Allocate our host string
		host = JS_malloc(cx, 16);
		if (!host) { *rval = JSVAL_NULL; return true; }
		
		// Generate host string
		tmp = host_info->h_addr_list[0];
		sprintf(host, "%hhu.%hhu.%hhu.%hhu", tmp[0], tmp[1], tmp[2], tmp[3]);
		
		// Return
		JSString *ip = JS_NewString(cx, host, strlen(host));
		*rval = STRING_TO_JSVAL(ip);
		return true;
	}
	
	// Return
	*rval = JSVAL_NULL;
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
