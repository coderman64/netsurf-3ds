/*
 * Copyright 2014 Michael Drake <tlsa@netsurf-browser.org>
 * Copyright 2014 Vincent Sanders <vince@netsurf-browser.org>
 *
 * This file is part of the convert_font tool used to convert font
 * glyph data into a compilable representation.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 *   * The above copyright notice and this permission notice shall be
 *     included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>

#define GLYPH_LEN	16
#define BUCKETS		512
#define CHUNK_SIZE	(64 * 1024)
#define HEADER_MAX	2000

#define SECTION_SIZE	(sizeof(uint16_t) * 256)

const char *labels[4] = {
	"      Regular",
	"       Italic",
	"         Bold",
	"Bold & Italic"
};

const char *var_lables[4] = {
	"fb_regular",
	"fb_italic",
	"fb_bold",
	"fb_bold_italic"
};

const char *short_labels[4] = {
	"  ",
	" i",
	"b ",
	"bi"
};

enum font_style {
	REGULAR		= 0,
	ITALIC		= (1 << 0),
	BOLD		= (1 << 1),
	ITALIC_BOLD	= (1 << 2)
};

enum log_level {
	LOG_DEBUG,
	LOG_INFO,
	LOG_RESULT,
	LOG_WARNING,
	LOG_ERROR
};

enum log_level level;

typedef struct glyph_entry {
	union {
		uint32_t u32[GLYPH_LEN / 4];
		uint8_t u8[GLYPH_LEN];
	} data;
	uint32_t index;
	struct glyph_entry *next;
} glyph_entry;

/** Scratch glyph for generated code points */
uint8_t code_point[GLYPH_LEN];

/** Hash table */
glyph_entry *ht[BUCKETS];

