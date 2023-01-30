/* config-kde.c
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

#include "config-kde.h"

#include "px-plugin-config.h"
#include "px-manager.h"

static void px_config_iface_init (PxConfigInterface *iface);
G_MODULE_EXPORT void peas_register_types (PeasObjectModule *module);

typedef enum {
  KDE_PROXY_TYPE_NONE = 0,
  KDE_PROXY_TYPE_MANUAL,
  KDE_PROXY_TYPE_PAC,
  KDE_PROXY_TYPE_WPAD,
  KDE_PROXY_TYPE_SYSTEM,
} KdeProxyType;

struct _PxConfigKde {
  GObject parent_instance;

  gboolean available;

  char *no_proxy;
  char *http_proxy;
  char *https_proxy;
  char *ftp_proxy;
  char *socks_proxy;
  KdeProxyType proxy_type;
  char *pac_script;
};

G_DEFINE_FINAL_TYPE_WITH_CODE (PxConfigKde,
                               px_config_kde,
                               G_TYPE_OBJECT,
                               G_IMPLEMENT_INTERFACE (PX_TYPE_CONFIG, px_config_iface_init))

static void
px_config_kde_read_config (PxConfigKde *self,
                           char        *proxy_file)
{
  g_autoptr (GError) error = NULL;
  g_autofree char *line = NULL;
  g_autoptr (GFile) file = NULL;
  g_autoptr (GFileInputStream) istr = NULL;
  g_autoptr (GDataInputStream) dstr = NULL;


  file = g_file_new_for_path (proxy_file);
  if (!file) {
    g_print ("Could not create file\n");
    return;
  }

  istr = g_file_read (file, NULL, NULL);
  if (!istr) {
    g_print ("Could not read file\n");
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
      g_string_replace (value, " ", ":", 0);

      if (strcmp (kv[0], "httpsProxy") == 0) {
        self->https_proxy = g_strdup (value->str);
      } else if (strcmp (kv[0], "httpProxy") == 0) {
        self->http_proxy = g_strdup (value->str);
      } else if (strcmp (kv[0], "ftpProxy") == 0) {
        self->ftp_proxy = g_strdup (value->str);
      } else if (strcmp (kv[0], "socksProxy") == 0) {
        self->socks_proxy = g_strdup (value->str);
      } else if (strcmp (kv[0], "NoProxyFor") == 0) {
        self->no_proxy = g_strdup (value->str);
      } else if (strcmp (kv[0], "Proxy Config Script") == 0) {
        self->pac_script = g_strdup (value->str);
      } else if (strcmp (kv[0], "ProxyType") == 0) {
        self->proxy_type = atoi (value->str);
      }
    }
  } while (line);
}

static void
px_config_kde_init (PxConfigKde *self)
{
  const char *test_file = g_getenv ("PX_CONFIG_KDE");
  g_autofree char *config = test_file ? g_strdup (test_file) : g_build_filename (g_get_user_config_dir (), "kioslaverc", NULL);

  self->available = g_file_test (config, G_FILE_TEST_EXISTS);
  if (self->available)
    px_config_kde_read_config (self, config);
}

static void
px_config_kde_dispose (GObject *object)
{
  G_OBJECT_CLASS (px_config_kde_parent_class)->dispose (object);
}

static void
px_config_kde_class_init (PxConfigKdeClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = px_config_kde_dispose;
}

static gboolean
px_config_kde_is_available (PxConfig *config)
{
  PxConfigKde *self = PX_CONFIG_KDE (config);

  return self->available && g_getenv ("KDE_FULL_SESSION") != NULL;
}

static gboolean
px_config_kde_get_config (PxConfig      *config,
                          GUri          *uri,
                          GStrvBuilder  *builder,
                          GError       **error)
{
  PxConfigKde *self = PX_CONFIG_KDE (config);
  const char *scheme = g_uri_get_scheme (uri);
  g_autofree char *proxy = NULL;

  if (!self->proxy_type)
    return TRUE;

  if (self->no_proxy && strstr (self->no_proxy, g_uri_get_host (uri)))
    return TRUE;

  switch (self->proxy_type) {
    case KDE_PROXY_TYPE_MANUAL:
    case KDE_PROXY_TYPE_SYSTEM:
      if (g_strcmp0 (scheme, "ftp") == 0) {
        proxy = g_strdup (self->ftp_proxy);
      } else if (g_strcmp0 (scheme, "https") == 0) {
        proxy = g_strdup (self->https_proxy);
      } else if (g_strcmp0 (scheme, "http") == 0) {
        proxy = g_strdup (self->http_proxy);
      } else if (g_strcmp0 (scheme, "socks") == 0) {
        proxy = g_strdup (self->socks_proxy);
      }
      break;
    case KDE_PROXY_TYPE_WPAD:
      proxy = g_strdup ("wpad://");
      break;
    case KDE_PROXY_TYPE_PAC:
      proxy = g_strdup_printf ("pac+%s", self->pac_script);
      break;
    case KDE_PROXY_TYPE_NONE:
    default:
      break;
  }

  if (proxy)
    g_strv_builder_add (builder, proxy);

  return TRUE;
}

static void
px_config_iface_init (PxConfigInterface *iface)
{
  iface->is_available = px_config_kde_is_available;
  iface->get_config = px_config_kde_get_config;
}

G_MODULE_EXPORT void
peas_register_types (PeasObjectModule *module)
{
  peas_object_module_register_extension_type (module,
                                              PX_TYPE_CONFIG,
                                              PX_CONFIG_TYPE_KDE);
}
