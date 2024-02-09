/*
 * Copyright 2004 James Bursa <bursa@users.sourceforge.net>
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

#ifndef _NETSURF_RISCOS_BITMAP_H_
#define _NETSURF_RISCOS_BITMAP_H_

#include "netsurf/bitmap.h"

struct osspriteop_area;
struct osspriteop_header;
struct hlcache_handle;
struct bitmap;

/** bitmap operations table */
extern struct gui_bitmap_table *riscos_bitmap_table;

/** save with full alpha channel (if not opaque) */
#define BITMAP_SAVE_FULL_ALPHA	(1 << 0)

/**
 * RISC OS wimp toolkit bitmap.
 */
struct bitmap {
	int width; /**< width of bitmap */
	int height; /**< height of bitmap */

	bool opaque; /**< Whether the bitmap is opaque. */
	bool clear;  /**< Whether the bitmap should be initialised to zeros. */

	struct osspriteop_area *sprite_area; /**< Uncompressed data, or NULL */
};

/**
 * Convert bitmap to 8bpp sprite.
 *
 * \param bitmap the bitmap to convert.
 * \return The converted sprite.
 */
struct osspriteop_area *riscos_bitmap_convert_8bpp(struct bitmap *bitmap);

/**
 * Render content into bitmap.
 *
 * \param bitmap the bitmap to draw to
 * \param content content structure to render
 * \return true on success and bitmap updated else false
 */
nserror riscos_bitmap_render(struct bitmap *bitmap, struct hlcache_handle *content);

/**
 * Overlay a sprite onto the given bitmap
 *
 * \param bitmap  bitmap object
 * \param s       8bpp sprite to be overlayed onto bitmap
 */
void riscos_bitmap_overlay_sprite(struct bitmap *bitmap, const struct osspriteop_header *s);

/**
 * Create a bitmap.
 *
 * \param  width   width of image in pixels
 * \param  height  height of image in pixels
 * \param  flags   flags for bitmap creation.
 * \return an opaque struct bitmap, or NULL on memory exhaustion
 */
void *riscos_bitmap_create(int width, int height, enum gui_bitmap_flags flags);

/**
 * Free a bitmap.
 *
 * \param  vbitmap  a bitmap, as returned by bitmap_create()
 */
void riscos_bitmap_destroy(void *vbitmap);

/**
 * Return a pointer to the pixel data in a bitmap.
 *
 * The pixel data is packed as BITMAP_FORMAT, possibly with padding at
 * the end of rows. The width of a row in bytes is given by
 * riscos_bitmap_get_rowstride().
 *
 * \param vbitmap A bitmap as returned by riscos_bitmap_create()
 * \return pointer to the pixel buffer
 */
unsigned char *riscos_bitmap_get_buffer(void *vbitmap);

/**
 * Gets whether a bitmap should be plotted opaque
 *
 * \param vbitmap A bitmap, as returned by riscos_bitmap_create()
 */
bool riscos_bitmap_get_opaque(void *vbitmap);

/**
 * Save a bitmap in the platform's native format.
 *
 * \param  vbitmap  a bitmap, as returned by bitmap_create()
 * \param  path	   pathname for file
 * \param  flags   modify the behaviour of the save
 * \return true on success, false on error and error reported
 */
bool riscos_bitmap_save(void *vbitmap, const char *path, unsigned flags);

#endif
