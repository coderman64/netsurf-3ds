/*
 * Copyright 2008 Vincent Sanders <vince@simtec.co.uk>
 *
 * Framebuffer interface
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

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <libnsfb.h>
#include <libnsfb_plot.h>
#include <libnsfb_event.h>
#include <libnsfb_cursor.h>

#include "utils/utils.h"
#include "utils/log.h"
#include "utils/utf8.h"
#include "netsurf/browser_window.h"
#include "netsurf/plotters.h"
#include "netsurf/bitmap.h"

#include "framebuffer/gui.h"
#include "framebuffer/fbtk.h"
#include "framebuffer/framebuffer.h"
#include "framebuffer/font.h"
#include "framebuffer/bitmap.h"

/* netsurf framebuffer library handle */
static nsfb_t *nsfb;


/**
 * \brief Sets a clip rectangle for subsequent plot operations.
 *
 * \param ctx The current redraw context.
 * \param clip The rectangle to limit all subsequent plot
 *              operations within.
 * \return NSERROR_OK on success else error code.
 */
static nserror
framebuffer_plot_clip(const struct redraw_context *ctx, const struct rect *clip)
{
	nsfb_bbox_t nsfb_clip;
	nsfb_clip.x0 = clip->x0;
	nsfb_clip.y0 = clip->y0;
	nsfb_clip.x1 = clip->x1;
	nsfb_clip.y1 = clip->y1;

	if (!nsfb_plot_set_clip(nsfb, &nsfb_clip)) {
		return NSERROR_INVALID;
	}
	return NSERROR_OK;
}


/**
 * Plots an arc
 *
 * plot an arc segment around (x,y), anticlockwise from angle1
 *  to angle2. Angles are measured anticlockwise from
 *  horizontal, in degrees.
 *
 * \param ctx The current redraw context.
 * \param style Style controlling the arc plot.
 * \param x The x coordinate of the arc.
 * \param y The y coordinate of the arc.
 * \param radius The radius of the arc.
 * \param angle1 The start angle of the arc.
 * \param angle2 The finish angle of the arc.
 * \return NSERROR_OK on success else error code.
 */
static nserror
framebuffer_plot_arc(const struct redraw_context *ctx,
	       const plot_style_t *style,
	       int x, int y, int radius, int angle1, int angle2)
{
	if (!nsfb_plot_arc(nsfb, x, y, radius, angle1, angle2, style->fill_colour)) {
		return NSERROR_INVALID;
	}
	return NSERROR_OK;
}


/**
 * Plots a circle
 *
 * Plot a circle centered on (x,y), which is optionally filled.
 *
 * \param ctx The current redraw context.
 * \param style Style controlling the circle plot.
 * \param x x coordinate of circle centre.
 * \param y y coordinate of circle centre.
 * \param radius circle radius.
 * \return NSERROR_OK on success else error code.
 */
static nserror
framebuffer_plot_disc(const struct redraw_context *ctx,
		const plot_style_t *style,
		int x, int y, int radius)
{
	nsfb_bbox_t ellipse;
	ellipse.x0 = x - radius;
	ellipse.y0 = y - radius;
	ellipse.x1 = x + radius;
	ellipse.y1 = y + radius;

	if (style->fill_type != PLOT_OP_TYPE_NONE) {
		nsfb_plot_ellipse_fill(nsfb, &ellipse, style->fill_colour);
	}

	if (style->stroke_type != PLOT_OP_TYPE_NONE) {
		nsfb_plot_ellipse(nsfb, &ellipse, style->stroke_colour);
	}
	return NSERROR_OK;
}


/**
 * Plots a line
 *
 * plot a line from (x0,y0) to (x1,y1). Coordinates are at
 *  centre of line width/thickness.
 *
 * \param ctx The current redraw context.
 * \param style Style controlling the line plot.
 * \param line A rectangle defining the line to be drawn
 * \return NSERROR_OK on success else error code.
 */
