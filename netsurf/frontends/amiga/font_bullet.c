/*
 * Copyright 2008 - 2019 Chris Young <chris@unsatisfactorysoftware.co.uk>
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
 * Amiga font handling implementation
 */

#include "amiga/os3support.h"

#include <stdlib.h>

#ifndef __amigaos4__
#include <proto/bullet.h>
#endif
#include <proto/diskfont.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/utility.h>

#include <diskfont/diskfonttag.h>
#include <diskfont/oterrors.h>

#include "utils/log.h"
#include "utils/nsoption.h"
#include "utils/utf8.h"
#include "utils/utils.h"

#include "amiga/memory.h"
#include "amiga/misc.h"
#include "amiga/font.h"
#include "amiga/font_bullet.h"
#include "amiga/font_cache.h"
#include "amiga/font_scan.h"


#define NSA_UNICODE_FONT PLOT_FONT_FAMILY_COUNT

#define NSA_NORMAL 0
#define NSA_ITALIC 1
#define NSA_BOLD 2
#define NSA_BOLDITALIC 3
#define NSA_OBLIQUE 4
#define NSA_BOLDOBLIQUE 6

#define NSA_VALUE_BOLDX (1 << 12)
#define NSA_VALUE_BOLDY 0
#define NSA_VALUE_SHEARSIN (1 << 14)
#define NSA_VALUE_SHEARCOS (1 << 16)

#define NSA_FONT_EMWIDTH(s) (s / PLOT_STYLE_SCALE) * (ami_font_dpi_get_xdpi() / 72.0)

const uint16 sc_table[] = {
		0x0061, 0x1D00, /* a */
		0x0062, 0x0299, /* b */
		0x0063, 0x1D04, /* c */
		0x0064, 0x1D05, /* d */
		0x0065, 0x1D07, /* e */
		0x0066, 0xA730, /* f */
		0x0067, 0x0262, /* g */
		0x0068, 0x029C, /* h */
		0x0069, 0x026A, /* i */
		0x006A, 0x1D0A, /* j */
		0x006B, 0x1D0B, /* k */
		0x006C, 0x029F, /* l */
		0x006D, 0x1D0D, /* m */
		0x006E, 0x0274, /* n */
		0x006F, 0x1D0F, /* o */
		0x0070, 0x1D18, /* p */
		0x0071, 0xA7AF, /* q */
		0x0072, 0x0280, /* r */
		0x0073, 0xA731, /* s */
		0x0074, 0x1D1B, /* t */
		0x0075, 0x1D1C, /* u */
		0x0076, 0x1D20, /* v */
		0x0077, 0x1D21, /* w */
		0x0078, 0xA7EF, /* x (proposed) (Adobe codepoint 0xF778) */
		0x0079, 0x028F, /* y */
		0x007A, 0x1D22, /* z */

		0x00C6, 0x1D01, /* ae */
		0x0153, 0x0276, /* oe */

#if 0
/* TODO: fill in the non-small caps character ids for these */
		0x0000, 0x1D03, /* barred b */
		0x0000, 0x0281, /* inverted r */
		0x0000, 0x1D19, /* reversed r */
		0x0000, 0x1D1A, /* turned r */
		0x0000, 0x029B, /* g with hook */
		0x0000, 0x1D06, /* eth Ð */
		0x0000, 0x1D0C, /* l with stroke */
		0x0000, 0xA7FA, /* turned m */
		0x0000, 0x1D0E, /* reversed n */
		0x0000, 0x1D10, /* open o */
		0x0000, 0x1D15, /* ou */
		0x0000, 0x1D23, /* ezh */
		0x0000, 0x1D26, /* gamma */
		0x0000, 0x1D27, /* lamda */
		0x0000, 0x1D28, /* pi */
		0x0000, 0x1D29, /* rho */
		0x0000, 0x1D2A, /* psi */
		0x0000, 0x1D2B, /* el */
		0x0000, 0xA776, /* rum */

		0x0000, 0x1DDB, /* combining g */
		0x0000, 0x1DDE, /* combining l */
		0x0000, 0x1DDF, /* combining m */
		0x0000, 0x1DE1, /* combining n */
		0x0000, 0x1DE2, /* combining r */

		0x0000, 0x1DA6, /* modifier i */
		0x0000, 0x1DA7, /* modifier i with stroke */
		0x0000, 0x1DAB, /* modifier l */
		0x0000, 0x1DB0, /* modifier n */
		0x0000, 0x1DB8, /* modifier u */
#endif
		0, 0};

