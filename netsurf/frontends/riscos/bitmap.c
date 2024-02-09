/*
 * Copyright 2004 James Bursa <bursa@users.sourceforge.net>
 * Copyright 2005 Richard Wilson <info@tinct.net>
 * Copyright 2008 Adrian Lees <adrianl@users.sourceforge.net>
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
 * RISC OS implementation of bitmap operations.
 *
 * This implements the interface given by image/bitmap.h using RISC OS
 * sprites.
 */

#include <assert.h>
#include <stdbool.h>
#include <string.h>
#include <swis.h>
#include <rufl.h>
#include <unixlib/local.h>
#include <oslib/colourtrans.h>
#include <oslib/osfile.h>
#include <oslib/osfind.h>
#include <oslib/osgbpb.h>
#include <oslib/osspriteop.h>
#include <oslib/wimp.h>

#include "utils/nsoption.h"
#include "utils/filename.h"
#include "utils/log.h"
#include "utils/messages.h"
#include "netsurf/plotters.h"
#include "netsurf/content.h"

#include "riscos/gui.h"
#include "riscos/image.h"
#include "riscos/palettes.h"
#include "riscos/content-handlers/sprite.h"
#include "riscos/tinct.h"
#include "riscos/bitmap.h"

/** Colour in the overlay sprite that allows the bitmap to show through */
#define OVERLAY_INDEX 0xfe

/** Size of buffer used when constructing mask data to be saved */
#define SAVE_CHUNK_SIZE 4096

/**
 * Whether we can use 32bpp sprites
 */
static int thumbnail_32bpp_available = -1;

/**
 * Sprite output context saving
 */
struct thumbnail_save_area {
	osspriteop_save_area *save_area;
	int context1;
	int context2;
	int context3;
};

/**
 * Initialise a bitmaps sprite area.
 *
 * \param  bitmap  the bitmap to initialise
 * \return true if bitmap initialised else false.
 */

static bool bitmap_initialise(struct bitmap *bitmap)
{
	unsigned int area_size;
	osspriteop_area *sprite_area;
	osspriteop_header *sprite;

	assert(!bitmap->sprite_area);

	area_size = 16 + 44 + bitmap->width * bitmap->height * 4;
	if (bitmap->clear)
		bitmap->sprite_area = calloc(1, area_size);
	else
		bitmap->sprite_area = malloc(area_size);

	if (!bitmap->sprite_area)
		return false;

	/* area control block */
	sprite_area = bitmap->sprite_area;
	sprite_area->size = area_size;
	sprite_area->sprite_count = 1;
	sprite_area->first = 16;
	sprite_area->used = area_size;

	/* sprite control block */
	sprite = (osspriteop_header *) (sprite_area + 1);
	sprite->size = area_size - 16;
	memset(sprite->name, 0x00, 12);
	strncpy(sprite->name, "bitmap", 12);
	sprite->width = bitmap->width - 1;
	sprite->height = bitmap->height - 1;
	sprite->left_bit = 0;
	sprite->right_bit = 31;
	sprite->image = sprite->mask = 44;
	sprite->mode = tinct_SPRITE_MODE;

	return true;
}


/* exported interface documented in riscos/bitmap.h */
void *riscos_bitmap_create(int width, int height, enum gui_bitmap_flags flags)
{
	struct bitmap *bitmap;

	if (width == 0 || height == 0)
		return NULL;

	bitmap = calloc(1, sizeof(struct bitmap));
	if (!bitmap)
		return NULL;
	bitmap->width = width;
	bitmap->height = height;
	bitmap->opaque = (flags & BITMAP_OPAQUE) == BITMAP_OPAQUE;
	bitmap->clear = (flags & BITMAP_CLEAR) == BITMAP_CLEAR;

	return bitmap;
}


/* exported interface documented in riscos/bitmap.h */
unsigned char *riscos_bitmap_get_buffer(void *vbitmap)
{
	struct bitmap *bitmap = (struct bitmap *) vbitmap;
	assert(bitmap);

	/* dynamically create the buffer */
	if (bitmap->sprite_area == NULL) {
		if (!bitmap_initialise(bitmap))
			return NULL;
	}

	/* image data area should exist */
	if (bitmap->sprite_area)
		return ((unsigned char *) (bitmap->sprite_area)) + 16 + 44;

	return NULL;
}


