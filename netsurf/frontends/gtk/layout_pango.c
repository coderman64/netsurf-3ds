/*
 * Copyright 2005 James Bursa <bursa@users.sourceforge.net>
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
 * GTK implementation of layout handling using pango.
 *
 * Pango is used handle and render fonts.
 */


#include <assert.h>
#include <stdio.h>
#include <gtk/gtk.h>

#include "utils/log.h"
#include "utils/nsoption.h"
#include "netsurf/inttypes.h"
#include "netsurf/layout.h"
#include "netsurf/plot_style.h"

#include "gtk/layout_pango.h"
#include "gtk/plotters.h"

static PangoContext *nsfont_pango_context = NULL;
static PangoLayout *nsfont_pango_layout = NULL;

static inline void nsfont_pango_check(void)
{
	if (nsfont_pango_context == NULL) {
		NSLOG(netsurf, INFO, "Creating nsfont_pango_context.");
		nsfont_pango_context = gdk_pango_context_get();
	}
	
	if (nsfont_pango_layout == NULL) {
		NSLOG(netsurf, INFO, "Creating nsfont_pango_layout.");
		nsfont_pango_layout = pango_layout_new(nsfont_pango_context);
	}
}

/**
 * Measure the width of a string.
 *
 * \param[in] fstyle plot style for this text
 * \param[in] string UTF-8 string to measure
 * \param[in] length length of string, in bytes
 * \param[out] width updated to width of string[0..length)
 * \return NSERROR_OK and width updated or appropriate error code on faliure
 */
static nserror
nsfont_width(const plot_font_style_t *fstyle,
	     const char *string,
	     size_t length,
	     int *width)
{
	PangoFontDescription *desc;

	if (length == 0) {
		*width = 0;
		return NSERROR_OK;
	}
	
	nsfont_pango_check();

	desc = nsfont_style_to_description(fstyle);
	pango_layout_set_font_description(nsfont_pango_layout, desc);
	pango_font_description_free(desc);

	pango_layout_set_text(nsfont_pango_layout, string, length);

	pango_layout_get_pixel_size(nsfont_pango_layout, width, 0);

	NSLOG(netsurf, DEEPDEBUG,
	      "fstyle: %p string:\"%.*s\", length: %" PRIsizet ", width: %dpx",
	      fstyle, (int)length, string, length, *width);
	 

	return NSERROR_OK;
}


/**
 * Find the position in a string where an x coordinate falls.
 *
 * \param[in] fstyle style for this text
 * \param[in] string UTF-8 string to measure
 * \param[in] length length of string, in bytes
 * \param[in] x coordinate to search for
 * \param[out] char_offset updated to offset in string of actual_x, [0..length]
 * \param[out] actual_x updated to x coordinate of character closest to x
 * \return NSERROR_OK and char_offset and actual_x updated or appropriate
 *          error code on faliure
 */
