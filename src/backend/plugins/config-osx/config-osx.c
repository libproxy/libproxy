/* config-osx.c
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

#include <SystemConfiguration/SystemConfiguration.h>

#include <gio/gio.h>

#include "config-osx.h"

#include "px-plugin-config.h"
#include "px-manager.h"

static void px_config_iface_init (PxConfigInterface *iface);

struct _PxConfigOsX {
  GObject parent_instance;
};

G_DEFINE_FINAL_TYPE_WITH_CODE (PxConfigOsX,
                               px_config_osx,
                               G_TYPE_OBJECT,
                               G_IMPLEMENT_INTERFACE (PX_TYPE_CONFIG, px_config_iface_init))

enum {
  PROP_0,
  PROP_CONFIG_OPTION
};

static void
px_config_osx_set_property (GObject      *object,
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
px_config_osx_get_property (GObject    *object,
                            guint       prop_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
  switch (prop_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
px_config_osx_init (PxConfigOsX *self)
{
}

static void
px_config_osx_class_init (PxConfigOsXClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = px_config_osx_set_property;
  object_class->get_property = px_config_osx_get_property;

  g_object_class_override_property (object_class, PROP_CONFIG_OPTION, "config-option");
}

static gboolean
px_config_osx_is_available (PxConfig *self)
{
  return TRUE;
}

static CFNumberRef
getobj (CFDictionaryRef  settings,
        char            *key)
{
  CFStringRef k;
  CFNumberRef retval;

  if (!settings)
    return NULL;

  k = CFStringCreateWithCString (NULL, key, kCFStringEncodingMacRoman);
  if (!k)
    return NULL;

  retval = (CFNumberRef)CFDictionaryGetValue (settings, k);

  CFRelease (k);
  return retval;
}

static CFStringRef
getobj_str (CFDictionaryRef  settings,
            char            *key)
{
  CFStringRef k;
  CFStringRef retval;

  if (!settings)
    return NULL;

  k = CFStringCreateWithCString (NULL, key, kCFStringEncodingMacRoman);
  if (!k)
    return NULL;

  retval = (CFStringRef)CFDictionaryGetValue (settings, k);

  CFRelease (k);
  return retval;
}

static CFArrayRef
getobj_array (CFDictionaryRef  settings,
              char            *key)
{
  CFStringRef k;
  CFArrayRef retval;

  if (!settings)
    return NULL;

  k = CFStringCreateWithCString (NULL, key, kCFStringEncodingMacRoman);
  if (!k)
    return NULL;

  retval = (CFArrayRef)CFDictionaryGetValue (settings, k);

  CFRelease (k);
  return retval;
}

static char *
str (CFStringRef ref)
{
  CFIndex size = CFStringGetLength (ref) + 1;
  char *ret = g_malloc0 (size);

  CFStringGetCString (ref, ret, size, kCFStringEncodingUTF8);

  return ret;
}

static gboolean
getint (CFDictionaryRef  settings,
        char            *key,
        int64_t         *answer)
{
  CFNumberRef n = getobj (settings, key);

  if (!n)
    return FALSE;

  if (!CFNumberGetValue (n, kCFNumberSInt64Type, answer))
    return FALSE;

  return TRUE;
}

static gboolean
getbool (CFDictionaryRef  settings,
         char            *key)
{
  int64_t i = 0;

  if (!getint (settings, key, &i))
    return FALSE;

  return i != 0;
}

static char *
str_to_upper (const char *str)
{
  char *ret = NULL;
  int idx;

  if (!str)
    return NULL;

  ret = g_malloc0 (strlen (str) + 1);

  for (idx = 0; idx < strlen (str); idx++)
    ret[idx] = g_ascii_toupper (str[idx]);

  return ret;
}

static char *
protocol_url (CFDictionaryRef  settings,
              char            *protocol)
{
  g_autofree char *tmp = NULL;
  g_autoptr (GString) ret = NULL;
  g_autofree char *host = NULL;
  int64_t port;
  CFStringRef ref;

  /* Check ProtocolEnabled */
  tmp = g_strconcat (protocol, "Enable", NULL);
  if (!getbool (settings, tmp)) {
    g_debug ("%s: %s not set", __FUNCTION__, tmp);
    return NULL;
  }
  g_clear_pointer (&tmp, g_free);

  /* Get ProtocolPort */
  tmp = g_strconcat (protocol, "Port", NULL);
  getint (settings, tmp, &port);
  if (!port) {
    g_debug ("%s: %s not set", __FUNCTION__, tmp);
    return NULL;
  }
  g_clear_pointer (&tmp, g_free);

  /* Get ProtocolProxy */
  tmp = g_strconcat (protocol, "Proxy", NULL);
  ref = getobj_str (settings, tmp);
  g_clear_pointer (&tmp, g_free);

  host = str (ref);
  if (!host || strlen (host) == 0)
    return NULL;

  if (strcmp (protocol, "HTTP") == 0 || strcmp (protocol, "HTTPS") == 0 || strcmp (protocol, "FTP") == 0 || strcmp (protocol, "Gopher") == 0)
    ret = g_string_new ("http://");
  else if (strcmp (protocol, "RTSP") == 0)
    ret = g_string_new ("rtsp://");
  else if (strcmp (protocol, "SOCKS") == 0)
    ret = g_string_new ("socks://");
  else
    return NULL;

  g_string_append_printf (ret, "%s:%lld", host, port);

  return g_strdup (ret->str);
}

