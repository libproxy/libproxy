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

struct _PxConfigEnv {
  GObject parent_instance;

  GStrv no_proxy;
  const char *ftp_proxy;
  const char *http_proxy;
  const char *https_proxy;
};

G_DEFINE_FINAL_TYPE_WITH_CODE (PxConfigEnv,
                               px_config_env,
                               G_TYPE_OBJECT,
                               G_IMPLEMENT_INTERFACE (PX_TYPE_CONFIG, px_config_iface_init))

static void
px_config_env_init (PxConfigEnv *self)
{
  const char *no_proxy;

  /* Collect data in init() to speed up get_config() calls */
  no_proxy = g_getenv ("no_proxy");
  if (!no_proxy)
    no_proxy = g_getenv ("NO_PROXY");

  if (no_proxy)
    self->no_proxy = g_strsplit (no_proxy, ",", -1);

  self->ftp_proxy = g_getenv ("ftp_proxy");
  if (!self->ftp_proxy )
    self->ftp_proxy = g_getenv ("FTP_PROXY");

  self->https_proxy = g_getenv ("https_proxy");
  if (!self->https_proxy)
    self->https_proxy = g_getenv ("HTTPS_PROXY");

  self->http_proxy = g_getenv ("http_proxy");
  if (!self->http_proxy)
    self->http_proxy = g_getenv ("HTTP_PROXY");

}

static void
px_config_env_dispose (GObject *object)
{
  PxConfigEnv *self = PX_CONFIG_ENV (object);

  g_clear_pointer (&self->no_proxy, g_strfreev);

  G_OBJECT_CLASS (px_config_env_parent_class)->dispose (object);
}

static void
px_config_env_class_init (PxConfigEnvClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = px_config_env_dispose;
}

static gboolean
px_config_env_is_available (PxConfig *self)
{
  return TRUE;
}

static gboolean
px_config_env_get_config (PxConfig      *config,
                          GUri          *uri,
                          GStrvBuilder  *builder,
                          GError       **error)
{
  PxConfigEnv *self = PX_CONFIG_ENV (config);
  const char *proxy = NULL;
  const char *scheme = g_uri_get_scheme (uri);

  /* TODO:
   * - Are host names resolved to IPs??
   * - case insensitive check?
   */
  if (self->no_proxy && (g_strv_contains ((const char * const *)self->no_proxy, g_uri_get_host (uri)) || g_strv_contains ((const char * const *)self->no_proxy, "*"))) {
    return TRUE;
  }

  if (g_strcmp0 (scheme, "ftp") == 0)
    proxy = self->ftp_proxy;
  else if (g_strcmp0 (scheme, "https") == 0)
    proxy = self->https_proxy;

  /* TODO: Is this what we want as a fallback? What about ALL_PROXY? */
  if (!proxy)
    proxy = self->http_proxy;

  /* TODO: Where should we add proxy url validation ? */
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