static nserror
framebuffer_plot_line(const struct redraw_context *ctx,
		const plot_style_t *style,
		const struct rect *line)
{
	nsfb_bbox_t rect;
	nsfb_plot_pen_t pen;

	rect.x0 = line->x0;
	rect.y0 = line->y0;
	rect.x1 = line->x1;
	rect.y1 = line->y1;

	if (style->stroke_type != PLOT_OP_TYPE_NONE) {

		if (style->stroke_type == PLOT_OP_TYPE_DOT) {
			pen.stroke_type = NFSB_PLOT_OPTYPE_PATTERN;
			pen.stroke_pattern = 0xAAAAAAAA;
		} else if (style->stroke_type == PLOT_OP_TYPE_DASH) {
			pen.stroke_type = NFSB_PLOT_OPTYPE_PATTERN;
			pen.stroke_pattern = 0xF0F0F0F0;
		} else {
			pen.stroke_type = NFSB_PLOT_OPTYPE_SOLID;
		}

		pen.stroke_colour = style->stroke_colour;
		pen.stroke_width = plot_style_fixed_to_int(style->stroke_width);
		nsfb_plot_line(nsfb, &rect, &pen);
	}

	return NSERROR_OK;
}


/**
 * Plots a rectangle.
 *
 * The rectangle can be filled an outline or both controlled
 *  by the plot style The line can be solid, dotted or
 *  dashed. Top left corner at (x0,y0) and rectangle has given
 *  width and height.
 *
 * \param ctx The current redraw context.
 * \param style Style controlling the rectangle plot.
 * \param nsrect A rectangle defining the line to be drawn
 * \return NSERROR_OK on success else error code.
 */
static nserror
framebuffer_plot_rectangle(const struct redraw_context *ctx,
		     const plot_style_t *style,
		     const struct rect *nsrect)
{
	nsfb_bbox_t rect;
	bool dotted = false;
	bool dashed = false;

	rect.x0 = nsrect->x0;
	rect.y0 = nsrect->y0;
	rect.x1 = nsrect->x1;
	rect.y1 = nsrect->y1;

	if (style->fill_type != PLOT_OP_TYPE_NONE) {
		nsfb_plot_rectangle_fill(nsfb, &rect, style->fill_colour);
	}

	if (style->stroke_type != PLOT_OP_TYPE_NONE) {
		if (style->stroke_type == PLOT_OP_TYPE_DOT) {
			dotted = true;
		}

		if (style->stroke_type == PLOT_OP_TYPE_DASH) {
			dashed = true;
		}

		nsfb_plot_rectangle(nsfb, &rect,
				plot_style_fixed_to_int(style->stroke_width),
				style->stroke_colour, dotted, dashed);
	}
	return NSERROR_OK;
}


/**
 * Plot a polygon
 *
 * Plots a filled polygon with straight lines between
 * points. The lines around the edge of the ploygon are not
 * plotted. The polygon is filled with the non-zero winding
 * rule.
 *
 * \param ctx The current redraw context.
 * \param style Style controlling the polygon plot.
 * \param p verticies of polygon
 * \param n number of verticies.
 * \return NSERROR_OK on success else error code.
 */
static nserror
framebuffer_plot_polygon(const struct redraw_context *ctx,
		   const plot_style_t *style,
		   const int *p,
		   unsigned int n)
{
	if (!nsfb_plot_polygon(nsfb, p, n, style->fill_colour)) {
		return NSERROR_INVALID;
	}
	return NSERROR_OK;
}


/**
 * Plots a path.
 *
 * Path plot consisting of cubic Bezier curves. Line and fill colour is
 *  controlled by the plot style.
 *
 * \param ctx The current redraw context.
 * \param pstyle Style controlling the path plot.
 * \param p elements of path
 * \param n nunber of elements on path
 * \param transform A transform to apply to the path.
 * \return NSERROR_OK on success else error code.
 */
