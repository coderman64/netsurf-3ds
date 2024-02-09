/*
 * Copyright 2008 Vincent Sanders <vince@simtec.co.uk>
 * Copyright 2009 Mark Benjamin <netsurf-browser.org.MarkBenjamin@dfgh.net>
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
 * win32 implementation of the bitmap operations.
 */

#include "utils/config.h"

#include <inttypes.h>
#include <sys/types.h>
#include <string.h>
#include <windows.h>

#include "utils/log.h"
#include "netsurf/bitmap.h"
#include "netsurf/plotters.h"
#include "netsurf/content.h"

#include "windows/plot.h"
#include "windows/bitmap.h"

/**
 * Create a bitmap.
 *
 * \param  width   width of image in pixels
 * \param  height  height of image in pixels
 * \param  state   flags   flags for bitmap creation
 * \return an opaque struct bitmap, or NULL on memory exhaustion
 */
static void *win32_bitmap_create(int width, int height, enum gui_bitmap_flags flags)
{
	struct bitmap *bitmap;
	BITMAPV5HEADER *pbmi;
	HBITMAP windib;
	uint8_t *pixdata;

	NSLOG(netsurf, INFO, "width %d, height %d, flags %u", width, height,
	      (unsigned)flags);

	pbmi = calloc(1, sizeof(BITMAPV5HEADER));
	if (pbmi == NULL) {
		return NULL;
	}

	pbmi->bV5Size = sizeof(BITMAPV5HEADER);
	pbmi->bV5Width = width;
	pbmi->bV5Height = -height;
	pbmi->bV5Planes = 1;
	pbmi->bV5BitCount = 32;
	pbmi->bV5Compression = BI_BITFIELDS;

	pbmi->bV5RedMask = 0xff; /* red mask */
	pbmi->bV5GreenMask = 0xff00; /* green mask */
	pbmi->bV5BlueMask = 0xff0000; /* blue mask */
	pbmi->bV5AlphaMask = 0xff000000; /* alpha mask */

	windib = CreateDIBSection(NULL, (BITMAPINFO *)pbmi, DIB_RGB_COLORS, (void **)&pixdata, NULL, 0);

	if (windib == NULL) {
		free(pbmi);
		return NULL;
	}

	bitmap = calloc(1 , sizeof(struct bitmap));
	if (bitmap == NULL) {
		DeleteObject(windib);
		free(pbmi);
		return NULL;
	}

	bitmap->width = width;
	bitmap->height = height;
	bitmap->windib = windib;
	bitmap->pbmi = pbmi;
	bitmap->pixdata = pixdata;
	if ((flags & BITMAP_OPAQUE) != 0) {
		bitmap->opaque = true;
	} else {
		bitmap->opaque = false;
	}

	NSLOG(netsurf, INFO, "bitmap %p", bitmap);

	return bitmap;
}


/**
 * Return a pointer to the pixel data in a bitmap.
 *
 * The pixel data is packed as BITMAP_FORMAT, possibly with padding at the end
 * of rows. The width of a row in bytes is given by bitmap_get_rowstride().
 *
 * \param  bitmap  a bitmap, as returned by bitmap_create()
 * \return pointer to the pixel buffer
 */
static unsigned char *bitmap_get_buffer(void *bitmap)
{
	struct bitmap *bm = bitmap;
	if (bitmap == NULL) {
		NSLOG(netsurf, INFO, "NULL bitmap!");
		return NULL;
	}

	return bm->pixdata;
}


/**
 * Find the width of a pixel row in bytes.
 *
 * \param  bitmap  a bitmap, as returned by bitmap_create()
 * \return width of a pixel row in the bitmap
 */
static size_t bitmap_get_rowstride(void *bitmap)
{
	struct bitmap *bm = bitmap;

	if (bitmap == NULL) {
		NSLOG(netsurf, INFO, "NULL bitmap!");
		return 0;
	}

	return (bm->width) * 4;
}


/**
 * Free a bitmap.
 *
 * \param  bitmap  a bitmap, as returned by bitmap_create()
 */
void win32_bitmap_destroy(void *bitmap)
{
	struct bitmap *bm = bitmap;

	if (bitmap == NULL) {
		NSLOG(netsurf, INFO, "NULL bitmap!");
		return;
	}

	DeleteObject(bm->windib);
	free(bm->pbmi);
	free(bm);
}


/**
 * The bitmap image has changed, so flush any persistant cache.
 *
 * \param  bitmap  a bitmap, as returned by bitmap_create()
 */
static void bitmap_modified(void *bitmap) {
}

/**
 * Sets whether a bitmap should be plotted opaque
 *
 * \param  bitmap  a bitmap, as returned by bitmap_create()
 * \param  opaque  whether the bitmap should be plotted opaque
 */