/**
 * Sets whether a bitmap should be plotted opaque
 *
 * \param  vbitmap  a bitmap, as returned by bitmap_create()
 * \param  opaque  whether the bitmap should be plotted opaque
 */
static void bitmap_set_opaque(void *vbitmap, bool opaque)
{
	struct bitmap *bitmap = (struct bitmap *) vbitmap;
	assert(bitmap);

	bitmap->opaque = opaque;
}


/**
 * Find the width of a pixel row in bytes.
 *
 * \param vbitmap A bitmap, as returned by riscos_bitmap_create()
 * \return width of a pixel row in the bitmap
 */
static size_t bitmap_get_rowstride(void *vbitmap)
{
	struct bitmap *bitmap = (struct bitmap *) vbitmap;
	return bitmap->width * 4;
}


/* exported interface documented in riscos/bitmap.h */
bool riscos_bitmap_get_opaque(void *vbitmap)
{
	struct bitmap *bitmap = (struct bitmap *) vbitmap;
	assert(bitmap);
	return bitmap->opaque;
}


/* exported interface documented in riscos/bitmap.h */
void riscos_bitmap_destroy(void *vbitmap)
{
	struct bitmap *bitmap = (struct bitmap *) vbitmap;

	assert(bitmap);

	/* destroy bitmap */
	if (bitmap->sprite_area) {
		free(bitmap->sprite_area);
	}

	free(bitmap);
}


