/*
 * Copyright 2005 James Bursa <bursa@users.sourceforge.net>
 *           2008 Vincent Sanders <vince@simtec.co.uk>
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

#include <inttypes.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

#include "utils/nsoption.h"
#include "utils/utf8.h"
#include "netsurf/utf8.h"
#include "netsurf/layout.h"
#include "netsurf/plot_style.h"

#include "framebuffer/gui.h"
#include "framebuffer/font.h"

#include <font-ns-sans.h>

#define GLYPH_LEN		16

uint8_t code_point[GLYPH_LEN];
uint8_t glyph_x2[GLYPH_LEN * 4];

#define SEVEN_SET	((1 << 0) | (1 << 1) | (1 << 2) | (1 << 3) | 	\
			 (1 << 4) | (1 << 5) | (1 << 6))

#define THREE_SSS	 ((1 << 0) | (1 << 1) | (1 << 2))
#define THREE_S_S	 ((1 << 0) |            (1 << 2))
#define THREE__SS	 ((1 << 0) | (1 << 1)           )
#define THREE_SS_	 (           (1 << 1) | (1 << 2))
#define THREE_S__	                        (1 << 2)
#define THREE__S_	             (1 << 1)
#define THREE___S	  (1 << 0)

uint8_t frag[16][5] = {
	{ THREE_SSS,
	  THREE_S_S,
	  THREE_S_S,
	  THREE_S_S,
	  THREE_SSS },

	{ THREE__S_,
	  THREE_SS_,
	  THREE__S_,
	  THREE__S_,
	  THREE_SSS },

	{ THREE_SS_,
	  THREE___S,
	  THREE__S_,
	  THREE_S__,
	  THREE_SSS },

	{ THREE_SS_,
	  THREE___S,
	  THREE_SS_,
	  THREE___S,
	  THREE_SS_ },

	{ THREE_S_S,
	  THREE_S_S,
	  THREE_SSS,
	  THREE___S,
	  THREE___S },

	{ THREE_SSS,
	  THREE_S__,
	  THREE_SSS,
	  THREE___S,
	  THREE_SSS },

	{ THREE__SS,
	  THREE_S__,
	  THREE_SSS,
	  THREE_S_S,
	  THREE_SSS },

	{ THREE_SSS,
	  THREE___S,
	  THREE__S_,
	  THREE__S_,
	  THREE__S_ },

	{ THREE_SSS,
	  THREE_S_S,
	  THREE_SSS,
	  THREE_S_S,
	  THREE_SSS },

	{ THREE_SSS,
	  THREE_S_S,
	  THREE_SSS,
	  THREE___S,
	  THREE___S },

	{ THREE__S_,
	  THREE_S_S,
	  THREE_SSS,
	  THREE_S_S,
	  THREE_S_S },

	{ THREE_SS_,
	  THREE_S_S,
	  THREE_SS_,
	  THREE_S_S,
	  THREE_SS_ },

	{ THREE__S_,
	  THREE_S_S,
	  THREE_S__,
	  THREE_S_S,
	  THREE__S_ },

	{ THREE_SS_,
	  THREE_S_S,
	  THREE_S_S,
	  THREE_S_S,
	  THREE_SS_ },

	{ THREE_SSS,
	  THREE_S__,
	  THREE_SS_,
	  THREE_S__,
	  THREE_SSS },

	{ THREE_SSS,
	  THREE_S__,
	  THREE_SS_,
	  THREE_S__,
	  THREE_S__ }
};

static uint8_t * get_codepoint(uint32_t id, bool italic)
{
	int shift = 0;
	int l;
	int r;

	if (!italic)
		shift = 1;

	l =       (id >> 12);
	r = 0xf & (id >>  8);

	code_point[ 0] = SEVEN_SET << shift;

	code_point[ 2] = (frag[l][0] << (4 + shift)) | (frag[r][0] << shift);
	code_point[ 3] = (frag[l][1] << (4 + shift)) | (frag[r][1] << shift);
	code_point[ 4] = (frag[l][2] << (4 + shift)) | (frag[r][2] << shift);
	code_point[ 5] = (frag[l][3] << (4 + shift)) | (frag[r][3] << shift);
	code_point[ 6] = (frag[l][4] << (4 + shift)) | (frag[r][4] << shift);

	shift = 1;

	l = 0xf & (id >>  4);
	r = 0xf & id;

	code_point[ 8] = (frag[l][0] << (4 + shift)) | (frag[r][0] << shift);
	code_point[ 9] = (frag[l][1] << (4 + shift)) | (frag[r][1] << shift);
	code_point[10] = (frag[l][2] << (4 + shift)) | (frag[r][2] << shift);
	code_point[11] = (frag[l][3] << (4 + shift)) | (frag[r][3] << shift);
	code_point[12] = (frag[l][4] << (4 + shift)) | (frag[r][4] << shift);

	code_point[14] = SEVEN_SET << shift;

	return (uint8_t *)code_point;
}


bool fb_font_init(void)
{
	return true;
}

bool fb_font_finalise(void)
{
	return true;
}

enum fb_font_style
fb_get_font_style(const plot_font_style_t *fstyle)
{
	enum fb_font_style style = FB_REGULAR;

	if (fstyle->weight >= 700)
		style |= FB_BOLD;
	if ((fstyle->flags & FONTF_ITALIC) || (fstyle->flags & FONTF_OBLIQUE))
		style |= FB_ITALIC;

	return style;
}

int
fb_get_font_size(const plot_font_style_t *fstyle)
{
	int size = fstyle->size * 10 /
			(((nsoption_int(font_min_size) * 3 +
			   nsoption_int(font_size)) / 4) * PLOT_STYLE_SCALE);
	if (size > 2)
		size = 2;
	else if (size <= 0)
		size = 1;

	return size;
}

/** Lookup table to scale 4 bits to 8 bits, so e.g. 0101 --> 00110011 */
const uint8_t glyph_lut[16] = {
		0x00, 0x03, 0x0c, 0x0f,
		0x30, 0x33, 0x3c, 0x3f,
		0xc0, 0xc3, 0xcc, 0xcf,
		0xf0, 0xf3, 0xfc, 0xff
};

