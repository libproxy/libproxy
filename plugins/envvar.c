#include <stdlib.h>
#include <string.h>

#include "proxy_plugin.h"

PXConfig *get_config_cb(PXProxyFactory *self)
{
	char *proxy = getenv("http_proxy");
	if (!proxy)  return NULL;
	PXConfig *config = malloc(sizeof(PXConfig));
	if (!config) return NULL;
	config->url = strdup(proxy);
	if (!config->url) { free(config); return NULL; }
	config->ignore = strdup(""); // TODO: Make this pay attention to 'no_proxy' later
	if (!config->ignore) { free(config->url); free(config); return NULL; }
	return config;
}

bool on_proxyfactory_instantiate(PXProxyFactory *self)
{
	return px_proxy_factory_config_set(self, PX_CONFIG_BACKEND_ENVVAR, get_config_cb);
}

void on_proxyfactory_destantiate(PXProxyFactory *self)
{
	px_proxy_factory_config_set(self, PX_CONFIG_BACKEND_ENVVAR, NULL);
}
