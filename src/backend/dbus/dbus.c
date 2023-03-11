/* dbus.c
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

#include "px-manager.h"
#include "px-interface.h"

#include <gio/gio.h>

static void
handle_method_call (GDBusConnection       *connection,
                    const gchar           *sender,
                    const gchar           *object_path,
                    const gchar           *interface_name,
                    const gchar           *method_name,
                    GVariant              *parameters,
                    GDBusMethodInvocation *invocation,
                    gpointer               user_data)
{
  PxManager *manager = PX_MANAGER (user_data);
  GVariantBuilder *result;
  g_auto (GStrv) proxies = NULL;
  g_autoptr (GError) error = NULL;
  const gchar *url;
  int idx;

  if (g_strcmp0 (method_name, "query") != 0) {
    g_warning ("Invalid method name '%s', aborting.", method_name);
    g_dbus_method_invocation_return_error (invocation,
                                           PX_MANAGER_ERROR,
                                           PX_MANAGER_ERROR_UNKNOWN_METHOD,
                                           "Unknown method");
    return;
  }

  g_variant_get (parameters, "(&s)", &url);

  proxies = px_manager_get_proxies_sync (manager, url, &error);
  if (error) {
    g_warning ("Could not query proxy servers: %s", error->message);
    g_dbus_method_invocation_return_gerror (invocation, error);
    return;
  }

  result = g_variant_builder_new (G_VARIANT_TYPE ("as"));
  if (proxies) {
    for (idx = 0; proxies[idx]; idx++) {
      g_variant_builder_add (result, "s", proxies[idx]);
    }
  }

  g_dbus_method_invocation_return_value (invocation,
                                         g_variant_new ("(as)", result));
}

static GVariant *
handle_get_property (GDBusConnection  *connection,
                     const gchar      *sender,
                     const gchar      *object_path,
                     const gchar      *interface_name,
                     const gchar      *property_name,
                     GError          **error,
                     gpointer          user_data)
{
  GVariant *ret = NULL;

  if (g_strcmp0 (property_name, "APIVersion") == 0)
    ret = g_variant_new_string ("1.0");

  return ret;
}

static const GDBusInterfaceVTable interface_vtable = {
  handle_method_call,
  handle_get_property
};

static void
on_bus_acquired (GDBusConnection *connection,
                 const gchar     *name,
                 gpointer         user_data)
{
  g_autoptr (GError) error = NULL;
  PxManager *manager = NULL;

  manager = px_manager_new ();
  g_dbus_connection_register_object (connection,
                                     "/org/libproxy/proxy",
                                     (GDBusInterfaceInfo *)&org_libproxy_proxy_interface,
                                     &interface_vtable,
                                     manager,
                                     g_object_unref,
                                     &error);
  if (error) {
    g_warning ("Could not register dbus object: %s", error->message);
    g_main_loop_quit (user_data);
    return;
  }
}

static void
on_name_lost (GDBusConnection *connection,
              const gchar     *name,
              gpointer         user_data)
{
  if (!connection) {
    g_warning ("Can't connect proxy bus");
    g_main_loop_quit (user_data);
  } else {
    g_error ("Unknown name lost error");
  }
}

int
main (int    argc,
      char **argv)
{
  GMainLoop *loop;

  loop = g_main_loop_new (NULL, FALSE);

  g_bus_own_name (G_BUS_TYPE_SESSION,
                  "org.libproxy.proxy",
                  G_BUS_NAME_OWNER_FLAGS_NONE,
                  on_bus_acquired,
                  NULL,
                  on_name_lost,
                  loop,
                  NULL);

  g_main_loop_run (loop);

  return 0;
}