static const uint8_t *
glyph_scale_2(const uint8_t *glyph_data)
{
	const uint8_t *glyph_max = glyph_data + GLYPH_LEN;
	uint8_t *pos = glyph_x2;

	do {
		*pos++ = glyph_lut[*glyph_data >> 4];
		*pos++ = glyph_lut[*glyph_data & 0xf];
		*pos++ = glyph_lut[*glyph_data >> 4];
		*pos++ = glyph_lut[*glyph_data & 0xf];
	} while (++glyph_data < glyph_max);

	return glyph_x2;
}

const uint8_t *
fb_get_glyph(uint32_t ucs4, enum fb_font_style style, int scale)
{
	const uint8_t *glyph_data;
	unsigned int section;
	unsigned int offset;
	uint16_t g_offset;

	/* Internal font has no glyphs beyond U+FFFF and there isn't
	 * space to render a >4 digit codepoint; just show replacement
	 * character. */
	if (ucs4 > 0xffff)
		ucs4 = 0xfffd;

	switch (style) {
	case FB_BOLD_ITALIC:
		section = fb_bold_italic_section_table[ucs4 / 256];
		if (section != 0 || ucs4 / 256 == 0) {
			offset = section * 256 + (ucs4 & 0xff);
			g_offset = fb_bold_italic_sections[offset] * 16;
			if (g_offset != 0) {
				glyph_data = &font_glyph_data[g_offset];
				break;
			}
		}
		/* Fall through. */
	case FB_BOLD:
		section = fb_bold_section_table[ucs4 / 256];
		if (section != 0 || ucs4 / 256 == 0) {
			offset = section * 256 + (ucs4 & 0xff);
			g_offset = fb_bold_sections[offset] * 16;
			if (g_offset != 0) {
				glyph_data = &font_glyph_data[g_offset];
				break;
			}
		}
		/* Fall through. */
	case FB_ITALIC:
		section = fb_italic_section_table[ucs4 / 256];
		if (section != 0 || ucs4 / 256 == 0) {
			offset = section * 256 + (ucs4 & 0xff);
			g_offset = fb_italic_sections[offset] * 16;
			if (g_offset != 0) {
				glyph_data = &font_glyph_data[g_offset];
				break;
			}
		}
		/* Fall through. */
	case FB_REGULAR:
		section = fb_regular_section_table[ucs4 / 256];
		if (section != 0 || ucs4 / 256 == 0) {
			offset = section * 256 + (ucs4 & 0xff);
			g_offset = fb_regular_sections[offset] * 16;
			if (g_offset != 0) {
				glyph_data = &font_glyph_data[g_offset];
				break;
			}
		}
		/* Fall through. */
	default:
		glyph_data = get_codepoint(ucs4, style & FB_ITALIC);
		break;
	}

	switch (scale) {
	case 1:
		break;
	case 2:
		glyph_data = glyph_scale_2(glyph_data);
		break;
	default:
		assert(scale >= 1 && scale <= 2);
		break;
	}

	return glyph_data;
}

