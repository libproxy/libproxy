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
#include "px-backend-config.h"

#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>

#include "px-manager.h"
#include "px-plugin-config.h"
#include "px-plugin-pacrunner.h"

#ifdef HAVE_CONFIG_ENV
#include <plugins/config-env/config-env.h>
#endif

#ifdef HAVE_CONFIG_GNOME
#include <plugins/config-gnome/config-gnome.h>
#endif

#ifdef HAVE_CONFIG_KDE
#include <plugins/config-kde/config-kde.h>
#endif

#ifdef HAVE_CONFIG_OSX
#include <plugins/config-osx/config-osx.h>
#endif

#ifdef HAVE_CONFIG_SYSCONFIG
#include <plugins/config-sysconfig/config-sysconfig.h>
#endif

#ifdef HAVE_CONFIG_WINDOWS
#include <plugins/config-windows/config-windows.h>
#endif

#ifdef HAVE_PACRUNNER_DUKTAPE
#include <plugins/pacrunner-duktape/pacrunner-duktape.h>
#endif

#ifdef HAVE_CURL
#include <curl/curl.h>
#endif

enum {
  PROP_0,
  PROP_CONFIG_PLUGIN,
  PROP_CONFIG_OPTION,
  PROP_FORCE_ONLINE,
  LAST_PROP
};

static GParamSpec *obj_properties[LAST_PROP];

/**
 * PxManager:
 *
 * Manage libproxy modules
 */

struct _PxManager {
  GObject parent_instance;
  GList *config_plugins;
  GList *pacrunner_plugins;
  GNetworkMonitor *network_monitor;
#ifdef HAVE_CURL
  CURL *curl;
#endif

  char *config_plugin;
  char *config_option;

  gboolean force_online;
  gboolean online;
  gboolean wpad;
  GBytes *pac_data;
  char *pac_url;

