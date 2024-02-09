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

#include <sys/types.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

#include "netsurf/inttypes.h"
#include "utils/nsoption.h"
#include "utils/log.h"
#include "netsurf/bitmap.h"
#include "netsurf/mouse.h"

#include "atari/bitmap.h"
#include "atari/plot/plot.h"


/*
 * param bpp bits per pixel,
 * param w width of the buffer (in pixel)
 * param h height of the buffer (in pixel)
 * param flags MFDB_FLAG_NOALLOC | MFDB_FLAG_ZEROMEM | MFDB_FLAG_STAND
 * returns size of the fd_addr buffer required or allocated
*/
int init_mfdb(int bpp, int w, int h, uint32_t flags, MFDB * out )
{
	int dststride;
	dststride = MFDB_STRIDE( w );
	int size = MFDB_SIZE( bpp, dststride, h );
	if( bpp > 0 ) {
		if( (flags & MFDB_FLAG_NOALLOC) == 0  ) {
			out->fd_addr = malloc( size );
			if( out->fd_addr == NULL ){
				return( 0 );
			}
			if( (flags & MFDB_FLAG_ZEROMEM) ){
				memset( out->fd_addr, 0, size );
			}
		}
		out->fd_stand = (flags & MFDB_FLAG_STAND) ? 1 : 0;
		out->fd_nplanes = (short)bpp;
		out->fd_r1 = out->fd_r2 = out->fd_r3 = 0;
	} else {
		memset( out, 0, sizeof(MFDB) );
	}
	out->fd_w = dststride;
	out->fd_h = h;
	out->fd_wdwidth = dststride >> 4;
	return( size );
}


/**
 * Create a bitmap.
 *
 * \param  w	width of image in pixels
 * \param  h 	height of image in pixels
 * \param  bpp	number of BYTES per pixel
 * \param  rowstride 	linewidth in bytes
 * \param  state 	a flag word indicating the initial state
 * \param  pixdata 	NULL or an memory address to use as the bitmap pixdata
 * \return an opaque struct bitmap, or NULL on memory exhaustion
 */
static void *atari_bitmap_create_ex( int w, int h, short bpp, int rowstride, enum gui_bitmap_flags flags, void * pixdata )
{
	struct bitmap * bitmap;

	NSLOG(netsurf, INFO,
	  "width %d (rowstride: %d, bpp: %d), height %d, flags %u", w,
	  rowstride, bpp, h, (unsigned)flags);

	if( rowstride == 0) {
		rowstride = bpp * w;
	}

	assert( rowstride >= (w * bpp) );
	bitmap = calloc(1 , sizeof(struct bitmap) );
	if (bitmap) {
		if( pixdata == NULL) {
			bitmap->pixdata = calloc(1, (rowstride * h)+128);
		}
		else {
			bitmap->pixdata = pixdata;
		}

	if (bitmap->pixdata != NULL) {
			bitmap->width = w;
			bitmap->height = h;
			bitmap->opaque = (flags & BITMAP_OPAQUE) ? true : false;
			bitmap->bpp = bpp;
			bitmap->resized = NULL;
			bitmap->rowstride = rowstride;
	} else {
			free(bitmap);
			bitmap=NULL;
			NSLOG(netsurf, INFO, "Out of memory!");
		}
	}
	NSLOG(netsurf, INFO, "bitmap %p", bitmap);
	return bitmap;
}


/* exported interface documented in atari/bitmap.h */
void *atari_bitmap_create(int w, int h, enum gui_bitmap_flags flags)
{
	return atari_bitmap_create_ex( w, h, NS_BMP_DEFAULT_BPP, w * NS_BMP_DEFAULT_BPP, flags, NULL );
}

/**
 * The bitmap image has changed, so flush any persistant cache.
 *
 * \param  bitmap  a bitmap, as returned by bitmap_create()
 */
static void bitmap_modified(void *bitmap)
{
	struct bitmap *bm = bitmap;
	if( bm->resized != NULL ) {
		atari_bitmap_destroy( bm->resized );
		bm->resized = NULL;
	}
	if( bm->converted ){
		if( bm->pixdata != bm->native.fd_addr ){
			free( bm->native.fd_addr );
		}
		bm->native.fd_addr = NULL;
		bm->converted = false;
	}
}


/* exported interface documented in atari/bitmap.h */
void *atari_bitmap_realloc( int w, int h, short bpp, int rowstride, unsigned int state, void * bmp )
{
	struct bitmap * bitmap = bmp;
	int newsize = rowstride * h;

	if( bitmap == NULL ) {
		return( NULL );
	}

	assert( bitmap->pixdata != NULL );
	int oldsize = bitmap->rowstride * bitmap->height;
	bool doalloc = (state & BITMAP_GROW) ? (newsize > oldsize) : (newsize != oldsize);
	if( newsize > oldsize )
		assert( doalloc == true );
	if( doalloc ) {
		// TODO: set red band to a specific value and check the red band
		// on bitmap_destroy()
		bitmap->pixdata = realloc( bitmap->pixdata, newsize + 128 );
		if( bitmap->pixdata == NULL )
			return( NULL );
	}

	if( state & BITMAP_CLEAR ){
		memset( bitmap->pixdata, 0x00, newsize + 128  );
	}

	bitmap->width = w;
	bitmap->height = h;
	bitmap->bpp = bpp;
	bitmap->resized = NULL;
	bitmap->rowstride = rowstride;
	bitmap_modified( bitmap );
	return( bitmap );
}


