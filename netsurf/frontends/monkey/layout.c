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

/**
 * \file
 * monkey implementation of font layout.
 */

#include <stddef.h>

#include "utils/utf8.h"
#include "netsurf/plot_style.h"
#include "netsurf/layout.h"

#include "monkey/layout.h"

static nserror nsfont_width(const plot_font_style_t *fstyle,
			    const char *string, size_t length,
			    int *width)
{
	*width = (fstyle->size * utf8_bounded_length(string, length)) / PLOT_STYLE_SCALE;
	return NSERROR_OK;
}

/**
 * Find the position in a string where an x coordinate falls.
 *
 * \param  fstyle       style for this text
 * \param  string       UTF-8 string to measure
 * \param  length       length of string
 * \param  x            x coordinate to search for
 * \param  char_offset  updated to offset in string of actual_x, [0..length]
 * \param  actual_x     updated to x coordinate of character closest to x
 * \return  true on success, false on error and error reported
 */

static nserror nsfont_position_in_string(const plot_font_style_t *fstyle,
					 const char *string, size_t length,
					 int x, size_t *char_offset, int *actual_x)
{
	*char_offset = x / (fstyle->size / PLOT_STYLE_SCALE);
	if (*char_offset > length)
		*char_offset = length;
	*actual_x = *char_offset * (fstyle->size / PLOT_STYLE_SCALE);
	return NSERROR_OK;
}


/**
 * Find where to split a string to make it fit a width.
 *
 * \param  fstyle       style for this text
 * \param  string       UTF-8 string to measure
 * \param  length       length of string, in bytes
 * \param  x            width available
 * \param  char_offset  updated to offset in string of actual_x, [1..length]
 * \param  actual_x     updated to x coordinate of character closest to x
 * \return  true on success, false on error and error reported
 *
 * On exit, char_offset indicates first character after split point.
 *
 * Note: char_offset of 0 should never be returned.
 *
 *   Returns:
 *     char_offset giving split point closest to x, where actual_x <= x
 *   else
 *     char_offset giving split point closest to x, where actual_x > x
 *
 * Returning char_offset == length means no split possible
 */

static nserror nsfont_split(const plot_font_style_t *fstyle,
			    const char *string, size_t length,
			    int x, size_t *char_offset, int *actual_x)
{
	int c_off = *char_offset = x / (fstyle->size / PLOT_STYLE_SCALE);
	if (*char_offset > length) {
		*char_offset = length;
	} else {
		while (*char_offset > 0) {
			if (string[*char_offset] == ' ')
				break;
			(*char_offset)--;
		}
		if (*char_offset == 0) {
			*char_offset = c_off;
			while (*char_offset < length && string[*char_offset] != ' ') {
				(*char_offset)++;
			}
		}
	}
	*actual_x = *char_offset * (fstyle->size / PLOT_STYLE_SCALE);
	return NSERROR_OK;
}

static struct gui_layout_table layout_table = {
	.width = nsfont_width,
	.position = nsfont_position_in_string,
	.split = nsfont_split,
};

struct gui_layout_table *monkey_layout_table = &layout_table;
