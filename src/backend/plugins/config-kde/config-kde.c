/* config-kde.c
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

#include "config-kde.h"

#include "px-plugin-config.h"
#include "px-manager.h"

static void px_config_iface_init (PxConfigInterface *iface);

typedef enum {
  KDE_PROXY_TYPE_NONE = 0,
  KDE_PROXY_TYPE_MANUAL,
  KDE_PROXY_TYPE_PAC,
  KDE_PROXY_TYPE_WPAD,
  KDE_PROXY_TYPE_SYSTEM,
} KdeProxyType;

struct _PxConfigKde {
  GObject parent_instance;

  char *config_file;
  gboolean available;
  GFileMonitor *monitor;

  GStrv no_proxy;
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

enum {
  PROP_0,
  PROP_CONFIG_OPTION
};

static void px_config_kde_set_config_file (PxConfigKde *self,
                                           char        *proxy_file);

static void
on_file_changed (GFileMonitor      *monitor,
                 GFile             *file,
                 GFile             *other_file,
                 GFileMonitorEvent  event_type,
                 gpointer           user_data)
{
  PxConfigKde *self = PX_CONFIG_KDE (user_data);

  g_debug ("%s: Reloading configuration\n", __FUNCTION__);
  px_config_kde_set_config_file (self, g_file_get_path (file));
}

static void
px_config_kde_set_config_file (PxConfigKde *self,
                               char        *proxy_file)
{
  g_autoptr (GError) error = NULL;
  g_autofree char *line = NULL;
  g_autoptr (GFile) file = NULL;
  g_autoptr (GFileInputStream) istr = NULL;
  g_autoptr (GDataInputStream) dstr = NULL;
  const char *desktops;

  self->available = FALSE;

  desktops = getenv ("XDG_CURRENT_DESKTOP");
  if (!desktops)
    return;

  /* Remember that XDG_CURRENT_DESKTOP is a list of strings. */
  if (strstr (desktops, "KDE") == NULL)
    return;

  g_clear_pointer (&self->config_file, g_free);
  self->config_file = proxy_file ? g_strdup (proxy_file) : g_build_filename (g_get_user_config_dir (), "kioslaverc", NULL);

  file = g_file_new_for_path (self->config_file);
  if (!file) {
    g_debug ("%s: Could not create file for %s", __FUNCTION__, self->config_file);
    return;
  }

  istr = g_file_read (file, NULL, NULL);
  if (!istr) {
    g_debug ("%s: Could not read file %s", __FUNCTION__, self->config_file);
    return;
  }

  dstr = g_data_input_stream_new (G_INPUT_STREAM (istr));
  if (!dstr)
    return;

  g_clear_object (&self->monitor);
  self->monitor = g_file_monitor (file, G_FILE_MONITOR_NONE, NULL, &error);
  if (!self->monitor)
    g_warning ("Could not add a file monitor for %s, error: %s", g_file_get_uri (file), error->message);
  else
    g_signal_connect_object (G_OBJECT (self->monitor), "changed", G_CALLBACK (on_file_changed), self, 0);

  do {
    g_clear_pointer (&line, g_free);

    line = g_data_input_stream_read_line (dstr, NULL, NULL, &error);
    if (line) {
      g_auto (GStrv) kv = NULL;
      g_autoptr (GString) value = NULL;
      kv = g_strsplit (line, "=", 2);

      if (g_strv_length (kv) != 2)
        continue;

      value = g_string_new (kv[1]);
      g_string_replace (value, "\"", "", 0);
      g_string_replace (value, " ", ":", 0);
      g_string_replace (value, "\r", "", 0);

      if (strcmp (kv[0], "httpsProxy") == 0) {
        self->https_proxy = g_strdup (value->str);
      } else if (strcmp (kv[0], "httpProxy") == 0) {
        self->http_proxy = g_strdup (value->str);
      } else if (strcmp (kv[0], "ftpProxy") == 0) {
        self->ftp_proxy = g_strdup (value->str);
      } else if (strcmp (kv[0], "socksProxy") == 0) {
        self->socks_proxy = g_strdup (value->str);
      } else if (strcmp (kv[0], "NoProxyFor") == 0) {
        self->no_proxy = g_strsplit (value->str, ",", -1);
      } else if (strcmp (kv[0], "Proxy Config Script") == 0) {
        self->pac_script = g_strdup (value->str);
      } else if (strcmp (kv[0], "ProxyType") == 0) {
        self->proxy_type = atoi (value->str);
      }
    }
  } while (line);

  self->available = TRUE;
}