/**
 * Return a pointer to the pixel data in a bitmap.
 *
 * \param  bitmap  a bitmap, as returned by bitmap_create()
 * \return pointer to the pixel buffer
 *
 * The pixel data is packed as BITMAP_FORMAT, possibly with padding at the end
 * of rows. The width of a row in bytes is given by bitmap_get_rowstride().
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


/* exported interface documented in atari/bitmap.h */
size_t atari_bitmap_buffer_size(void *bitmap)
{
	struct bitmap * bm = bitmap;
	if( bm == NULL )
		return 0;
	return( bm->rowstride * bm->height );
}


/* exported interface documented in atari/bitmap.h */
size_t atari_bitmap_get_rowstride(void *bitmap)
{
	struct bitmap *bm = bitmap;

	if (bitmap == NULL) {
		NSLOG(netsurf, INFO, "NULL bitmap!");
		return 0;
	}
	return bm->rowstride;
}


/* exported interface documented in atari/bitmap.h */
void atari_bitmap_destroy(void *bitmap)
{
	struct bitmap *bm = bitmap;

	if (bitmap == NULL) {
		NSLOG(netsurf, INFO, "NULL bitmap!");
		return;
	}

	if( bm->resized != NULL ) {
		atari_bitmap_destroy(bm->resized);
	}
	if( bm->converted && ( bm->native.fd_addr != bm->pixdata ) ) {
		free( bm->native.fd_addr );
	}
	free(bm->pixdata);
	free(bm);
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


/* exported interface documented in atari/bitmap.h */
bool atari_bitmap_get_opaque(void *bitmap)
{
	struct bitmap *bm = bitmap;

        if (bitmap == NULL) {
                NSLOG(netsurf, INFO, "NULL bitmap!");
                return false;
        }

	return bm->opaque;
}


/* exported interface documented in atari/bitmap.h */
int atari_bitmap_get_width(void *bitmap)
{
	struct bitmap *bm = bitmap;

	if (bitmap == NULL) {
		NSLOG(netsurf, INFO, "NULL bitmap!");
		return 0;
	}

	return(bm->width);
}


/* exported interface documented in atari/bitmap.h */
int atari_bitmap_get_height(void *bitmap)
{
	struct bitmap *bm = bitmap;

	if (bitmap == NULL) {
		NSLOG(netsurf, INFO, "NULL bitmap!");
		return 0;
	}
	return(bm->height);
}

/* exported interface documented in atari/bitmap.h */
bool atari_bitmap_resize(struct bitmap *img, HermesHandle hermes_h,
		HermesFormat *fmt, int nw, int nh)
{
	enum gui_bitmap_flags flags = 0;
	short bpp = img->bpp;
	int stride = atari_bitmap_get_rowstride( img );
	int err;

	if( img->resized != NULL ) {
		if( img->resized->width != nw || img->resized->height != nh ) {
			atari_bitmap_destroy( img->resized );
			img->resized = NULL;
		} else {
			/* the bitmap is already resized */
			return(true);
		}
	}

	/* allocate the mem for resized bitmap */
	if (img->opaque == true) {
		flags |= BITMAP_OPAQUE;
	}
	img->resized = atari_bitmap_create_ex( nw, nh, bpp, nw*bpp, flags, NULL );
	if( img->resized == NULL ) {
			printf("W: %d, H: %d, bpp: %d\n", nw, nh, bpp);
			assert(img->resized);
			return(false);
	}

	/* allocate an converter, only for resizing */
	err = Hermes_ConverterRequest( hermes_h,
			fmt,
			fmt
	);
	if( err == 0 ) {
		return(false);
	}

	err = Hermes_ConverterCopy( hermes_h,
		img->pixdata,
		0,		/* x src coord of top left in pixel coords */
		0,		/* y src coord of top left in pixel coords */
		atari_bitmap_get_width( img ),
		atari_bitmap_get_height( img ),
		stride, 	/* stride as bytes */
		img->resized->pixdata,
		0,		/* x dst coord of top left in pixel coords */
		0,		/* y dst coord of top left in pixel coords */
		nw, nh,
		atari_bitmap_get_rowstride(img->resized) /* stride as bytes */
	);
	if( err == 0 ) {
		atari_bitmap_destroy( img->resized );
		img->resized = NULL;
		return(false);
	}

	return(true);
}

static nserror bitmap_render(struct bitmap *bitmap, struct hlcache_handle *content)
{
	return NSERROR_NOT_IMPLEMENTED;
}

static struct gui_bitmap_table bitmap_table = {
	.create = atari_bitmap_create,
	.destroy = atari_bitmap_destroy,
	.set_opaque = bitmap_set_opaque,
	.get_opaque = atari_bitmap_get_opaque,
	.get_buffer = bitmap_get_buffer,
	.get_rowstride = atari_bitmap_get_rowstride,
	.get_width = atari_bitmap_get_width,
	.get_height = atari_bitmap_get_height,
	.modified = bitmap_modified,
	.render = bitmap_render,
};

struct gui_bitmap_table *atari_bitmap_table = &bitmap_table;

/*
 * Local Variables:
 * c-basic-offset:8
 * End:
 */
