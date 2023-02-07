/* px-manager.c
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

#include "config.h"

#include "px-manager.h"
#include "px-plugin-config.h"
#include "px-plugin-pacrunner.h"
#include "px-plugin-download.h"

#include <libpeas/peas.h>

enum {
  PROP_0,
  PROP_PLUGINS_DIR,
  PROP_CONFIG_PLUGIN,
  LAST_PROP
};

static GParamSpec *obj_properties[LAST_PROP];

/**
 * PxManager:
 *
 * Manage libproxy modules
 */

/* TODO: Move to private structure */
struct _PxManager {
  GObject parent_instance;
  PeasEngine *engine;
  PeasExtensionSet *config_set;
  PeasExtensionSet *pacrunner_set;
  PeasExtensionSet *download_set;
  GNetworkMonitor *network_monitor;
  char *plugins_dir;
  GCancellable *cancellable;

  char *config_plugin;

  gboolean online;
  gboolean wpad;
  GBytes *pac_data;
  char *pac_url;
};

G_DEFINE_TYPE (PxManager, px_manager, G_TYPE_OBJECT)

G_DEFINE_QUARK (px - manager - error - quark, px_manager_error)

static void
px_manager_on_network_changed (GNetworkMonitor *monitor,
                               gboolean         network_available,
                               gpointer         user_data)
{
  PxManager *self = PX_MANAGER (user_data);

  g_debug ("%s: Network connection changed, clearing pac data", __FUNCTION__);

  self->wpad = FALSE;
  self->online = network_available;
  g_clear_pointer (&self->pac_url, g_free);
  g_clear_pointer (&self->pac_data, g_bytes_unref);
}

static void
px_manager_constructed (GObject *object)
{
  PxManager *self = PX_MANAGER (object);
  const GList *list;

  if (g_getenv ("PX_DEBUG")) {
    const gchar *g_messages_debug;

    g_messages_debug = g_getenv ("G_MESSAGES_DEBUG");

    if (!g_messages_debug) {
      g_setenv ("G_MESSAGES_DEBUG", G_LOG_DOMAIN, TRUE);
    } else {
      g_autofree char *new_g_messages_debug = NULL;

      new_g_messages_debug = g_strconcat (g_messages_debug, " ", G_LOG_DOMAIN, NULL);
      g_setenv ("G_MESSAGES_DEBUG", new_g_messages_debug, TRUE);
    }
  }

  self->engine = peas_engine_get_default ();

  peas_engine_add_search_path (self->engine, self->plugins_dir, NULL);

  self->config_set = peas_extension_set_new (self->engine, PX_TYPE_CONFIG, NULL);
  self->pacrunner_set = peas_extension_set_new (self->engine, PX_TYPE_PACRUNNER, NULL);
  self->download_set = peas_extension_set_new (self->engine, PX_TYPE_DOWNLOAD, NULL);

  list = peas_engine_get_plugin_list (self->engine);
  for (; list && list->data; list = list->next) {
    PeasPluginInfo *info = PEAS_PLUGIN_INFO (list->data);
    PeasExtension *extension = peas_extension_set_get_extension (self->config_set, info);
    gboolean available = TRUE;

    /* In case user requested a specific module, just load that one */
    if (self->config_plugin && g_str_has_prefix (peas_plugin_info_get_module_name (info), "config-")) {
      if (g_strcmp0 (peas_plugin_info_get_module_name (info), self->config_plugin) == 0)
        peas_engine_load_plugin (self->engine, info);
    } else {
      peas_engine_load_plugin (self->engine, info);
    }

    extension = peas_extension_set_get_extension (self->config_set, info);
    if (extension) {
      PxConfigInterface *ifc = PX_CONFIG_GET_IFACE (extension);

      available = ifc->is_available (PX_CONFIG (extension));
    }

    if (!available)
      peas_engine_unload_plugin (self->engine, info);
  }

  self->pac_data = NULL;

  self->network_monitor = g_network_monitor_get_default ();
  self->online = g_network_monitor_get_network_available (self->network_monitor);
  g_signal_connect_object (G_OBJECT (self->network_monitor), "network-changed", G_CALLBACK (px_manager_on_network_changed), self, 0);
}

