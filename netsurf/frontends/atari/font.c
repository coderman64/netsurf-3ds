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

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>

#include "netsurf/inttypes.h"
#include "utils/utf8.h"
#include "utils/log.h"
#include "utils/nsoption.h"
#include "netsurf/layout.h"
#include "netsurf/mouse.h"
#include "netsurf/plotters.h"

#include "atari/gui.h"
#include "atari/plot/fontplot.h"
#include "atari/plot/plot.h"
#include "atari/findfile.h"
#include "atari/font.h"

extern FONT_PLOTTER fplotter;


/**
 * Find the position in a string where an x coordinate falls.
 *
 * \param[in] fstyle style for this text
 * \param[in] string UTF-8 string to measure
 * \param[in] length length of string, in bytes
 * \param[in] x coordinate to search for
 * \param[out] char_offset updated to offset in string of actual_x, [0..length]
 * \param[out] actual_x updated to x coordinate of character closest to x
 * \return NSERROR_OK and char_offset and actual_x updated or appropriate error code on faliure
 */
static nserror
atari_font_position(const plot_font_style_t *fstyle,
		    const char *string,
		    size_t length,
		    int x,
		    size_t *char_offset,
		    int *actual_x)
{
	float scale = plot_get_scale();

	if (scale != 1.0) {
		plot_font_style_t newstyle = *fstyle;
		newstyle.size = (int)((float)fstyle->size*scale);
		fplotter->pixel_pos(fplotter, &newstyle, string, length, x,
				    char_offset, actual_x);
	} else {
		fplotter->pixel_pos(fplotter, fstyle, string, length, x,
				    char_offset, actual_x);
	}

	return NSERROR_OK;
}


/**
 * Find where to split a string to make it fit a width.
 *
 * \param[in] fstyle       style for this text
 * \param[in] string       UTF-8 string to measure
 * \param[in] length       length of string, in bytes
 * \param[in] x            width available
 * \param[out] char_offset updated to offset in string of actual_x, [1..length]
 * \param[out] actual_x updated to x coordinate of character closest to x
 * \return NSERROR_OK or appropriate error code on faliure
 *
 * On exit, char_offset indicates first character after split point.
 *
 * \note char_offset of 0 must never be returned.
 *
 *   Returns:
 *     char_offset giving split point closest to x, where actual_x <= x
 *   else
 *     char_offset giving split point closest to x, where actual_x > x
 *
 * Returning char_offset == length means no split possible
 */
static nserror
atari_font_split(const plot_font_style_t *fstyle,
		 const char *string,
		 size_t length,
		 int x,
		 size_t *char_offset,
		 int *actual_x)
{
	float scale = plot_get_scale();

	if (scale != 1.0) {
		plot_font_style_t newstyle = *fstyle;
		newstyle.size = (int)((float)fstyle->size*scale);
		fplotter->str_split(fplotter, &newstyle, string, length, x,
				    char_offset, actual_x);
	} else {
		fplotter->str_split(fplotter, fstyle, string, length, x,
				    char_offset, actual_x);
	}

	return NSERROR_OK;
}


/**
 * Measure the width of a string.
 *
 * \param[in] fstyle plot style for this text
 * \param[in] str UTF-8 string to measure
 * \param[in] length length of string, in bytes
 * \param[out] width updated to width of string[0..length)
 * \return NSERROR_OK and width updated or appropriate error code on faliure
 */
static nserror
atari_font_width(const plot_font_style_t *fstyle,
		 const char *str,
		 size_t length,
		 int *width)
{
	float scale = plot_get_scale();

	if (scale != 1.0) {
		plot_font_style_t newstyle = *fstyle;
		newstyle.size = (int)((float)fstyle->size*scale);
		fplotter->str_width(fplotter, &newstyle, str, length, width);
	} else {
		fplotter->str_width(fplotter, fstyle, str, length, width);
	}

	return NSERROR_OK;
}


static struct gui_layout_table layout_table = {
	.width = atari_font_width,
	.position = atari_font_position,
	.split = atari_font_split,
};

struct gui_layout_table *atari_layout_table = &layout_table;
