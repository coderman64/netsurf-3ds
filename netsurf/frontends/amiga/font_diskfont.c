/*
 * Copyright 2008 - 2016 Chris Young <chris@unsatisfactorysoftware.co.uk>
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

#include "amiga/os3support.h"

#include <stdlib.h>
#include <string.h>
#include <proto/diskfont.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/utility.h>

#include <graphics/rpattr.h>

#include "utils/log.h"
#include "utils/utf8.h"
#include "utils/nsoption.h"

#include "amiga/font.h"
#include "amiga/font_diskfont.h"
#include "amiga/gui.h"
#include "amiga/plotters.h"
#include "amiga/utf8.h"

#define MAX_FONT_NAME_SIZE 33

static plot_font_style_t *prev_fstyle = NULL;
static struct TextFont *prev_font = NULL;
static struct RastPort temp_rp;

static struct TextFont *ami_font_bm_open(struct RastPort *rp, const plot_font_style_t *fstyle)
{
	struct TextFont *bmfont = NULL;
	struct TextAttr tattr;
	char *fontname;
	char font[MAX_FONT_NAME_SIZE];

	if((prev_fstyle != NULL) && (prev_font != NULL) &&
		(fstyle->family == prev_fstyle->family) &&
		(fstyle->size == prev_fstyle->size) &&
		(fstyle->flags == prev_fstyle->flags) &&
		(fstyle->weight == prev_fstyle->weight)) {
		return prev_font;
	}

	if(rp == NULL) return NULL;

	tattr.ta_Flags = 0;

	switch(fstyle->family)
	{
		case PLOT_FONT_FAMILY_SANS_SERIF:
			fontname = nsoption_charp(font_sans);
		break;
		case PLOT_FONT_FAMILY_SERIF:
			fontname = nsoption_charp(font_serif);
		break;
		case PLOT_FONT_FAMILY_MONOSPACE:
			fontname = nsoption_charp(font_mono);
		break;
		case PLOT_FONT_FAMILY_CURSIVE:
			fontname = nsoption_charp(font_cursive);
		break;
		case PLOT_FONT_FAMILY_FANTASY:
			fontname = nsoption_charp(font_fantasy);
		break;
		default:
			return NULL;
		break;
	}

	tattr.ta_Style = FS_NORMAL;

	if (fstyle->flags & FONTF_OBLIQUE)
		tattr.ta_Style = FSF_ITALIC;

	if (fstyle->flags & FONTF_ITALIC)
		tattr.ta_Style = FSF_ITALIC;

	if (fstyle->weight >= 700)
		tattr.ta_Style |= FSF_BOLD;

	snprintf(font, MAX_FONT_NAME_SIZE, "%s.font", fontname);
	tattr.ta_Name = font;
	tattr.ta_YSize = fstyle->size / PLOT_STYLE_SCALE;
	NSLOG(netsurf, INFO, "font: %s/%d", tattr.ta_Name, tattr.ta_YSize);

	if(prev_font != NULL) CloseFont(prev_font);

	if((bmfont = OpenDiskFont(&tattr))) {
		SetRPAttrs(rp, RPTAG_Font, bmfont, TAG_DONE);
	}

	if(prev_fstyle != NULL) {
		memcpy(prev_fstyle, fstyle, sizeof(plot_font_style_t));
		prev_font = bmfont;
	}

	return bmfont;
}

static size_t ami_font_bm_convert_local_to_utf8_offset(const char *utf8string, size_t length, UWORD offset)
{
	size_t chr = 0;

	for(UWORD i = 0; i < offset; i++) {
		chr = utf8_next(utf8string, length, chr);
		if(chr > length) return length;
	}
	
	return chr;
}


static nserror amiga_bm_nsfont_width(const plot_font_style_t *fstyle,
		const char *string, size_t length,
		int *width)
{
	char *localtext = NULL;

	*width = length;

	struct TextFont *bmfont = ami_font_bm_open(&temp_rp, fstyle);
	if(bmfont == NULL) return NSERROR_INVALID;

	if(utf8_to_local_encoding(string, length, &localtext) != NSERROR_OK) {
		return NSERROR_INVALID;
	}

	*width = TextLength(&temp_rp, localtext, (UWORD)strlen(localtext));
	free(localtext);

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

static nserror amiga_bm_nsfont_position_in_string(const plot_font_style_t *fstyle,
		const char *string, size_t length,
		int x, size_t *char_offset, int *actual_x)
{
	struct TextExtent extent;
	struct TextFont *bmfont;
	char *localtext = NULL;
	UWORD co = 0;

	bmfont = ami_font_bm_open(&temp_rp, fstyle);
	if(bmfont == NULL) return NSERROR_INVALID;

	if(utf8_to_local_encoding(string, length, &localtext) != NSERROR_OK) {
		return NSERROR_INVALID;
	}

	co = TextFit(&temp_rp, localtext, (UWORD)strlen(localtext),
						&extent, NULL, 1, x, 32767);
	*char_offset = ami_font_bm_convert_local_to_utf8_offset(string, length, co);
	*actual_x = extent.te_Extent.MaxX;

	free(localtext);

	return NSERROR_OK;
}


/**
 * Find where to split a string to make it fit a width.
 *
 * \param  fstyle       style for this text
 * \param  string       UTF-8 string to measure
 * \param  length       length of string
 * \param  x            width available
 * \param  char_offset  updated to offset in string of actual_x, [1..length]
 * \param  actual_x     updated to x coordinate of character closest to x
 * \return  true on success, false on error and error reported
 *
 * On exit, char_offset indicates first character after split point.
 *
 * Note: char_offset of 0 should never be returned.
 *
 * Returns:
 * char_offset giving split point closest to x, where actual_x <= x
 * else
 * char_offset giving split point closest to x, where actual_x > x
 *
 * Returning char_offset == length means no split possible
 */

