/*
 * Copyright 2010 Ole Loots <ole@monochrom.net>
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
 * Atari bitmap handling implementation.
 */

#ifndef NS_ATARI_BITMAP_H
#define NS_ATARI_BITMAP_H

#include <gem.h>
#include <Hermes/Hermes.h>

#define NS_BMP_DEFAULT_BPP 4

/* Flags for init_mfdb function: */
#define MFDB_FLAG_STAND	  0x01
#define MFDB_FLAG_ZEROMEM 0x02
#define MFDB_FLAG_NOALLOC 0x04

#define BITMAP_SHRINK	  0
#define BITMAP_GROW	  1024 /* Don't realloc when bitmap size shrinks */
#define BITMAP_CLEAR	  2048 /* Zero bitmap memory                     */


/**
 * Calculates MFDB compatible rowstride (in number of bits)
 */
#define MFDB_STRIDE( w ) (((w & 15) != 0) ? (w | 15)+1 : w)


/**
 * Calculate size of an mfdb,
 *
 * \param bpp Bits per pixel.
 * \param stride Word aligned rowstride (width) as returned by MFDB_STRIDE,
 * \param h Height in pixels.
*/
#define MFDB_SIZE( bpp, stride, h ) ( ((stride >> 3) * h) * bpp )

extern struct gui_bitmap_table *atari_bitmap_table;

struct bitmap {
	int width;
	int height;
	uint8_t *pixdata;
	bool opaque;
	short bpp;				/* number of BYTES! per pixel */
	size_t rowstride;
	struct bitmap * resized;
	MFDB native;
	bool converted;
};



/**
 * setup an MFDB struct and allocate memory for it when it is needed.
 *
 * If bpp == 0, this function assumes that the MFDB shall point to the
 *       screen and will not allocate any memory (mfdb.fd_addr == 0).
 *
 * \return 0 when the memory allocation fails (out of memory),
 *         otherwise it returns the size of the mfdb.fd_addr as number
 *         of bytes.
 */
int init_mfdb(int bpp, int w, int h, uint32_t flags, MFDB * out );

/**
 * Create a bitmap.
 *
 * \param  w  width of image in pixels
 * \param  h  width of image in pixels
 * \param  state   a flag word indicating the initial state
 * \return an opaque struct bitmap, or NULL on memory exhaustion
 */
void *atari_bitmap_create(int w, int h, enum gui_bitmap_flags flags);

/**
 * Find the width of a pixel row in bytes.
 *
 * \param  bitmap  a bitmap, as returned by bitmap_create()
 * \return width of a pixel row in the bitmap
 */
size_t atari_bitmap_get_rowstride(void *bitmap);

/**
 * Free a bitmap.
 *
 * \param  bitmap a bitmap, as returned by bitmap_create()
 */
void atari_bitmap_destroy(void *bitmap);

/**
 * Get bitmap width
 *
 * \param  bitmap a bitmap, as returned by bitmap_create()
 */
int atari_bitmap_get_width(void *bitmap);

/**
 * Get bitmap height
 *
 * \param  bitmap a bitmap, as returned by bitmap_create()
 */
int atari_bitmap_get_height(void *bitmap);

/**
 * Gets whether a bitmap should be plotted opaque
 *
 * \param  bitmap  a bitmap, as returned by bitmap_create()
 */
bool atari_bitmap_get_opaque(void *bitmap);

size_t atari_bitmap_buffer_size(void *bitmap);

bool atari_bitmap_resize(struct bitmap *img, HermesHandle hermes_h, HermesFormat *fmt, int nw, int nh);

void *atari_bitmap_realloc( int w, int h, short bpp, int rowstride, unsigned int state, void * bmp );

#endif
