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

/**
 * \file
 * RISC OS screen plotter implementation.
 */

#include <stdbool.h>
#include <math.h>
#include "oslib/colourtrans.h"
#include "oslib/draw.h"
#include "oslib/os.h"

#include "utils/log.h"
#include "netsurf/plotters.h"

#include "riscos/bitmap.h"
#include "riscos/image.h"
#include "riscos/gui.h"
#include "riscos/font.h"
#include "riscos/oslib_pre7.h"


int ro_plot_origin_x = 0;
int ro_plot_origin_y = 0;
struct rect ro_plot_clip_rect;

/** One version of the A9home OS is incapable of drawing patterned lines */
bool ro_plot_patterned_lines = true;

/**
 * plot a path on RISC OS
 */
static nserror
ro_plot_draw_path(const draw_path * const path,
		  int width,
		  colour c,
		  bool dotted,
		  bool dashed)
{
	static const draw_line_style line_style = {
		draw_JOIN_MITRED,
		draw_CAP_BUTT,
		draw_CAP_BUTT,
		0, 0x7fffffff,
		0, 0, 0, 0
	};
	draw_dash_pattern dash = { 0, 1, { 512 } };
	const draw_dash_pattern *dash_pattern = 0;
	os_error *error;

	if (width < 1)
		width = 1;

	if (ro_plot_patterned_lines) {
		if (dotted) {
			dash.elements[0] = 512 * width;
			dash_pattern = &dash;
		} else if (dashed) {
			dash.elements[0] = 1536 * width;
			dash_pattern = &dash;
		}
	}

	error = xcolourtrans_set_gcol(c << 8, 0, os_ACTION_OVERWRITE, 0, 0);
	if (error) {
		NSLOG(netsurf, INFO, "xcolourtrans_set_gcol: 0x%x: %s",
		      error->errnum, error->errmess);
		return NSERROR_INVALID;
	}

	error = xdraw_stroke(path, 0, 0, 0, width * 2 * 256,
			&line_style, dash_pattern);
	if (error) {
		NSLOG(netsurf, INFO, "xdraw_stroke: 0x%x: %s", error->errnum,
		      error->errmess);
		return NSERROR_INVALID;
	}

	return NSERROR_OK;
}


/**
 * \brief Sets a clip rectangle for subsequent plot operations.
 *
 * \param ctx The current redraw context.
 * \param clip The rectangle to limit all subsequent plot
 *              operations within.
 * \return NSERROR_OK on success else error code.
 */
