#include <stdbool.h>

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

typedef void      (*PXProxyFactoryVoidCallback)     (PXProxyFactory *self);
typedef bool      (*PXProxyFactoryBoolCallback)     (PXProxyFactory *self);
typedef void     *(*PXProxyFactoryPtrCallback)      (PXProxyFactory *self);

typedef char     *(*PXPACRunnerCallback)            (PXProxyFactory *self, const char *pac, const char *url, const char *hostname);

bool              px_proxy_factory_config_set       (PXProxyFactory *self, PXConfigBackend config, PXProxyFactoryPtrCallback callback);
PXConfigBackend   px_proxy_factory_config_get_active(PXProxyFactory *self);
void              px_proxy_factory_wpad_restart     (PXProxyFactory *self);
void              px_proxy_factory_on_get_proxy_add (PXProxyFactory *self, PXProxyFactoryVoidCallback callback);
void              px_proxy_factory_on_get_proxy_del (PXProxyFactory *self, PXProxyFactoryVoidCallback callback);
bool              px_proxy_factory_pac_runner_set   (PXProxyFactory *self, PXPACRunnerCallback callback);
