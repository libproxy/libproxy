/* download-soup.h
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

#pragma once

#include <glib.h>
#include <libpeas/peas.h>

G_BEGIN_DECLS

#define PX_DOWNLOAD_TYPE_SOUP         (px_download_soup_get_type ())

G_DECLARE_FINAL_TYPE (PxDownloadSoup, px_download_soup, PX, DOWNLOAD_SOUP, GObject)

struct _PxDownloadSoup {
  GObject parent_instance;

  SoupSession *session;
};

G_END_DECLS