#define LOG(lev, fmt, ...)				\
	if (lev >= level)				\
		printf(fmt, ##__VA_ARGS__);

/**
 * Get hash for glyph data
 * \param g Glyph data (GLYPH_LEN bytes)
 * \return glyph's hash
 */
static inline uint32_t glyph_hash(const uint8_t *g)
{
	uint32_t hash = 0x811c9dc5;
	unsigned int len = GLYPH_LEN;

	while (len > 0) {
		hash *= 0x01000193;
		hash ^= *g++;
		len--;
	}

	return hash;
}


/**
 * Check whether glyphs are identical (compares glyph data)
 *
 * \param g1 First glyph's data (GLYPH_LEN bytes)
 * \param g2 Second glyph's data (GLYPH_LEN bytes)
 * \return true iff both glyphs are identical, else false
 */
static inline bool glyphs_match(const uint8_t *g1, const uint8_t *g2)
{
	return (memcmp(g1, g2, GLYPH_LEN) == 0);
}


/**
 * Add a glyph to a hash chain (or free, and return pointer to existing glyph)
 *
 * Note that if new glyph already exists in chain, it is freed and a pointer to
 * the existing glyph is returned.  If the glyph does not exist in the chain
 * it is added and its pointer is returned.
 *
 * \param head Head of hash chain
 * \param new New glyph to add (may be freed)
 * \return pointer to glyph in hash chain
 */
static glyph_entry * glyph_add_to_chain(glyph_entry **head, glyph_entry *new)
{
	glyph_entry *e = *head;

	if (*head == NULL) {
		new->next = NULL;
		*head = new;
		return new;
	}

	do {
		if (glyphs_match(new->data.u8, e->data.u8)) {
			free(new);
			return e;
		}
		if (e->next == NULL)
			break;
		e = e->next;
	} while (1);

	new->next = e->next;
	e->next = new;
	return new;
}


/**
 * Free a glyph entry chain
 *
 * \param head Head of hash chain
 */
static void free_chain(glyph_entry *head)
{
	glyph_entry *e = head;

	if (head == NULL)
		return;

	while (e != NULL) {
		head = e->next;
		free(e);
		e = head;
	};
}


/**
 * Add new glyph to hash table (or free, and return pointer to existing glyph)
 *
 * Note that if new glyph already exists in table, it is freed and a pointer to
 * the existing glyph is returned.  If the glyph does not exist in the table
 * it is added and its pointer is returned.
 *
 * \param new New glyph to add (may be freed)
 * \return pointer to glyph in hash table
 */
static glyph_entry * glyph_add_to_table(glyph_entry *new)
{
	uint32_t hash = glyph_hash(new->data.u8);

	return glyph_add_to_chain(&ht[hash % BUCKETS], new);
}


/**
 * Free glyph table.
 */
static void free_table(void)
{
	int i;

	for (i = 0; i < BUCKETS; i++) {
		free_chain(ht[i]);
	}
}

struct parse_context {
	enum {
		START,
		IN_HEADER,
		BEFORE_ID,
		GLYPH_ID,
		BEFORE_GLYPH_DATA,
		IN_GLYPH_DATA
	} state;			/**< Current parser state */

	union {
		struct {
			bool new_line;
		} in_header;
		struct {
			bool new_line;
			bool u;
		} before_id;
		struct {
			int c;
		} g_id;
		struct {
			bool new_line;
			bool prev_h;
			bool prev_s;
			int c;
		} before_gd;
		struct {
			int line;
			int pos;
			int styles;
			int line_styles;
			glyph_entry *e[4];
		} in_gd;
	} data;				/**< The state specific data */

	int id;				/**< Current ID */

	int codepoints;			/**< Glyphs containing codepoints */
	int count[4];			/**< Count of glyphs in file */
};

struct font_data {
	char header[HEADER_MAX];
	int header_len;

	uint8_t section_table[4][256];
	uint8_t sec_count[4];
	uint16_t *sections[4];

	glyph_entry *e[0xffff];
	int glyphs;
};

static bool generate_font_header(const char *path, struct font_data *data)
{
	FILE *fp;
	int s;

	fp = fopen(path, "wb");
	if (fp == NULL) {
		LOG(LOG_ERROR, "Couldn't open header file \"%s\"\n", path);
		return false;
	}

	fprintf(fp, "/*\n");
	fwrite(data->header, 1, data->header_len, fp);
	fprintf(fp, " */\n\n");
	fprintf(fp, "/* Don't edit this file, it was generated from the "
			"plain text source data. */\n\n");


	for (s = 0; s < 4; s++) {
		fprintf(fp, "extern const uint8_t *%s_section_table;\n",
			var_lables[s]);
		fprintf(fp, "extern const uint16_t *%s_sections;\n",
			var_lables[s]);

	}

	fprintf(fp, "extern const uint8_t *font_glyph_data;\n");

	fprintf(fp, "\n\n");

	fclose(fp);

	return true;

}

static bool generate_font_source(const char *path, struct font_data *data)
{
	int s, i, y;
	int limit;
	FILE *fp;

	fp = fopen(path, "wb");
	if (fp == NULL) {
		LOG(LOG_ERROR, "Couldn't open output file \"%s\"\n", path);
		return false;
	}

	fprintf(fp, "/*\n");
	fwrite(data->header, 1, data->header_len, fp);
	fprintf(fp, " */\n\n");
	fprintf(fp, "/* Don't edit this file, it was generated from the "
			"plain text source data. */\n\n");

	fprintf(fp, "#include <stdint.h>\n");
	fprintf(fp, "\n");

	for (s = 0; s < 4; s++) {

		fprintf(fp, "static const uint8_t %s_section_table_c[256] = {\n",
				var_lables[s]);

		for (i = 0; i < 256; i++) {
			if (i == 255)
				fprintf(fp, "0x%.2X\n",
						data->section_table[s][i]);
			else if (i % 8 == 7)
				fprintf(fp, "0x%.2X,\n",
						data->section_table[s][i]);
			else if (i % 8 == 0)
				fprintf(fp, "\t0x%.2X, ",
						data->section_table[s][i]);
			else
				fprintf(fp, "0x%.2X, ",
						data->section_table[s][i]);
		}

		fprintf(fp, "};\nconst uint8_t *%s_section_table = &%s_section_table_c[0];\n\n",
			var_lables[s], var_lables[s]);
		fprintf(fp, "static const uint16_t %s_sections_c[%i] = {\n",
				var_lables[s], data->sec_count[s] * 256);

		limit = data->sec_count[s] * 256;
		for (i = 0; i < limit; i++) {
			uint16_t offset = data->sections[s][i];
			if (i == limit - 1)
				fprintf(fp, "0x%.4X\n", offset);
			else if (i % 4 == 3)
				fprintf(fp, "0x%.4X,\n", offset);
			else if (i % 4 == 0)
				fprintf(fp, "\t0x%.4X, ", offset);
			else
				fprintf(fp, "0x%.4X, ", offset);
		}

		fprintf(fp, "};\nconst uint16_t *%s_sections = &%s_sections_c[0];\n\n", var_lables[s], var_lables[s]);
	}

	fprintf(fp, "static const uint8_t font_glyph_data_c[%i] = {\n",
			(data->glyphs + 1) * 16);

	fprintf(fp, "\t0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,\n"
		    "\t0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,\n");

	limit = data->glyphs;
	for (i = 0; i < limit; i++) {
		glyph_entry *e = data->e[i];

		for (y = 0; y < 16; y++) {
			if (i == limit - 1 && y == 15)
				fprintf(fp, "0x%.2X\n", e->data.u8[y]);
			else if (y % 8 == 7)
				fprintf(fp, "0x%.2X,\n", e->data.u8[y]);
			else if (y % 8 == 0)
				fprintf(fp, "\t0x%.2X, ", e->data.u8[y]);
			else
				fprintf(fp, "0x%.2X, ", e->data.u8[y]);
		}
	}

	fprintf(fp, "};\n");
	fprintf(fp, "const uint8_t *font_glyph_data = &font_glyph_data_c[0];\n\n");

	fclose(fp);

	return true;
}

static bool add_glyph_to_data(glyph_entry *add, int id, int style,
		struct font_data *d)
{
	glyph_entry *e;
	int offset;
	int s;

	/* Find out if 'add' is unique, and get its unique table entry */
	e = glyph_add_to_table(add);
	if (e == add) {
		/* Unique glyph */
		d->e[d->glyphs++] = e;
		e->index = d->glyphs;
		if (d->glyphs >= 0xfffd) {
			LOG(LOG_ERROR, "  Too many glyphs for internal data "
					"representation\n");
			return false;
		}
	} else {
		/* Duplicate glyph */
		LOG(LOG_DEBUG, "  U+%.4X (%s) is duplicate\n",
				id, short_labels[style]);
	}

	/* Find glyph's section */
	s = id / 256;

	/* Allocate section if needed */
	if ((s == 0 && d->sections[style] == NULL) ||
			(s != 0 && d->section_table[style][s] == 0)) {
		size_t size = (d->sec_count[style] + 1) * SECTION_SIZE;
		uint16_t *temp = realloc(d->sections[style], size);
		if (temp == NULL) {
			LOG(LOG_ERROR, "  Couldn't increase sections "
					"allocation\n");
			return false;
		}
		memset(temp + d->sec_count[style] * 256, 0,
				SECTION_SIZE);
		d->section_table[style][s] = d->sec_count[style];
		d->sections[style] = temp;
		d->sec_count[style]++;
	}

	offset = d->section_table[style][s] * 256 + (id & 0xff);
	d->sections[style][offset] = e->index;

	return true;
}


static bool check_glyph_data_valid(int pos, char c)
{
	int offset = pos % 11;

	if (pos == 44) {
		if (c != '\n') {
			LOG(LOG_ERROR, "  Invalid glyph data: "
					"expecting '\\n', got '%c' (%i)\n",
					c, c);
			return false;
		} else {
			return true;
		}
	} else if (pos < 3) {
		if (c != ' ') {
			LOG(LOG_ERROR, "  Invalid glyph data: "
					"expecting ' ', got '%c' (%i)\n",
					c, c);
			return false;
		} else {
			return true;
		}
	} else if (offset == 0) {
		if (c != '\n' && c != ' ') {
			LOG(LOG_ERROR, "  Invalid glyph data: "
					"expecting '\\n' or ' ', "
					"got '%c' (%i)\n",
					c, c);
			return false;
		} else {
			return true;
		}
	} else if (offset < 3) {
		if (c != ' ') {
			LOG(LOG_ERROR, "  Invalid glyph data: "
					"expecting ' ', got '%c' (%i)\n",
					c, c);
			return false;
		} else {
			return true;
		}
	} else if (offset >= 3 && pos < 11) {
		if (c != '.' && c != '#') {
			LOG(LOG_ERROR, "  Invalid glyph data: "
					"expecting '.' or '#', "
					"got '%c' (%i)\n",
					c, c);
			return false;
		} else {
			return true;
		}
	}

	/* offset must be >=3 */
	if (c != '.' && c != '#' && c != ' ') {
		LOG(LOG_ERROR, "  Invalid glyph data: "
		    "expecting '.', '#', or ' ', "
		    "got '%c' (%i)\n",
		    c, c);
		return false;
	}

	return true;
}

#define SEVEN_SET	((1 << 0) | (1 << 1) | (1 << 2) | (1 << 3) | 	\
			 (1 << 4) | (1 << 5) | (1 << 6))