static void bitmap_set_opaque(void *bitmap, bool opaque)
{
	struct bitmap *bm = bitmap;

	if (bitmap == NULL) {
		NSLOG(netsurf, INFO, "NULL bitmap!");
		return;
	}

	NSLOG(netsurf, INFO, "setting bitmap %p to %s", bm,
	      opaque ? "opaque" : "transparent");
	bm->opaque = opaque;
}


/**
 * Gets whether a bitmap should be plotted opaque
 *
 * \param  bitmap  a bitmap, as returned by bitmap_create()
 */
static bool bitmap_get_opaque(void *bitmap)
{
	struct bitmap *bm = bitmap;

	if (bitmap == NULL) {
		NSLOG(netsurf, INFO, "NULL bitmap!");
		return false;
	}

	return bm->opaque;
}

static int bitmap_get_width(void *bitmap)
{
	struct bitmap *bm = bitmap;

	if (bitmap == NULL) {
		NSLOG(netsurf, INFO, "NULL bitmap!");
		return 0;
	}

	return(bm->width);
}

static int bitmap_get_height(void *bitmap)
{
	struct bitmap *bm = bitmap;

	if (bitmap == NULL) {
		NSLOG(netsurf, INFO, "NULL bitmap!");
		return 0;
	}

	return(bm->height);
}

struct bitmap *bitmap_scale(struct bitmap *prescale, int width, int height)
{
	struct bitmap *ret = malloc(sizeof(struct bitmap));
	int i, ii, v, vv;
	uint32_t *retpixdata, *inpixdata; /* 4 byte types for quicker
					   * transfer */
	if (ret == NULL)
		return NULL;

	retpixdata = malloc(width * height * 4);
	if (retpixdata == NULL) {
		free(ret);
		return NULL;
	}

	inpixdata = (uint32_t *)prescale->pixdata;
	ret->pixdata = (uint8_t *)retpixdata;
	ret->height = height;
	ret->width = width;
	for (i = 0; i < height; i++) {
		v = i * width;
		vv = (int)((i * prescale->height) / height) * prescale->width;
		for (ii = 0; ii < width; ii++) {
			retpixdata[v + ii] = inpixdata[vv + (int)
						       ((ii * prescale->width) / width)];
		}
	}
	return ret;

}


static nserror
bitmap_render(struct bitmap *bitmap, struct hlcache_handle *content)
{
	int width;
	int height;
	HDC hdc, bufferdc, minidc;
	struct bitmap *fsbitmap;
	struct redraw_context ctx = {
		.interactive = false,
		.background_images = true,
		.plot = &win_plotters
	};

	width = min(max(content_get_width(content), bitmap->width), 1024);
	height = ((width * bitmap->height) + (bitmap->width / 2)) /
		bitmap->width;

	NSLOG(netsurf, INFO, "bitmap %p for content %p width %d, height %d",
	      bitmap, content, width, height);

	/* create two memory device contexts to put the bitmaps in */
	bufferdc = CreateCompatibleDC(NULL);
	if ((bufferdc == NULL)) {
		return NSERROR_NOMEM;
	}

	minidc = CreateCompatibleDC(NULL);
	if ((minidc == NULL)) {
		DeleteDC(bufferdc);
		return NSERROR_NOMEM;
	}

	/* create a full size bitmap and plot into it */
	fsbitmap = win32_bitmap_create(width, height, BITMAP_CLEAR | BITMAP_OPAQUE);

	SelectObject(bufferdc, fsbitmap->windib);

	hdc = plot_hdc;
	plot_hdc = bufferdc;
	/* render the content  */
	content_scaled_redraw(content, width, height, &ctx);
	plot_hdc = hdc;

	/* scale bitmap bufferbm into minibm */
	SelectObject(minidc, bitmap->windib);

	bitmap->opaque = true;

	StretchBlt(minidc, 0, 0, bitmap->width, bitmap->height, bufferdc, 0, 0, width, height, SRCCOPY);

	DeleteDC(bufferdc);
	DeleteDC(minidc);
	win32_bitmap_destroy(fsbitmap);

	return NSERROR_OK;
}

static struct gui_bitmap_table bitmap_table = {
	.create = win32_bitmap_create,
	.destroy = win32_bitmap_destroy,
	.set_opaque = bitmap_set_opaque,
	.get_opaque = bitmap_get_opaque,
	.get_buffer = bitmap_get_buffer,
	.get_rowstride = bitmap_get_rowstride,
	.get_width = bitmap_get_width,
	.get_height = bitmap_get_height,
	.modified = bitmap_modified,
	.render = bitmap_render,
};

struct gui_bitmap_table *win32_bitmap_table = &bitmap_table;
