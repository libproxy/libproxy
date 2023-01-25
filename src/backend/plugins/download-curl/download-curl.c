/* download-curl.c
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
#include <curl/curl.h>

#include "download-curl.h"

#include "px-plugin-download.h"
#include "px-manager.h"

static void px_download_iface_init (PxDownloadInterface *iface);
G_MODULE_EXPORT void peas_register_types (PeasObjectModule *module);

G_DEFINE_FINAL_TYPE_WITH_CODE (PxDownloadCurl,
                               px_download_curl,
                               G_TYPE_OBJECT,
                               G_IMPLEMENT_INTERFACE (PX_TYPE_DOWNLOAD, px_download_iface_init))

static void
px_download_curl_init (PxDownloadCurl *self)
{
  self->curl = curl_easy_init ();
}

static void
px_download_curl_class_init (PxDownloadCurlClass *klass)
{
}

static size_t
store_data (void   *contents,
            size_t  size,
            size_t  nmemb,
            void   *user_pointer)
{
  GString *string = user_pointer;
  size_t real_size = size * nmemb;

  g_string_append_len (string, contents, real_size);
  return real_size;
}

static GBytes *
px_download_curl_download (PxDownload *download, const char *uri)
{
  PxDownloadCurl *self = PX_DOWNLOAD_CURL (download);
  g_autoptr (GBytes) bytes = NULL;
  CURLcode res;
  g_autoptr (GString) data = g_string_new ("");
  const char *url = uri;

  if (g_str_has_prefix (url, "pac+"))
    url += 4;

  curl_easy_setopt (self->curl, CURLOPT_URL, url);
  curl_easy_setopt (self->curl, CURLOPT_WRITEFUNCTION, store_data);
  curl_easy_setopt (self->curl, CURLOPT_WRITEDATA, data);

  res = curl_easy_perform (self->curl);
  if (res != CURLE_OK) {
    g_debug ("Could not download data: %s", curl_easy_strerror (res));
    return NULL;
  }

  return g_steal_pointer (&bytes);
}

static void
px_download_iface_init (PxDownloadInterface *iface)
{
  iface->download = px_download_curl_download;
}

G_MODULE_EXPORT void
peas_register_types (PeasObjectModule *module)
{
  peas_object_module_register_extension_type (module,
                                              PX_TYPE_DOWNLOAD,
                                              PX_DOWNLOAD_TYPE_CURL);
}