static nserror
framebuffer_plot_path(const struct redraw_context *ctx,
		const plot_style_t *pstyle,
		const float *p,
		unsigned int n,
		const float transform[6])
{
	NSLOG(netsurf, INFO, "path unimplemented");
	return NSERROR_OK;
}


/**
 * Plot a bitmap
 *
 * Tiled plot of a bitmap image. (x,y) gives the top left
 * coordinate of an explicitly placed tile. From this tile the
 * image can repeat in all four directions -- up, down, left
 * and right -- to the extents given by the current clip
 * rectangle.
 *
 * The bitmap_flags say whether to tile in the x and y
 * directions. If not tiling in x or y directions, the single
 * image is plotted. The width and height give the dimensions
 * the image is to be scaled to.
 *
 * \param ctx The current redraw context.
 * \param bitmap The bitmap to plot
 * \param x The x coordinate to plot the bitmap
 * \param y The y coordiante to plot the bitmap
 * \param width The width of area to plot the bitmap into
 * \param height The height of area to plot the bitmap into
 * \param bg the background colour to alpha blend into
 * \param flags the flags controlling the type of plot operation
 * \return NSERROR_OK on success else error code.
 */
static nserror
framebuffer_plot_bitmap(const struct redraw_context *ctx,
		  struct bitmap *bitmap,
		  int x, int y,
		  int width,
		  int height,
		  colour bg,
		  bitmap_flags_t flags)
{
	nsfb_bbox_t loc;
	nsfb_bbox_t clipbox;
	bool repeat_x = (flags & BITMAPF_REPEAT_X);
	bool repeat_y = (flags & BITMAPF_REPEAT_Y);
	int bmwidth;
	int bmheight;
	int bmstride;
	enum nsfb_format_e bmformat;
	unsigned char *bmptr;
	nsfb_t *bm = (nsfb_t *)bitmap;

	/* x and y define coordinate of top left of of the initial explicitly
	 * placed tile. The width and height are the image scaling and the
	 * bounding box defines the extent of the repeat (which may go in all
	 * four directions from the initial tile).
	 */

	if (!(repeat_x || repeat_y)) {
		/* Not repeating at all, so just plot it */
		loc.x0 = x;
		loc.y0 = y;
		loc.x1 = loc.x0 + width;
		loc.y1 = loc.y0 + height;

		if (!nsfb_plot_copy(bm, NULL, nsfb, &loc)) {
			return NSERROR_INVALID;
		}
		return NSERROR_OK;
	}

	nsfb_plot_get_clip(nsfb, &clipbox);
	nsfb_get_geometry(bm, &bmwidth, &bmheight, &bmformat);
	nsfb_get_buffer(bm, &bmptr, &bmstride);

	/* Optimise tiled plots of 1x1 bitmaps by replacing with a flat fill
	 * of the area.  Can only be done when image is fully opaque. */
	if ((bmwidth == 1) && (bmheight == 1)) {
		if ((*(nsfb_colour_t *)bmptr & 0xff000000) != 0) {
			if (!nsfb_plot_rectangle_fill(nsfb, &clipbox,
						      *(nsfb_colour_t *)bmptr)) {
				return NSERROR_INVALID;
			}
			return NSERROR_OK;
		}
	}

	/* Optimise tiled plots of bitmaps scaled to 1x1 by replacing with
	 * a flat fill of the area.  Can only be done when image is fully
	 * opaque. */
	if ((width == 1) && (height == 1)) {
		if (framebuffer_bitmap_get_opaque(bm)) {
			/** TODO: Currently using top left pixel. Maybe centre
			 *        pixel or average value would be better. */
			if (!nsfb_plot_rectangle_fill(nsfb, &clipbox,
						      *(nsfb_colour_t *)bmptr)) {
				return NSERROR_INVALID;
			}
			return NSERROR_OK;
		}
	}

	/* get left most tile position */
	if (repeat_x) {
		for (; x > clipbox.x0; x -= width);
	}

	/* get top most tile position */
	if (repeat_y) {
		for (; y > clipbox.y0; y -= height);
	}

	/* set up top left tile location */
	loc.x0 = x;
	loc.y0 = y;
	loc.x1 = loc.x0 + width;
	loc.y1 = loc.y0 + height;

	/* plot tiling across and down to extents */
	nsfb_plot_bitmap_tiles(nsfb, &loc,
			repeat_x ? ((clipbox.x1 - x) + width  - 1) / width  : 1,
			repeat_y ? ((clipbox.y1 - y) + height - 1) / height : 1,
			(nsfb_colour_t *)bmptr, bmwidth, bmheight,
			bmstride * 8 / 32, bmformat == NSFB_FMT_ABGR8888);

	return NSERROR_OK;
}


