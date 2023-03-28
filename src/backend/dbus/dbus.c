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

static gboolean replace;
static gboolean use_system;

static GApplication *app;

const GOptionEntry options[] = {
  { "replace", 'r', 0, G_OPTION_ARG_NONE, &replace, "Replace running daemon.", NULL },
  { "system", 's', 0, G_OPTION_ARG_NONE, &use_system, "Use system bus.", NULL },
  { NULL }
};

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

  g_application_hold (app);
  if (g_strcmp0 (method_name, "GetProxiesFor") != 0) {
    g_warning ("Invalid method name '%s', aborting.", method_name);
    g_dbus_method_invocation_return_error (invocation,
                                           PX_MANAGER_ERROR,
                                           PX_MANAGER_ERROR_UNKNOWN_METHOD,
                                           "Unknown method");
    g_application_release (app);
    return;
  }

  g_variant_get (parameters, "(&s)", &url);

  proxies = px_manager_get_proxies_sync (manager, url, &error);
  if (error) {
    g_warning ("Could not query proxy servers: %s", error->message);
    g_dbus_method_invocation_return_gerror (invocation, error);
    g_application_release (app);
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
  g_application_release (app);
}

static const GDBusInterfaceVTable interface_vtable = {
  handle_method_call,
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
  g_application_release (user_data);

  if (error) {
    g_warning ("Could not register dbus object: %s", error->message);
    g_application_quit (user_data);
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
    g_application_quit (user_data);
  } else {
    g_warning ("Unknown name lost error");
    g_application_quit (user_data);
  }
}

static void
activate (GApplication *application)
{
  GBusNameOwnerFlags flags;

  flags = G_BUS_NAME_OWNER_FLAGS_ALLOW_REPLACEMENT;
  if (replace)
    flags |= G_BUS_NAME_OWNER_FLAGS_REPLACE;

  g_bus_own_name (use_system ? G_BUS_TYPE_SYSTEM : G_BUS_TYPE_SESSION,
                  "org.libproxy.proxy",
                  flags,
                  on_bus_acquired,
                  NULL,
                  on_name_lost,
                  app,
                  NULL);

  g_application_hold (app);
}

int
main (int    argc,
      char **argv)
{
  GOptionContext *context;
  g_autoptr (GError) error = NULL;

  replace = FALSE;
  use_system = FALSE;

  context = g_option_context_new ("");
  g_option_context_set_summary (context, "Libproxy D-Bus Service");
  g_option_context_add_main_entries (context, options, "libproxy");

  if (!g_option_context_parse (context, &argc, &argv, &error)) {
    g_printerr ("%s: %s", g_get_application_name (), error->message);
    g_printerr ("\n");
    g_printerr ("Try \"%s --help\" for more information.",
                g_get_prgname ());
    g_printerr ("\n");
    g_option_context_free (context);
    return 1;
  }

  app = g_application_new ("org.libproxy.proxy-service",
#if GLIB_CHECK_VERSION (2, 73, 0)
                           G_APPLICATION_DEFAULT_FLAGS
#else
                           G_APPLICATION_FLAGS_NONE
#endif
                           );

  g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);

  /* Set application timeout to 60 seconds */
  g_application_set_inactivity_timeout (app, 60000);

  return g_application_run (app, argc, argv);
}
