/* config-gnome.c
 *
 * Copyright 2022-2023 The Libproxy Team
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

#include <gio/gio.h>

#include "config-gnome.h"

#include "px-plugin-config.h"
#include "px-manager.h"

struct _PxConfigGnome {
  GObject parent_instance;
  GSettings *proxy_settings;
  GSettings *http_proxy_settings;
  GSettings *https_proxy_settings;
  GSettings *ftp_proxy_settings;
  GSettings *socks_proxy_settings;
  gboolean available;
};

typedef enum {
  GNOME_PROXY_MODE_NONE,
  GNOME_PROXY_MODE_MANUAL,
  GNOME_PROXY_MODE_AUTO
} GnomeProxyMode;

static void px_config_iface_init (PxConfigInterface *iface);

G_DEFINE_FINAL_TYPE_WITH_CODE (PxConfigGnome,
                               px_config_gnome,
                               G_TYPE_OBJECT,
                               G_IMPLEMENT_INTERFACE (PX_TYPE_CONFIG, px_config_iface_init))

enum {
  PROP_0,
  PROP_CONFIG_OPTION
};

static void
px_config_gnome_init (PxConfigGnome *self)
{
  GSettingsSchemaSource *source;
  GSettingsSchema *proxy_schema;
  const char *desktops;

  self->available = FALSE;

  desktops = getenv ("XDG_CURRENT_DESKTOP");
  if (!desktops)
    return;

  /* Remember that XDG_CURRENT_DESKTOP is a list of strings. */
  if (strstr (desktops, "GNOME") == NULL)
    return;

  source = g_settings_schema_source_get_default ();
  if (!source) {
    g_warning ("GNOME desktop detected but no schemes installed, aborting.");
    return;
  }

  proxy_schema = g_settings_schema_source_lookup (source, "org.gnome.system.proxy", TRUE);

  self->available = proxy_schema != NULL;
  g_clear_pointer (&proxy_schema, g_settings_schema_unref);

  if (!self->available)
    return;

  self->proxy_settings = g_settings_new ("org.gnome.system.proxy");
  self->http_proxy_settings = g_settings_new ("org.gnome.system.proxy.http");
  self->https_proxy_settings = g_settings_new ("org.gnome.system.proxy.https");
  self->ftp_proxy_settings = g_settings_new ("org.gnome.system.proxy.ftp");
  self->socks_proxy_settings = g_settings_new ("org.gnome.system.proxy.socks");
}

static void
px_config_gnome_set_property (GObject      *object,
                              guint         prop_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  switch (prop_id) {
    case PROP_CONFIG_OPTION:
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
px_config_gnome_get_property (GObject    *object,
                              guint       prop_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  switch (prop_id) {
    case PROP_CONFIG_OPTION:
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
px_config_gnome_class_init (PxConfigGnomeClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = px_config_gnome_set_property;
  object_class->get_property = px_config_gnome_get_property;

  g_object_class_override_property (object_class, PROP_CONFIG_OPTION, "config-option");
}

static gboolean
px_config_gnome_is_available (PxConfig *config)
{
  PxConfigGnome *self = PX_CONFIG_GNOME (config);

  return self->available;
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
  if (type && host && strlen (type) > 0 && strlen (host) > 0 && port != 0) {
    g_autoptr (GString) tmp = g_string_new (type);

    g_string_append (tmp, "://");
    if (auth)
      g_string_append_printf (tmp, "%s:%s@", username, password);

    g_string_append_printf (tmp, "%s:%d", host, port);

    px_strv_builder_add_proxy (builder, tmp->str);
  }
}

static void
px_config_gnome_get_config (PxConfig     *config,
                            GUri         *uri,
                            GStrvBuilder *builder)
{
  PxConfigGnome *self = PX_CONFIG_GNOME (config);
  g_autofree char *proxy = NULL;
  GnomeProxyMode mode;

  if (!self->available)
    return;

  mode = g_settings_get_enum (self->proxy_settings, "mode");
  if (mode == GNOME_PROXY_MODE_NONE)
    return;

  if (px_manager_is_ignore (uri, g_settings_get_strv (self->proxy_settings, "ignore-hosts")))
    return;

  if (mode == GNOME_PROXY_MODE_AUTO) {
    char *autoconfig_url = g_settings_get_string (self->proxy_settings, "autoconfig-url");

    if (strlen (autoconfig_url) != 0)
      proxy = g_strdup_printf ("pac+%s", autoconfig_url);
    else
      proxy = g_strdup ("wpad://");

    px_strv_builder_add_proxy (builder, proxy);
  } else if (mode == GNOME_PROXY_MODE_MANUAL) {
    g_autofree char *username = g_settings_get_string (self->http_proxy_settings, "authentication-user");
    g_autofree char *password = g_settings_get_string (self->http_proxy_settings, "authentication-password");
    const char *scheme = g_uri_get_scheme (uri);
    gboolean auth = g_settings_get_boolean (self->http_proxy_settings, "use-authentication");

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
                      g_settings_get_int (self->socks_proxy_settings, "port"),
                      auth,
                      username,
                      password);
    }
  }
}

static void
px_config_iface_init (PxConfigInterface *iface)
{
  iface->name = "config-gnome";
  iface->priority = PX_CONFIG_PRIORITY_DEFAULT;
  iface->is_available = px_config_gnome_is_available;
  iface->get_config = px_config_gnome_get_config;
}