#define THREE_SSS	((1 << 0) | (1 << 1) | (1 << 2))
#define THREE_S_S	((1 << 0) |            (1 << 2))
#define THREE__SS	((1 << 0) | (1 << 1)           )
#define THREE_SS_	(           (1 << 1) | (1 << 2))
#define THREE_S__			       (1 << 2)
#define THREE__S_		    (1 << 1)
#define THREE___S	 (1 << 0)

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

static void build_codepoint(int id, bool italic, uint8_t *code_point)
{
	int shift = 0;
	int l;
	int r;

	if (!italic)
		shift = 1;

	l =       (id >> 12);
	r = 0xf & (id >>  8);

	code_point[ 0] = 0;
	code_point[ 1] = SEVEN_SET << shift;
	code_point[ 2] = 0;

	code_point[ 3] = (frag[l][0] << (4 + shift)) | (frag[r][0] << shift);
	code_point[ 4] = (frag[l][1] << (4 + shift)) | (frag[r][1] << shift);
	code_point[ 5] = (frag[l][2] << (4 + shift)) | (frag[r][2] << shift);
	code_point[ 6] = (frag[l][3] << (4 + shift)) | (frag[r][3] << shift);
	code_point[ 7] = (frag[l][4] << (4 + shift)) | (frag[r][4] << shift);

	code_point[ 8] = 0;

	shift = 1;

	l = 0xf & (id >>  4);
	r = 0xf &  id       ;

	code_point[ 9] = (frag[l][0] << (4 + shift)) | (frag[r][0] << shift);
	code_point[10] = (frag[l][1] << (4 + shift)) | (frag[r][1] << shift);
	code_point[11] = (frag[l][2] << (4 + shift)) | (frag[r][2] << shift);
	code_point[12] = (frag[l][3] << (4 + shift)) | (frag[r][3] << shift);
	code_point[13] = (frag[l][4] << (4 + shift)) | (frag[r][4] << shift);

	code_point[14] = 0;
	code_point[15] = SEVEN_SET << shift;
}

