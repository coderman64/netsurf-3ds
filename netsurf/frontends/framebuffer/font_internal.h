/*
 * Copyright 2008 Vincent Sanders <vince@simtec.co.uk>
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

#ifndef NETSURF_FB_FONT_INTERNAL_H
#define NETSURF_FB_FONT_INTERNAL_H

#include <stdbool.h>

struct fb_font_desc {
    const char *name;
    int width, height, pitch;
};

#define FB_FONT_WIDTH		8
#define FB_FONT_HEIGHT		16
#define FB_FONT_PITCH		8

enum fb_font_style {
	FB_REGULAR	= 0,
	FB_ITALIC	= (1 << 0),
	FB_BOLD		= (1 << 1),
	FB_BOLD_ITALIC	= (FB_ITALIC | FB_BOLD)
};

enum fb_font_style fb_get_font_style(const plot_font_style_t *fstyle);
int fb_get_font_size(const plot_font_style_t *fstyle);

#define codepoint_displayable(u) \
	(!(u >= 0x200b && u <= 0x200f))

const uint8_t *fb_get_glyph(uint32_t ucs4, enum fb_font_style style, int scale);

#endif /* NETSURF_FB_FONT_INTERNAL_H */

