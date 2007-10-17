#include <stdlib.h>
#include <string.h>

#include <misc.h>
#include <proxy_factory.h>

#include <gconf/gconf-client.h>

static bool
in_gnome_session()
{
	return true;
}

pxConfig *
gconf_config_cb(pxProxyFactory *self)
{
	// This is bound to be poor performing, ideally we should share a gconf client
	GConfClient *client = gconf_client_get_default();
	if (!client) return NULL;
	
	// Get the mode
	char *mode = gconf_client_get_string(client, "/system/proxy/mode", NULL);
	if (!mode) { g_object_unref(client); return NULL; }
	
	// Create the basic Config
	pxConfig *config = px_malloc0(sizeof(pxConfig));
	config->ignore   = px_strdup(""); // TODO: implement ignores
	
	// Mode is direct://
	if (!strcmp(mode, "none"))
		config->url = px_strdup("direct://");

	// Mode is wpad:// or pac+http://...
	else if (!strcmp(mode, "auto"))
	{
		char *tmp = gconf_client_get_string(client, "/system/proxy/autoconfig_url", NULL);
		if (px_url_is_valid(tmp))
			config->url = g_strdup_printf("pac+%s", tmp); 
		else
			config->url = px_strdup("wpad://");
		px_free(tmp);
	}
	
	// Mode is http://... or socks://...
	else if (!strcmp(mode, "manual"))
	{
		char *type = px_strdup("http");
		char *host = gconf_client_get_string(client, "/system/http_proxy/host", NULL);
		int   port = gconf_client_get_int   (client, "/system/http_proxy/port", NULL);
		if (port < 0 || port > 65535) port = 0;
		
		// If http proxy is not set, try socks
		if (!host || !strcmp(host, "") || !port)
		{
			if (type) px_free(type);
			if (host) px_free(host);
			
			type = px_strdup("socks");
			host = gconf_client_get_string(client, "/system/proxy/socks_host", NULL);
			port = gconf_client_get_int   (client, "/system/proxy/socks_port", NULL);
			if (port < 0 || port > 65535) port = 0;
		}
		
		// If host and port were found, build config url
		if (host && strcmp(host, "") && port)
			config->url = g_strdup_printf("%s://%s:%d", type, host, port);
		
		// Fall back to auto-detect
		else
			config->url = px_strdup("wpad://");
		
		if (type) px_free(type);
		if (host) px_free(host);		
	}
	
	// Fall back to auto-detect
	else
		config->url = px_strdup("wpad://");
	
	g_object_unref(client);
	px_free(mode);
	return config;
}

void
gconf_on_get_proxy(pxProxyFactory *self)
{
	if (in_gnome_session())
		px_proxy_factory_config_add(self, "gconf", PX_CONFIG_CATEGORY_SESSION, gconf_config_cb);
	else
		px_proxy_factory_config_del(self, "gconf");
}

bool
on_proxy_factory_instantiate(pxProxyFactory *self)
{
	g_type_init();
	px_proxy_factory_on_get_proxy_add(self, gconf_on_get_proxy);
	return true;
}

void
on_proxy_factory_destantiate(pxProxyFactory *self)
{
	px_proxy_factory_on_get_proxy_del(self, gconf_on_get_proxy);
	px_proxy_factory_config_del(self, "gconf");
}
