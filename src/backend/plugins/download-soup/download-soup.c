/* download-soup.c
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

#include <libpeas/peas.h>
#include <libsoup/soup.h>

#include "download-soup.h"

#include "px-plugin-download.h"
#include "px-manager.h"

static void px_download_iface_init (PxDownloadInterface *iface);
G_MODULE_EXPORT void peas_register_types (PeasObjectModule *module);

G_DEFINE_FINAL_TYPE_WITH_CODE (PxDownloadSoup,
                               px_download_soup,
                               G_TYPE_OBJECT,
                               G_IMPLEMENT_INTERFACE (PX_TYPE_DOWNLOAD, px_download_iface_init))

static void
px_download_soup_init (PxDownloadSoup *self)
{
  self->session = soup_session_new ();
}

static void
px_download_soup_class_init (PxDownloadSoupClass *klass)
{
}

static GBytes *
px_download_soup_download (PxDownload *download,
                           const char *uri)
{
  PxDownloadSoup *self = PX_DOWNLOAD_SOUP (download);
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

static void
px_download_iface_init (PxDownloadInterface *iface)
{
  iface->download = px_download_soup_download;
}

G_MODULE_EXPORT void
peas_register_types (PeasObjectModule *module)
{
  peas_object_module_register_extension_type (module,
                                              PX_TYPE_DOWNLOAD,
                                              PX_DOWNLOAD_TYPE_SOUP);
}
