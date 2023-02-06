/* config-gnome.c
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
#include <glib.h>

#include "config-gnome.h"
#include "px-plugin-config.h"

struct _PxConfigGnome {
  GObject parent_instance;
  GSettings *proxy_settings;
  GSettings *http_proxy_settings;
  GSettings *https_proxy_settings;
  GSettings *ftp_proxy_settings;
  GSettings *socks_proxy_settings;
  gboolean settings_found;
};

enum {
  GNOME_PROXY_MODE_NONE,
  GNOME_PROXY_MODE_MANUAL,
  GNOME_PROXY_MODE_AUTO
};

static void px_config_iface_init (PxConfigInterface *iface);
void peas_register_types (PeasObjectModule *module);

G_DEFINE_FINAL_TYPE_WITH_CODE (PxConfigGnome,
                               px_config_gnome,
                               G_TYPE_OBJECT,
                               G_IMPLEMENT_INTERFACE (PX_TYPE_CONFIG, px_config_iface_init))

static void
px_config_gnome_init (PxConfigGnome *self)
{
  GSettingsSchemaSource *source = g_settings_schema_source_get_default ();
  GSettingsSchema *proxy_schema;

  if (!source)
    return;

  proxy_schema = g_settings_schema_source_lookup (source, "org.gnome.system.proxy", TRUE);

  self->settings_found = proxy_schema != NULL;
  g_clear_pointer (&proxy_schema, g_settings_schema_unref);

  if (!self->settings_found)
    return;

  self->proxy_settings = g_settings_new ("org.gnome.system.proxy");
  self->http_proxy_settings = g_settings_new ("org.gnome.system.proxy.http");
  self->https_proxy_settings = g_settings_new ("org.gnome.system.proxy.https");
  self->ftp_proxy_settings = g_settings_new ("org.gnome.system.proxy.ftp");
  self->socks_proxy_settings = g_settings_new ("org.gnome.system.proxy.socks");
}

static void
px_config_gnome_class_init (PxConfigGnomeClass *klass)
{
}

static gboolean
px_config_gnome_is_available (PxConfig *config)
{
  PxConfigGnome *self = PX_CONFIG_GNOME (config);
  const char *desktops;

  if (!self->settings_found)
    return FALSE;

  desktops = getenv ("XDG_CURRENT_DESKTOP");
  if (!desktops)
    return FALSE;

  /* Remember that XDG_CURRENT_DESKTOP is a list of strings. */
  return strstr (desktops, "GNOME") != NULL;
}

static void
store_response (GStrvBuilder *builder,
                const char   *type,
                char         *host,
                int           port,
                gboolean      auth,
                char         *username,
                char         *password)
{
  if (host && port != 0) {
    GString *tmp = g_string_new (type);

    g_string_append (tmp, "://");
    if (auth)
      g_string_append_printf (tmp, "%s:%s@", username, password);

    g_string_append_printf (tmp, "%s:%d", host, port);

    g_strv_builder_add (builder, g_string_free (tmp, FALSE));
  }
}

static void
px_config_gnome_get_config (PxConfig     *config,
                            GUri         *uri,
                            GStrvBuilder *builder)
{
  PxConfigGnome *self = PX_CONFIG_GNOME (config);
  g_autofree char *proxy = NULL;
  int mode;

  mode = g_settings_get_enum (self->proxy_settings, "mode");
  if (mode == GNOME_PROXY_MODE_AUTO) {
    char *autoconfig_url = g_settings_get_string (self->proxy_settings, "autoconfig-url");

    if (strlen (autoconfig_url) != 0)
      proxy = g_strdup_printf ("pac+%s", autoconfig_url);
    else
      proxy = g_strdup ("wpad://");

    g_strv_builder_add (builder, proxy);
  } else if (mode == GNOME_PROXY_MODE_MANUAL) {
    gboolean auth = g_settings_get_boolean (self->http_proxy_settings, "use-authentication");
    g_autofree char *username = g_settings_get_string (self->http_proxy_settings, "authentication-user");
    g_autofree char *password = g_settings_get_string (self->http_proxy_settings, "authentication-password");
    const char *scheme = g_uri_get_scheme (uri);

    if (g_strcmp0 (scheme, "http") == 0) {
      g_autofree char *host = g_settings_get_string (self->http_proxy_settings, "host");
      store_response (builder,
                      "http",
                      host,
                      g_settings_get_int (self->http_proxy_settings, "port"),
                      auth,
                      username,
                      password);
    } else if (g_strcmp0 (scheme, "https") == 0) {
      g_autofree char *host = g_settings_get_string (self->https_proxy_settings, "host");
      store_response (builder,
                      "http",
                      host,
                      g_settings_get_int (self->https_proxy_settings, "port"),
                      auth,
                      username,
                      password);
    } else if (g_strcmp0 (scheme, "ftp") == 0) {
      g_autofree char *host = g_settings_get_string (self->ftp_proxy_settings, "host");
      store_response (builder,
                      "http",
                      host,
                      g_settings_get_int (self->ftp_proxy_settings, "port"),
                      auth,
                      username,
                      password);
    } else {
      g_autofree char *host = g_settings_get_string (self->socks_proxy_settings, "host");
      store_response (builder,
                      "socks",
                      host,
                      g_settings_get_int (self->ftp_proxy_settings, "port"),
                      auth,
                      username,
                      password);
    }
  }
}

static void
px_config_iface_init (PxConfigInterface *iface)
{
  iface->is_available = px_config_gnome_is_available;
  iface->get_config = px_config_gnome_get_config;
}

void
peas_register_types (PeasObjectModule *module)
{
  peas_object_module_register_extension_type (module,
                                              PX_TYPE_CONFIG,
                                              PX_CONFIG_TYPE_GNOME);
}
