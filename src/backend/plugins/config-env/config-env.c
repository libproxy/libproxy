/* config-env.c
 *
 * Copyright 2022-2023 Jan-Michael Brummer
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include <libpeas/peas.h>

#include "config-env.h"

#include "px-plugin-config.h"
#include "px-manager.h"

static void px_config_iface_init (PxConfigInterface *iface);
G_MODULE_EXPORT void peas_register_types (PeasObjectModule *module);

G_DEFINE_FINAL_TYPE_WITH_CODE (PxConfigEnv,
                               px_config_env,
                               G_TYPE_OBJECT,
                               G_IMPLEMENT_INTERFACE (PX_TYPE_CONFIG, px_config_iface_init))

static void
px_config_env_init (PxConfigEnv *self)
{
}

static void
px_config_env_class_init (PxConfigEnvClass *klass)
{
}

static gboolean
px_config_env_is_available (PxConfig *self)
{
  return TRUE;
}

static gboolean
px_config_env_get_config (PxConfig      *self,
                          GUri          *uri,
                          GStrvBuilder  *builder,
                          GError       **error)
{
  const char *proxy = NULL;
  const char *scheme = g_uri_get_scheme (uri);
  const char *ignore = NULL;

  ignore = g_getenv ("no_proxy");
  if (!ignore)
    ignore = g_getenv ("NO_PROXY");

  if (ignore && strstr (ignore, g_uri_get_host (uri)))
    return TRUE;

  if (g_strcmp0 (scheme, "ftp") == 0) {
    proxy = g_getenv ("ftp_proxy");
    if (!proxy)
      proxy = g_getenv ("FTP_PROXY");
  } else if (g_strcmp0 (scheme, "https") == 0) {
    proxy = g_getenv ("https_proxy");
    if (!proxy)
      proxy = g_getenv ("HTTPS_PROXY");
  }

  if (!proxy) {
    proxy = g_getenv ("http_proxy");
    if (!proxy)
      proxy = g_getenv ("HTTP_PROXY");
  }

  if (proxy)
    g_strv_builder_add (builder, proxy);

  return TRUE;
}

static void
px_config_iface_init (PxConfigInterface *iface)
{
  iface->is_available = px_config_env_is_available;
  iface->get_config = px_config_env_get_config;
}

G_MODULE_EXPORT void
peas_register_types (PeasObjectModule *module)
{
  peas_object_module_register_extension_type (module,
                                              PX_TYPE_CONFIG,
                                              PX_CONFIG_TYPE_ENV);
}