static nserror amiga_bm_nsfont_split(const plot_font_style_t *fstyle,
		const char *string, size_t length,
		int x, size_t *char_offset, int *actual_x)
{
	struct TextExtent extent;
	UWORD co, offset;
	char *charp;
	char *localtext;

	struct TextFont *bmfont = ami_font_bm_open(&temp_rp, fstyle);
	if(bmfont == NULL) return NSERROR_INVALID;

	if(utf8_to_local_encoding(string, length, &localtext) != NSERROR_OK) {
		return NSERROR_INVALID;
	}

	offset = TextFit(&temp_rp, localtext, (UWORD)strlen(localtext),
				&extent, NULL, 1, (UWORD)x, 32767);

	co = offset;
	charp = localtext + co;
	

	while((*charp != ' ') && (co > 0)) {
		charp--;
		co--;
	}

	if(co == 0) {
		co = offset;
		charp = localtext + co;
		while((*charp != ' ') && (co < strlen(localtext))) {
			charp++;
			co++;
		}
	}
	
	if((co > 0) && (co < strlen(localtext))) {
		*actual_x = TextLength(&temp_rp, localtext, co);
		*char_offset = ami_font_bm_convert_local_to_utf8_offset(string, length, co);
	} else {
		*actual_x = x;
		*char_offset = length;
	}

	free(localtext);

	return NSERROR_OK;
}

static ULONG amiga_bm_nsfont_text(struct RastPort *rp, const char *string, ULONG length,
			const plot_font_style_t *fstyle, ULONG dx, ULONG dy, bool aa)
{
	if(!string || string[0]=='\0') return 0;
	if(!length) return 0;
	if(rp == NULL) return 0;

	struct TextFont *bmfont = ami_font_bm_open(rp, fstyle);
	char *localtext = NULL;
	if(bmfont == NULL) return 0;
	if(utf8_to_local_encoding(string, length, &localtext) == NSERROR_OK) {
		Move(rp, dx, dy);
		Text(rp, localtext, (UWORD)strlen(localtext));
		free(localtext);
	}

	return 0;
}

const struct ami_font_functions ami_font_diskfont_table = {
	amiga_bm_nsfont_width,
	amiga_bm_nsfont_position_in_string,
	amiga_bm_nsfont_split,
	amiga_bm_nsfont_text
};

void ami_font_diskfont_init(void)
{
	/* Set up table */
	ami_nsfont = &ami_font_diskfont_table;

	/* Alloc space to hold currently open font - doesn't matter if this fails */
	prev_fstyle = calloc(1, sizeof(plot_font_style_t));

	/* Init temp RastPort */
	InitRastPort(&temp_rp);
}

void ami_font_diskfont_fini(void)
{
	if(prev_font != NULL) CloseFont(prev_font);
	if(prev_fstyle != NULL) free(prev_fstyle);
}

