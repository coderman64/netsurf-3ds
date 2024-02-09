/*
 * Copyright 2018 Vincent Sanders <vince@netsurf-browser.org>
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

/**
 * \file
 * Fetch operation implementation for win32
 */

#include <stdlib.h>
#include <string.h>

#include "utils/log.h"
#include "utils/file.h"
#include "utils/filepath.h"
#include "content/fetch.h"
#include "netsurf/fetch.h"

#include "windows/fetch.h"
#include "windows/gui.h"

/**
 * determine the MIME type of a local file.
 *
 * \param unix_path The unix style path to the file.
 * \return The mime type of the file.
 */
static const char *fetch_filetype(const char *unix_path)
{
	int l;
	NSLOG(netsurf, INFO, "unix path %s", unix_path);
	l = strlen(unix_path);
	if (2 < l && strcasecmp(unix_path + l - 3, "css") == 0)
		return "text/css";
	if (2 < l && strcasecmp(unix_path + l - 3, "jpg") == 0)
		return "image/jpeg";
	if (3 < l && strcasecmp(unix_path + l - 4, "jpeg") == 0)
		return "image/jpeg";
	if (2 < l && strcasecmp(unix_path + l - 3, "gif") == 0)
		return "image/gif";
	if (2 < l && strcasecmp(unix_path + l - 3, "png") == 0)
		return "image/png";
	if (2 < l && strcasecmp(unix_path + l - 3, "jng") == 0)
		return "image/jng";
	if (2 < l && strcasecmp(unix_path + l - 3, "svg") == 0)
		return "image/svg";
	if (2 < l && strcasecmp(unix_path + l - 3, "bmp") == 0)
		return "image/x-ms-bmp";
	return "text/html";
}

/**
 * Translate resource to full win32 url.
 *
 * Transforms a resource: path into a full URL. The returned URL
 * is used as the target for a redirect. The caller takes ownership of
 * the returned nsurl including unrefing it when finished with it.
 *
 * \param path The path of the resource to locate.
 * \return A string containing the full URL of the target object or
 *         NULL if no suitable resource can be found.
 */
static nsurl *nsw32_get_resource_url(const char *path)
{
	char buf[PATH_MAX];
	nsurl *url = NULL;

	netsurf_path_to_nsurl(filepath_sfind(G_resource_pathv, buf, path), &url);

	return url;
}


/* exported interface documented in windows/fetch.h */
nserror
nsw32_get_resource_data(const char *path,
			const uint8_t **data_out,
			size_t *data_len_out)
{
	HRSRC reshandle;
	HGLOBAL datahandle;
	uint8_t *data;
	DWORD data_len;

	reshandle = FindResource(NULL, path, "USER");
	if (reshandle == NULL) {
		return NSERROR_NOT_FOUND;
	}

	data_len = SizeofResource(NULL, reshandle);
	if (data_len == 0) {
		return NSERROR_NOT_FOUND;
	}

	datahandle = LoadResource(NULL, reshandle);
	if (datahandle == NULL) {
		return NSERROR_NOT_FOUND;
	}
	data = LockResource(datahandle);
	if (data == NULL) {
		return NSERROR_NOT_FOUND;
	}

	*data_out = data;
	*data_len_out = data_len;

	return NSERROR_OK;
}


/** win32 fetch operation table */
static struct gui_fetch_table fetch_table = {
	.filetype = fetch_filetype,

	.get_resource_url = nsw32_get_resource_url,
	.get_resource_data = nsw32_get_resource_data,
};

struct gui_fetch_table *win32_fetch_table = &fetch_table;
