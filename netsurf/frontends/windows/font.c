/*
 * Copyright 2009 - 2014 Vincent Sanders <vince@netsurf-browser.org>
 * Copyright 2009 - 2013 Michael Drake <tlsa@netsurf-browser.org>
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
 * Windows font handling and character encoding implementation.
 */

#include "utils/config.h"
#include <assert.h>
#include <windows.h>

#include "netsurf/inttypes.h"
#include "utils/log.h"
#include "utils/nsoption.h"
#include "utils/utf8.h"
#include "netsurf/layout.h"
#include "netsurf/utf8.h"
#include "netsurf/plot_style.h"

#include "windows/font.h"

HWND font_hwnd;

/* exported interface documented in windows/font.h */
nserror
utf8_to_font_encoding(const struct font_desc* font,
		      const char *string,
		      size_t len,
		      char **result)
{
	return utf8_to_enc(string, font->encoding, len, result);
}


/**
 * Convert a string to UCS2 from UTF8
 *
 * \param[in] string source string
 * \param[in] len source string length
 * \param[out] result The UCS2 string
 */
static nserror
utf8_to_local_encoding(const char *string, size_t len, char **result)
{
	return utf8_to_enc(string, "UCS-2", len, result);
}


/**
 * Convert a string to UTF8 from local encoding
 *
 * \param[in] string source string
 * \param[in] len source string length
 * \param[out] result The UTF8 string
 */
static nserror
utf8_from_local_encoding(const char *string, size_t len, char **result)
{
	assert(string && result);

	if (len == 0) {
		len = strlen(string);
	}

	*result = strndup(string, len);
	if (!(*result)) {
		return NSERROR_NOMEM;
	}

	return NSERROR_OK;
}


/* exported interface documented in windows/font.h */
HFONT get_font(const plot_font_style_t *style)
{
	char *face = NULL;
	DWORD family;
	int nHeight;
	HDC hdc;
	HFONT font;

	switch(style->family) {
	case PLOT_FONT_FAMILY_SERIF:
		face = strdup(nsoption_charp(font_serif));
		family = FF_ROMAN | DEFAULT_PITCH;
		break;

	case PLOT_FONT_FAMILY_MONOSPACE:
		face = strdup(nsoption_charp(font_mono));
		family = FF_MODERN | DEFAULT_PITCH;
		break;

	case PLOT_FONT_FAMILY_CURSIVE:
		face = strdup(nsoption_charp(font_cursive));
		family = FF_SCRIPT | DEFAULT_PITCH;
		break;

	case PLOT_FONT_FAMILY_FANTASY:
		face = strdup(nsoption_charp(font_fantasy));
		family = FF_DECORATIVE | DEFAULT_PITCH;
		break;

	case PLOT_FONT_FAMILY_SANS_SERIF:
	default:
		face = strdup(nsoption_charp(font_sans));
		family = FF_SWISS | DEFAULT_PITCH;
		break;
	}

	nHeight = -10;

	hdc = GetDC(font_hwnd);
	nHeight = -MulDiv(style->size, GetDeviceCaps(hdc, LOGPIXELSY), 72 * PLOT_STYLE_SCALE);
	ReleaseDC(font_hwnd, hdc);

	font = CreateFont(nHeight, /* height */
			  0, /* width */
			  0, /* escapement*/
			  0, /* orientation */
			  style->weight,
			  (style->flags & FONTF_ITALIC) ? TRUE : FALSE,
			  FALSE, /* underline */
			  FALSE, /* strike */
			  DEFAULT_CHARSET, /* for locale */
			  OUT_DEFAULT_PRECIS, /* general 'best match' */
			  CLIP_DEFAULT_PRECIS,
			  DEFAULT_QUALITY,
			  family,
			  face); /* name of font face */

	if (face != NULL) {
		free(face);
	}

	if (font == NULL) {
		if (style->family == PLOT_FONT_FAMILY_MONOSPACE) {
			font = (HFONT) GetStockObject(ANSI_FIXED_FONT);
		} else {
			font = (HFONT) GetStockObject(ANSI_VAR_FONT);
		}
	}

	if (font == NULL) {
		font = (HFONT) GetStockObject(SYSTEM_FONT);
	}

	return font;
}

/* size of temporary wide character string for computing string width */
#define WSTRLEN 4096


/**
 * Measure the width of a string.
 *
 * \param[in] style plot style for this text
 * \param[in] utf8str string encoded in UTF-8 to measure
 * \param[in] utf8len length of string, in bytes
 * \param[out] width updated to width of string[0..length)
 * \return NSERROR_OK on success otherwise appropriate error code
 */
