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

#include <stdio.h>

#include "utils/utils.h"
#include "utils/errors.h"
#include "netsurf/plotters.h"

#include "monkey/output.h"

/**
 * \brief Sets a clip rectangle for subsequent plot operations.
 *
 * \param ctx The current redraw context.
 * \param clip The rectangle to limit all subsequent plot
 *              operations within.
 * \return NSERROR_OK on success else error code.
 */
static nserror
monkey_plot_clip(const struct redraw_context *ctx, const struct rect *clip)
{
	moutf(MOUT_PLOT, "CLIP X0 %d Y0 %d X1 %d Y1 %d",
		clip->x0, clip->y0, clip->x1, clip->y1);
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
monkey_plot_arc(const struct redraw_context *ctx,
		const plot_style_t *style,
		int x, int y, int radius, int angle1, int angle2)
{
	moutf(MOUT_PLOT, "ARC X %d Y %d RADIUS %d ANGLE1 %d ANGLE2 %d",
	      x, y, radius, angle1, angle2);
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
monkey_plot_disc(const struct redraw_context *ctx,
		 const plot_style_t *style,
		 int x, int y, int radius)
{
	moutf(MOUT_PLOT, "DISC X %d Y %d RADIUS %d", x, y, radius);
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
monkey_plot_line(const struct redraw_context *ctx,
		 const plot_style_t *style,
		 const struct rect *line)
{
	moutf(MOUT_PLOT, "LINE X0 %d Y0 %d X1 %d Y1 %d",
		line->x0, line->y0, line->x1, line->y1);
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
monkey_plot_rectangle(const struct redraw_context *ctx,
		      const plot_style_t *style,
		      const struct rect *rect)
{
	moutf(MOUT_PLOT, "RECT X0 %d Y0 %d X1 %d Y1 %d",
		rect->x0, rect->y0, rect->x1, rect->y1);
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
monkey_plot_polygon(const struct redraw_context *ctx,
		    const plot_style_t *style,
		    const int *p,
		    unsigned int n)
{
	moutf(MOUT_PLOT, "POLYGON VERTICIES %d", n);
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
monkey_plot_path(const struct redraw_context *ctx,
		 const plot_style_t *pstyle,
		 const float *p,
		 unsigned int n,
		 const float transform[6])
{
	moutf(MOUT_PLOT, "PATH VERTICIES %d WIDTH %f",
		n, plot_style_fixed_to_float(pstyle->stroke_width));
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
monkey_plot_bitmap(const struct redraw_context *ctx,
		   struct bitmap *bitmap,
		   int x, int y,
		   int width,
		   int height,
		   colour bg,
		   bitmap_flags_t flags)
{
	moutf(MOUT_PLOT, "BITMAP X %d Y %d WIDTH %d HEIGHT %d",
	      x, y, width, height);
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
monkey_plot_text(const struct redraw_context *ctx,
		 const struct plot_font_style *fstyle,
		 int x,
		 int y,
		 const char *text,
		 size_t length)
{
	moutf(MOUT_PLOT, "TEXT X %d Y %d STR %.*s", x, y, (int)length, text);
	return NSERROR_OK;
}


/** monkey plotter operations table */
static const struct plotter_table plotters = {
	.clip = monkey_plot_clip,
	.arc = monkey_plot_arc,
	.disc = monkey_plot_disc,
	.line = monkey_plot_line,
	.rectangle = monkey_plot_rectangle,
	.polygon = monkey_plot_polygon,
	.path = monkey_plot_path,
	.bitmap = monkey_plot_bitmap,
	.text = monkey_plot_text,
	.option_knockout = true,
};

const struct plotter_table* monkey_plotters = &plotters;
