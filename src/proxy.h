enum _PXConfigBackend {
	PX_CONFIG_BACKEND_AUTO    = 0,
	PX_CONFIG_BACKEND_NONE    = 0,
	PX_CONFIG_BACKEND_WIN32   = (1 << 0),
	PX_CONFIG_BACKEND_KCONFIG = (1 << 1),
	PX_CONFIG_BACKEND_GCONF   = (1 << 2),
	PX_CONFIG_BACKEND_USER    = (1 << 3),
	PX_CONFIG_BACKEND_SYSTEM  = (1 << 4),
	PX_CONFIG_BACKEND_ENVVAR  = (1 << 5),
	PX_CONFIG_BACKEND__LAST   = PX_CONFIG_BACKEND_ENVVAR
};

typedef struct _PXProxyFactory  PXProxyFactory;
typedef enum   _PXConfigBackend PXConfigBackend;

PXProxyFactory *px_proxy_factory_new      (PXConfigBackend config);
char           *px_proxy_factory_get_proxy(PXProxyFactory *self, char *url);
void            px_proxy_factory_free     (PXProxyFactory *self);