/* exported interface documented in riscos/bitmap.h */
bool riscos_bitmap_save(void *vbitmap, const char *path, unsigned flags)
{
	struct bitmap *bitmap = (struct bitmap *) vbitmap;
	os_error *error;

	if (bitmap == NULL) {
		ro_warn_user("SaveError", messages_get("SprIsNull"));
		return false;
	}

	if (!bitmap->sprite_area) {
		riscos_bitmap_get_buffer(bitmap);
	}
	if (!bitmap->sprite_area)
		return false;

	if (riscos_bitmap_get_opaque(bitmap)) {
		error = xosspriteop_save_sprite_file(osspriteop_USER_AREA,
				(bitmap->sprite_area), path);
		if (error) {
			NSLOG(netsurf, INFO,
			      "xosspriteop_save_sprite_file: 0x%x: %s",
			      error->errnum,
			      error->errmess);
			ro_warn_user("SaveError", error->errmess);
			return false;
		}
		return true;
	} else {
		/* to make the saved sprite useful we must convert from 'Tinct'
		 * format to either a bi-level mask or a Select-style full
		 * alpha channel */
		osspriteop_area *area = bitmap->sprite_area;
		osspriteop_header *hdr = (void *) ((char *) area + area->first);
		unsigned width = hdr->width + 1, height = hdr->height + 1;
		unsigned image_size = height * width * 4;
		unsigned char *chunk_buf;
		unsigned *p, *elp, *eip;
		unsigned mask_size;
		size_t chunk_pix;
		struct {
			osspriteop_area   area;
			osspriteop_header hdr;
		} file_hdr;
		os_fw fw;

		/* we only support 32bpp sprites */
		if ((((unsigned)hdr->mode >> 27)&15) != 6) {
			assert(!"Unsupported sprite format in bitmap_save");
			return false;
		}

		chunk_buf = malloc(SAVE_CHUNK_SIZE);
		if (!chunk_buf) {
			ro_warn_user("NoMemory", NULL);
			return false;
		}

		file_hdr.area = *area;
		file_hdr.hdr  = *hdr;

		if (flags & BITMAP_SAVE_FULL_ALPHA) {
			mask_size = ((width + 3) & ~3) * height;
			chunk_pix = SAVE_CHUNK_SIZE;
			file_hdr.hdr.mode = (os_mode)((unsigned)file_hdr.hdr.mode
					| (1U<<31));
		} else {
			mask_size = (((width + 31) & ~31)/8) * height;
			chunk_pix = SAVE_CHUNK_SIZE<<3;
			file_hdr.hdr.mode = (os_mode)((unsigned)file_hdr.hdr.mode
					& ~(1U<<31));
		}

		file_hdr.area.sprite_count = 1;
		file_hdr.area.first = sizeof(file_hdr.area);
		file_hdr.area.used = sizeof(file_hdr) + image_size + mask_size;

		file_hdr.hdr.image = sizeof(file_hdr.hdr);
		file_hdr.hdr.mask = file_hdr.hdr.image + image_size;
		file_hdr.hdr.size = file_hdr.hdr.mask + mask_size;

		error = xosfind_openoutw(0, path, NULL, &fw);
		if (error) {
			NSLOG(netsurf, INFO, "xosfind_openoutw: 0x%x: %s",
			      error->errnum, error->errmess);
			free(chunk_buf);
			ro_warn_user("SaveError", error->errmess);
			return false;
		}

		p = (void *) ((char *) hdr + hdr->image);

		/* write out the area header, sprite header and image data */
		error = xosgbpb_writew(fw, (byte*)&file_hdr + 4,
				sizeof(file_hdr)-4, NULL);
		if (!error)
			error = xosgbpb_writew(fw, (byte*)p, image_size, NULL);
		if (error) {
			NSLOG(netsurf, INFO, "xosgbpb_writew: 0x%x: %s",
			      error->errnum, error->errmess);
			free(chunk_buf);
			xosfind_closew(fw);
			ro_warn_user("SaveError", error->errmess);
			return false;
		}

		/* then write out the mask data in chunks */
		eip = p + (width * height);  /* end of image */
		elp = p + width;  /* end of line */

		while (p < eip) {
			unsigned char *dp = chunk_buf;
			unsigned *ep = p + chunk_pix;
			if (ep > elp) ep = elp;

			if (flags & BITMAP_SAVE_FULL_ALPHA) {
				while (p < ep) {
					*dp++ = ((unsigned char*)p)[3];
					p++;
				}
			}
			else {
				unsigned char mb = 0;
				int msh = 0;
				while (p < ep) {
					if (((unsigned char*)p)[3]) mb |= (1 << msh);
					if (++msh >= 8) {
						*dp++ = mb;
						msh = 0;
						mb = 0;
					}
					p++;
				}
				if (msh > 0) *dp++ = mb;
			}

			if (p >= elp) {  /* end of line yet? */
				/* align to word boundary */
				while ((int)dp & 3) *dp++ = 0;
				/* advance end of line pointer */
				elp += width;
			}
			error = xosgbpb_writew(fw, (byte*)chunk_buf, dp-chunk_buf, NULL);
			if (error) {
				NSLOG(netsurf, INFO,
				      "xosgbpb_writew: 0x%x: %s",
				      error->errnum,
				      error->errmess);
				free(chunk_buf);
				xosfind_closew(fw);
				ro_warn_user("SaveError", error->errmess);
				return false;
			}
		}

		error = xosfind_closew(fw);
		if (error) {
			NSLOG(netsurf, INFO, "xosfind_closew: 0x%x: %s",
			      error->errnum, error->errmess);
			ro_warn_user("SaveError", error->errmess);
		}

		error = xosfile_set_type(path, osfile_TYPE_SPRITE);
		if (error) {
			NSLOG(netsurf, INFO, "xosfile_set_type: 0x%x: %s",
			      error->errnum, error->errmess);
			ro_warn_user("SaveError", error->errmess);
		}

		free(chunk_buf);
		return true;
	}
}


/**
 * The bitmap image has changed, so flush any persistent cache.
 *
 * \param  vbitmap  a bitmap, as returned by bitmap_create()
 */
static void bitmap_modified(void *vbitmap)
{
	(void)(vbitmap);
}


/**
 * Get the width of a bitmap.
 *
 * \param vbitmap A bitmap, as returned by bitmap_create()
 * \return The bitmaps width in pixels.
 */
static int bitmap_get_width(void *vbitmap)
{
	struct bitmap *bitmap = (struct bitmap *) vbitmap;
	return bitmap->width;
}


/**
 * Get the height of a bitmap.
 *
 * \param vbitmap A bitmap, as returned by bitmap_create()
 * \return The bitmaps height in pixels.
 */
static int bitmap_get_height(void *vbitmap)
{
	struct bitmap *bitmap = (struct bitmap *) vbitmap;
	return bitmap->height;
}