  GMutex mutex;
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

static gint
config_order_compare (gconstpointer a,
                      gconstpointer b)
{
  PxConfig *config_a = (PxConfig *)a;
  PxConfig *config_b = (PxConfig *)b;
  PxConfigInterface *ifc_a = PX_CONFIG_GET_IFACE (config_a);
  PxConfigInterface *ifc_b = PX_CONFIG_GET_IFACE (config_b);

  if (ifc_a->priority < ifc_b->priority)
    return -1;

  if (ifc_a->priority == ifc_b->priority)
    return 0;

  return 1;
}

static void
px_manager_add_config_plugin (PxManager *self,
                              GType      type)
{
  PxConfig *config = g_object_new (type, "config-option", self->config_option, NULL);
  PxConfigInterface *ifc = PX_CONFIG_GET_IFACE (config);
  const char *env = g_getenv ("PX_FORCE_CONFIG");
  const char *force_config = self->config_plugin ? self->config_plugin : env;

  if (!force_config || g_strcmp0 (ifc->name, force_config) == 0)
    self->config_plugins = g_list_insert_sorted (self->config_plugins, config, config_order_compare);
}

static void
px_manager_add_pacrunner_plugin (PxManager *self,
                                 GType      type)
{
  PxPacRunner *pacrunner = g_object_new (type, NULL);

  self->pacrunner_plugins = g_list_append (self->pacrunner_plugins, pacrunner);
}

static void
px_manager_constructed (GObject *object)
{
  PxManager *self = PX_MANAGER (object);

  if (g_getenv ("PX_DEBUG")) {
    const gchar *g_messages_debug;

    g_messages_debug = g_getenv ("G_MESSAGES_DEBUG");

    if (!g_messages_debug) {
      g_setenv ("G_MESSAGES_DEBUG", G_LOG_DOMAIN, TRUE);
    } else {
      g_autofree char *new_g_messages_debug = NULL;

      new_g_messages_debug = g_strconcat (g_messages_debug, " ", G_LOG_DOMAIN, NULL);
      if (new_g_messages_debug)
        g_setenv ("G_MESSAGES_DEBUG", new_g_messages_debug, TRUE);
    }
  }

#ifdef HAVE_CONFIG_ENV
  px_manager_add_config_plugin (self, PX_CONFIG_TYPE_ENV);
#endif
#ifdef HAVE_CONFIG_GNOME
  px_manager_add_config_plugin (self, PX_CONFIG_TYPE_GNOME);
#endif
#ifdef HAVE_CONFIG_KDE
  px_manager_add_config_plugin (self, PX_CONFIG_TYPE_KDE);
#endif
#ifdef HAVE_CONFIG_OSX
  px_manager_add_config_plugin (self, PX_CONFIG_TYPE_OSX);
#endif
#ifdef HAVE_CONFIG_SYSCONFIG
  px_manager_add_config_plugin (self, PX_CONFIG_TYPE_SYSCONFIG);
#endif
#ifdef HAVE_CONFIG_WINDOWS
  px_manager_add_config_plugin (self, PX_CONFIG_TYPE_WINDOWS);
#endif

  g_debug ("Active config plugins:");
  for (GList *list = self->config_plugins; list && list->data; list = list->next) {
    PxConfig *config = list->data;
    PxConfigInterface *ifc = PX_CONFIG_GET_IFACE (config);

    g_debug (" - %s", ifc->name);
  }

#ifdef HAVE_PACRUNNER_DUKTAPE
  px_manager_add_pacrunner_plugin (self, PX_PACRUNNER_TYPE_DUKTAPE);
#endif

  self->pac_data = NULL;

  if (!self->force_online) {
    self->network_monitor = g_network_monitor_get_default ();
    g_signal_connect_object (G_OBJECT (self->network_monitor), "network-changed", G_CALLBACK (px_manager_on_network_changed), self, 0);
    px_manager_on_network_changed (self->network_monitor, g_network_monitor_get_network_available (self->network_monitor), self);
  } else {
    px_manager_on_network_changed (NULL, TRUE, self);
  }

  g_debug ("%s: Up and running", __FUNCTION__);
}

static void
px_manager_dispose (GObject *object)
{
  PxManager *self = PX_MANAGER (object);

  for (GList *list = self->config_plugins; list && list->data; list = list->next)
    g_clear_object (&list->data);

  for (GList *list = self->pacrunner_plugins; list && list->data; list = list->next)
    g_clear_object (&list->data);

  g_clear_pointer (&self->config_plugin, g_free);

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
    case PROP_CONFIG_PLUGIN:
      self->config_plugin = g_strdup (g_value_get_string (value));
      break;
    case PROP_CONFIG_OPTION:
      self->config_option = g_strdup (g_value_get_string (value));
      break;
    case PROP_FORCE_ONLINE:
      self->force_online = g_value_get_boolean (value);
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

  obj_properties[PROP_CONFIG_PLUGIN] = g_param_spec_string ("config-plugin",
                                                            NULL,
                                                            NULL,
                                                            NULL,
                                                            G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

  obj_properties[PROP_CONFIG_OPTION] = g_param_spec_string ("config-option",
                                                            NULL,
                                                            NULL,
                                                            NULL,
                                                            G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

  obj_properties[PROP_FORCE_ONLINE] = g_param_spec_boolean ("force-online",
                                                            NULL,
                                                            NULL,
                                                            FALSE,
                                                            G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, LAST_PROP, obj_properties);
}

static void
px_manager_init (PxManager *self)
{
}

/**
 * px_manager_new_with_options:
 * @optname1: name of first property to set
 * @...: value of @optname1, followed by additional property/value pairs
 *
 * Create a new `PxManager` with the specified options.
 *
 * Returns: the newly created `PxManager`
 */
PxManager *
px_manager_new_with_options (const char *optname1,
                             ...)
{
  PxManager *self;
  va_list ap;

  va_start (ap, optname1);
  self = (PxManager *)g_object_new_valist (PX_TYPE_MANAGER, optname1, ap);
  va_end (ap);

  return self;
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
  return px_manager_new_with_options (NULL);
}

#ifdef HAVE_CURL
static size_t
store_data (void   *contents,
            size_t  size,
            size_t  nmemb,
            void   *user_pointer)
{
  GByteArray *byte_array = user_pointer;
  size_t real_size = size * nmemb;

  g_byte_array_append (byte_array, contents, real_size);

  return real_size;
}
#endif

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
#ifdef HAVE_CURL
  GByteArray *byte_array = g_byte_array_new ();
  CURLcode res;
  const char *url = uri;

  if (!self->curl)
    self->curl = curl_easy_init ();

  if (!self->curl)
    return NULL;

  if (g_str_has_prefix (url, "pac+"))
    url += 4;

  if (curl_easy_setopt (self->curl, CURLOPT_NOSIGNAL, 1) != CURLE_OK)
    g_debug ("Could not set NOSIGNAL, continue");

  if (curl_easy_setopt (self->curl, CURLOPT_FOLLOWLOCATION, 1) != CURLE_OK)
    g_debug ("Could not set FOLLOWLOCATION, continue");

  if (curl_easy_setopt (self->curl, CURLOPT_NOPROXY, "*") != CURLE_OK) {
    g_warning ("Could not set NOPROXY, ABORT!");
    return NULL;
  }

  if (curl_easy_setopt (self->curl, CURLOPT_CONNECTTIMEOUT, 30) != CURLE_OK)
    g_debug ("Could not set CONENCTIONTIMEOUT, continue");

  if (curl_easy_setopt (self->curl, CURLOPT_USERAGENT, "libproxy") != CURLE_OK)
    g_debug ("Could not set USERAGENT, continue");

  if (curl_easy_setopt (self->curl, CURLOPT_URL, url) != CURLE_OK) {
    g_warning ("Could not set URL, ABORT!");
    return NULL;
  }

  if (curl_easy_setopt (self->curl, CURLOPT_WRITEFUNCTION, store_data) != CURLE_OK) {
    g_warning ("Could not set WRITEFUNCTION, ABORT!");
    return NULL;
  }

  if (curl_easy_setopt (self->curl, CURLOPT_WRITEDATA, byte_array) != CURLE_OK) {
    g_warning ("Could not set WRITEDATA, ABORT!");
    return NULL;
  }

  res = curl_easy_perform (self->curl);
  if (res != CURLE_OK) {
    g_debug ("%s: Could not download data: %s", __FUNCTION__, curl_easy_strerror (res));
    return NULL;
  }

  return g_byte_array_free_to_bytes (byte_array);
#else
  return NULL;
#endif
}

/**
 * px_manager_get_configuration:
 * @self: a px manager
 * @uri: PAC uri
 * @error: a #GError
 *
 * Get raw proxy configuration for gien @uri.
 *
 * Returns: (transfer full) (nullable): a newly created `GStrv` containing configuration data for @uri.
 */
char **
px_manager_get_configuration (PxManager  *self,
                              GUri       *uri,
                              GError    **error)
{
  g_autoptr (GStrvBuilder) builder = g_strv_builder_new ();

  for (GList *list = self->config_plugins; list && list->data; list = list->next) {
    PxConfig *config = PX_CONFIG (list->data);
    PxConfigInterface *ifc = PX_CONFIG_GET_IFACE (config);

    ifc->get_config (config, uri, builder);
  }

  return g_strv_builder_end (builder);
}

static void
px_manager_run_pac (PxPacRunner  *pacrunner,
                    GBytes       *pac,
                    GUri         *uri,
                    GStrvBuilder *builder)
{
  PxPacRunnerInterface *ifc = PX_PAC_RUNNER_GET_IFACE (pacrunner);
  g_auto (GStrv) proxies_split = NULL;
  char *pac_response;

  pac_response = ifc->run (PX_PAC_RUNNER (pacrunner), uri);

  /* Split line to handle multiple proxies */
  proxies_split = g_strsplit (pac_response, ";", -1);

  for (int idx = 0; idx < g_strv_length (proxies_split); idx++) {
    char *line = g_strstrip (proxies_split[idx]);
    g_auto (GStrv) word_split = g_strsplit (line, " ", -1);
    g_autoptr (GUri) proxy_uri = NULL;
    char *method;
    char *server;

    /* Check for syntax "METHOD SERVER" */
    if (g_strv_length (word_split) == 2) {
      g_autofree char *uri_string = NULL;
      g_autofree char *proxy_string = NULL;

      method = word_split[0];
      server = word_split[1];

      uri_string = g_strconcat ("http://", server, NULL);
      proxy_uri = g_uri_parse (uri_string, G_URI_FLAGS_PARSE_RELAXED, NULL);
      if (!proxy_uri)
        continue;

      if (g_ascii_strncasecmp (method, "proxy", 5) == 0) {
        proxy_string = g_uri_to_string (proxy_uri);
      } else if (g_ascii_strncasecmp (method, "socks4a", 7) == 0) {
        proxy_string = g_strconcat ("socks4a://", server, NULL);
      } else if (g_ascii_strncasecmp (method, "socks4", 6) == 0) {
        proxy_string = g_strconcat ("socks4://", server, NULL);
      } else if (g_ascii_strncasecmp (method, "socks5", 6) == 0) {
        proxy_string = g_strconcat ("socks5://", server, NULL);
      } else if (g_ascii_strncasecmp (method, "socks", 5) == 0) {
        proxy_string = g_strconcat ("socks://", server, NULL);
      }

      px_strv_builder_add_proxy (builder, proxy_string);
    } else {
      /* Syntax not found, returning direct */
      px_strv_builder_add_proxy (builder, "direct://");
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
px_manager_set_pac (PxManager *self)
{
  GList *list;

  for (list = self->pacrunner_plugins; list && list->data; list = list->next) {
    PxPacRunner *pacrunner = PX_PAC_RUNNER (list->data);
    PxPacRunnerInterface *ifc = PX_PAC_RUNNER_GET_IFACE (pacrunner);

    if (!ifc->set_pac (PX_PAC_RUNNER (pacrunner), self->pac_data))
      return FALSE;
  }

  return TRUE;
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
        if (!px_manager_set_pac (self)) {
          g_warning ("%s: Unable to set PAC from %s while online = %d!", __FUNCTION__, self->pac_url, self->online);
          g_clear_pointer (&self->pac_url, g_free);
          g_clear_pointer (&self->pac_data, g_bytes_unref);
          ret = FALSE;
        }
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
 * Returns: (transfer full) (nullable): a newly created `GStrv` containing proxy related information.
 */
char **
px_manager_get_proxies_sync (PxManager   *self,
                             const char  *url,
                             GError     **error)
{
  g_autoptr (GStrvBuilder) builder = NULL;
  g_autoptr (GUri) uri = NULL;
  g_auto (GStrv) config = NULL;

  g_mutex_lock (&self->mutex);

  builder = g_strv_builder_new ();
  uri = g_uri_parse (url, G_URI_FLAGS_PARSE_RELAXED, error);

  g_debug ("%s: url=%s online=%d", __FUNCTION__, url ? url : "?", self->online);
  if (!uri || !self->online) {
    px_strv_builder_add_proxy (builder, "direct://");
    g_mutex_unlock (&self->mutex);
    return g_strv_builder_end (builder);
  }

  config = px_manager_get_configuration (self, uri, error);

  for (int idx = 0; idx < g_strv_length (config); idx++) {
    GUri *conf_url = g_uri_parse (config[idx], G_URI_FLAGS_PARSE_RELAXED, NULL);

    g_debug ("%s: Config[%d] = %s", __FUNCTION__, idx, config[idx]);

    if (px_manager_expand_wpad (self, conf_url) || px_manager_expand_pac (self, conf_url)) {
      GList *list;

      for (list = self->pacrunner_plugins; list && list->data; list = list->next) {
        PxPacRunner *pacrunner = PX_PAC_RUNNER (list->data);

        px_manager_run_pac (pacrunner, self->pac_data, uri, builder);
      }
    } else if (!g_str_has_prefix (g_uri_get_scheme (conf_url), "wpad") && !g_str_has_prefix (g_uri_get_scheme (conf_url), "pac+")) {
      px_strv_builder_add_proxy (builder, g_uri_to_string (conf_url));
    }
  }

  /* In case no proxy could be found, assume direct connection */
  if (((GPtrArray *)builder)->len == 0)
    px_strv_builder_add_proxy (builder, "direct://");

  for (int idx = 0; idx < ((GPtrArray *)builder)->len; idx++)
    g_debug ("%s: Proxy[%d] = %s", __FUNCTION__, idx, (char *)((GPtrArray *)builder)->pdata[idx]);

  g_mutex_unlock (&self->mutex);
  return g_strv_builder_end (builder);
}

void
px_strv_builder_add_proxy (GStrvBuilder *builder,
                           const char   *value)
{
  for (int idx = 0; idx < ((GPtrArray *)builder)->len; idx++) {
    if (g_strcmp0 ((char *)((GPtrArray *)builder)->pdata[idx], value) == 0)
      return;
  }

  g_strv_builder_add (builder, value);
}

static gboolean
ignore_domain (GUri *uri,
               char *ignore)
{
  g_auto (GStrv) ignore_split = NULL;
  const char *host = g_uri_get_host (uri);
  char *ignore_host;
  int ignore_port = -1;
  int port;

  if (g_strcmp0 (ignore, "*") == 0)
    return TRUE;

  if (!host)
    return FALSE;

  ignore_split = g_strsplit (ignore, ":", -1);
  port = g_uri_get_port (uri);

  /* Get our ignore pattern's hostname and port */
  ignore_host = ignore_split[0];
  if  (g_strv_length (ignore_split) == 2)
    ignore_port = atoi (ignore_split[1]);

  /* Hostname match (domain.com or domain.com:80) */
  if (g_strcmp0 (host, ignore_host) == 0)
    return (ignore_port == -1 || port == ignore_port);

  /**
   * Treat the following three options as a wildcard for a domain:
   *  - .domain.com or .domain.com:80
   *  - *.domain.com or *.domain.com:80
   *  - domain.com or domain.com:80
   */
  if (strlen (ignore_host) > 2) {
    if (ignore_host[0] == '.' && ((g_ascii_strncasecmp (host, ignore_host + 1, strlen (host)) == 0) || g_str_has_suffix (host, ignore_host)))
      return (ignore_port == -1 || port == ignore_port);

    if (ignore_host[0] == '*' && ignore_host[1] == '.' && ((g_ascii_strncasecmp (host, ignore_host + 2, strlen (host)) == 0) || g_str_has_suffix (host, ignore_host + 1)))
      return (ignore_port == -1 || port == ignore_port);

    if (strlen (host) > strlen (ignore_host) && host[strlen (host) - strlen (ignore_host) - 1] == '.' && g_str_has_suffix (host, ignore_host))
      return (ignore_port == -1 || port == ignore_port);
  }

  /* No match was found */
  return FALSE;
}

static gboolean
ignore_hostname (GUri *uri,
                 char *ignore)
{
  const char *host = g_uri_get_host (uri);

  if (!host)
    return FALSE;

  if (g_strcmp0 (ignore, "<local>") == 0 && strchr (host, ':') == NULL && strchr (host, '.') == NULL)
    return TRUE;

  return FALSE;
}

static gboolean
ignore_ip (GUri *uri,
           char *ignore)
{
  g_autoptr (GInetAddress) uri_address = NULL;
  g_autoptr (GInetAddress) ignore_address = NULL;
  g_auto (GStrv) ignore_split = NULL;
  g_autoptr (GError) error = NULL;
  const char *uri_host = g_uri_get_host (uri);
  int port = g_uri_get_port (uri);
  int ignore_port = 0;
  gboolean result;

  if (!uri_host)
    return FALSE;

  uri_address = g_inet_address_new_from_string (uri_host);

  /*
   * IPv4/CIDR
   * IPv4/IPv4
   * IPv6/CIDR
   * IPv6/IPv6
   *
   * uri must be in ip string format, no host name resolution is done
   */
  if (uri_address && strchr (ignore, '/')) {
    GInetAddressMask *address_mask = g_inet_address_mask_new_from_string (ignore, &error);

    if (!address_mask) {
      g_warning ("Could not parse ignore mask: %s", error->message);
      return FALSE;
    }

    if (g_inet_address_mask_matches (address_mask, uri_address))
      return TRUE;
  }

  /*
   * IPv4
   * IPv6
   */
  if (!g_hostname_is_ip_address (uri_host) || !g_hostname_is_ip_address (ignore))
    return FALSE;

  /*
   * IPv4:port
   * [IPv6]:port
   */
  ignore_split = g_strsplit (ignore, ":", -1);
  if  (g_strv_length (ignore_split) == 2)
    ignore_port = atoi (ignore_split[1]);

  ignore_address = g_inet_address_new_from_string (ignore);
  result = g_inet_address_equal (uri_address, ignore_address);

  return ignore_port != 0 ? ((port == ignore_port) && result) : result;
}

gboolean
px_manager_is_ignore (GUri  *uri,
                      GStrv  ignores)
{
  if (!ignores)
    return FALSE;

  for (int idx = 0; idx < g_strv_length (ignores); idx++) {
    if (ignore_hostname (uri, ignores[idx]))
      return TRUE;

    if (ignore_domain (uri, ignores[idx]))
      return TRUE;

    if (ignore_ip (uri, ignores[idx]))
      return TRUE;
  }

  return FALSE;
}
