/*
 * Copyright 2008 François Revol <mmu_man@users.sourceforge.net>
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
 * Beos font layout handling interface.
 */

#ifndef NS_BEOS_FONT_H
#define NS_BEOS_FONT_H

#include "netsurf/plot_style.h"

bool nsfont_paint(const plot_font_style_t *fstyle,
		const char *string, size_t length,
		int x, int y);

void nsbeos_style_to_font(BFont &font, const struct plot_font_style *fstyle);

extern struct gui_layout_table *beos_layout_table;

#endif