static nserror
win32_font_width(const plot_font_style_t *style,
		 const char *utf8str,
		 size_t utf8len,
		 int *width)
{
	nserror ret = NSERROR_OK;
	HDC hdc;
	HFONT font;
	HFONT fontbak;
	SIZE sizl; /* size in logical units */
	BOOL wres;
	int wclen; /* wide char length */
	static WCHAR wstr[WSTRLEN]; /* temporary wide char string */

	if (utf8len == 0) {
		*width = 0;
		return ret;
	}

	hdc = GetDC(NULL);
	font = get_font(style);
	fontbak = SelectObject(hdc, font);

	wclen = MultiByteToWideChar(CP_UTF8, 0, utf8str, utf8len, wstr, WSTRLEN);
	if (wclen != 0) {
		wres = GetTextExtentPoint32W(hdc, wstr, wclen, &sizl);
		if (wres == FALSE) {
			ret = NSERROR_INVALID;
		} else {
			*width = sizl.cx;
		}
	} else {
		ret = NSERROR_NOSPACE;
	}

	font = SelectObject(hdc, fontbak);
	DeleteObject(font);
	ReleaseDC(NULL, hdc);

	return ret;
}


/**
 * Find the position in a string where an x coordinate falls.
 *
 * \param  style css_style for this text, with style->font_size.size ==
 *			CSS_FONT_SIZE_LENGTH
 * \param  utf8str string to measure encoded in UTF-8
 * \param  utf8len length of string
 * \param  x coordinate to search for
 * \param  char_offset updated to offset in string of actual_x, [0..length]
 * \param  actual_x updated to x coordinate of character closest to x
 * \return NSERROR_OK on success otherwise appropriate error code
 */
static nserror
win32_font_position(const plot_font_style_t *style,
		    const char *utf8str,
		    size_t utf8len,
		    int x,
		    size_t *char_offset,
		    int *actual_x)
{
	HDC hdc;
	HFONT font;
	HFONT fontbak;
	SIZE s;
	int offset;
	nserror ret = NSERROR_OK;

	/* deal with zero length input or invalid search co-ordiate */
	if ((utf8len == 0) || (x < 1)) {
		*char_offset = 0;
		*actual_x = 0;
		return ret;
	}

	hdc = GetDC(NULL);
	font = get_font(style);
	fontbak = SelectObject(hdc, font);

	if ((GetTextExtentExPointA(hdc, utf8str, utf8len, x, &offset, NULL, &s) != 0) &&
	    (GetTextExtentPoint32A(hdc, utf8str, offset, &s) != 0)) {
		*char_offset = (size_t)offset;
		*actual_x = s.cx;
	} else {
		ret = NSERROR_UNKNOWN;
	}

	font = SelectObject(hdc, fontbak);
	DeleteObject(font);
	ReleaseDC(NULL, hdc);

	return ret;
}


/**
 * Find where to split a string to make it fit a width.
 *
 * \param  style	css_style for this text, with style->font_size.size ==
 *			CSS_FONT_SIZE_LENGTH
 * \param  string	UTF-8 string to measure
 * \param  length	length of string
 * \param  x		width available
 * \param[out] offset updated to offset in string of actual_x, [0..length]
 * \param  actual_x	updated to x coordinate of character closest to x
 * \return NSERROR_OK on success otherwise appropriate error code
 *
 * On exit, [char_offset == 0 ||
 *	   string[char_offset] == ' ' ||
 *	   char_offset == length]
 */
static nserror
win32_font_split(const plot_font_style_t *style,
		 const char *string,
		 size_t length,
		 int x,
		 size_t *offset,
		 int *actual_x)
{
	nserror res;
	int c_off;

	/* get the offset into teh string on the proposed position */
	res = win32_font_position(style, string, length, x, offset, actual_x);
	if (res != NSERROR_OK) {
		return res;
	}

	/* return the whole string fits in the proposed length */
	if (*offset == length) {
		return NSERROR_OK;
	}

	c_off = *offset;

	/* walk backwards through string looking for space to break on */
	while ((string[*offset] != ' ') &&
	       (*offset > 0)) {
		(*offset)--;
	}

	/* walk forwards through string looking for space if back failed */
	if (*offset == 0) {
		*offset = c_off;
		while ((*offset < length) &&
		       (string[*offset] != ' ')) {
			(*offset)++;
		}
	}

	/* find the actual string width of the break */
	res = win32_font_width(style, string, *offset, actual_x);

	NSLOG(netsurf, DEEPDEBUG,
	      "ret %d Split %u chars at %ipx: Split at char %i (%ipx) - %.*s",
	      res, length, x, *offset, *actual_x, *offset, string);

	return res;
}


/** win32 font operations table */
static struct gui_layout_table layout_table = {
	.width = win32_font_width,
	.position = win32_font_position,
	.split = win32_font_split,
};

struct gui_layout_table *win32_layout_table = &layout_table;

/** win32 utf8 encoding operations table */
static struct gui_utf8_table utf8_table = {
	.utf8_to_local = utf8_to_local_encoding,
	.local_to_utf8 = utf8_from_local_encoding,
};

struct gui_utf8_table *win32_utf8_table = &utf8_table;