static void
px_config_kde_init (PxConfigKde *self)
{
  px_config_kde_set_config_file (self, NULL);
}

static void
px_config_kde_dispose (GObject *object)
{
  PxConfigKde *self = PX_CONFIG_KDE (object);

  g_clear_pointer (&self->config_file, g_free);
  g_clear_object (&self->monitor);
  g_clear_pointer (&self->no_proxy, g_strfreev);
  g_clear_pointer (&self->http_proxy, g_free);
  g_clear_pointer (&self->https_proxy, g_free);
  g_clear_pointer (&self->ftp_proxy, g_free);
  g_clear_pointer (&self->socks_proxy, g_free);
  g_clear_pointer (&self->pac_script, g_free);

  G_OBJECT_CLASS (px_config_kde_parent_class)->dispose (object);
}

static void
px_config_kde_set_property (GObject      *object,
                            guint         prop_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
  PxConfigKde *config = PX_CONFIG_KDE (object);

  switch (prop_id) {
    case PROP_CONFIG_OPTION:
      px_config_kde_set_config_file (config, g_value_dup_string (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
px_config_kde_get_property (GObject    *object,
                            guint       prop_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
  PxConfigKde *config = PX_CONFIG_KDE (object);

  switch (prop_id) {
    case PROP_CONFIG_OPTION:
      g_value_set_string (value, config->config_file);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
px_config_kde_class_init (PxConfigKdeClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = px_config_kde_dispose;
  object_class->set_property = px_config_kde_set_property;
  object_class->get_property = px_config_kde_get_property;

  g_object_class_override_property (object_class, PROP_CONFIG_OPTION, "config-option");
}

static gboolean
px_config_kde_is_available (PxConfig *config)
{
  PxConfigKde *self = PX_CONFIG_KDE (config);

  return self->available;
}

static void
px_config_kde_get_config (PxConfig     *config,
                          GUri         *uri,
                          GStrvBuilder *builder)
{
  PxConfigKde *self = PX_CONFIG_KDE (config);
  const char *scheme;
  g_autofree char *proxy = NULL;

  if (!self->available)
    return;

  if (self->proxy_type == KDE_PROXY_TYPE_NONE)
    return;

  if (px_manager_is_ignore (uri, self->no_proxy))
    return;

  scheme = g_uri_get_scheme (uri);

  switch (self->proxy_type) {
    case KDE_PROXY_TYPE_MANUAL:
    case KDE_PROXY_TYPE_SYSTEM:
      /* System is the same as manual, except that a button for auto dection
       * is shown. Based on this manual fields are set.
       */
      if (g_strcmp0 (scheme, "ftp") == 0) {
        proxy = g_strdup (self->ftp_proxy);
      } else if (g_strcmp0 (scheme, "https") == 0) {
        proxy = g_strdup (self->https_proxy);
      } else if (g_strcmp0 (scheme, "http") == 0) {
        proxy = g_strdup (self->http_proxy);
      } else if (self->socks_proxy && strlen (self->socks_proxy) > 0) {
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
    px_strv_builder_add_proxy (builder, proxy);
}

static void
px_config_iface_init (PxConfigInterface *iface)
{
  iface->name = "config-kde";
  iface->priority = PX_CONFIG_PRIORITY_DEFAULT;
  iface->is_available = px_config_kde_is_available;
  iface->get_config = px_config_kde_get_config;
}