static GStrv
get_ignore_list (CFDictionaryRef proxies)
{
  CFArrayRef ref = getobj_array (proxies, "ExceptionsList");
  g_autoptr (GStrvBuilder) ret = g_strv_builder_new ();

  if (!ref)
    return g_strv_builder_end (ret);

  for (int idx = 0; idx < CFArrayGetCount (ref); idx++) {
    CFStringRef s = (CFStringRef)CFArrayGetValueAtIndex (ref, idx);

    px_strv_builder_add_proxy (ret, str (s));
  }

  if (getbool (proxies, "ExcludeSimpleHostnames"))
    px_strv_builder_add_proxy (ret, "127.0.0.1");

  return g_strv_builder_end (ret);
}

static void
px_config_osx_get_config (PxConfig     *self,
                          GUri         *uri,
                          GStrvBuilder *builder)
{
  const char *proxy = NULL;
  CFDictionaryRef proxies = SCDynamicStoreCopyProxies (NULL);
  g_auto (GStrv) ignore_list = NULL;

  if (!proxies) {
    g_warning ("Unable to fetch proxy configuration");
    return;
  }

  ignore_list = get_ignore_list (proxies);

  if (px_manager_is_ignore (uri, ignore_list))
    return;

  if (getbool (proxies, "ProxyAutoDiscoveryEnable")) {
    CFRelease (proxies);
    px_strv_builder_add_proxy (builder, "wpad://");
    return;
  }

  if (getbool (proxies, "ProxyAutoConfigEnable")) {
    CFStringRef ref = getobj_str (proxies, "ProxyAutoConfigURLString");
    g_autofree char *tmp = str (ref);
    GUri *tmp_uri = g_uri_parse (tmp, G_URI_FLAGS_PARSE_RELAXED, NULL);

    if (tmp_uri) {
      g_autofree char *ret = g_strdup_printf ("pac+%s", g_uri_to_string (tmp_uri));
      CFRelease (proxies);
      px_strv_builder_add_proxy (builder, ret);
      return;
    }
  } else {
    const char *scheme = g_uri_get_scheme (uri);
    g_autofree char *capital_scheme = str_to_upper (scheme);

    proxy = protocol_url (proxies, capital_scheme);

    if (!proxy)
      proxy = protocol_url (proxies, "SOCKS");
  }

  if (proxy)
    px_strv_builder_add_proxy (builder, proxy);
}

static void
px_config_iface_init (PxConfigInterface *iface)
{
  iface->name = "config-osx";
  iface->priority = PX_CONFIG_PRIORITY_DEFAULT;
  iface->is_available = px_config_osx_is_available;
  iface->get_config = px_config_osx_get_config;
}
