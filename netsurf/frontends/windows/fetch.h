/*
 * Copyright 2014 Vincent Sanders <vince@netsurf-browser.org>
 *
 * This file is part of NetSurf, http://www.netsurf-browser.org/
 *
 * NetSurf is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * NetSurf is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _NETSURF_WINDOWS_FILETYPE_H_
#define _NETSURF_WINDOWS_FILETYPE_H_

/**
 * win32 API fetch operation table
 */
extern struct gui_fetch_table *win32_fetch_table;

/**
 * Translate resource to win32 resource data.
 *
 * Obtains the data for a resource directly
 *
 * \param path The path of the resource to locate.
 * \param data Pointer to recive data into
 * \param data_len Pointer to length of returned data
 * \return NSERROR_OK and the data and length values updated
 *         else appropriate error code.
 */
nserror nsw32_get_resource_data(const char *path, const uint8_t **data_out, size_t *data_len_out);

#endif