static void
px_manager_dispose (GObject *object)
{
  PxManager *self = PX_MANAGER (object);
  const GList *list;

  if (self->cancellable) {
    g_cancellable_cancel (self->cancellable);
    g_clear_object (&self->cancellable);
  }

  list = peas_engine_get_plugin_list (self->engine);
  for (; list && list->data; list = list->next) {
    PeasPluginInfo *info = PEAS_PLUGIN_INFO (list->data);

    if (peas_plugin_info_is_loaded (info))
      peas_engine_unload_plugin (self->engine, info);
  }

  g_clear_pointer (&self->config_plugin, g_free);
  g_clear_pointer (&self->plugins_dir, g_free);
  g_clear_object (&self->engine);

  G_OBJECT_CLASS (px_manager_parent_class)->dispose (object);
}

static void
px_manager_set_property (GObject      *object,
                         guint         prop_id,
                         const GValue *value,
                         GParamSpec   *pspec)
{
  PxManager *self = PX_MANAGER (object);

  switch (prop_id) {
    case PROP_PLUGINS_DIR:
      self->plugins_dir = g_strdup (g_value_get_string (value));
      break;
    case PROP_CONFIG_PLUGIN:
      self->config_plugin = g_strdup (g_value_get_string (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
px_manager_get_property (GObject    *object,
                         guint       prop_id,
                         GValue     *value,
                         GParamSpec *pspec)
{
  switch (prop_id) {
    case PROP_PLUGINS_DIR:
      break;
    case PROP_CONFIG_PLUGIN:
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
px_manager_class_init (PxManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed = px_manager_constructed;
  object_class->dispose = px_manager_dispose;
  object_class->set_property = px_manager_set_property;
  object_class->get_property = px_manager_get_property;

  obj_properties[PROP_PLUGINS_DIR] = g_param_spec_string ("plugins-dir",
                                                          NULL,
                                                          NULL,
                                                          NULL,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

  obj_properties[PROP_CONFIG_PLUGIN] = g_param_spec_string ("config-plugin",
                                                            NULL,
                                                            NULL,
                                                            NULL,
                                                            G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, LAST_PROP, obj_properties);
}

static void
px_manager_init (PxManager *self)
{
}

/**
 * px_manager_new:
 *
 * Create a new `PxManager`.
 *
 * Returns: the newly created `PxManager`
 */
PxManager *
px_manager_new (void)
{
  return g_object_new (PX_TYPE_MANAGER, "plugins-dir", PX_PLUGINS_DIR, NULL);
}

struct DownloadData {
  const char *uri;
  GBytes *bytes;
  GError **error;
};

static void
download_pac (PeasExtensionSet *set,
              PeasPluginInfo   *info,
              PeasExtension    *extension,
              gpointer          data)
{
  PxDownloadInterface *ifc = PX_DOWNLOAD_GET_IFACE (extension);
  struct DownloadData *download_data = data;

  g_debug ("%s: Download PAC '%s' using plugin '%s'", __FUNCTION__, download_data->uri, peas_plugin_info_get_module_name (info));
  if (!download_data->bytes)
    download_data->bytes = ifc->download (PX_DOWNLOAD (extension), download_data->uri);
}

/**
 * px_manager_pac_download:
 * @self: a px manager
 * @uri: PAC uri
 *
 * Downloads a PAC file from provided @url.
 *
 * Returns: (nullable): a newly created `GBytes` containing PAC data, or %NULL on error.
 */
GBytes *
px_manager_pac_download (PxManager  *self,
                         const char *uri)
{
  struct DownloadData download_data = {
    .uri = uri,
    .bytes = NULL,
  };

  peas_extension_set_foreach (self->download_set, download_pac, &download_data);
  return download_data.bytes;
}

struct ConfigData {
  GStrvBuilder *builder;
  GUri *uri;
};

/**
 * Strategy:
 *  - Traverse through all plugins in extension set and ask for configuration data.
 *  - A plugin can either return the proxy server for given uri OR nothing! for direc access.
 */
static void
get_config (PeasExtensionSet *set,
            PeasPluginInfo   *info,
            PeasExtension    *extension,
            gpointer          data)
{
  PxConfigInterface *ifc = PX_CONFIG_GET_IFACE (extension);
  struct ConfigData *config_data = data;

  g_debug ("%s: Asking plugin '%s' for configuration", __FUNCTION__, peas_plugin_info_get_module_name (info));
  ifc->get_config (PX_CONFIG (extension), config_data->uri, config_data->builder);
}

/**
 * px_manager_get_configuration:
 * @self: a px manager
 * @uri: PAC uri
 * @error: a #GError
 *
 * Get raw proxy configuration for gien @uri.
 *
 * Returns: (nullable): a newly created `GStrv` containing configuration data for @uri.
 */
char **
px_manager_get_configuration (PxManager  *self,
                              GUri       *uri,
                              GError    **error)
{
  g_autoptr (GStrvBuilder) builder = g_strv_builder_new ();
  struct ConfigData config_data = {
    .uri = uri,
    .builder = builder,
  };

  peas_extension_set_foreach (self->config_set, get_config, &config_data);

  return g_strv_builder_end (builder);
}

struct PacData {
  GBytes *pac;
  GUri *uri;
  GStrvBuilder *builder;
};

static void
px_manager_run_pac (PeasExtensionSet *set,
                    PeasPluginInfo   *info,
                    PeasExtension    *extension,
                    gpointer          data)
{
  PxPacRunnerInterface *ifc = PX_PAC_RUNNER_GET_IFACE (extension);
  struct PacData *pac_data = data;
  g_auto (GStrv) proxies_split = NULL;
  char *pac_response;

  if (!ifc->set_pac (PX_PAC_RUNNER (extension), pac_data->pac))
    return;

  pac_response = ifc->run (PX_PAC_RUNNER (extension), pac_data->uri);

  /* Split line to handle multiple proxies */
  proxies_split = g_strsplit (pac_response, ";", -1);

  for (int idx = 0; idx < g_strv_length (proxies_split); idx++) {
    char *line = g_strstrip (proxies_split[idx]);
    g_auto (GStrv) word_split = g_strsplit (line, " ", -1);
    g_autoptr (GUri) uri = NULL;
    char *method;
    char *server;

    /* Check for syntax "METHOD SERVER" */
    if (g_strv_length (word_split) == 2) {
      g_autofree char *uri_string = NULL;
      g_autofree char *proxy_string = NULL;

      method = word_split[0];
      server = word_split[1];

      uri_string = g_strconcat ("http://", server, NULL);
      uri = g_uri_parse (uri_string, G_URI_FLAGS_PARSE_RELAXED, NULL);
      if (!uri)
        continue;

      if (g_ascii_strncasecmp (method, "proxy", 5) == 0) {
        proxy_string = g_uri_to_string (uri);
      } else if (g_ascii_strncasecmp (method, "socks", 5) == 0) {
        proxy_string = g_strconcat ("socks://", server, NULL);
      } else if (g_ascii_strncasecmp (method, "socks4", 6) == 0) {
        proxy_string = g_strconcat ("socks4://", server, NULL);
      } else if (g_ascii_strncasecmp (method, "socks4a", 7) == 0) {
        proxy_string = g_strconcat ("socks4a://", server, NULL);
      } else if (g_ascii_strncasecmp (method, "socks5", 6) == 0) {
        proxy_string = g_strconcat ("socks5://", server, NULL);
      }

      g_strv_builder_add (pac_data->builder, proxy_string);
    } else {
      /* Syntax not found, returning direct */
      g_strv_builder_add (pac_data->builder, "direct://");
    }
  }
}

static gboolean
px_manager_expand_wpad (PxManager *self,
                        GUri      *uri)
{
  const char *scheme = g_uri_get_scheme (uri);
  gboolean ret = FALSE;

  if (g_strcmp0 (scheme, "wpad") == 0) {
    ret = TRUE;

    if (!self->wpad) {
      g_clear_pointer (&self->pac_data, g_bytes_unref);
      g_clear_pointer (&self->pac_url, g_free);
      self->wpad = TRUE;
    }

    if (!self->pac_data) {
      GUri *wpad_url = g_uri_parse ("http://wpad/wpad.dat", G_URI_FLAGS_PARSE_RELAXED, NULL);

      g_debug ("%s: Trying to find the PAC using WPAD...", __FUNCTION__);
      self->pac_url = g_uri_to_string (wpad_url);
      self->pac_data = px_manager_pac_download (self, self->pac_url);
      if (!self->pac_data) {
        g_clear_pointer (&self->pac_url, g_free);
        ret = FALSE;
      }
    }
  }

  return ret;
}

static gboolean
px_manager_expand_pac (PxManager *self,
                       GUri      *uri)
{
  gboolean ret = FALSE;
  const char *scheme = g_uri_get_scheme (uri);

  if (g_str_has_prefix (scheme, "pac+")) {
    ret = TRUE;

    if (self->wpad)
      self->wpad = FALSE;

    if (self->pac_data) {
      g_autofree char *uri_str = g_uri_to_string (uri);

      if (g_strcmp0 (self->pac_url, uri_str) != 0) {
        g_clear_pointer (&self->pac_url, g_free);
        g_clear_pointer (&self->pac_data, g_bytes_unref);
      }
    }

    if (!self->pac_data) {
      self->pac_url = g_uri_to_string (uri);
      self->pac_data = px_manager_pac_download (self, self->pac_url);

      if (!self->pac_data) {
        g_warning ("%s: Unable to download PAC from %s while online = %d!", __FUNCTION__, self->pac_url, self->online);
        g_clear_pointer (&self->pac_url, g_free);
        ret = FALSE;
      } else {
        g_debug ("%s: PAC recevied!", __FUNCTION__);
      }
    }
  }

  return ret;
}

/**
 * px_manager_get_proxies_sync:
 * @self: a px manager
 * @url: a url
 *
 * Get proxies for giben @url.
 *
 * Returns: (nullable): a newly created `GStrv` containing proxy related information.
 */
char **
px_manager_get_proxies_sync (PxManager   *self,
                             const char  *url,
                             GError     **error)
{
  /* GList *list; */
  g_autoptr (GStrvBuilder) builder = g_strv_builder_new ();
  g_autoptr (GUri) uri = g_uri_parse (url, G_URI_FLAGS_PARSE_RELAXED, error);
  g_auto (GStrv) config = NULL;

  g_debug ("%s: url=%s online=%d", __FUNCTION__, url ? url : "?", self->online);
  if (!uri || !self->online) {
    g_strv_builder_add (builder, "direct://");
    return g_strv_builder_end (builder);
  }

  config = px_manager_get_configuration (self, uri, error);

  g_debug ("%s: Config is:", __FUNCTION__);
  for (int idx = 0; idx < g_strv_length (config); idx++) {
    GUri *conf_url = g_uri_parse (config[idx], G_URI_FLAGS_PARSE_RELAXED, NULL);

    g_debug ("%s:\t- Config[%d] = %s\n", __FUNCTION__, idx, config[idx]);

    if (px_manager_expand_wpad (self, conf_url) || px_manager_expand_pac (self, conf_url)) {
      struct PacData pac_data = {
        .pac = self->pac_data,
        .uri = uri,
        .builder = builder,
      };
      peas_extension_set_foreach (self->pacrunner_set, px_manager_run_pac, &pac_data);
    } else if (!g_str_has_prefix (g_uri_get_scheme (conf_url), "wpad")) {
      g_strv_builder_add (builder, g_uri_to_string (conf_url));
    }
  }

  /* In case no proxy could be found, assume direct connection */
  if (((GPtrArray *)builder)->len == 0)
    g_strv_builder_add (builder, "direct://");

  return g_strv_builder_end (builder);
}