static nserror
nsfont_position_in_string(const plot_font_style_t *fstyle,
			  const char *string,
			  size_t length,
			  int x,
			  size_t *char_offset,
			  int *actual_x)
{
	int index;
	PangoFontDescription *desc;
	PangoRectangle pos;

	nsfont_pango_check();

	desc = nsfont_style_to_description(fstyle);
	pango_layout_set_font_description(nsfont_pango_layout, desc);
	pango_font_description_free(desc);

	pango_layout_set_text(nsfont_pango_layout, string, length);

	if (pango_layout_xy_to_index(nsfont_pango_layout,
				     x * PANGO_SCALE, 
				     0, &index, 0) == FALSE) {
		index = length;
	}

	pango_layout_index_to_pos(nsfont_pango_layout, index, &pos);

	*char_offset = index;
	*actual_x = PANGO_PIXELS(pos.x);

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
nsfont_split(const plot_font_style_t *fstyle,
	     const char *string,
	     size_t length,
	     int x,
	     size_t *char_offset,
	     int *actual_x)
{
	int index = length;
	PangoFontDescription *desc;
	PangoContext *context;
	PangoLayout *layout;
	PangoLayoutLine *line;

	context = gdk_pango_context_get();
	layout = pango_layout_new(context);

	desc = nsfont_style_to_description(fstyle);
	pango_layout_set_font_description(layout, desc);
	pango_font_description_free(desc);

	pango_layout_set_text(layout, string, length);

	/* Limit width of layout to the available width */
	pango_layout_set_width(layout, x * PANGO_SCALE);

	/* Request word wrapping */
	pango_layout_set_wrap(layout, PANGO_WRAP_WORD);

	/* Prevent pango treating linebreak characters as line breaks */
	pango_layout_set_single_paragraph_mode(layout, TRUE);

	/* Obtain the second line of the layout (if there is one) */
	line = pango_layout_get_line_readonly(layout, 1);
	if (line != NULL) {
		/* Pango split the text. The line's start_index indicates the 
		 * start of the character after the line break. */
		index = line->start_index;
	}

	g_object_unref(layout);
	g_object_unref(context);

	*char_offset = index;
	/* Obtain the pixel offset of the split character */
	nsfont_width(fstyle, string, index, actual_x);

	return NSERROR_OK;
}


/**
 * Render a string.
 *
 * \param  x	   x coordinate
 * \param  y	   y coordinate
 * \param  string  UTF-8 string to measure
 * \param  length  length of string
 * \param  fstyle  plot style for this text
 * \return  true on success, false on error and error reported
 */
nserror nsfont_paint(int x, int y, const char *string, size_t length,
		const plot_font_style_t *fstyle)
{
	PangoFontDescription *desc;
	PangoLayoutLine *line;

	if (length == 0)
		return NSERROR_OK;

	nsfont_pango_check();

	desc = nsfont_style_to_description(fstyle);
	pango_layout_set_font_description(nsfont_pango_layout, desc);
	pango_font_description_free(desc);

	pango_layout_set_text(nsfont_pango_layout, string, length);

	line = pango_layout_get_line_readonly(nsfont_pango_layout, 0);
	cairo_move_to(current_cr, x, y);
	nsgtk_set_colour(fstyle->foreground);
	pango_cairo_show_layout_line(current_cr, line);

	return NSERROR_OK;
}


/* exported interface documented in gtk/layout_pango.h */
PangoFontDescription *
nsfont_style_to_description(const plot_font_style_t *fstyle)
{
	unsigned int size;
	PangoFontDescription *desc;
	PangoStyle style = PANGO_STYLE_NORMAL;

	switch (fstyle->family) {
	case PLOT_FONT_FAMILY_SERIF:
		desc = pango_font_description_from_string(nsoption_charp(font_serif));
		break;
	case PLOT_FONT_FAMILY_MONOSPACE:
		desc = pango_font_description_from_string(nsoption_charp(font_mono));
		break;
	case PLOT_FONT_FAMILY_CURSIVE:
		desc = pango_font_description_from_string(nsoption_charp(font_cursive));
		break;
	case PLOT_FONT_FAMILY_FANTASY:
		desc = pango_font_description_from_string(nsoption_charp(font_fantasy));
		break;
	case PLOT_FONT_FAMILY_SANS_SERIF:
	default:
		desc = pango_font_description_from_string(nsoption_charp(font_sans));
		break;
	}

	size = (fstyle->size * PANGO_SCALE) / PLOT_STYLE_SCALE;

	if (fstyle->flags & FONTF_ITALIC)
		style = PANGO_STYLE_ITALIC;
	else if (fstyle->flags & FONTF_OBLIQUE)
		style = PANGO_STYLE_OBLIQUE;

	pango_font_description_set_style(desc, style);

	pango_font_description_set_weight(desc, (PangoWeight) fstyle->weight);

	pango_font_description_set_size(desc, size);

	if (fstyle->flags & FONTF_SMALLCAPS) {
		pango_font_description_set_variant(desc, 
				PANGO_VARIANT_SMALL_CAPS);
	} else {
		pango_font_description_set_variant(desc, PANGO_VARIANT_NORMAL);
	}

	return desc;
}

static struct gui_layout_table layout_table = {
	.width = nsfont_width,
	.position = nsfont_position_in_string,
	.split = nsfont_split,
};

struct gui_layout_table *nsgtk_layout_table = &layout_table;