#undef SEVEN_SET
#undef THREE_SSS
#undef THREE_S_S
#undef THREE__SS
#undef THREE_SS_
#undef THREE_S__
#undef THREE__S_
#undef THREE___S

static bool glyph_is_codepoint(const glyph_entry *e, int id, int style)
{
	bool italic = false;

	if (style == 1 || style == 3) {
		italic = true;
	}

	build_codepoint(id, italic, code_point);

	return glyphs_match(code_point, e->data.u8);
}


static bool parse_glyph_data(struct parse_context *ctx, char c,
		struct font_data *d)
{
	int glyph = ctx->data.in_gd.pos / 11;
	int g_pos = ctx->data.in_gd.pos % 11 - 3;
	uint8_t *row;
	bool ok;
	int i;

	/* Check that character is valid */
	if (check_glyph_data_valid(ctx->data.in_gd.pos, c) == false) {
		LOG(LOG_ERROR, "  Error in U+%.4X data: "
				"glyph line: %i, pos: %i\n",
				ctx->id,
				ctx->data.in_gd.line,
				ctx->data.in_gd.pos);
		goto error;
	}

	/* Allocate glyph data if needed */
	if (ctx->data.in_gd.line == 0 &&
			(c == '.' || c == '#')) {
		if (ctx->data.in_gd.e[glyph] == NULL) {
			ctx->data.in_gd.e[glyph] =
					calloc(sizeof(struct glyph_entry), 1);
			if (ctx->data.in_gd.e[glyph] == NULL) {
				LOG(LOG_ERROR, "  Couldn't allocate memory for "
						"glyph entry\n");
				goto error;
			}

			ctx->data.in_gd.styles |= 1 << glyph;
		}
	}

