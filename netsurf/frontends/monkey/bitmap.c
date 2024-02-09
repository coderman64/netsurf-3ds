/*
 * Copyright 2011 Daniel Silverstone <dsilvers@digital-scurf.org>
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

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "utils/errors.h"
#include "netsurf/bitmap.h"

#include "monkey/output.h"
#include "monkey/bitmap.h"

struct bitmap {
	void *ptr;
	size_t rowstride;
	int width;
	int height;
	bool opaque;
};

static void *bitmap_create(int width, int height, enum gui_bitmap_flags flags)
{
	struct bitmap *ret = calloc(sizeof(*ret), 1);
	if (ret == NULL)
		return NULL;

	ret->width = width;
	ret->height = height;
	ret->opaque = (flags & BITMAP_OPAQUE) == BITMAP_OPAQUE;

	ret->ptr = calloc(width, height * 4);

	if (ret->ptr == NULL) {
		free(ret);
		return NULL;
	}

	return ret;
}

static void bitmap_destroy(void *bitmap)
{
	struct bitmap *bmap = bitmap;
	free(bmap->ptr);
	free(bmap);
}

static void bitmap_set_opaque(void *bitmap, bool opaque)
{
	struct bitmap *bmap = bitmap;

	bmap->opaque = opaque;
}

static bool bitmap_get_opaque(void *bitmap)
{
	struct bitmap *bmap = bitmap;

	return bmap->opaque;
}

static unsigned char *bitmap_get_buffer(void *bitmap)
{
	struct bitmap *bmap = bitmap;
  
	return (unsigned char *)(bmap->ptr);
}

static size_t bitmap_get_rowstride(void *bitmap)
{
	struct bitmap *bmap = bitmap;
	return bmap->width * 4;
}

static void bitmap_modified(void *bitmap)
{
	return;
}

static int bitmap_get_width(void *bitmap)
{
	struct bitmap *bmap = bitmap;
	return bmap->width;
}

static int bitmap_get_height(void *bitmap)
{
	struct bitmap *bmap = bitmap;
	return bmap->height;
}

static nserror bitmap_render(struct bitmap *bitmap,
			     struct hlcache_handle *content)
{
	moutf(MOUT_GENERIC, "BITMAP RENDER");
	return NSERROR_OK;
}

static struct gui_bitmap_table bitmap_table = {
	.create = bitmap_create,
	.destroy = bitmap_destroy,
	.set_opaque = bitmap_set_opaque,
	.get_opaque = bitmap_get_opaque,
	.get_buffer = bitmap_get_buffer,
	.get_rowstride = bitmap_get_rowstride,
	.get_width = bitmap_get_width,
	.get_height = bitmap_get_height,
	.modified = bitmap_modified,
	.render = bitmap_render,
};

struct gui_bitmap_table *monkey_bitmap_table = &bitmap_table;
