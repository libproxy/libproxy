/* px-manager.c
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

#include "config.h"

#include "px-manager.h"
#include "px-plugin-config.h"
#include "px-plugin-pacrunner.h"

#include <libsoup/soup.h>
#include <libpeas/peas.h>

enum {
  PROP_0,
  PROP_PLUGINS_DIR,
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
  char *plugins_dir;
  GCancellable *cancellable;
  SoupSession *session;

  gboolean wpad;
  GBytes *pac_data;
  char *pac_url;
};

G_DEFINE_TYPE (PxManager, px_manager, G_TYPE_OBJECT)

G_DEFINE_QUARK (px - manager - error - quark, px_manager_error)

static void
px_manager_constructed (GObject *object)
{
  PxManager *self = PX_MANAGER (object);
  const GList *list;
  const char *requested_config_plugin = g_getenv ("PX_CONFIG_PLUGIN");

  self->session = soup_session_new ();

  self->engine = peas_engine_get_default ();

  peas_engine_add_search_path (self->engine, self->plugins_dir, NULL);

  self->config_set = peas_extension_set_new (self->engine, PX_TYPE_CONFIG, NULL);
  self->pacrunner_set = peas_extension_set_new (self->engine, PX_TYPE_PACRUNNER, NULL);
  list = peas_engine_get_plugin_list (self->engine);
  for (; list && list->data; list = list->next) {
    PeasPluginInfo *info = PEAS_PLUGIN_INFO (list->data);
    PeasExtension *extension = peas_extension_set_get_extension (self->config_set, info);
    gboolean available = TRUE;

    if (!peas_plugin_info_is_loaded (info)) {
      /* In case user requested a specific module, just load that one */
      if (requested_config_plugin) {
        if (g_strcmp0 (peas_plugin_info_get_module_name (info), requested_config_plugin) == 0)
          peas_engine_load_plugin (self->engine, info);
      } else {
        peas_engine_load_plugin (self->engine, info);
      }
    }

    extension = peas_extension_set_get_extension (self->config_set, info);
    if (extension) {
      PxConfigInterface *ifc = PX_CONFIG_GET_IFACE (extension);

      available = ifc->is_available (PX_CONFIG (extension));
    }

    if (!available)
      peas_engine_unload_plugin (self->engine, info);
  }
}

static void
px_manager_dispose (GObject *object)
{
  PxManager *self = PX_MANAGER (object);

  g_cancellable_cancel (self->cancellable);
  g_clear_object (&self->cancellable);
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
                                                          "Plugins Dir",
                                                          "The directory where plugins are stored.",
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
  g_autoptr (SoupMessage) msg = soup_message_new (SOUP_METHOD_GET, uri);
  g_autoptr (GError) error = NULL;
  g_autoptr (GBytes) bytes = NULL;

  bytes = soup_session_send_and_read (
    self->session,
    msg,
    NULL,     /* Pass a GCancellable here if you want to cancel a download */
    &error);
  if (!bytes || soup_message_get_status (msg) != SOUP_STATUS_OK) {
    g_debug ("Failed to download: %s\n", error ? error->message : "");
    return NULL;
  }

  return g_steal_pointer (&bytes);
}

struct ConfigData {
  GStrvBuilder *builder;
  GUri *uri;
  GError **error;
};

static void
get_config (PeasExtensionSet *set,
            PeasPluginInfo   *info,
            PeasExtension    *extension,
            gpointer          data)
{
  PxConfigInterface *ifc = PX_CONFIG_GET_IFACE (extension);
  struct ConfigData *config_data = data;

  g_print ("%s: Asking plugin '%s' for configuration\n", __FUNCTION__, peas_plugin_info_get_module_name (info));
  ifc->get_config (PX_CONFIG (extension), config_data->uri, config_data->builder, config_data->error);
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
    .error = error,
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
  char *ret;

  ifc->set_pac (PX_PAC_RUNNER (extension), pac_data->pac);
  ret = ifc->run (PX_PAC_RUNNER (extension), pac_data->uri);
  if (ret)
    g_strv_builder_add (pac_data->builder, ret);
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
      g_clear_object (&self->pac_data);
      g_clear_pointer (&self->pac_url, g_free);
      self->wpad = TRUE;
    }

    if (!self->pac_data) {
      GUri *wpad_url = g_uri_parse ("http://wpad/wpad.data", G_URI_FLAGS_PARSE_RELAXED, NULL);

      g_print ("Trying to find the PAC using WPAD...\n");
      self->pac_url = g_uri_to_string (wpad_url);
      self->pac_data = px_manager_pac_download (self, self->pac_url);
      if (!self->pac_data)
        g_clear_object (&self->pac_url);
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
        g_clear_object (&self->pac_data);
      }
    }

    if (!self->pac_data) {
      self->pac_url = g_uri_to_string (uri);
      self->pac_data = px_manager_pac_download (self, self->pac_url);

      if (!self->pac_data)
        g_error ("Unable to download PAC!");
      else
        g_debug ("PAC recevied!\n");
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

  if (!uri) {
    g_strv_builder_add (builder, "direct://");
    return g_strv_builder_end (builder);
  }

  /* TODO: Check topology */
  config = px_manager_get_configuration (self, uri, error);

  g_print ("Config is:\n");
  for (int idx = 0; idx < g_strv_length (config); idx++) {
    GUri *conf_url = g_uri_parse (config[idx], G_URI_FLAGS_PARSE_RELAXED, NULL);

    g_print ("\t- %s\n", config[idx]);

    if (px_manager_expand_wpad (self, conf_url) || px_manager_expand_pac (self, conf_url)) {
      struct PacData pac_data = {
        .pac = self->pac_data,
        .uri = uri,
        .builder = builder,
      };
      peas_extension_set_foreach (self->pacrunner_set, px_manager_run_pac, &pac_data);
    } else {
      g_strv_builder_add (builder, g_uri_to_string (conf_url));
    }
  }

  /* In case no proxy could be found, assume direct connection */
  if (((GPtrArray *)builder)->len == 0)
    g_strv_builder_add (builder, "direct://");

  return g_strv_builder_end (builder);
}