	/* Build glyph data */
	if (c == '#') {
		row = &ctx->data.in_gd.e[glyph]->data.u8[ctx->data.in_gd.line];
		*row += 1 << (7 - g_pos);

		ctx->data.in_gd.line_styles |= 1 << glyph;
	} else if (c == '.') {
		ctx->data.in_gd.line_styles |= 1 << glyph;
	}

	/* Deal with current position */
	if (c == '\n') {
		if (ctx->data.in_gd.line == 0) {
			if (ctx->data.in_gd.e[0] == NULL) {
				LOG(LOG_ERROR, "  Error in U+%.4X data: "
						"\"Regular\" glyph style must "
						"be present\n", ctx->id);
				goto error;
			}
		} else if (ctx->data.in_gd.styles !=
				ctx->data.in_gd.line_styles) {
			LOG(LOG_ERROR, "  Error in U+%.4X data: "
					"glyph line: %i "
					"styles don't match first line\n",
					ctx->id,
					ctx->data.in_gd.line);
			goto error;
		}

		ctx->data.in_gd.pos = 0;
		ctx->data.in_gd.line++;
		ctx->data.in_gd.line_styles = 0;
	} else {
		ctx->data.in_gd.pos++;
	}

	/* If we've got all the glyph data, tidy up and advance state */
	if (ctx->data.in_gd.line == 16) {
		for (i = 0; i < 4; i++) {
			if (ctx->data.in_gd.e[i] != NULL) {
				ctx->count[i] += 1;
				if (glyph_is_codepoint(ctx->data.in_gd.e[i],
						ctx->id, i)) {
					LOG(LOG_DEBUG, "  U+%.4X (%s) is "
							"codepoint\n",
							ctx->id,
							short_labels[i]);
					ctx->codepoints += 1;
					free(ctx->data.in_gd.e[i]);
					ctx->data.in_gd.e[i] = NULL;
					continue;
				}

				ok = add_glyph_to_data(ctx->data.in_gd.e[i],
						ctx->id, i, d);
				if (!ok) {
					goto error;
				}
			}
		}

		ctx->data.before_id.new_line = false;
		ctx->data.before_id.u = false;
		ctx->state = BEFORE_ID;
	}

	return true;

error:

	for (i = 0; i < 4; i++) {
		free(ctx->data.in_gd.e[i]);
	}

	return false;
}