/* exported interface documented in riscos/bitmap.h */
void riscos_bitmap_overlay_sprite(struct bitmap *bitmap,
				  const osspriteop_header *s)
{
	const os_colour *palette;
	const byte *sp, *mp;
	bool masked = false;
	bool alpha = false;
	os_error *error;
	int dp_offset;
	int sp_offset;
	unsigned *dp;
	int x, y;
	int w, h;

	assert(sprite_bpp(s) == 8);

	if ((unsigned)s->mode & 0x80000000U)
		alpha = true;

	error = xosspriteop_read_sprite_info(osspriteop_PTR,
			(osspriteop_area *)0x100,
			(osspriteop_id)s,
			&w, &h, NULL, NULL);
	if (error) {
		NSLOG(netsurf, INFO, "xosspriteop_read_sprite_info: 0x%x:%s",
		      error->errnum, error->errmess);
		return;
	}
	sp_offset = ((s->width + 1) * 4) - w;

	if (w > bitmap->width)
		w = bitmap->width;
	if (h > bitmap->height)
		h = bitmap->height;

	dp_offset = bitmap_get_rowstride(bitmap) / 4;

	dp = (void*)riscos_bitmap_get_buffer(bitmap);
	if (!dp)
		return;
	sp = (byte*)s + s->image;
	mp = (byte*)s + s->mask;

	sp += s->left_bit / 8;
	mp += s->left_bit / 8;

	if (s->image > (int)sizeof(*s))
		palette = (os_colour*)(s + 1);
	else
		palette = default_palette8;

	if (s->mask != s->image) {
		masked = true;
		bitmap_set_opaque(bitmap, false);
	}

	/* (partially-)transparent pixels in the overlayed sprite retain
	 * their transparency in the output bitmap; opaque sprite pixels
	 * are also propagated to the bitmap, except those which are the
	 * OVERLAY_INDEX colour which allow the original bitmap contents to
	 * show through */
	for (y = 0; y < h; y++) {
		unsigned *sdp = dp;
		for(x = 0; x < w; x++) {
			os_colour d = ((unsigned)palette[(*sp) << 1]) >> 8;
			if (*sp++ == OVERLAY_INDEX)
				d = *dp;
			if (masked) {
				if (alpha)
					d |= ((*mp << 24) ^ 0xff000000U);
				else if (*mp)
					d |= 0xff000000U;
			}
			*dp++ = d;
			mp++;
		}
		dp = sdp + dp_offset;
		sp += sp_offset;
		mp += sp_offset;
	}
}


/**
 * Creates an 8bpp canvas.
 *
 * \param  bitmap  the bitmap to clone the size of
 * \return a sprite area containing an 8bpp sprite
 */
static osspriteop_area *thumbnail_create_8bpp(struct bitmap *bitmap)
{
	unsigned image_size = ((bitmap->width + 3) & ~3) * bitmap->height;
	bool opaque = riscos_bitmap_get_opaque(bitmap);
	osspriteop_header *sprite_header = NULL;
	osspriteop_area *sprite_area = NULL;
	unsigned area_size;

	/* clone the sprite */
	area_size = sizeof(osspriteop_area) +
			sizeof(osspriteop_header) +
			image_size +
			2048;

	if (!opaque) area_size += image_size;

	sprite_area = (osspriteop_area *)malloc(area_size);
	if (!sprite_area) {
		NSLOG(netsurf, INFO, "no memory for malloc()");
		return NULL;
	}
	sprite_area->size = area_size;
	sprite_area->sprite_count = 1;
	sprite_area->first = 16;
	sprite_area->used = area_size;
	sprite_header = (osspriteop_header *)(sprite_area + 1);
	sprite_header->size = area_size - sizeof(osspriteop_area);
	memset(sprite_header->name, 0x00, 12);
	strcpy(sprite_header->name, "bitmap");
	sprite_header->left_bit = 0;
	sprite_header->height = bitmap->height - 1;
	sprite_header->mode = os_MODE8BPP90X90;
	sprite_header->right_bit = ((bitmap->width << 3) - 1) & 31;
	sprite_header->width = ((bitmap->width + 3) >> 2) - 1;
	sprite_header->image = sizeof(osspriteop_header) + 2048;
	sprite_header->mask = sizeof(osspriteop_header) + 2048;
	if (!opaque) sprite_header->mask += image_size;

	/* create the palette. we don't read the necessary size like
	 * we really should as we know it's going to have 256 entries
	 * of 8 bytes = 2048. */
	xcolourtrans_read_palette((osspriteop_area *)os_MODE8BPP90X90,
			(osspriteop_id)0,
			(os_palette *)(sprite_header + 1), 2048,
			(colourtrans_palette_flags)(1 << 1), 0);
	return sprite_area;
}