static nserror utf8_to_local(const char *string,
				       size_t len,
				       char **result)
{
	return utf8_to_enc(string, "CP1252", len, result);

}

static nserror utf8_from_local(const char *string,
					size_t len,
					char **result)
{
	*result = malloc(len + 1);
	if (*result == NULL) {
		return NSERROR_NOMEM;
	}

	memcpy(*result, string, len);

	(*result)[len] = '\0';

	return NSERROR_OK;
}


/* exported interface documented in framebuffer/freetype_font.h */
nserror
fb_font_width(const plot_font_style_t *fstyle,
	      const char *string,
	      size_t length,
	      int *width)
{
        size_t nxtchr = 0;

	*width = 0;
        while (nxtchr < length) {
		uint32_t ucs4;
		ucs4 = utf8_to_ucs4(string + nxtchr, length - nxtchr);
		if (codepoint_displayable(ucs4)) {
			*width += FB_FONT_WIDTH;
		}

		nxtchr = utf8_next(string, length, nxtchr);
        }

	*width *= fb_get_font_size(fstyle);
	return NSERROR_OK;
}


/* exported interface documented in framebuffer/freetype_font.h */
nserror
fb_font_position(const plot_font_style_t *fstyle,
		 const char *string,
		 size_t length,
		 int x,
		 size_t *char_offset,
		 int *actual_x)
{
        const int width = fb_get_font_size(fstyle) * FB_FONT_WIDTH;
        size_t nxtchr = 0;
	int x_pos = 0;

        while (nxtchr < length) {
		uint32_t ucs4;
                if (abs(x_pos - x) <= (width / 2))
                        break;

		ucs4 = utf8_to_ucs4(string + nxtchr, length - nxtchr);
		if (codepoint_displayable(ucs4)) {
			x_pos += width;
		}

                nxtchr = utf8_next(string, length, nxtchr);
        }

        *actual_x = x_pos;

        *char_offset = nxtchr;
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
static nserror
fb_font_split(const plot_font_style_t *fstyle,
	      const char *string,
	      size_t length,
	      int x,
	      size_t *char_offset,
	      int *actual_x)
{
        const int width = fb_get_font_size(fstyle) * FB_FONT_WIDTH;
        size_t nxtchr = 0;
        int last_space_x = 0;
        int last_space_idx = 0;

        *actual_x = 0;
        while (nxtchr < length) {
		uint32_t ucs4;

                if (string[nxtchr] == ' ') {
                        last_space_x = *actual_x;
                        last_space_idx = nxtchr;
                }

		ucs4 = utf8_to_ucs4(string + nxtchr, length - nxtchr);
		if (codepoint_displayable(ucs4)) {
			*actual_x += width;
		}

                if (*actual_x > x && last_space_idx != 0) {
                        /* string has exceeded available width and we've
                         * found a space; return previous space */
                        *actual_x = last_space_x;
                        *char_offset = last_space_idx;
                        return NSERROR_OK;
                }

                nxtchr = utf8_next(string, length, nxtchr);
        }

        *char_offset = nxtchr;

	return NSERROR_OK;
}


static struct gui_layout_table layout_table = {
	.width = fb_font_width,
	.position = fb_font_position,
	.split = fb_font_split,
};

struct gui_layout_table *framebuffer_layout_table = &layout_table;


static struct gui_utf8_table utf8_table = {
	.utf8_to_local = utf8_to_local,
	.local_to_utf8 = utf8_from_local,
};

struct gui_utf8_table *framebuffer_utf8_table = &utf8_table;


/*
 * Local Variables:
 * c-basic-offset:8
 * End:
 */