static void parse_init(struct parse_context *ctx)
{
	memset(ctx, 0, sizeof(struct parse_context));
}

static bool get_hex_digit_value(char c, int *v)
{
	if (c >= '0' && c <= '9')
		*v = (c - '0');
	else if (c >= 'A' && c <= 'F')
		*v = (10 + c - 'A');
	else {
		LOG(LOG_ERROR, "Invalid hex digit '%c' (%i)\n", c, c);
		return false;
	}

	return true;
}

static bool assemble_codepoint(const char* c, int n, int *id)
{
	bool ok;
	int v;

	ok = get_hex_digit_value(*c, &v);
	if (!ok) {
		return false;
	}

	*id += v << (4 * (3 - n));

	return true;
}

static bool parse_chunk(struct parse_context *ctx, const char *buf, size_t len,
		struct font_data *d)
{
	int i;
	bool ok;
	int count[4];
	const char *pos = buf;
	const char *end = buf + len;

	for (i = 0; i < 4; i++) {
		count[i] = ctx->count[i];
	}

	while (pos < end) {
		if (*pos == '\r') {
			LOG(LOG_ERROR, "Detected \'\\r\': Bad line ending\n");
			return false;
		}

		switch (ctx->state) {
		case START:
			if (*pos != '*') {
				LOG(LOG_ERROR, "First character must be '*'\n");
				printf("Got: %c (%i)\n", *pos, *pos);
				return false;
			}
			d->header_len = 0;
			ctx->data.in_header.new_line = true;
			ctx->state = IN_HEADER;

			/* Fall through */
		case IN_HEADER:
			if (ctx->data.in_header.new_line == true) {
				if (*pos != '*') {
					LOG(LOG_INFO, "  Got header "
							"(%i bytes)\n",
							d->header_len);
					LOG(LOG_DEBUG, "  Header:\n\n%.*s\n",
							d->header_len,
							d->header);
					ctx->data.before_id.new_line = false;
					ctx->data.before_id.u = false;
					ctx->state = BEFORE_ID;
					continue;
				} else if (*pos == '*') {
					d->header[d->header_len++] = ' ';
				}
				ctx->data.in_header.new_line = false;

			} else if (*pos == '\n') {
				ctx->data.in_header.new_line = true;
			}

			if (d->header_len == HEADER_MAX) {
				LOG(LOG_ERROR, "  Header too long "
						"(>%i bytes)\n",
						d->header_len);
				return false;
			}

			d->header[d->header_len++] = *pos;
			break;

		case BEFORE_ID:
			if (*pos == '+' &&
					ctx->data.before_id.new_line == true &&
					ctx->data.before_id.u == true) {
				ctx->data.g_id.c = 0;
				ctx->id = 0;
				ctx->state = GLYPH_ID;
				break;

			} else if (*pos == 'U' &&
					ctx->data.before_id.new_line == true) {
				ctx->data.before_id.u = true;

			} else if (*pos == '\n') {
				ctx->data.before_id.new_line = true;
				ctx->data.before_id.u = false;

			} else {
				ctx->data.before_id.new_line = false;
				ctx->data.before_id.u = false;
			}
			break;

		case GLYPH_ID:
			ok = assemble_codepoint(pos, ctx->data.g_id.c++,
					&ctx->id);
			if (!ok) {
				LOG(LOG_ERROR, "  Invalid glyph ID\n");
				return false;
			}

			if (ctx->data.g_id.c == 4) {
				ctx->data.before_gd.new_line = false;
				ctx->data.before_gd.prev_h = false;
				ctx->data.before_gd.prev_s = false;
				ctx->data.before_gd.c = 0;
				ctx->state = BEFORE_GLYPH_DATA;
				break;
			}
			break;

		case BEFORE_GLYPH_DATA:
			/* Skip until end of dashed line */
			if (*pos == '\n' && ctx->data.before_gd.c == 53) {
				ctx->state = IN_GLYPH_DATA;
				ctx->data.in_gd.e[0] = NULL;
				ctx->data.in_gd.e[1] = NULL;
				ctx->data.in_gd.e[2] = NULL;
				ctx->data.in_gd.e[3] = NULL;
				ctx->data.in_gd.line = 0;
				ctx->data.in_gd.pos = 0;
				ctx->data.in_gd.line_styles = 0;
				ctx->data.in_gd.styles = 0;
				break;

			} else if (*pos == '\n') {
				ctx->data.before_gd.new_line = true;
				ctx->data.before_gd.prev_h = false;
				ctx->data.before_gd.prev_s = false;
				ctx->data.before_gd.c = 0;
			} else if (*pos == '-' &&
					ctx->data.before_gd.new_line == true) {
				assert(ctx->data.before_gd.c == 0);
				ctx->data.before_gd.new_line = false;
				ctx->data.before_gd.c++;
				ctx->data.before_gd.prev_h = true;
			} else if (*pos == ' ' &&
					ctx->data.before_gd.prev_h == true) {
				assert(ctx->data.before_gd.prev_s == false);
				ctx->data.before_gd.c++;
				ctx->data.before_gd.prev_h = false;
				ctx->data.before_gd.prev_s = true;
			} else if (*pos == '-' &&
					ctx->data.before_gd.prev_s == true) {
				assert(ctx->data.before_gd.prev_h == false);
				ctx->data.before_gd.c++;
				ctx->data.before_gd.prev_h = true;
				ctx->data.before_gd.prev_s = false;
			} else {
				ctx->data.before_gd.new_line = false;
				ctx->data.before_gd.prev_h = false;
				ctx->data.before_gd.prev_s = false;
				ctx->data.before_gd.c = 0;
			}
			break;

		case IN_GLYPH_DATA:
			ok = parse_glyph_data(ctx, *pos, d);
			if (!ok) {
				return false;
			}

			break;
		}

		pos++;
	}

