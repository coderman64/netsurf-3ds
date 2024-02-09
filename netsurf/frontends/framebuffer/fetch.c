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

/** \file
 * Interfaces for fetch table.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "utils/nsurl.h"
#include "utils/log.h"
#include "utils/filepath.h"
#include "utils/file.h"
#include "netsurf/fetch.h"

#include "framebuffer/findfile.h"
#include "framebuffer/fetch.h"


/**
 * Translate resource to full url.
 *
 * Transforms a resource: path into a full URL. The returned URL
 * is used as the target for a redirect. The caller takes ownership of
 * the returned nsurl including unrefing it when finished with it.
 *
 * \param path The path of the resource to locate.
 * \return A string containing the full URL of the target object or
 *         NULL if no suitable resource can be found.
 */
static nsurl *get_resource_url(const char *path)
{
	char buf[PATH_MAX];
	nsurl *url = NULL;

	if (strcmp(path, "favicon.ico") == 0)
		path = "favicon.png";

	netsurf_path_to_nsurl(filepath_sfind(respaths, buf, path), &url);

	return url;
}

/**
 * filetype -- determine the MIME type of a local file
 */
static const char *fetch_filetype(const char *unix_path)
{
	int l;
	NSLOG(netsurf, INFO, "unix path %s", unix_path);
	l = strlen(unix_path);
	if (2 < l && strcasecmp(unix_path + l - 3, "css") == 0)
		return "text/css";
	if (2 < l && strcasecmp(unix_path + l - 3, "f79") == 0)
		return "text/css";
	if (2 < l && strcasecmp(unix_path + l - 3, "jpg") == 0)
		return "image/jpeg";
	if (3 < l && strcasecmp(unix_path + l - 4, "jpeg") == 0)
		return "image/jpeg";
	if (2 < l && strcasecmp(unix_path + l - 3, "gif") == 0)
		return "image/gif";
	if (2 < l && strcasecmp(unix_path + l - 3, "png") == 0)
		return "image/png";
	if (2 < l && strcasecmp(unix_path + l - 3, "b60") == 0)
		return "image/png";
	if (2 < l && strcasecmp(unix_path + l - 3, "jng") == 0)
		return "image/jng";
	if (2 < l && strcasecmp(unix_path + l - 3, "svg") == 0)
		return "image/svg";
	if (2 < l && strcasecmp(unix_path + l - 3, "bmp") == 0)
		return "image/bmp";
	return "text/html";
}

/* table for fetch operations */
static struct gui_fetch_table fetch_table = {
	.filetype = fetch_filetype,

	.get_resource_url = get_resource_url,
};

struct gui_fetch_table *framebuffer_fetch_table = &fetch_table;