static lwc_string *glypharray[0xffff + 1];

static struct List ami_diskfontlib_list;

static inline int32 ami_font_plot_glyph(struct OutlineFont *ofont, struct RastPort *rp,
		uint16 *restrict char1, uint16 *restrict char2, uint32 x, uint32 y, uint32 emwidth, bool aa);
static inline int32 ami_font_width_glyph(struct OutlineFont *ofont, 
		const uint16 *restrict char1, const uint16 *restrict char2, uint32 emwidth);
static struct OutlineFont *ami_open_outline_font(const plot_font_style_t *fstyle,
		const uint16 *codepoint);
static inline ULONG ami_font_unicode_width(const char *string, ULONG length,
		const plot_font_style_t *fstyle, ULONG x, ULONG y, bool aa);

static inline int amiga_nsfont_utf16_char_length(const uint16 *char1)
{
	if (__builtin_expect(((*char1 < 0xD800) || (0xDBFF < *char1)), 1)) {
		return 1;
	} else {
		return 2;
	}
}

static inline uint32 amiga_nsfont_decode_surrogate(const uint16 *char1)
{
	if(__builtin_expect((amiga_nsfont_utf16_char_length(char1) == 2), 0)) {
		return ((uint32)char1[0] << 10) + char1[1] - 0x35FDC00;
	} else {
		return (uint32)*char1;
	}
}

