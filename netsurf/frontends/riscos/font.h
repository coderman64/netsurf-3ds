/*
 * Copyright 2014 Vincent Sanders <vince@netsurf-browser.org>
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

/** \file riscos/font.h
 * RISC OS font interface.
 */

#ifndef _NETSURF_RISCOS_FONT_H_
#define _NETSURF_RISCOS_FONT_H_

#include <rufl.h>

struct plot_font_style;
extern struct gui_layout_table *riscos_layout_table;

/** desktop font, size and style being used */
extern char ro_gui_desktop_font_family[];
extern int ro_gui_desktop_font_size;
extern rufl_style ro_gui_desktop_font_style;

void nsfont_init(void);
bool nsfont_exists(const char *font_family);
const char *nsfont_fallback_font(void);
bool nsfont_paint(const struct plot_font_style *fstyle, const char *string,
		size_t length, int x, int y);
void nsfont_read_style(const struct plot_font_style *fstyle,
		const char **font_family, unsigned int *font_size,
		rufl_style *font_style);
void ro_gui_wimp_get_desktop_font(void);

#endif
