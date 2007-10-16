#include <stdlib.h>

#include <misc.h>
#include <proxy_factory.h>

#include <gconf/gconf.h>

pxConfig *
gconf_config_cb(pxProxyFactory *self)
{
	return NULL;
}

bool
on_proxy_factory_instantiate(pxProxyFactory *self)
{
	return px_proxy_factory_config_add(self, "gconf", PX_CONFIG_CATEGORY_SESSION, gconf_config_cb);
}

void
on_proxy_factory_destantiate(pxProxyFactory *self)
{
	px_proxy_factory_config_del(self, "gconf");
}
