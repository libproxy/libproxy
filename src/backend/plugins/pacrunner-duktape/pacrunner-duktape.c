/* pacrunner-duktape.c
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

#include <unistd.h>
#ifdef __WIN32__
#include <ws2tcpip.h>
#else
#include <netdb.h>
#include <netinet/in.h>
#endif

#include "pacrunner-duktape.h"
#include "pacutils.h"
#include "px-plugin-pacrunner.h"

#include "duktape.h"

struct _PxPacRunnerDuktape {
  GObject parent_instance;
  duk_context *ctx;
};

static void px_pacrunner_iface_init (PxPacRunnerInterface *iface);

G_DEFINE_FINAL_TYPE_WITH_CODE (PxPacRunnerDuktape,
                               px_pacrunner_duktape,
                               G_TYPE_OBJECT,
                               G_IMPLEMENT_INTERFACE (PX_TYPE_PACRUNNER, px_pacrunner_iface_init))


static duk_ret_t
dns_resolve (duk_context *ctx)
{
  const char *hostname = NULL;
  struct addrinfo *info;
  char tmp[INET6_ADDRSTRLEN + 1];

  if (duk_get_top (ctx) != 1) {
    /* Invalid number of arguments */
    return 0;
  }

  /* We do not need to free the string - It's managed by Duktape. */
  hostname = duk_get_string (ctx, 0);
  if (!hostname)
    return 0;

  /* Look it up */
  if (getaddrinfo (hostname, NULL, NULL, &info))
    return 0;

  /* Try for IPv4 */
  if (getnameinfo (info->ai_addr,
                   info->ai_addrlen,
                   tmp,
                   INET6_ADDRSTRLEN + 1,
                   NULL,
                   0,
                   NI_NUMERICHOST)) {
    freeaddrinfo (info);
    duk_push_null (ctx);
    return 1;
  }
  freeaddrinfo (info);

  /* Create the return value */
  duk_push_string (ctx, tmp);

  return 1;
}

static duk_ret_t
my_ip_address (duk_context *ctx)
{
  char hostname[1024];

  hostname[sizeof (hostname) - 1] = '\0';

  if (!gethostname (hostname, sizeof (hostname) - 1)) {
    duk_push_string (ctx, hostname);
    return dns_resolve (ctx);
  }

  return duk_error (ctx, DUK_ERR_ERROR, "Unable to find hostname!");
}

static duk_ret_t
alert (duk_context *ctx)
{
  const char *str = NULL;

  /* do nothing if PX_DEBUG_PACALERT environment is not set */
  if (!getenv ("PX_DEBUG_PACALERT"))
    return 0;

  /* only get first argument of alert() as string */
  str = duk_get_string (ctx, 0);
  if (!str)
    return 0;

  fprintf (stderr, "PAC-alert: %s\n", str);
  return 0;
}

static void
px_pacrunner_duktape_init (PxPacRunnerDuktape *self)
{
  self->ctx = duk_create_heap_default ();
  if (!self->ctx)
    return;

  duk_push_c_function (self->ctx, dns_resolve, 1);
  duk_put_global_string (self->ctx, "dnsResolve");

  duk_push_c_function (self->ctx, my_ip_address, 1);
  duk_put_global_string (self->ctx, "myIpAddress");

  duk_push_c_function (self->ctx, alert, 1);
  duk_put_global_string (self->ctx, "alert");

  duk_push_string (self->ctx, JAVASCRIPT_ROUTINES);
  if (duk_peval_noresult (self->ctx))
    goto error;

  return;

error:
  duk_destroy_heap (self->ctx);
}

static void
px_pacrunner_duktape_dispose (GObject *object)
{
  PxPacRunnerDuktape *self = PX_PACRUNNER_DUKTAPE (object);

  g_clear_pointer (&self->ctx, duk_destroy_heap);

  G_OBJECT_CLASS (px_pacrunner_duktape_parent_class)->dispose (object);
}

static void
px_pacrunner_duktape_class_init (PxPacRunnerDuktapeClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = px_pacrunner_duktape_dispose;
}

static gboolean
px_pacrunner_duktape_set_pac (PxPacRunner *pacrunner,
                              GBytes      *pac_data)
{
  PxPacRunnerDuktape *self = PX_PACRUNNER_DUKTAPE (pacrunner);
  gsize len;
  gconstpointer content = g_bytes_get_data (pac_data, &len);

  duk_push_lstring (self->ctx, content, len);

  if (duk_peval_noresult (self->ctx)) {
    return FALSE;
  }

  return TRUE;
}

static char *
px_pacrunner_duktape_run (PxPacRunner *pacrunner,
                          GUri        *uri)
{
  PxPacRunnerDuktape *self = PX_PACRUNNER_DUKTAPE (pacrunner);
  duk_int_t result;

  duk_get_global_string (self->ctx, "FindProxyForURL");
  duk_push_string (self->ctx, g_uri_to_string (uri));
  duk_push_string (self->ctx, g_uri_get_host (uri));
  result = duk_pcall (self->ctx, 2);

  if (result == 0) {
    const char *proxy = duk_get_string (self->ctx, 0);
    char *proxy_string;

    if (!proxy) {
      duk_pop (self->ctx);
      return g_strdup ("");
    }

    proxy_string = g_strdup (proxy);

    duk_pop (self->ctx);

    return proxy_string;
  }

  duk_pop (self->ctx);
  return g_strdup ("");
}

static void
px_pacrunner_iface_init (PxPacRunnerInterface *iface)
{
  iface->set_pac = px_pacrunner_duktape_set_pac;
  iface->run = px_pacrunner_duktape_run;
}
