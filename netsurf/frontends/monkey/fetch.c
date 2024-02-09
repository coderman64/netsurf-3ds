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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>

#include "utils/errors.h"
#include "utils/file.h"
#include "utils/nsurl.h"
#include "utils/filepath.h"
#include "netsurf/fetch.h"

#include "monkey/filetype.h"
#include "monkey/fetch.h"

extern char **respaths;


static nsurl *gui_get_resource_url(const char *path)
{
	char buf[PATH_MAX];
	nsurl *url = NULL;

	netsurf_path_to_nsurl(filepath_sfind(respaths, buf, path), &url);

	return url;
}

static struct gui_fetch_table fetch_table = {
	.filetype = monkey_fetch_filetype,

	.get_resource_url = gui_get_resource_url,
};

struct gui_fetch_table *monkey_fetch_table = &fetch_table;
