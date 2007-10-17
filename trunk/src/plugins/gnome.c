#include <stdlib.h>
#include <string.h>

#include <misc.h>
#include <proxy_factory.h>

#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <gconf/gconf-client.h>

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
	int argc = 0;
	GdkScreen *screen = NULL;
	const char *wm = NULL;
	
	// Connect to X
	if (!gdk_init_check(&argc, NULL))  goto not_gnome_session;
	
	// Get the default screen
	screen = gdk_screen_get_default();
	if (!screen)                       goto not_gnome_session;
	
	// Get the window manager of that screen
	wm = gdk_x11_screen_get_window_manager_name(screen);
	if (!wm || strcmp(wm, "Metacity")) goto not_gnome_session;
	
	// Close the connection to X, so we don't crash if disconnected
	gdk_display_close(gdk_screen_get_display(screen));
	
	// Make sure this config is registered
	px_proxy_factory_config_add(self, "gnome", PX_CONFIG_CATEGORY_SESSION, gconf_config_cb);
	return;
	
	// We aren't in a gnome session, so make sure this is config is NOT registered
	not_gnome_session:
		if (screen)
			gdk_display_close(gdk_screen_get_display(screen));
		px_proxy_factory_config_del(self, "gnome");
}

bool
on_proxy_factory_instantiate(pxProxyFactory *self)
{
	// Note that we instantiate like this because SESSION config plugins
	// are only suppossed to remain registered while the application
	// is actually IN that session.  So for instance, if the app
	// was run in GNU screen and is taken out of the GNOME sesion
	// it was started in, we should handle that gracefully.
	g_type_init();
	px_proxy_factory_on_get_proxy_add(self, gconf_on_get_proxy);
	return true;
}

void
on_proxy_factory_destantiate(pxProxyFactory *self)
{
	px_proxy_factory_on_get_proxy_del(self, gconf_on_get_proxy);
	px_proxy_factory_config_del(self, "gnome");
}
