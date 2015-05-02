/*******************************************************************************
 * libproxy - A library for proxy configuration
 * Copyright (C) 2015 Dominique Leuenberger <dimstar@opensuse.org>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 ******************************************************************************/

#include <gio/gio.h>
#include <stdlib.h>
#include <stdio.h>

/* Import libproxy API */
#include <proxy.h>

/* ---------------------------------------------------------------------------------------------------- */

static GDBusNodeInfo *introspection_data = NULL;

/* Introspection data for the service we are exporting */
static const gchar introspection_xml[] =
  "<node>"
  "  <interface name='org.libproxy.proxy'>"
  "    <method name='query'>"
  "      <annotation name='org.libproxy.proxy' value='OnMethod'/>"
  "      <arg type='s' name='url'      direction='in'/>"
  "      <arg type='as' name='response' direction='out'/>"
  "    </method>"
  "  </interface>"
  "</node>";

/* ---------------------------------------------------------------------------------------------------- */

/* proxyFactory object is cached for the entire life-time of the dbus service */
pxProxyFactory *pf;

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

  const gchar *url;
  if (g_strcmp0 (method_name, "query") == 0)
    {
      g_variant_get (parameters, "(&s)", &url);
      gchar **proxies = px_proxy_factory_get_proxies(pf, url);

      GVariantBuilder *result = g_variant_builder_new (G_VARIANT_TYPE ("as"));

      for (int j=0; proxies[j]; j++)
      {
          g_variant_builder_add(result, "&s", proxies[j]);
	  free(proxies[j]);
      }
      g_free (proxies);

      g_dbus_method_invocation_return_value (invocation,
                                             g_variant_new("(as)", result));
    }
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
  GVariant *ret;

  ret = NULL;
  if (g_strcmp0 (property_name, "APIVersion") == 0)
    {
      ret = g_variant_new_string ("1.0");
    }

  return ret;
}

/* for now */
static const GDBusInterfaceVTable interface_vtable =
{
  handle_method_call,
  handle_get_property
};

/* ---------------------------------------------------------------------------------------------------- */

static void
on_bus_acquired (GDBusConnection *connection,
                 const gchar     *name,
                 gpointer         user_data)
{
  guint registration_id;

  registration_id = g_dbus_connection_register_object (connection,
                                                       "/org/libproxy/proxy",
                                                       introspection_data->interfaces[0],
                                                       &interface_vtable,
                                                       NULL,  /* user_data */
                                                       NULL,  /* user_data_free_func */
                                                       NULL); /* GError** */
  g_assert (registration_id > 0);

}

static void
on_name_acquired (GDBusConnection *connection,
                  const gchar     *name,
                  gpointer         user_data)
{
}

static void
on_name_lost (GDBusConnection *connection,
              const gchar     *name,
              gpointer         user_data)
{
  exit (1);
}

int
main (int argc, char *argv[])
{
  guint owner_id;
  GMainLoop *loop;

  /* We are lazy here - we don't want to manually provide
   * the introspection data structures - so we just build
   * them from XML.
   */
  introspection_data = g_dbus_node_info_new_for_xml (introspection_xml, NULL);
  g_assert (introspection_data != NULL);

  owner_id = g_bus_own_name (G_BUS_TYPE_SESSION,
                             "org.libproxy.proxy",
                             G_BUS_NAME_OWNER_FLAGS_NONE,
                             on_bus_acquired,
                             on_name_acquired,
                             on_name_lost,
                             NULL,
                             NULL);

  pf = px_proxy_factory_new();

  loop = g_main_loop_new (NULL, FALSE);
  g_main_loop_run (loop);

  g_bus_unown_name (owner_id);

  g_dbus_node_info_unref (introspection_data);

  /* Destroy the proxy factory object */
  px_proxy_factory_free(pf);

  return 0;
}

