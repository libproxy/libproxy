/* pacrunner-duktape-test.c
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "px-plugin-pacrunner.h"
#include "plugins/pacrunner-duktape/pacrunner-duktape.h"

#include <gio/gio.h>

static void
test_pac_state_reset (void)
{
  g_autoptr (PxPacRunnerDuktape) pacrunner = NULL;
  g_autoptr (GBytes) pac_a = NULL;
  g_autoptr (GBytes) pac_b = NULL;
  g_autoptr (GUri) uri = NULL;
  g_autofree char *proxy = NULL;
  PxPacRunnerInterface *iface;

  const char *pac_a_script =
    "globalThis.__lp_taint = 'set-by-pac-a';"
    "function FindProxyForURL(url, host) {"
    "  return 'DIRECT';"
    "}";

  const char *pac_b_script =
    "function FindProxyForURL(url, host) {"
    "  return (typeof __lp_taint !== 'undefined')"
    "    ? 'PROXY 127.0.0.1:9'"
    "    : 'DIRECT';"
    "}";

  pacrunner = g_object_new (PX_PACRUNNER_TYPE_DUKTAPE, NULL);
  iface = PX_PAC_RUNNER_GET_IFACE (pacrunner);

  pac_a = g_bytes_new_static (pac_a_script, strlen (pac_a_script));
  g_assert_true (iface->set_pac (PX_PAC_RUNNER (pacrunner), pac_a));

  pac_b = g_bytes_new_static (pac_b_script, strlen (pac_b_script));
  g_assert_true (iface->set_pac (PX_PAC_RUNNER (pacrunner), pac_b));

  uri = g_uri_parse ("https://www.example.com", G_URI_FLAGS_NONE, NULL);
  proxy = iface->run (PX_PAC_RUNNER (pacrunner), uri);

  g_assert_cmpstr (proxy, ==, "DIRECT");
}

int
main (int    argc,
      char **argv)
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/pacrunner/duktape/state-reset", test_pac_state_reset);

  return g_test_run ();
}