#ifdef FB_USE_FREETYPE
/**
 * Text plotting.
 *
 * \param ctx The current redraw context.
 * \param fstyle plot style for this text
 * \param x x coordinate
 * \param y y coordinate
 * \param text UTF-8 string to plot
 * \param length length of string, in bytes
 * \return NSERROR_OK on success else error code.
 */
static nserror
framebuffer_plot_text(const struct redraw_context *ctx,
		const struct plot_font_style *fstyle,
		int x,
		int y,
		const char *text,
		size_t length)
{
	uint32_t ucs4;
	size_t nxtchr = 0;
	FT_Glyph glyph;
	FT_BitmapGlyph bglyph;
	nsfb_bbox_t loc;

	while (nxtchr < length) {
		ucs4 = utf8_to_ucs4(text + nxtchr, length - nxtchr);
		nxtchr = utf8_next(text, length, nxtchr);

		glyph = fb_getglyph(fstyle, ucs4);
		if (glyph == NULL)
			continue;

		if (glyph->format == FT_GLYPH_FORMAT_BITMAP) {
			bglyph = (FT_BitmapGlyph)glyph;

			loc.x0 = x + bglyph->left;
			loc.y0 = y - bglyph->top;
			loc.x1 = loc.x0 + bglyph->bitmap.width;
			loc.y1 = loc.y0 + bglyph->bitmap.rows;

			/* now, draw to our target surface */
			if (bglyph->bitmap.pixel_mode == FT_PIXEL_MODE_MONO) {
			    nsfb_plot_glyph1(nsfb,
					     &loc,
					     bglyph->bitmap.buffer,
					     bglyph->bitmap.pitch,
					     fstyle->foreground);
			} else {
			    nsfb_plot_glyph8(nsfb,
					     &loc,
					     bglyph->bitmap.buffer,
					     bglyph->bitmap.pitch,
					     fstyle->foreground);
			}
		}
		x += glyph->advance.x >> 16;

	}
	return NSERROR_OK;

}

#else

/**
 * Text plotting.
 *
 * \param ctx The current redraw context.
 * \param fstyle plot style for this text
 * \param x x coordinate
 * \param y y coordinate
 * \param text UTF-8 string to plot
 * \param length length of string, in bytes
 * \return NSERROR_OK on success else error code.
 */
static nserror
framebuffer_plot_text(const struct redraw_context *ctx,
		const struct plot_font_style *fstyle,
		int x,
		int y,
		const char *text,
		size_t length)
{
    enum fb_font_style style = fb_get_font_style(fstyle);
    int size = fb_get_font_size(fstyle);
    const uint8_t *chrp;
    size_t nxtchr = 0;
    nsfb_bbox_t loc;
    uint32_t ucs4;
    int p = FB_FONT_PITCH * size;
    int w = FB_FONT_WIDTH * size;
    int h = FB_FONT_HEIGHT * size;

    y -= ((h * 3) / 4);
    /* the coord is the bottom-left of the pixels offset by 1 to make
     * it work since fb coords are the top-left of pixels */
    y += 1;

    while (nxtchr < length) {
	ucs4 = utf8_to_ucs4(text + nxtchr, length - nxtchr);
	nxtchr = utf8_next(text, length, nxtchr);

	if (!codepoint_displayable(ucs4))
		continue;

	loc.x0 = x;
	loc.y0 = y;
	loc.x1 = loc.x0 + w;
	loc.y1 = loc.y0 + h;

	chrp = fb_get_glyph(ucs4, style, size);
	nsfb_plot_glyph1(nsfb, &loc, chrp, p, fstyle->foreground);

	x += w;

    }

    return NSERROR_OK;
}
#endif