/**
 * Switches output to the specified sprite and returns the previous context.
 */
static struct thumbnail_save_area*
thumbnail_switch_output(osspriteop_area *sprite_area,
			osspriteop_header *sprite_header)
{
	struct thumbnail_save_area *save_area;
	int size;

	/* create a save area */
	save_area = calloc(sizeof(struct thumbnail_save_area), 1);
	if (save_area == NULL) return NULL;

	/* allocate OS_SpriteOp save area */
	if (xosspriteop_read_save_area_size(osspriteop_PTR, sprite_area,
			(osspriteop_id)sprite_header, &size)) {
		free(save_area);
		return NULL;
	}

	/* create the save area */
	save_area->save_area = malloc((unsigned)size);
	if (save_area->save_area == NULL) {
		free(save_area);
		return NULL;
	}
	save_area->save_area->a[0] = 0;

	/* switch output to sprite */
	if (xosspriteop_switch_output_to_sprite(osspriteop_PTR, sprite_area,
			(osspriteop_id)sprite_header, save_area->save_area,
			0, &save_area->context1, &save_area->context2,
			&save_area->context3)) {
		free(save_area->save_area);
		free(save_area);
		return NULL;
	}
	return save_area;
}


/**
 * Restores output to the specified context, and destroys it.
 */
static void thumbnail_restore_output(struct thumbnail_save_area *save_area)
{
	/* we don't care if we err, as there's nothing we can do about it */
	xosspriteop_switch_output_to_sprite(osspriteop_PTR,
			(osspriteop_area *)save_area->context1,
			(osspriteop_id)save_area->context2,
			(osspriteop_save_area *)save_area->context3,
			0, 0, 0, 0);
	free(save_area->save_area);
	free(save_area);
}


/**
 * Convert a bitmap to 8bpp.
 *
 * \param  bitmap  the bitmap to convert
 * \return a sprite area containing an 8bpp sprite
 */
osspriteop_area *riscos_bitmap_convert_8bpp(struct bitmap *bitmap)
{
	struct thumbnail_save_area *save_area;
	osspriteop_area *sprite_area = NULL;
	osspriteop_header *sprite_header = NULL;

	sprite_area = thumbnail_create_8bpp(bitmap);
	if (!sprite_area)
		return NULL;
	sprite_header = (osspriteop_header *)(sprite_area + 1);


	/* switch output and redraw */
	save_area = thumbnail_switch_output(sprite_area, sprite_header);
	if (save_area == NULL) {
		if (thumbnail_32bpp_available != 1)
			free(sprite_area);
		return false;
	}
	_swix(Tinct_Plot, _IN(2) | _IN(3) | _IN(4) | _IN(7),
			(osspriteop_header *)(bitmap->sprite_area + 1),
			0, 0,
			tinct_ERROR_DIFFUSE);
	thumbnail_restore_output(save_area);

	if (sprite_header->image != sprite_header->mask) {
		/* build the sprite mask from the alpha channel */
		void *buf = riscos_bitmap_get_buffer(bitmap);
		unsigned *dp = (unsigned *) buf;
		if (!dp)
			return sprite_area;
		int w = bitmap_get_width(bitmap);
		int h = bitmap_get_height(bitmap);
		int dp_offset = bitmap_get_rowstride(bitmap) / 4 - w;
		int mp_offset = ((sprite_header->width + 1) * 4) - w;
		byte *mp = (byte*)sprite_header + sprite_header->mask;
		bool alpha = ((unsigned)sprite_header->mode & 0x80000000U) != 0;

		while (h-- > 0) {
			int x = 0;
			for(x = 0; x < w; x++) {
				unsigned d = *dp++;
				if (alpha)
					*mp++ = (d >> 24) ^ 0xff;
				else
					*mp++ = (d < 0xff000000U) ? 0 : 0xff;
			}
			dp += dp_offset;
			mp += mp_offset;
		}
	}

	return sprite_area;
}




/**
 * Check to see whether 32bpp sprites are available.
 *
 * Rather than using Wimp_ReadSysInfo we test if 32bpp sprites are available
 * in case the user has a 3rd party patch to enable them.
 */
