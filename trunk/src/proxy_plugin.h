#include "proxy.h"

// URLs look like this:
//   http://host:port
//   socks://host:port
//   pac+http://pac_host:port/path/to/pac
//   wpad://
//   direct://
// TODO: ignore syntax TBD
struct _PXConfig {
	char *url;
	char *ignore;
};
typedef struct _PXConfig PXConfig;

typedef void     *(*PXProxyFactoryCallback)         (PXProxyFactory *self);
typedef char     *(*PXPACRunnerCallback)            (PXProxyFactory *self, const char *pac, const char *url, const char *hostname);

void              px_proxy_factory_config_set       (PXProxyFactory *self, PXConfigBackend config, PXProxyFactoryCallback callback);
void              px_proxy_factory_config_changed   (PXProxyFactory *self);
PXConfigBackend   px_proxy_factory_config_get_active(PXProxyFactory *self);

void              px_proxy_factory_network_changed  (PXProxyFactory *self);
void              px_proxy_factory_on_get_proxy_add (PXProxyFactory *self, PXProxyFactoryCallback callback);
void              px_proxy_factory_on_get_proxy_del (PXProxyFactory *self, PXProxyFactoryCallback callback);
void              px_proxy_factory_pac_runner_add   (PXProxyFactory *self, PXPACRunnerCallback callback);
void              px_proxy_factory_pac_runner_del   (PXProxyFactory *self, PXPACRunnerCallback callback);