/** framebuffer plot operation table */
const struct plotter_table fb_plotters = {
	.clip = framebuffer_plot_clip,
	.arc = framebuffer_plot_arc,
	.disc = framebuffer_plot_disc,
	.line = framebuffer_plot_line,
	.rectangle = framebuffer_plot_rectangle,
	.polygon = framebuffer_plot_polygon,
	.path = framebuffer_plot_path,
	.bitmap = framebuffer_plot_bitmap,
	.text = framebuffer_plot_text,
	.option_knockout = true,
};


static bool framebuffer_format_from_bpp(int bpp, enum nsfb_format_e *fmt)
{
	switch (bpp) {
	case 32:
		*fmt = NSFB_FMT_XRGB8888;
		break;

	case 24:
		*fmt = NSFB_FMT_RGB888;
		break;

	case 16:
		*fmt = NSFB_FMT_RGB565;
		break;

	case 8:
		*fmt = NSFB_FMT_I8;
		break;

	case 4:
		*fmt = NSFB_FMT_I4;
		break;

	case 1:
		*fmt = NSFB_FMT_I1;
		break;

	default:
		NSLOG(netsurf, INFO, "Bad bits per pixel (%d)\n", bpp);
		return false;
	}

	return true;
}



nsfb_t *
framebuffer_initialise(const char *fename, int width, int height, int bpp)
{
    enum nsfb_type_e fbtype;
    enum nsfb_format_e fbfmt;

    /* bpp is a proxy for the framebuffer format */
    if (framebuffer_format_from_bpp(bpp, &fbfmt) == false) {
	return NULL;
    }

    fbtype = nsfb_type_from_name(fename);
    if (fbtype == NSFB_SURFACE_NONE) {
	NSLOG(netsurf, INFO,
              "The %s surface is not available from libnsfb\n", fename);
	return NULL;
    }

    nsfb = nsfb_new(fbtype);
    if (nsfb == NULL) {
	NSLOG(netsurf, INFO, "Unable to create %s fb surface\n", fename);
	return NULL;
    }

    if (nsfb_set_geometry(nsfb, width, height, fbfmt) == -1) {
	NSLOG(netsurf, INFO, "Unable to set surface geometry\n");
	nsfb_free(nsfb);
	return NULL;
    }

    nsfb_cursor_init(nsfb);

    if (nsfb_init(nsfb) == -1) {
	NSLOG(netsurf, INFO, "Unable to initialise nsfb surface\n");
	nsfb_free(nsfb);
	return NULL;
    }

    return nsfb;

}

bool
framebuffer_resize(nsfb_t *nsfb, int width, int height, int bpp)
{
    enum nsfb_format_e fbfmt;

    /* bpp is a proxy for the framebuffer format */
    if (framebuffer_format_from_bpp(bpp, &fbfmt) == false) {
	return false;
    }

    if (nsfb_set_geometry(nsfb, width, height, fbfmt) == -1) {
	NSLOG(netsurf, INFO, "Unable to change surface geometry\n");
	return false;
    }

    return true;

}

void
framebuffer_finalise(void)
{
    nsfb_free(nsfb);
}

bool
framebuffer_set_cursor(struct fbtk_bitmap *bm)
{
    return nsfb_cursor_set(nsfb, (nsfb_colour_t *)bm->pixdata, bm->width, bm->height, bm->width, bm->hot_x, bm->hot_y);
}

nsfb_t *framebuffer_set_surface(nsfb_t *new_nsfb)
{
	nsfb_t *old_nsfb;
	old_nsfb = nsfb;
	nsfb = new_nsfb;
	return old_nsfb;
}
