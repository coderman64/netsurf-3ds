/*
 * Copyright 2008, 2009, 2012, 2016 Chris Young <chris@unsatisfactorysoftware.co.uk>
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

#ifndef AMIGA_FONT_H
#define AMIGA_FONT_H

#include "netsurf/plotters.h"
#include "utils/errors.h"
#include <graphics/rastport.h>
#include <graphics/text.h>

void ami_font_init(void);
void ami_font_fini(void);

/* DPI stuff */
void ami_font_setdevicedpi(int id);
ULONG ami_font_dpi_get_devicedpi(void);
ULONG ami_font_dpi_get_xdpi(void);

/* Simple diskfont functions for graphics.library use (not page rendering) */
struct TextFont *ami_font_open_disk_font(struct TextAttr *tattr);
void ami_font_close_disk_font(struct TextFont *tfont);

/* Font engine tables */
struct ami_font_functions {
	nserror (*width)(const plot_font_style_t *fstyle,
			const char *string, size_t length,
			int *width);

	nserror (*posn)(const plot_font_style_t *fstyle,
			const char *string, size_t length,
			int x, size_t *char_offset, int *actual_x);

	nserror (*split)(const plot_font_style_t *fstyle,
			const char *string, size_t length,
			int x, size_t *char_offset, int *actual_x);

	ULONG (*text)(struct RastPort *rp, const char *string,
			ULONG length, const plot_font_style_t *fstyle,
			ULONG x, ULONG y, bool aa);
};

const struct ami_font_functions *ami_nsfont;

extern struct gui_layout_table *ami_layout_table;

#endif

