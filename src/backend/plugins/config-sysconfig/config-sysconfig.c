/* config-sysconfig.c
 *
 * Copyright 2023 The Libproxy Team
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

#include "config-sysconfig.h"

#include "px-manager.h"
#include "px-plugin-config.h"

struct _PxConfigSysConfig {
  GObject parent_instance;

  char *proxy_file;
  char *config_option;
  gboolean available;

  gboolean proxy_enabled;
  char *https_proxy;
  char *http_proxy;
  char *ftp_proxy;
  char *no_proxy;
};

static void px_config_iface_init (PxConfigInterface *iface);
G_MODULE_EXPORT void peas_register_types (PeasObjectModule *module);

G_DEFINE_FINAL_TYPE_WITH_CODE (PxConfigSysConfig,
                               px_config_sysconfig,
                               G_TYPE_OBJECT,
                               G_IMPLEMENT_INTERFACE (PX_TYPE_CONFIG, px_config_iface_init))

enum {
  PROP_0,
  PROP_CONFIG_OPTION
};

static
void
px_config_sysconfig_set_config_file (PxConfigSysConfig *self,
                                     const char        *config_file)
{
  g_autofree char *config = NULL;
  g_autoptr (GFile) file = NULL;
  g_autoptr (GError) error = NULL;
  g_autoptr (GFileInputStream) istr = NULL;
  g_autoptr (GDataInputStream) dstr = NULL;
  char *line = NULL;

  g_clear_pointer (&self->config_option, g_free);
  self->config_option = config_file ? g_strdup (config_file) : NULL;

  self->proxy_file = g_strdup (self->config_option ? self->config_option : "/etc/sysconfig/proxy");
  self->available = FALSE;

  file = g_file_new_for_path (self->proxy_file);
  if (!file) {
    g_debug ("%s: Could not create file", __FUNCTION__);
    return;
  }

  istr = g_file_read (file, NULL, NULL);
  if (!istr) {
    g_debug ("%s: Could not read file", __FUNCTION__);
    return;
  }

  dstr = g_data_input_stream_new (G_INPUT_STREAM (istr));
  if (!dstr)
    return;

  do {
    g_clear_pointer (&line, g_free);

    line = g_data_input_stream_read_line (dstr, NULL, NULL, &error);
    if (line) {
      g_auto (GStrv) kv = NULL;
      g_autoptr (GString) value = NULL;
      kv = g_strsplit (line, "=", -1);

      if (g_strv_length (kv) != 2)
        continue;

      value = g_string_new (kv[1]);
      g_string_replace (value, "\"", "", 0);
      g_string_replace (value, "\r", "", 0);

      if (strcmp (kv[0], "PROXY_ENABLED") == 0) {
        self->proxy_enabled = g_ascii_strncasecmp (value->str, "yes", 3) == 0;
      } else if (strcmp (kv[0], "HTTPS_PROXY") == 0) {
        self->https_proxy = g_strdup (value->str);
      } else if (strcmp (kv[0], "HTTP_PROXY") == 0) {
        self->http_proxy = g_strdup (value->str);
      } else if (strcmp (kv[0], "FTP_PROXY") == 0) {
        self->ftp_proxy = g_strdup (value->str);
      } else if (strcmp (kv[0], "NO_PROXY") == 0) {
        self->no_proxy = g_strdup (value->str);
      }
    }
  } while (line);

  self->available = TRUE;
}

static void
px_config_sysconfig_init (PxConfigSysConfig *self)
{
  px_config_sysconfig_set_config_file (self, NULL);
}

static void
px_config_sysconfig_set_property (GObject      *object,
                                  guint         prop_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  PxConfigSysConfig *config = PX_CONFIG_SYSCONFIG (object);

  switch (prop_id) {
    case PROP_CONFIG_OPTION:
      px_config_sysconfig_set_config_file (config, g_value_dup_string (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
px_config_sysconfig_get_property (GObject    *object,
                                  guint       prop_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
  PxConfigSysConfig *config = PX_CONFIG_SYSCONFIG (object);

  switch (prop_id) {
    case PROP_CONFIG_OPTION:
      g_value_set_string (value, config->config_option);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
px_config_sysconfig_class_init (PxConfigSysConfigClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = px_config_sysconfig_set_property;
  object_class->get_property = px_config_sysconfig_get_property;

  g_object_class_override_property (object_class, PROP_CONFIG_OPTION, "config-option");
}

static gboolean
px_config_sysconfig_is_available (PxConfig *config)
{
  PxConfigSysConfig *self = PX_CONFIG_SYSCONFIG (config);

  return self->available;
}

static void
px_config_sysconfig_get_config (PxConfig     *config,
                                GUri         *uri,
                                GStrvBuilder *builder)
{
  PxConfigSysConfig *self = PX_CONFIG_SYSCONFIG (config);
  const char *scheme = g_uri_get_scheme (uri);
  g_autofree char *proxy = NULL;

  if (!self->proxy_enabled)
    return;

  if (self->no_proxy && strstr (self->no_proxy, g_uri_get_host (uri)))
    return;

  if (g_strcmp0 (scheme, "ftp") == 0) {
    proxy = g_strdup (self->ftp_proxy);
  } else if (g_strcmp0 (scheme, "https") == 0) {
    proxy = g_strdup (self->https_proxy);
  } else if (g_strcmp0 (scheme, "http") == 0) {
    proxy = g_strdup (self->http_proxy);
  }

  if (proxy)
    g_strv_builder_add (builder, proxy);
}

static void
px_config_iface_init (PxConfigInterface *iface)
{
  iface->is_available = px_config_sysconfig_is_available;
  iface->get_config = px_config_sysconfig_get_config;
}

G_MODULE_EXPORT void
peas_register_types (PeasObjectModule *module)
{
  peas_object_module_register_extension_type (module,
                                              PX_TYPE_CONFIG,
                                              PX_CONFIG_TYPE_SYSCONFIG);
}
