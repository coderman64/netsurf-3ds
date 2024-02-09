/*
 * This file is part of RUfl
 * Licensed under the MIT License,
 *                http://www.opensource.org/licenses/mit-license
 * Copyright 2006 James Bursa <james@semichrome.net>
 */

#include <inttypes.h>
#include <limits.h>
#include <oslib/font.h>
#include "rufl.h"
#ifdef __CC_NORCROFT
#include "strfuncs.h"
#endif


/**
 * The available Unicode codepoints represented by a font. The entire Unicode
 * range (U+0000 - U+10FFFF) may be covered by the font, but only codepoints
 * in the Basic Multilingual Plane (i.e. U+0000 - U+FFFF) can be represented
 * without the need for extension structures.
 *
 * Fonts which provide glyphs for astral characters will set the extension
 * bit in the structure size field. If set, this indicates that an additional
 * character set structure follows immediately after this one. The plane id
 * field in the structure metadata indicates which plane the structure relates
 * to. Planes are specified in ascending order (as the most commonly used
 * codepoints occur in earlier planes). Planes for which the font has no
 * glyphs are omitted entirely.
 *
 * Each plane is subdivided into 256 codepoint blocks (each block representing
 * 256 contiguous codepoints). Note, however, that two index values are
 * reserved (to indicate full or empty blocks) so only 254 partial blocks may
 * be represented. As of Unicode 13, all planes have at least two blocks
 * unused (or, in the case of the surrogate ranges in the Basic Multilingual
 * Plane, defined as containing no characters), so all valid codepoints should
 * be representable using this scheme.
 *
 * The size of the structure is 4 + 256 + 32 * blocks. A typical 200 glyph
 * font might represent codepoints in 10 blocks, using 580 bytes of storage.
 * A plane with glyphs in every block (but no block fully populated) requires
 * the maximum possible structure size of (4 + 256 + 32 * 254 =) 8388 bytes.
 * The maximum storage required for (the unlikely scenario of) a font
 * providing glyphs in every block in each of the 17 Unicode planes is
 * 17 * 8388 = 142596 bytes.
 *
 * The primary aim of this structure is to make lookup fast.
 */
struct rufl_character_set {
	/** Structure metadata.
	 *
	 * This field contains metadata about the structure in the form:
	 *
	 *    3                   2                   1                   0
	 *  1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
	 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	 * |E|   PID   |     Reserved      |             Size              |
	 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	 *
	 * where:
	 *
	 *   extension (E): 1 bit
	 *     If set, another character set covering a different plane
	 *     follows.
	 *
	 *   plane id (PID): 5 bits
	 *     The 0-based index of the Unicode plane this structure relates
	 *     to. Valid values are in the range [0, 16], where 0 represents
	 *     the Basic Multilingual Plane, and 16 represents the
	 *     Supplementary Private Use Area - B.
	 *
	 *   reserved: 10 bits
	 *     These bits are currently unused and must be set to 0.
	 *
	 *   size: 16 bits
	 *     The total size of this structure, in bytes.
	 */
	uint32_t metadata;
#	define EXTENSION_FOLLOWS(x) ((x) & (1u<<31))
#	define PLANE_ID(x) (((x) >> 26) & 0x1f)
#	define PLANE_SIZE(x) ((x) & 0xffff)

	/** Index table.
	 *
	 * Each entry represents a block of 256 codepoints, so i[k] refers
	 * to codepoints [256*k, 256*(k+1)). The value is either BLOCK_EMPTY,
	 * BLOCK_FULL, or an offset into the block table.
	 * */
	uint8_t index[256];
	/** The block has no characters present. */
#	define BLOCK_EMPTY 254
	/** All characters in the block are present. */
#	define BLOCK_FULL 255

	/** Block table. Each entry is a 256-bit bitmap indicating which
	 * characters in the block are present and absent. */
	uint8_t block[254][32];
};


/** Part of struct rufl_unicode_map. */
struct rufl_unicode_map_entry {
	/** Unicode value (must be in Basic Multilingual Plane). */
	uint16_t u;
	/** Corresponding character. */
	uint8_t c;
};


/** Old font manager: mapping from Unicode to character code. This is simply
 * an array sorted by Unicode value, suitable for bsearch(). If a font has
 * support for multiple encodings, then it will have multiple unicode maps.
 * The encoding field contains the name of the encoding to pass to the 
 * font manager. This will be NULL if the font is a Symbol font. */
struct rufl_unicode_map {
	/** Corresponding encoding name */
	char *encoding;
	/** Number of valid entries in map. */
	size_t entries;
	/** Map from Unicode to character code. */
	struct rufl_unicode_map_entry map[256];
};


/** An entry in rufl_font_list. */
struct rufl_font_list_entry {
	/** Font identifier (name). */
	char *identifier;
	/** Character set of font. */
	struct rufl_character_set *charset;
	/** Number of Unicode mapping tables */
	size_t num_umaps;
	/** Mappings from Unicode to character code. */
	struct rufl_unicode_map *umap;
	/** Family that this font belongs to (index in rufl_family_list and
	 * rufl_family_map). */
	uint32_t family;
	/** Font weight (0 to 8). */
	uint32_t weight;
	/** Font slant (0 or 1). */
	uint32_t slant;
};
/** List of all available fonts. */
extern struct rufl_font_list_entry *rufl_font_list;
/** Number of entries in rufl_font_list. */
extern size_t rufl_font_list_entries;


