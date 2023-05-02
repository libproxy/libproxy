/* proxy.c
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

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "proxy.h"

void print_proxies (char **proxies);

/**
 * Prints an array of proxies. Proxies are space separated.
 * @proxies an array containing the proxies returned by libproxy.
 */
void
print_proxies (char **proxies)
{
  int j;

  if (!proxies) {
    printf ("\n");
    return;
  }

  for (j = 0; proxies[j]; j++)
    printf ("%s%s", proxies[j], proxies[j + 1] ? " " : "\n");
}

int
main (int    argc,
      char **argv)
{
  int i;
  char url[102400];       /* Should be plently long for most URLs */
  char **proxies;

  /* Create the proxy factory object */
  pxProxyFactory *pf = px_proxy_factory_new ();
  if (!pf) {
    fprintf (stderr, "An unknown error occurred!\n");
    return 1;
  }

  /* User entered some arguments on startup. skip interactive */
  if (argc > 1) {
    for (i = 1; i < argc; i++) {
      /*
       * Get an array of proxies to use. These should be used
       * in the order returned. Only move on to the next proxy
       * if the first one fails (etc).
       */
      proxies = px_proxy_factory_get_proxies (pf, argv[i]);
      print_proxies (proxies);
      px_proxy_factory_free_proxies (proxies);
    }
  }
  /* Interactive mode */
  else {
    /* For each URL we read on STDIN, get the proxies to use */
    for (url[0] = '\0'; fgets (url, 102400, stdin) != NULL;) {
      if (url[strlen (url) - 1] == '\n') url[strlen (url) - 1] = '\0';

      /*
       * Get an array of proxies to use. These should be used
       * in the order returned. Only move on to the next proxy
       * if the first one fails (etc).
       */
      proxies = px_proxy_factory_get_proxies (pf, url);
      print_proxies (proxies);
      px_proxy_factory_free_proxies (proxies);
    }
  }
  /* Destroy the proxy factory object */
  px_proxy_factory_free (pf);
  return 0;
}