	for (i = 0; i < 4; i++) {
		LOG(LOG_DEBUG, "  %s: %i gylphs\n", labels[i],
				ctx->count[i] - count[i]);
	}

	return true;
}


static bool load_font(const char *path, struct font_data **data)
{
	struct parse_context ctx;
	struct font_data *d;
	size_t file_len;
	size_t done;
	size_t len;
	int count;
	char *buf;
	FILE *fp;
	bool ok;
	int i;

	*data = NULL;

	fp = fopen(path, "rb");
	if (fp == NULL) {
		LOG(LOG_ERROR, "Couldn't open font data file\n");
		return false;
	}

	d = calloc(sizeof(struct font_data), 1);
	if (d == NULL) {
		LOG(LOG_ERROR, "Couldn't allocate memory for font data\n");
		fclose(fp);
		return false;
	}

	/* Find filesize */
	fseek(fp, 0L, SEEK_END);
	file_len = ftell(fp);
	if ((long)file_len == -1) {
		LOG(LOG_ERROR, "Could not size input file\n");
		free(d);
		fclose(fp);
		return false;
	}
	fseek(fp, 0L, SEEK_SET);
	LOG(LOG_DEBUG, "Input size: %zu bytes\n", file_len);

	/* Allocate buffer for data chunks */
	buf = malloc(CHUNK_SIZE);
	if (buf == NULL) {
		LOG(LOG_ERROR, "Couldn't allocate memory for input buffer\n");
		free(d);
		fclose(fp);
		return false;
	}

	/* Initialise parser */
	parse_init(&ctx);

	LOG(LOG_DEBUG, "Using chunk size of %i bytes\n", CHUNK_SIZE);

	/* Parse the input file in chunks */
	for (done = 0; done < file_len; done += CHUNK_SIZE) {
		LOG(LOG_INFO, "Parsing input chunk %zu\n", done / CHUNK_SIZE);

		/* Read chunk */
		len = fread(buf, 1, CHUNK_SIZE, fp);
		if (file_len - done < CHUNK_SIZE &&
				len != file_len - done) {
			LOG(LOG_WARNING, "Last chunk has suspicious size\n");
		} else if (file_len - done >= CHUNK_SIZE &&
				len != CHUNK_SIZE) {
			LOG(LOG_ERROR, "Problem reading file\n");
			free(buf);
			free(d);
			fclose(fp);
			return false;
		}

		/* Parse chunk */
		ok = parse_chunk(&ctx, buf, len, d);
		if (!ok) {
			free(buf);
			free(d);
			fclose(fp);
			return false;
		}
		LOG(LOG_DEBUG, "Parsed %zu bytes\n", done + len);
	}

	fclose(fp);

	if (ctx.state != BEFORE_ID) {
		LOG(LOG_ERROR, "Unexpected end of file\n");
		free(buf);
		free(d);
		return false;
	}

	LOG(LOG_INFO, "Parsing complete:\n");
	count = 0;
	for (i = 0; i < 4; i++) {
		LOG(LOG_INFO, "  %s: %i gylphs\n", labels[i], ctx.count[i]);
		count += ctx.count[i];
	}

	LOG(LOG_RESULT, "  Total %i gylphs "
			"(of which %i unique, %i codepoints, %i duplicates)\n",
			count, d->glyphs, ctx.codepoints,
			count - d->glyphs - ctx.codepoints);

	free(buf);

	*data = d;
	return true;
}