static nserror amiga_nsfont_width(const plot_font_style_t *fstyle,
		const char *string, size_t length,
		int *width)
{
	*width = ami_font_unicode_width(string, length, fstyle, 0, 0, false);

	if(*width <= 0) {
		*width = length; /* fudge */
	}

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

static nserror amiga_nsfont_position_in_string(const plot_font_style_t *fstyle,
		const char *string, size_t length,
		int x, size_t *char_offset, int *actual_x)
{
	uint16 *utf16 = NULL, *outf16 = NULL;
	uint16 *utf16next = NULL;
	struct OutlineFont *ofont, *ufont = NULL;
	int tx = 0;
	uint32 utf8_pos = 0;
	int utf16charlen;
	ULONG emwidth = (ULONG)NSA_FONT_EMWIDTH(fstyle->size);
	int32 tempx;

	if(utf8_to_enc(string,"UTF-16",length,(char **)&utf16) != NSERROR_OK) return NSERROR_INVALID;
	outf16 = utf16;
	if(!(ofont = ami_open_outline_font(fstyle, 0))) return NSERROR_INVALID;

	*char_offset = 0;
	*actual_x = 0;

	while (utf8_pos < length) {
		utf16charlen = amiga_nsfont_utf16_char_length(utf16);
		utf16next = &utf16[utf16charlen];

		tempx = ami_font_width_glyph(ofont, utf16, utf16next, emwidth);

		if (tempx == 0) {
			if (ufont == NULL)
				ufont = ami_open_outline_font(fstyle, utf16);

			if (ufont)
				tempx = ami_font_width_glyph(ufont, utf16,
						utf16next, emwidth);
		}

		tx += tempx;
		utf16 = utf16next;
		utf8_pos = utf8_next(string, length, utf8_pos);

		if(tx < x) {
			*actual_x = tx;
			*char_offset = utf8_pos;
		} else {
			if((x - *actual_x) > (tx - x)) {
				*actual_x = tx;
				*char_offset = utf8_pos;
			}
			free(outf16);
			return NSERROR_OK;
		}
	}

	*actual_x = tx;
	*char_offset = length;

	free(outf16);
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

static nserror amiga_nsfont_split(const plot_font_style_t *fstyle,
		const char *string, size_t length,
		int x, size_t *char_offset, int *actual_x)
{
	uint16 *utf16_str = NULL;
	const uint16 *utf16 = NULL;
	const uint16 *utf16next = NULL;
	struct OutlineFont *ofont, *ufont = NULL;
	int tx = 0;
	uint32 utf8_pos = 0;
	int32 tempx = 0;
	ULONG emwidth = (ULONG)NSA_FONT_EMWIDTH(fstyle->size);

	/* Get utf16 conversion of string for glyph measuring routines */
	if (utf8_to_enc(string, "UTF-16", length, (char **)&utf16_str) !=
			NSERROR_OK)
		return NSERROR_INVALID;

	utf16 = utf16_str;
	if (!(ofont = ami_open_outline_font(fstyle, 0)))
		return NSERROR_INVALID;

	*char_offset = 0;
	*actual_x = 0;

	if (*utf16 == 0xFEFF) utf16++;

	while (utf8_pos < length) {
		if ((*utf16 < 0xD800) || (0xDBFF < *utf16))
			utf16next = utf16 + 1;
		else
			utf16next = utf16 + 2;

		tempx = ami_font_width_glyph(ofont, utf16, utf16next, emwidth);

		if (tempx == 0) {
			if (ufont == NULL)
				ufont = ami_open_outline_font(fstyle, utf16);

			if (ufont)
				tempx = ami_font_width_glyph(ufont, utf16,
						utf16next, emwidth);
		}

		/* Check whether we have a space */
		if (*(string + utf8_pos) == ' ') {
			/* Got a space */
			*actual_x = tx;
			*char_offset = utf8_pos;
		}

		tx += tempx;
		if ((x < tx) && (*char_offset != 0)) {
			/* Reached available width, and a space was found;
			 * split there. */
			free(utf16_str);
			return NSERROR_OK;
		}

		utf16 = utf16next;
		utf8_pos = utf8_next(string, length, utf8_pos);
	}

	free(utf16_str);

	/* No spaces to split at, or everything fits */
	assert(*char_offset == 0 || x >= tx);

	*char_offset = length;
	*actual_x = tx;
	return NSERROR_OK;
}

/**
 * Search for a font in the list and load from disk if not present
 */
static struct ami_font_cache_node *ami_font_open(const char *font, bool critical)
{
	if(font == NULL) {
		NSLOG(netsurf, INFO, "Requested NULL font");
		return NULL;
	}

	struct ami_font_cache_node *nodedata = ami_font_cache_locate(font);
	if(nodedata) return nodedata;

	nodedata = ami_font_cache_alloc_entry(font);

	if(nodedata == NULL) {
		amiga_warn_user("NoMemory", "");
		return NULL;
	}

	nodedata->font = OpenOutlineFont(font, &ami_diskfontlib_list, OFF_OPEN);
	
	if(!nodedata->font)
	{
		NSLOG(netsurf, INFO, "Requested font not found: %s", font);
		if(critical == true) amiga_warn_user("CompError", font);
		free(nodedata);
		return NULL;
	}

	nodedata->bold = (char *)GetTagData(OT_BName, 0, nodedata->font->olf_OTagList);
	if(nodedata->bold)
		NSLOG(netsurf, INFO, "Bold font defined for %s is %s", font,
		      nodedata->bold);
	else
		NSLOG(netsurf, INFO,
		      "Warning: No designed bold font defined for %s", font);

	nodedata->italic = (char *)GetTagData(OT_IName, 0, nodedata->font->olf_OTagList);
	if(nodedata->italic)
		NSLOG(netsurf, INFO, "Italic font defined for %s is %s",
		      font, nodedata->italic);
	else
		NSLOG(netsurf, INFO,
		      "Warning: No designed italic font defined for %s", font);

	nodedata->bolditalic = (char *)GetTagData(OT_BIName, 0, nodedata->font->olf_OTagList);
	if(nodedata->bolditalic)
		NSLOG(netsurf, INFO, "Bold-italic font defined for %s is %s",
		      font, nodedata->bolditalic);
	else
		NSLOG(netsurf, INFO,
		      "Warning: No designed bold-italic font defined for %s",
		      font);

	ami_font_cache_insert(nodedata, font);
	return nodedata;
}

/**
 * Open an outline font in the specified size and style
 *
 * \param fstyle font style structure
 * \param codepoint open a default font instead of the one specified by fstyle
 * \return outline font or NULL on error
 */
static struct OutlineFont *ami_open_outline_font(const plot_font_style_t *fstyle,
		const uint16 *codepoint)
{
	struct ami_font_cache_node *node;
	struct ami_font_cache_node *designed_node = NULL;
	struct OutlineFont *ofont;
	char *fontname;
	ULONG ysize;
	int tstyle = 0;
	plot_font_generic_family_t fontfamily;
	ULONG emboldenx = 0;
	ULONG emboldeny = 0;
	ULONG shearsin = 0;
	ULONG shearcos = (1 << 16);

	if(codepoint) fontfamily = NSA_UNICODE_FONT;
		else fontfamily = fstyle->family;

	switch(fontfamily)
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
		case NSA_UNICODE_FONT:
		default:
			if(__builtin_expect((amiga_nsfont_utf16_char_length(codepoint) == 2), 0)) {
				/* Multi-byte character */
				fontname = nsoption_charp(font_surrogate);
			} else {
				fontname = (char *)ami_font_scan_lookup(codepoint, glypharray);
			}
			if(fontname == NULL) return NULL;
		break;
	}

	node = ami_font_open(fontname, true);
	if(!node) return NULL;

	if (fstyle->flags & FONTF_OBLIQUE)
		tstyle = NSA_OBLIQUE;

	if (fstyle->flags & FONTF_ITALIC)
		tstyle = NSA_ITALIC;

	if (fstyle->weight >= 700)
		tstyle += NSA_BOLD;

	switch(tstyle)
	{
		case NSA_ITALIC:
			if(node->italic) designed_node = ami_font_open(node->italic, false);

			if(designed_node == NULL) {
				shearsin = NSA_VALUE_SHEARSIN;
				shearcos = NSA_VALUE_SHEARCOS;
			}
		break;

		case NSA_OBLIQUE:
			shearsin = NSA_VALUE_SHEARSIN;
			shearcos = NSA_VALUE_SHEARCOS;
		break;

		case NSA_BOLD:
			if(node->bold) designed_node = ami_font_open(node->bold, false);

			if(designed_node == NULL) {
				emboldenx = NSA_VALUE_BOLDX;
				emboldeny = NSA_VALUE_BOLDY;
			}
		break;

		case NSA_BOLDOBLIQUE:
			shearsin = NSA_VALUE_SHEARSIN;
			shearcos = NSA_VALUE_SHEARCOS;

			if(node->bold) designed_node = ami_font_open(node->bold, false);

			if(designed_node == NULL) {
				emboldenx = NSA_VALUE_BOLDX;
				emboldeny = NSA_VALUE_BOLDY;
			}
		break;

		case NSA_BOLDITALIC:
			if(node->bolditalic) designed_node = ami_font_open(node->bolditalic, false);

			if(designed_node == NULL) {
				emboldenx = NSA_VALUE_BOLDX;
				emboldeny = NSA_VALUE_BOLDY;
				shearsin = NSA_VALUE_SHEARSIN;
				shearcos = NSA_VALUE_SHEARCOS;
			}
		break;
	}

	/* Scale to 16.16 fixed point */
	ysize = fstyle->size * ((1 << 16) / PLOT_STYLE_SCALE);

	if(designed_node == NULL) {
		ofont = node->font;
	} else {
		ofont = designed_node->font;
	}

#ifndef __amigaos4__
	struct BulletBase *BulletBase = ofont->BulletBase;
#endif

	if(ESetInfo(AMI_OFONT_ENGINE,
			OT_DeviceDPI,   ami_font_dpi_get_devicedpi(),
			OT_PointHeight, ysize,
			OT_EmboldenX,   emboldenx,
			OT_EmboldenY,   emboldeny,
			OT_ShearSin,    shearsin,
			OT_ShearCos,    shearcos,
			TAG_END) == OTERR_Success)
		return ofont;

	return NULL;
}

static inline int32 ami_font_plot_glyph(struct OutlineFont *ofont, struct RastPort *rp,
		uint16 *restrict char1, uint16 *restrict char2, uint32 x, uint32 y,
		uint32 emwidth, bool aa)
{
	struct GlyphMap *glyph;
	UBYTE *glyphbm;
	int32 char_advance = 0;
	FIXED kern = 0;
	ULONG glyphmaptag;
	ULONG template_type;
	uint32 long_char_1 = 0, long_char_2 = 0;
#ifndef __amigaos4__
	struct BulletBase *BulletBase = ofont->BulletBase;
#endif

#ifndef __amigaos4__
	if (__builtin_expect(((*char1 >= 0xD800) && (*char1 <= 0xDBFF)), 0)) {
		/* We don't support UTF-16 surrogates yet, so just return. */
		return 0;
	}
	
	if (__builtin_expect(((*char2 >= 0xD800) && (*char2 <= 0xDBFF)), 0)) {
		/* Don't attempt to kern a UTF-16 surrogate */
		*char2 = 0;
	}
#endif

#ifdef __amigaos4__
	if(__builtin_expect(aa == true, 1)) {
		glyphmaptag = OT_GlyphMap8Bit;
		template_type = BLITT_ALPHATEMPLATE;
	} else {
#endif
		glyphmaptag = OT_GlyphMap;
#ifdef __amigaos4__
		template_type = BLITT_TEMPLATE;
	}
#endif
 
	long_char_1 = amiga_nsfont_decode_surrogate(char1);
	long_char_2 = amiga_nsfont_decode_surrogate(char2);
	/**\todo use OT_GlyphCode_32 so we get an error for old font engines */

	if(ESetInfo(AMI_OFONT_ENGINE,
			OT_GlyphCode, long_char_1,
			OT_GlyphCode2, long_char_2,
			TAG_END) == OTERR_Success)
	{
		if(EObtainInfo(AMI_OFONT_ENGINE,
			glyphmaptag, &glyph,
			TAG_END) == 0)
		{
			glyphbm = glyph->glm_BitMap;
			if(!glyphbm) return 0;

			if(rp) {
#ifdef __amigaos4__
				BltBitMapTags(BLITA_SrcX, glyph->glm_BlackLeft,
					BLITA_SrcY, glyph->glm_BlackTop,
					BLITA_DestX, x - glyph->glm_X0 + glyph->glm_BlackLeft,
					BLITA_DestY, y - glyph->glm_Y0 + glyph->glm_BlackTop,
					BLITA_Width, glyph->glm_BlackWidth,
					BLITA_Height, glyph->glm_BlackHeight,
					BLITA_Source, glyphbm,
					BLITA_SrcType, template_type,
					BLITA_Dest, rp,
					BLITA_DestType, BLITT_RASTPORT,
					BLITA_SrcBytesPerRow, glyph->glm_BMModulo,
					TAG_DONE);
#else
				/* On OS3 the glyph needs to be in chip RAM */
				void *chip_glyph = ami_memory_chip_alloc(glyph->glm_BMModulo * glyph->glm_BMRows);
				if(chip_glyph != NULL) {
					CopyMem(glyphbm, chip_glyph, glyph->glm_BMModulo * glyph->glm_BMRows);

					BltTemplate(chip_glyph + (glyph->glm_BMModulo * glyph->glm_BlackTop) +
						((glyph->glm_BlackLeft >> 4) << 1),
						glyph->glm_BlackLeft & 0xF, glyph->glm_BMModulo, rp,
						x - glyph->glm_X0 + glyph->glm_BlackLeft,
						y - glyph->glm_Y0 + glyph->glm_BlackTop,
						glyph->glm_BlackWidth, glyph->glm_BlackHeight);

					ami_memory_chip_free(chip_glyph);
				}
#endif
			}

			kern = 0;

			if(*char2) EObtainInfo(AMI_OFONT_ENGINE,
								OT_TextKernPair, &kern,
								TAG_END);

			char_advance = (ULONG)(((glyph->glm_Width - kern) * emwidth) / 65536);

			EReleaseInfo(AMI_OFONT_ENGINE,
				glyphmaptag, glyph,
				TAG_END);
				
			if(*char2) EReleaseInfo(AMI_OFONT_ENGINE,
				OT_TextKernPair, kern,
				TAG_END);
		}
	}

	return char_advance;
}

static inline int32 ami_font_width_glyph(struct OutlineFont *ofont, 
		const uint16 *restrict char1, const uint16 *restrict char2, uint32 emwidth)
{
	int32 char_advance = 0;
	FIXED kern = 0;
	struct MinList *gwlist = NULL;
	struct GlyphWidthEntry *gwnode;
	bool skip_c2 = false;
	uint32 long_char_1 = 0;
	uint32 long_char_2;
#ifndef __amigaos4__
	struct BulletBase *BulletBase = ofont->BulletBase;
#endif

#ifndef __amigaos4__
	if (__builtin_expect(((*char1 >= 0xD800) && (*char1 <= 0xDBFF)), 0)) {
		/* We don't support UTF-16 surrogates yet, so just return. */
		return 0;
	}
	
	if (__builtin_expect(((*char2 >= 0xD800) && (*char2 <= 0xDBFF)), 0)) {
		/* Don't attempt to kern a UTF-16 surrogate */
		skip_c2 = true;
	}
#endif

	if (*char2 < 0x0020) skip_c2 = true;

	long_char_1 = amiga_nsfont_decode_surrogate(char1);
	/**\todo use OT_GlyphCode_32 so we get an error for old font engines */

	if(ESetInfo(AMI_OFONT_ENGINE,
			OT_GlyphCode, long_char_1,
			OT_GlyphCode2, long_char_1,
			TAG_END) == OTERR_Success)
	{
		if(EObtainInfo(AMI_OFONT_ENGINE,
			OT_WidthList, &gwlist,
			TAG_END) == 0)
		{
			FIXED char1w = 0;
			gwnode = (struct GlyphWidthEntry *)GetHead((struct List *)gwlist);
			if(gwnode) char1w = gwnode->gwe_Width;

			kern = 0;

			if(!skip_c2) {
				long_char_2 = amiga_nsfont_decode_surrogate(char2);
				if(ESetInfo(AMI_OFONT_ENGINE,
						OT_GlyphCode, long_char_1,
						OT_GlyphCode2, long_char_2,
						TAG_END) == OTERR_Success)
				{
					EObtainInfo(AMI_OFONT_ENGINE,
								OT_TextKernPair, &kern,
								TAG_END);
				}
			}
			char_advance = (ULONG)(((char1w - kern) * emwidth) / 65536);
			
			if(!skip_c2) EReleaseInfo(AMI_OFONT_ENGINE,
				OT_TextKernPair, kern,
				TAG_END);
				
			EReleaseInfo(AMI_OFONT_ENGINE,
				OT_WidthList, gwlist,
				TAG_END);
		}
	}

	return char_advance;
}

static const uint16 *ami_font_translate_smallcaps(uint16 *utf16char)
{
	const uint16 *p;
	p = &sc_table[0];

	while (*p != 0)
	{
		if(*p == *utf16char) return &p[1];
		p++;
	}

	return utf16char;
}

static ULONG amiga_nsfont_text(struct RastPort *rp, const char *string, ULONG length,
			const plot_font_style_t *fstyle, ULONG dx, ULONG dy, bool aa)
{
	uint16 *restrict utf16 = NULL, *restrict outf16 = NULL;
	uint16 *restrict utf16charsc = 0, *restrict utf16nextsc = 0;
	uint16 *restrict utf16next = 0;
	int utf16charlen;
	struct OutlineFont *restrict ofont, *restrict ufont = NULL;
	uint32 x=0;
	int32 tempx = 0;
	ULONG emwidth = (ULONG)NSA_FONT_EMWIDTH(fstyle->size);
	uint16 utf16_a = 0x41;

	if(!string || string[0]=='\0') return 0;
	if(!length) return 0;
	if(rp == NULL) return 0;

	if(utf8_to_enc(string,"UTF-16",length,(char **)&utf16) != NSERROR_OK) return 0;
	outf16 = utf16;
	if(!(ofont = ami_open_outline_font(fstyle, 0))) {
		if(!(ofont = ami_open_outline_font(fstyle, &utf16_a))) return 0;
	}

	while(*utf16 != 0)
	{
		utf16charlen = amiga_nsfont_utf16_char_length(utf16);
		utf16next = &utf16[utf16charlen];

		if(fstyle->flags & FONTF_SMALLCAPS)
		{
			utf16charsc = (uint16 *)ami_font_translate_smallcaps(utf16);
			utf16nextsc = (uint16 *)ami_font_translate_smallcaps(utf16next);

			tempx = ami_font_plot_glyph(ofont, rp, utf16charsc, utf16nextsc,
								dx + x, dy, emwidth, aa);
		}
		else tempx = 0;

		if(tempx == 0) {
			tempx = ami_font_plot_glyph(ofont, rp, utf16, utf16next,
								dx + x, dy, emwidth, aa);
		}

		if(tempx == 0)
		{
			if(ufont == NULL)
			{
				ufont = ami_open_outline_font(fstyle, utf16);
			}

			if(ufont) {
				tempx = ami_font_plot_glyph(ufont, rp, utf16, utf16next,
											dx + x, dy, emwidth, aa);
			}
		}

		x += tempx;

		utf16 += utf16charlen;
	}

	free(outf16);
	return x;
}

static inline ULONG ami_font_unicode_width(const char *string, ULONG length,
			const plot_font_style_t *fstyle, ULONG dx, ULONG dy, bool aa)
{
	uint16 *restrict utf16 = NULL, *restrict outf16 = NULL;
	uint16 *restrict utf16charsc = 0, *restrict utf16nextsc = 0;
	uint16 *restrict utf16next = 0;
	int utf16charlen;
	struct OutlineFont *restrict ofont, *restrict ufont = NULL;
	uint32 x=0;
	int32 tempx = 0;
	ULONG emwidth = (ULONG)NSA_FONT_EMWIDTH(fstyle->size);
	uint16 utf16_a = 0x41;

	if(!string || string[0]=='\0') return 0;
	if(!length) return 0;

	if(utf8_to_enc(string,"UTF-16",length,(char **)&utf16) != NSERROR_OK) return 0;
	outf16 = utf16;
	if(!(ofont = ami_open_outline_font(fstyle, 0))) {
		if(!(ofont = ami_open_outline_font(fstyle, &utf16_a))) return 0;
	}

	while(*utf16 != 0)
	{
		utf16charlen = amiga_nsfont_utf16_char_length(utf16);
		utf16next = &utf16[utf16charlen];

		if(fstyle->flags & FONTF_SMALLCAPS)
		{
			utf16charsc = (uint16 *)ami_font_translate_smallcaps(utf16);
			utf16nextsc = (uint16 *)ami_font_translate_smallcaps(utf16next);

			tempx = ami_font_width_glyph(ofont, utf16charsc, utf16nextsc, emwidth);
		}
		else tempx = 0;

		if(tempx == 0) {
			tempx = ami_font_width_glyph(ofont, utf16, utf16next, emwidth);
		}

		if(tempx == 0)
		{
			if(ufont == NULL)
			{
				ufont = ami_open_outline_font(fstyle, utf16);
			}

			if(ufont)
			{
				tempx = ami_font_width_glyph(ufont, utf16, utf16next, emwidth);
			}
		}

		x += tempx;

		utf16 += utf16charlen;
	}

	free(outf16);
	return x;
}

void ami_font_bullet_close(void *nso)
{
	struct ami_font_cache_node *node = (struct ami_font_cache_node *)nso;
	CloseOutlineFont(node->font, &ami_diskfontlib_list);
}

const struct ami_font_functions ami_font_bullet_table = {
	amiga_nsfont_width,
	amiga_nsfont_position_in_string,
	amiga_nsfont_split,
	amiga_nsfont_text
};

void ami_font_bullet_init(void)
{
	/* Initialise Unicode font scanner */
	ami_font_initscanner(false, true);

	/* Initialise font caching etc lists */
	ami_font_cache_init();

	/* List for diskfont internal cache */
	NewList(&ami_diskfontlib_list);

	/* Set up table */
	ami_nsfont = &ami_font_bullet_table;
}

void ami_font_bullet_fini(void)
{
	ami_font_cache_fini();
	ami_font_finiscanner();
}

/* Font scanner */
void ami_font_initscanner(bool force, bool save)
{
	ami_font_scan_init(nsoption_charp(font_unicode_file), force, save, glypharray);
}

void ami_font_finiscanner(void)
{
	ami_font_scan_fini(glypharray);
}

void ami_font_savescanner(void)
{
	ami_font_scan_save(nsoption_charp(font_unicode_file), glypharray);
}

