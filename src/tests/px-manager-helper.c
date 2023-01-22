#include "px-manager.h"
#include "px-manager-helper.h"

PxManager *
px_test_manager_new (const char *config_plugin)
{
  g_autofree char *path = g_strdup_printf ("%s/../backend/plugins", g_getenv ("G_TEST_BUILDDIR"));

  return g_object_new (PX_TYPE_MANAGER,
                       "plugins-dir", path,
                       "config-plugin", config_plugin,
                       NULL);
}