static void log_usage(const char *argv0)
{
	level = LOG_INFO;
	LOG(LOG_INFO,
	    "Usage:\n"
	    "\t%s [options] <in_file> <out_file>\n"
	    "\n"
	    "Options:\n"
	    "\t--help    -h   Display this text\n"
	    "\t--quiet   -q   Don't show warnings\n"
	    "\t--verbose -v   Verbose output\n"
	    "\t--debug   -d   Full debug output\n",
	    argv0);
}

int main(int argc, char** argv)
{
	const char *in_path = NULL;
	const char *out_path = NULL;
	char *header_path = NULL;
	struct font_data *data;
	bool ok;
	int i;
	int opt;

	level = LOG_RESULT;

	/* Handle program arguments */
	struct option long_options[] = {
		{ "help",    no_argument,       NULL, 'h' },
		{ "quiet",   no_argument,       NULL, 'q' },
		{ "verbose", no_argument,       NULL, 'v' },
		{ "debug",   no_argument,       NULL, 'd' },
		{ "header",  required_argument, NULL, 'H' },
	};

	while ((opt = getopt_long(argc, argv, "hqvdH:", long_options, NULL)) != -1) {
		switch (opt) {
		case 'q':
			level = LOG_WARNING;
			break;

		case 'v':
			level = LOG_INFO;
			break;

		case 'd':
			level = LOG_DEBUG;
			break;

		case 'H':
			header_path = strdup(optarg);
			break;

		case 'h':
			log_usage(argv[0]);
			free(header_path);
			return EXIT_SUCCESS;

		default:
			log_usage(argv[0]);
			free(header_path);
			return EXIT_FAILURE;
		}
	}

	if ((argc - optind) < 2) {
		log_usage(argv[0]);
		free(header_path);
		return EXIT_FAILURE;
	}

	in_path = argv[optind];
	out_path = argv[optind + 1];

	LOG(LOG_DEBUG, "Using input path: \"%s\"\n", in_path);
	LOG(LOG_DEBUG, "Using output path: \"%s\"\n", out_path);

	ok = load_font(in_path, &data);
	if (!ok) {
		free_table();
		free(header_path);
		return EXIT_FAILURE;
	}

	ok = generate_font_source(out_path, data);
	if (ok && (header_path != NULL)) {
		ok = generate_font_header(header_path, data);
	}
	free(header_path);
	free_table();
	for (i = 0; i < 4; i++) {
		free(data->sections[i]);
	}
	free(data);
	if (!ok) {
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