static nserror
ro_plot_clip(const struct redraw_context *ctx, const struct rect *clip)
{
	os_error *error;
	char buf[12];

	int clip_x0 = clip->x0 * 2;
	int clip_y0 = clip->y1 * 2;
	int clip_x1 = clip->x1 * 2;
	int clip_y1 = clip->y0 * 2;

	/* Avoid artefacts due to clip rectangle offsetting in EX0 EY0 modes.
	 * The area the WIMP asked us to draw might have dimensions that are
	 * not a multiple of 2. */
	if (clip_x0 < ro_plot_clip_rect.x0) clip_x0 = ro_plot_clip_rect.x0;
	if (clip_x1 > ro_plot_clip_rect.x1) clip_x1 = ro_plot_clip_rect.x1;
	if (clip_y0 > ro_plot_clip_rect.y0) clip_y0 = ro_plot_clip_rect.y0;
	if (clip_y1 < ro_plot_clip_rect.y1) clip_y1 = ro_plot_clip_rect.y1;

	clip_x0 = ro_plot_origin_x + clip_x0;
	clip_y0 = ro_plot_origin_y - clip_y0;
	clip_x1 = ro_plot_origin_x + clip_x1 - 1;
	clip_y1 = ro_plot_origin_y - clip_y1 - 1;

	if (clip_x1 < clip_x0 || clip_y1 < clip_y0) {
		NSLOG(netsurf, INFO, "bad clip rectangle %i %i %i %i",
		      clip_x0, clip_y0, clip_x1, clip_y1);
		return NSERROR_BAD_SIZE;
	}

	buf[0] = os_VDU_SET_GRAPHICS_WINDOW;
	buf[1] = clip_x0;
	buf[2] = clip_x0 >> 8;
	buf[3] = clip_y0;
	buf[4] = clip_y0 >> 8;
	buf[5] = clip_x1;
	buf[6] = clip_x1 >> 8;
	buf[7] = clip_y1;
	buf[8] = clip_y1 >> 8;

	error = xos_writen(buf, 9);
	if (error) {
		NSLOG(netsurf, INFO, "xos_writen: 0x%x: %s", error->errnum,
		      error->errmess);
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
ro_plot_arc(const struct redraw_context *ctx,
	       const plot_style_t *style,
	       int x, int y, int radius, int angle1, int angle2)
{
	os_error *error;
	int sx, sy, ex, ey;
	double t;

	x = ro_plot_origin_x + x * 2;
	y = ro_plot_origin_y - y * 2;
	radius <<= 1;

	error = xcolourtrans_set_gcol(style->fill_colour << 8, 0,
			os_ACTION_OVERWRITE, 0, 0);

	if (error) {
		NSLOG(netsurf, INFO, "xcolourtrans_set_gcol: 0x%x: %s",
		      error->errnum, error->errmess);
		return NSERROR_INVALID;
	}

	t = ((double)angle1 * M_PI) / 180.0;
	sx = (x + (int)(radius * cos(t)));
	sy = (y + (int)(radius * sin(t)));

	t = ((double)angle2 * M_PI) / 180.0;
	ex = (x + (int)(radius * cos(t)));
	ey = (y + (int)(radius * sin(t)));

	error = xos_plot(os_MOVE_TO, x, y);	/* move to centre */
	if (error) {
		NSLOG(netsurf, INFO, "xos_plot: 0x%x: %s", error->errnum,
		      error->errmess);
		return NSERROR_INVALID;
	}

	error = xos_plot(os_MOVE_TO, sx, sy);	/* move to start */
	if (error) {
		NSLOG(netsurf, INFO, "xos_plot: 0x%x: %s", error->errnum,
		      error->errmess);
		return NSERROR_INVALID;
	}

	error = xos_plot(os_PLOT_ARC | os_PLOT_TO, ex, ey);	/* arc to end */
	if (error) {
		NSLOG(netsurf, INFO, "xos_plot: 0x%x: %s", error->errnum,
		      error->errmess);
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
 * \param x The x coordinate of the circle.
 * \param y The y coordinate of the circle.
 * \param radius The radius of the circle.
 * \return NSERROR_OK on success else error code.
 */
static nserror
ro_plot_disc(const struct redraw_context *ctx,
		const plot_style_t *style,
		int x, int y, int radius)
{
	os_error *error;
	if (style->fill_type != PLOT_OP_TYPE_NONE) {
		error = xcolourtrans_set_gcol(style->fill_colour << 8, 0,
					      os_ACTION_OVERWRITE, 0, 0);
		if (error) {
			NSLOG(netsurf, INFO,
			      "xcolourtrans_set_gcol: 0x%x: %s",
			      error->errnum,
			      error->errmess);
			return NSERROR_INVALID;
		}
		error = xos_plot(os_MOVE_TO,
				 ro_plot_origin_x + x * 2,
				 ro_plot_origin_y - y * 2);
		if (error) {
			NSLOG(netsurf, INFO, "xos_plot: 0x%x: %s",
			      error->errnum, error->errmess);
			return NSERROR_INVALID;
		}
		error = xos_plot(os_PLOT_CIRCLE | os_PLOT_BY, radius * 2, 0);
		if (error) {
			NSLOG(netsurf, INFO, "xos_plot: 0x%x: %s",
			      error->errnum, error->errmess);
			return NSERROR_INVALID;
		}
	}

	if (style->stroke_type != PLOT_OP_TYPE_NONE) {

		error = xcolourtrans_set_gcol(style->stroke_colour << 8, 0,
					      os_ACTION_OVERWRITE, 0, 0);
		if (error) {
			NSLOG(netsurf, INFO,
			      "xcolourtrans_set_gcol: 0x%x: %s",
			      error->errnum,
			      error->errmess);
			return NSERROR_INVALID;
		}
		error = xos_plot(os_MOVE_TO,
				 ro_plot_origin_x + x * 2,
				 ro_plot_origin_y - y * 2);
		if (error) {
			NSLOG(netsurf, INFO, "xos_plot: 0x%x: %s",
			      error->errnum, error->errmess);
			return NSERROR_INVALID;
		}
		error = xos_plot(os_PLOT_CIRCLE_OUTLINE | os_PLOT_BY,
				 radius * 2, 0);

		if (error) {
			NSLOG(netsurf, INFO, "xos_plot: 0x%x: %s",
			      error->errnum, error->errmess);
			return NSERROR_INVALID;
		}
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
ro_plot_line(const struct redraw_context *ctx,
		const plot_style_t *style,
		const struct rect *line)
{
	if (style->stroke_type != PLOT_OP_TYPE_NONE) {
		const int path[] = {
			draw_MOVE_TO,
			(ro_plot_origin_x + line->x0 * 2) * 256,
			(ro_plot_origin_y - line->y0 * 2 - 1) * 256,
			draw_LINE_TO,
			(ro_plot_origin_x + line->x1 * 2) * 256,
			(ro_plot_origin_y - line->y1 * 2 - 1) * 256,
			draw_END_PATH };
		bool dotted = false;
		bool dashed = false;

		if (style->stroke_type == PLOT_OP_TYPE_DOT)
			dotted = true;

		if (style->stroke_type == PLOT_OP_TYPE_DASH)
			dashed = true;

		return ro_plot_draw_path((const draw_path *)path,
				plot_style_fixed_to_int(style->stroke_width),
				style->stroke_colour,
				dotted, dashed);
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
 * \param rect A rectangle defining the line to be drawn
 * \return NSERROR_OK on success else error code.
 */
static nserror
ro_plot_rectangle(const struct redraw_context *ctx,
		     const plot_style_t *style,
		     const struct rect *rect)
{
	if (style->fill_type != PLOT_OP_TYPE_NONE) {
		os_error *error;
		error = xcolourtrans_set_gcol(style->fill_colour << 8,
						colourtrans_USE_ECFS_GCOL,
						os_ACTION_OVERWRITE, 0, 0);
		if (error) {
			NSLOG(netsurf, INFO,
			      "xcolourtrans_set_gcol: 0x%x: %s",
			      error->errnum,
			      error->errmess);
			return NSERROR_INVALID;
		}

		error = xos_plot(os_MOVE_TO,
				 ro_plot_origin_x + rect->x0 * 2,
				 ro_plot_origin_y - rect->y1 * 2);
		if (error) {
			NSLOG(netsurf, INFO, "xos_plot: 0x%x: %s",
			      error->errnum, error->errmess);
			return NSERROR_INVALID;
		}

		error = xos_plot(os_PLOT_RECTANGLE | os_PLOT_TO,
				 ro_plot_origin_x + rect->x1 * 2 - 1,
				 ro_plot_origin_y - rect->y0 * 2 - 1);
		if (error) {
			NSLOG(netsurf, INFO, "xos_plot: 0x%x: %s",
			      error->errnum, error->errmess);
			return NSERROR_INVALID;
		}
	}

	if (style->stroke_type != PLOT_OP_TYPE_NONE) {
		bool dotted = false;
		bool dashed = false;

		const int path[] = {
			draw_MOVE_TO,
			(ro_plot_origin_x + rect->x0 * 2) * 256,
			(ro_plot_origin_y - rect->y0 * 2 - 1) * 256,
			draw_LINE_TO,
			(ro_plot_origin_x + (rect->x1) * 2) * 256,
			(ro_plot_origin_y - rect->y0 * 2 - 1) * 256,
			draw_LINE_TO,
			(ro_plot_origin_x + (rect->x1) * 2) * 256,
			(ro_plot_origin_y - (rect->y1) * 2 - 1) * 256,
			draw_LINE_TO,
			(ro_plot_origin_x + rect->x0 * 2) * 256,
			(ro_plot_origin_y - (rect->y1) * 2 - 1) * 256,
			draw_CLOSE_LINE,
			(ro_plot_origin_x + rect->x0 * 2) * 256,
			(ro_plot_origin_y - rect->y0 * 2 - 1) * 256,
			draw_END_PATH
		};

		if (style->stroke_type == PLOT_OP_TYPE_DOT)
			dotted = true;

		if (style->stroke_type == PLOT_OP_TYPE_DASH)
			dashed = true;

		ro_plot_draw_path((const draw_path *)path,
				plot_style_fixed_to_int(style->stroke_width),
				style->stroke_colour,
				dotted,
				dashed);
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
ro_plot_polygon(const struct redraw_context *ctx,
		   const plot_style_t *style,
		   const int *p,
		   unsigned int n)
{
	int path[n * 3 + 2];
	unsigned int i;
	os_error *error;

	for (i = 0; i != n; i++) {
		path[i * 3 + 0] = draw_LINE_TO;
		path[i * 3 + 1] = (ro_plot_origin_x + p[i * 2 + 0] * 2) * 256;
		path[i * 3 + 2] = (ro_plot_origin_y - p[i * 2 + 1] * 2) * 256;
	}
	path[0] = draw_MOVE_TO;
	path[n * 3] = draw_END_PATH;
	path[n * 3 + 1] = 0;

	error = xcolourtrans_set_gcol(style->fill_colour << 8,
				      0, os_ACTION_OVERWRITE, 0, 0);
	if (error) {
		NSLOG(netsurf, INFO, "xcolourtrans_set_gcol: 0x%x: %s",
		      error->errnum, error->errmess);
		return NSERROR_INVALID;
	}
	error = xdraw_fill((draw_path *) path, 0, 0, 0);
	if (error) {
		NSLOG(netsurf, INFO, "xdraw_fill: 0x%x: %s", error->errnum,
		      error->errmess);
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
ro_plot_path(const struct redraw_context *ctx,
		const plot_style_t *pstyle,
		const float *p,
		unsigned int n,
		const float transform[6])
{
	static const draw_line_style line_style = {
		draw_JOIN_MITRED,
		draw_CAP_BUTT,
		draw_CAP_BUTT,
		0, 0x7fffffff,
		0, 0, 0, 0
	};
	int *path = 0;
	unsigned int i;
	os_trfm trfm;
	os_error *error;

	if (n == 0) {
		return NSERROR_OK;
	}

	if (p[0] != PLOTTER_PATH_MOVE) {
		NSLOG(netsurf, INFO, "path doesn't start with a move");
		goto error;
	}

	path = malloc(sizeof *path * (n + 10));
	if (!path) {
		NSLOG(netsurf, INFO, "out of memory");
		goto error;
	}

	for (i = 0; i < n; ) {
		if (p[i] == PLOTTER_PATH_MOVE) {
			path[i] = draw_MOVE_TO;
			path[i + 1] = p[i + 1] * 2 * 256;
			path[i + 2] = -p[i + 2] * 2 * 256;
			i += 3;
		} else if (p[i] == PLOTTER_PATH_CLOSE) {
			path[i] = draw_CLOSE_LINE;
			i++;
		} else if (p[i] == PLOTTER_PATH_LINE) {
			path[i] = draw_LINE_TO;
			path[i + 1] = p[i + 1] * 2 * 256;
			path[i + 2] = -p[i + 2] * 2 * 256;
			i += 3;
		} else if (p[i] == PLOTTER_PATH_BEZIER) {
			path[i] = draw_BEZIER_TO;
			path[i + 1] = p[i + 1] * 2 * 256;
			path[i + 2] = -p[i + 2] * 2 * 256;
			path[i + 3] = p[i + 3] * 2 * 256;
			path[i + 4] = -p[i + 4] * 2 * 256;
			path[i + 5] = p[i + 5] * 2 * 256;
			path[i + 6] = -p[i + 6] * 2 * 256;
			i += 7;
		} else {
			NSLOG(netsurf, INFO, "bad path command %f", p[i]);
			goto error;
		}
	}
	path[i] = draw_END_PATH;
	path[i + 1] = 0;

	trfm.entries[0][0] = transform[0] * 0x10000;
	trfm.entries[0][1] = transform[1] * 0x10000;
	trfm.entries[1][0] = transform[2] * 0x10000;
	trfm.entries[1][1] = transform[3] * 0x10000;
	trfm.entries[2][0] = (ro_plot_origin_x + transform[4] * 2) * 256;
	trfm.entries[2][1] = (ro_plot_origin_y - transform[5] * 2) * 256;

	if (pstyle->fill_colour != NS_TRANSPARENT) {
		error = xcolourtrans_set_gcol(pstyle->fill_colour << 8, 0,
				os_ACTION_OVERWRITE, 0, 0);
		if (error) {
			NSLOG(netsurf, INFO,
			      "xcolourtrans_set_gcol: 0x%x: %s",
			      error->errnum,
			      error->errmess);
			goto error;
		}

		error = xdraw_fill((draw_path *) path, 0, &trfm, 0);
		if (error) {
			NSLOG(netsurf, INFO, "xdraw_stroke: 0x%x: %s",
			      error->errnum, error->errmess);
			goto error;
		}
	}

	if (pstyle->stroke_colour != NS_TRANSPARENT) {
		error = xcolourtrans_set_gcol(pstyle->stroke_colour << 8, 0,
				os_ACTION_OVERWRITE, 0, 0);
		if (error) {
			NSLOG(netsurf, INFO,
			      "xcolourtrans_set_gcol: 0x%x: %s",
			      error->errnum,
			      error->errmess);
			goto error;
		}

		error = xdraw_stroke((draw_path *) path, 0, &trfm, 0,
				plot_style_fixed_to_int(
						pstyle->stroke_width) * 2 * 256,
				&line_style, 0);
		if (error) {
			NSLOG(netsurf, INFO, "xdraw_stroke: 0x%x: %s",
			      error->errnum, error->errmess);
			goto error;
		}
	}

	free(path);
	return NSERROR_OK;

error:
	free(path);
	return NSERROR_INVALID;
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
ro_plot_bitmap(const struct redraw_context *ctx,
	       struct bitmap *bitmap,
	       int x, int y,
	       int width,
	       int height,
	       colour bg,
	       bitmap_flags_t flags)
{
	const uint8_t *buffer;

	buffer = riscos_bitmap_get_buffer(bitmap);
	if (!buffer) {
		NSLOG(netsurf, INFO, "bitmap_get_buffer failed");
		return NSERROR_INVALID;
	}

	if (!image_redraw(bitmap->sprite_area,
			ro_plot_origin_x + x * 2,
			ro_plot_origin_y - y * 2,
			width, height,
			bitmap->width,
			bitmap->height,
			bg,
			flags & BITMAPF_REPEAT_X, flags & BITMAPF_REPEAT_Y,
			flags & BITMAPF_REPEAT_X || flags & BITMAPF_REPEAT_Y,
			riscos_bitmap_get_opaque(bitmap) ? IMAGE_PLOT_TINCT_OPAQUE :
			 IMAGE_PLOT_TINCT_ALPHA)) {
		return NSERROR_INVALID;
	}
	return NSERROR_OK;
}


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
ro_plot_text(const struct redraw_context *ctx,
		const struct plot_font_style *fstyle,
		int x,
		int y,
		const char *text,
		size_t length)
{
	os_error *error;

	error = xcolourtrans_set_font_colours(font_CURRENT,
			fstyle->background << 8, fstyle->foreground << 8,
			14, 0, 0, 0);
	if (error) {
		NSLOG(netsurf, INFO,
		      "xcolourtrans_set_font_colours: 0x%x: %s",
		      error->errnum,
		      error->errmess);
		return NSERROR_INVALID;
	}

	if (!nsfont_paint(fstyle, text, length,
			ro_plot_origin_x + x * 2,
			  ro_plot_origin_y - y * 2)) {
		return NSERROR_INVALID;
	}
	return NSERROR_OK;
}


/**
 * RISC OS plotter operation table
 */
const struct plotter_table ro_plotters = {
	.rectangle = ro_plot_rectangle,
	.line = ro_plot_line,
	.polygon = ro_plot_polygon,
	.clip = ro_plot_clip,
	.text = ro_plot_text,
	.disc = ro_plot_disc,
	.arc = ro_plot_arc,
	.bitmap = ro_plot_bitmap,
	.path = ro_plot_path,
	.option_knockout = true,
};