/** An entry in rufl_family_map. */
struct rufl_family_map_entry {
	/** This style does not exist in this family. */
#	define NO_FONT UINT_MAX
	/** Map from weight and slant to index in rufl_font_list, or NO_FONT. */
	uint32_t font[9][2];
};
/** Map from font family to fonts, rufl_family_list_entries entries. */
extern struct rufl_family_map_entry *rufl_family_map;


/** No font contains this character. */
#define NOT_AVAILABLE 0xffff

/** Number of slots in recent-use cache. This is the maximum number of RISC OS
 * font handles that will be used at any time by the library. */
#define rufl_CACHE_SIZE 10

/** An entry in rufl_cache. */
struct rufl_cache_entry {
	/** Font number (index in rufl_font_list), or rufl_CACHE_*. */
	uint32_t font;
	/** No font cached in this slot. */
#define rufl_CACHE_NONE UINT_MAX
	/** Font for rendering hex substitutions in this slot. */
#define rufl_CACHE_CORPUS (UINT_MAX - 1)
	/** Font size. */
	uint32_t size;
	/** Font encoding */
	const char *encoding;
	/** Value of rufl_cache_time when last used. */
	uint32_t last_used;
	/** RISC OS font handle. */
	font_f f;
};
/** Cache of rufl_CACHE_SIZE most recently used font handles. */
extern struct rufl_cache_entry rufl_cache[rufl_CACHE_SIZE];
/** Counter for measuring age of cache entries. */
extern uint32_t rufl_cache_time;

/** Font manager does not support Unicode. */
extern bool rufl_old_font_manager;

/** Font manager supports background blending */
extern bool rufl_can_background_blend;

rufl_code rufl_find_font_family(const char *family, rufl_style font_style,
		unsigned int *font, unsigned int *slanted,
		struct rufl_character_set **charset);
rufl_code rufl_find_font(unsigned int font, unsigned int font_size,
		const char *encoding, font_f *fhandle);
bool rufl_character_set_test(const struct rufl_character_set *charset,
		uint32_t u);

rufl_code rufl_substitution_table_init(void);
void rufl_substitution_table_fini(void);
unsigned int rufl_substitution_table_lookup(uint32_t u);
void rufl_substitution_table_dump(void);

#define rufl_utf8_read(s, l, u)						       \
	if (4 <= l && ((s[0] & 0xf8) == 0xf0) && ((s[1] & 0xc0) == 0x80) &&    \
			((s[2] & 0xc0) == 0x80) && ((s[3] & 0xc0) == 0x80)) {  \
		u = ((s[0] & 0x7) << 18) | ((s[1] & 0x3f) << 12) |	       \
				((s[2] & 0x3f) << 6) | (s[3] & 0x3f);	       \
		s += 4; l -= 4;						       \
		if (u < 0x10000) u = 0xfffd;				       \
	} else if (3 <= l && ((s[0] & 0xf0) == 0xe0) &&			       \
			((s[1] & 0xc0) == 0x80) &&			       \
			((s[2] & 0xc0) == 0x80)) {			       \
		u = ((s[0] & 0xf) << 12) | ((s[1] & 0x3f) << 6) |	       \
				(s[2] & 0x3f);				       \
		s += 3; l -= 3;						       \
		if (u < 0x800) u = 0xfffd;				       \
	} else if (2 <= l && ((s[0] & 0xe0) == 0xc0) &&			       \
			((s[1] & 0xc0) == 0x80)) {			       \
		u = ((s[0] & 0x3f) << 6) | (s[1] & 0x3f);		       \
		s += 2; l -= 2;						       \
		if (u < 0x80) u = 0xfffd;				       \
	} else if ((s[0] & 0x80) == 0) {				       \
		u = s[0];						       \
		s++; l--;						       \
	} else {							       \
		u = 0xfffd;						       \
		s++; l--;						       \
	}								       \
	if ((u >= 0xd800 && u <= 0xdfff) || u == 0xfffe || u == 0xffff) {      \
		u = 0xfffd;						       \
	}

#define rufl_CACHE_TEMPLATE "<Wimp$ScrapDir>.RUfl.CacheNNNN"
#define rufl_CACHE_VERSION 4


struct rufl_glyph_map_entry {
	const char *glyph_name;
	/* The glyph map contains codepoints in the BMP only */
	uint16_t u;
};

extern const struct rufl_glyph_map_entry rufl_glyph_map[];
extern const size_t rufl_glyph_map_size;


#if 1 /*ndef NDEBUG*/
#include <time.h>
extern bool rufl_log_got_start_time;
extern time_t rufl_log_start_time;
#define LOG(format, ...)						\
	do {								\
		if (rufl_log_got_start_time == false) {			\
			rufl_log_start_time = time(NULL);		\
			rufl_log_got_start_time = true;			\
		}							\
									\
		fprintf(stderr,"(%.6fs) " __FILE__ " %s %i: ",		\
				difftime(time(NULL),			\
					rufl_log_start_time),		\
				__func__, __LINE__);			\
		fprintf(stderr, format, __VA_ARGS__);			\
		fprintf(stderr, "\n");					\
	} while (0)
#else
#define LOG(format, ...) ((void) 0)
#endif