static void thumbnail_test(void)
{
	unsigned int area_size;
	osspriteop_area *sprite_area;

	/* try to create a 1x1 32bpp sprite */
	area_size = sizeof(osspriteop_area) +
			sizeof(osspriteop_header) + sizeof(int);
	if ((sprite_area = (osspriteop_area *)malloc(area_size)) == NULL) {
		NSLOG(netsurf, INFO,
		      "Insufficient memory to perform sprite test.");
		return;
	}
	sprite_area->size = area_size + 1;
	sprite_area->sprite_count = 0;
	sprite_area->first = 16;
	sprite_area->used = 16;
	if (xosspriteop_create_sprite(osspriteop_NAME, sprite_area,
			"test",	false, 1, 1, (os_mode)tinct_SPRITE_MODE))
		thumbnail_32bpp_available = 0;
	else
		thumbnail_32bpp_available = 1;
	free(sprite_area);
}


/* exported interface documented in riscos/bitmap.h */
nserror riscos_bitmap_render(struct bitmap *bitmap,
			     struct hlcache_handle *content)
{
	struct thumbnail_save_area *save_area;
	osspriteop_area *sprite_area = NULL;
	osspriteop_header *sprite_header = NULL;
	struct redraw_context ctx = {
		.interactive = false,
		.background_images = true,
		.plot = &ro_plotters
	};

	assert(content);
	assert(bitmap);

	NSLOG(netsurf, INFO, "content %p in bitmap %p", content, bitmap);

	/* check if we have access to 32bpp sprites natively */
	if (thumbnail_32bpp_available == -1) {
		thumbnail_test();
	}

	/* if we don't support 32bpp sprites then we redirect to an 8bpp
	 * image and then convert back.
	 */
	if (thumbnail_32bpp_available != 1) {
		sprite_area = thumbnail_create_8bpp(bitmap);
		if (!sprite_area)
			return false;
		sprite_header = (osspriteop_header *)(sprite_area + 1);
	} else {
		const uint8_t *pixbufp = riscos_bitmap_get_buffer(bitmap);
		if (!pixbufp || !bitmap->sprite_area)
			return false;
		sprite_area = bitmap->sprite_area;
		sprite_header = (osspriteop_header *)(sprite_area + 1);
	}

	/* set up the plotters */
	ro_plot_origin_x = 0;
	ro_plot_origin_y = bitmap->height * 2;

	/* switch output and redraw */
	save_area = thumbnail_switch_output(sprite_area, sprite_header);
	if (!save_area) {
		if (thumbnail_32bpp_available != 1)
			free(sprite_area);
		return false;
	}
	rufl_invalidate_cache();
	colourtrans_set_gcol(os_COLOUR_WHITE, colourtrans_SET_BG_GCOL,
			os_ACTION_OVERWRITE, 0);

	/* render the content */
	content_scaled_redraw(content, bitmap->width, bitmap->height, &ctx);

	thumbnail_restore_output(save_area);
	rufl_invalidate_cache();

	/* if we changed to 8bpp then go back to 32bpp */
	if (thumbnail_32bpp_available != 1) {
		const uint8_t *pixbufp = riscos_bitmap_get_buffer(bitmap);
		_kernel_oserror *error;

		if (!pixbufp || !bitmap->sprite_area) {
			free(sprite_area);
			return false;
		}
		error = _swix(Tinct_ConvertSprite, _INR(2,3),
				sprite_header,
				(osspriteop_header *)(bitmap->sprite_area + 1));
		free(sprite_area);
		if (error)
			return false;
	}

	bitmap_modified(bitmap);

	return NSERROR_OK;
}

static struct gui_bitmap_table bitmap_table = {
	.create = riscos_bitmap_create,
	.destroy = riscos_bitmap_destroy,
	.set_opaque = bitmap_set_opaque,
	.get_opaque = riscos_bitmap_get_opaque,
	.get_buffer = riscos_bitmap_get_buffer,
	.get_rowstride = bitmap_get_rowstride,
	.get_width = bitmap_get_width,
	.get_height = bitmap_get_height,
	.modified = bitmap_modified,
	.render = riscos_bitmap_render,
};

struct gui_bitmap_table *riscos_bitmap_table = &bitmap_table;
