/* config-xdp.c
 *
 * Copyright 2024 The Libproxy Team
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

#include "config-xdp.h"

#include "px-manager.h"
#include "px-plugin-config.h"

static void px_config_iface_init (PxConfigInterface *iface);

struct _PxConfigXdp {
  GObject parent_instance;
  gboolean available;
  GDBusProxy *proxy_resolver;
};

G_DEFINE_FINAL_TYPE_WITH_CODE (PxConfigXdp,
                               px_config_xdp,
                               G_TYPE_OBJECT,
                               G_IMPLEMENT_INTERFACE (PX_TYPE_CONFIG, px_config_iface_init))

enum {
  PROP_0,
  PROP_CONFIG_OPTION
};

static void
px_config_xdp_init (PxConfigXdp *self)
{
  g_autoptr (GDBusConnection) connection = NULL;
  g_autoptr (GError) error = NULL;
  g_autofree char *path = g_build_filename (g_get_user_runtime_dir (), "flatpak-info", NULL);

  self->available = FALSE;

  /* Test for Flatpak or Snap Enivronments */
  if (!g_file_test (path, G_FILE_TEST_EXISTS) && !g_getenv ("SNAP_NAME"))
    return;

  connection = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, &error);
  if (error) {
    g_warning ("Could not access dbus session: %s", error->message);
    return;
  }

  self->proxy_resolver = g_dbus_proxy_new_sync (connection, G_DBUS_PROXY_FLAGS_NONE, NULL, "org.freedesktop.portal.Desktop", "/org/freedesktop/portal/desktop", "org.freedesktop.portal.ProxyResolver", NULL, &error);
  if (error) {
    g_warning ("Could not access proxy resolver: %s", error->message);
    return;
  }

  self->available = TRUE;
}

static void
px_config_xdp_dispose (GObject *object)
{
  PxConfigXdp *self = PX_CONFIG_XDP (object);

  g_clear_object (&self->proxy_resolver);
}

static void
px_config_xdp_set_property (GObject      *object,
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
px_config_xdp_get_property (GObject    *object,
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
px_config_xdp_class_init (PxConfigXdpClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = px_config_xdp_dispose;
  object_class->set_property = px_config_xdp_set_property;
  object_class->get_property = px_config_xdp_get_property;

  g_object_class_override_property (object_class, PROP_CONFIG_OPTION, "config-option");
}

static void
px_config_xdp_get_config (PxConfig     *config,
                          GUri         *uri,
                          GStrvBuilder *builder)
{
  g_autoptr (GVariant) var = NULL;
  g_autoptr (GError) error = NULL;
  g_autoptr (GVariantIter) iter = NULL;
  PxConfigXdp *self = PX_CONFIG_XDP (config);
  g_autofree char *uri_str = NULL;
  const char *str;

  if (!self->available)
    return;

  uri_str = g_uri_to_string (uri);
  var = g_dbus_proxy_call_sync (self->proxy_resolver, "Lookup", g_variant_new ("(s)", uri_str), 0, -1, NULL, &error);
  if (error) {
    g_warning ("Could not query proxy: %s", error->message);
    return;
  }

  g_variant_get (var, "(as)", &iter);
  while (g_variant_iter_loop (iter, "s", &str)) {
    px_strv_builder_add_proxy (builder, str);
  }
}

static void
px_config_iface_init (PxConfigInterface *iface)
{
  iface->name = "config-xdp";
  iface->priority = PX_CONFIG_PRIORITY_DEFAULT;
  iface->get_config = px_config_xdp_get_config;
}
