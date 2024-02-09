#include <ftw.h>
#include <libgen.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "rufl.h"

/* dirty! */
#include "../src/rufl_internal.h"

#include "harness.h"
#include "testutils.h"

struct expumap {
	char *fontname;
	size_t num_umaps;
};

struct cfg {
	const char *datadir;

	struct expumap *expumaps;
	size_t n_expumaps;
};

static char template[] = "/tmp/oldfminitXXXXXX";
static const char *ptmp = NULL;
static struct cfg cfg;

static int ftw_cb(const char *path, const struct stat *sb,
		int typeflag, struct FTW *ftwbuf)
{
	(void) sb;
	(void) typeflag;
	(void) ftwbuf;

	remove(path);

	return 0;
}

static void cleanup(void)
{
	if (cfg.expumaps != NULL) {
		size_t i;

		for (i = 0; i < cfg.n_expumaps; i++) {
			free(cfg.expumaps[i].fontname);
		}
		free(cfg.expumaps);
	}

	if (ptmp == NULL)
		return;

	nftw(ptmp, ftw_cb, FOPEN_MAX, FTW_DEPTH | FTW_MOUNT | FTW_PHYS);
}

static void parse_cfg(const char *path, struct cfg *cfg,
		void (*cb)(struct cfg *cfg, const char *line, size_t len))
{
	FILE *fp;
	char wbuf[4096];
	size_t nleft = 0;

	fp = fopen(path, "r");
	assert(fp != NULL);

	while (!feof(fp)) {
		char buf[2048];
		size_t nread;
		const char *p, *s;

		nread = fread(buf, 1, sizeof(buf), fp);
		if (nread != sizeof(buf)) {
			assert(ferror(fp) == 0);
		}

		memcpy(wbuf + nleft, buf, nread);
		nleft += nread;

		for (p = s = wbuf; p < wbuf + nleft; p++) {
			if (*p == '\n') {
				cb(cfg, s, p - s);
				s = p+1;
			}
		}
		if (s != wbuf + nleft) {
			memmove(wbuf, s, p - s);
			nleft = p - s;
		} else {
			nleft = 0;
		}
		assert(nleft < sizeof(buf));
	}
	assert(nleft == 0);

	fclose(fp);
}

static void parse_expumaps(struct cfg *cfg, char *data, size_t len)
{
	char *p, *s;
	const char *font = NULL, *count = NULL;
	struct expumap *e;
	size_t num_umaps;

	for (p = s = data; p < data+len; p++) {
		if (*p == ' ' || *p == '\t') {
			*p = '\0';
			if (s != p) {
				if (font == NULL)
					font = s;
				else if (count == NULL)
					count = s;
			}
			s = p+1;
		}
	}
	if (count == NULL)
		count = s;

	num_umaps = strtoul(count, &p, 10);
	assert((size_t)(p-count) == strlen(count));

	e = realloc(cfg->expumaps, (cfg->n_expumaps + 1) * sizeof(*e));
	assert(e != NULL);

	cfg->expumaps = e;
	cfg->expumaps[cfg->n_expumaps].fontname = strdup(font);
	assert(cfg->expumaps[cfg->n_expumaps].fontname != NULL);
	cfg->expumaps[cfg->n_expumaps].num_umaps = num_umaps;
	cfg->n_expumaps++;
}

static void parse_directive(struct cfg *cfg, char *linecpy, size_t len)
{
	char *p, *s;
	const char *directive = NULL;

	for (p = s = linecpy; p < linecpy+len; p++) {
		if (*p == ' ' || *p == '\t') {
			*p = '\0';
			if (s != p && directive == NULL) {
				directive = s;
				s = p+1;
				break;
			}
			s = p+1;
		}
	}
	if (directive == NULL)
		directive = s;

	if (strcmp("\%expumaps", directive) == 0) {
		parse_expumaps(cfg, s, len - (s - linecpy));
	}
}

static void parse_encoding(struct cfg *cfg, char *linecpy, size_t len)
{
	char *p, *s;
	const char *font = NULL, *encoding = NULL, *file = NULL;
	char *path;

	for (p = s = linecpy; p < linecpy+len; p++) {
		if (*p == ' ' || *p == '\t') {
			*p = '\0';
			if (s != p) {
				if (font == NULL)
					font = s;
				else if (encoding == NULL)
					encoding = s;
				else if (file == NULL)
					file = s;
			}
			s = p+1;
		}
	}
	if (file == NULL)
		file = s;

	assert(font != NULL);
	assert(encoding != NULL);
	assert(file != NULL);

	path = malloc(strlen(cfg->datadir) + strlen(file) + 2);
	assert(path != NULL);
	strcpy(path, cfg->datadir);
	path[strlen(cfg->datadir)] = '/'; //XXX: platform-agnostic dirsep?
	strcpy(path+strlen(cfg->datadir)+1, file);

	rufl_test_harness_set_font_encoding(font, encoding, path);

	free(path);
}

static void line_cb(struct cfg *cfg, const char *line, size_t len)
{
	char *linecpy;

	if (len == 0 || line[0] == '#')
		return;

	linecpy = malloc(len + 1);
	assert(linecpy != NULL);
	memcpy(linecpy, line, len);
	linecpy[len] = '\0';

	if (line[0] == '%')
		parse_directive(cfg, linecpy, len);
	else
		parse_encoding(cfg, linecpy, len);

	free(linecpy);
}

static void read_config(const char *path, struct cfg *cfg)
{
	char *pathcpy;

	pathcpy = strdup(path);
	assert(pathcpy != NULL);

	cfg->datadir = dirname(pathcpy);

	parse_cfg(path, cfg, line_cb);

	free(pathcpy);
	cfg->datadir = NULL;
}

int main(int argc, const char **argv)
{
	int width, x;
	size_t offset;
	int32_t xkern, ykern, italic, ascent, descent, xheight, cap_height;
	int32_t x_bearing, y_bearing, mwidth, mheight, x_advance, y_advance;
	int8_t uline_position;
	uint8_t uline_thickness;
	os_box bbox;

	assert(2 == argc);

	ptmp = mkdtemp(template);
	assert(NULL != ptmp);
	atexit(cleanup);
	chdir(ptmp);

	rufl_test_harness_init(339, false, true);

	read_config(argv[1], &cfg);

	assert(rufl_OK == rufl_init());
	assert(NULL == rufl_fm_error);
	assert(3 == rufl_family_list_entries);
	assert(NULL != rufl_family_menu);

	if (cfg.expumaps != NULL) {
		size_t i, j;
		for (i = 0; i != cfg.n_expumaps; i++) {
			for (j = 0; j != rufl_font_list_entries; j++) {
				if (strcmp(cfg.expumaps[i].fontname, rufl_font_list[j].identifier) == 0) {
					assert(cfg.expumaps[i].num_umaps == rufl_font_list[j].num_umaps);
				}
			}
		}
	}

	assert(rufl_OK == rufl_font_metrics("Corpus", rufl_WEIGHT_500,
			&bbox, &xkern, &ykern, &italic,
			&ascent, &descent, &xheight, &cap_height,
			&uline_position, &uline_thickness));
	assert(0 == bbox.x0);
	assert(2 == bbox.x1);
	assert(0 == bbox.y0);
	assert(2 == bbox.y1);
	assert(0 == xkern);
	assert(0 == ykern);
	assert(0 == italic);
	assert(0 == ascent);
	assert(0 == descent);
	assert((bbox.y1 - bbox.y0) == cap_height);
	assert((cap_height / 2) == xheight);
	assert(0 == uline_position);
	assert(0 == uline_thickness);

	assert(rufl_OK == rufl_width("Corpus", rufl_WEIGHT_500, 160,
			"!\xc2\xa0", 3, &width));
	assert(50 == width);

	/* Place caret after first character */
	assert(rufl_OK == rufl_x_to_offset("Homerton", rufl_WEIGHT_500, 160,
			"!\xc2\xa0", 3, 25, &offset, &x));
	assert(1 == offset);
	assert(25 == x);

	/* Attempt to split after first character. As the split point is
	 * coincident with the start of the second character, however,
	 * the split point is placed after it. */
	assert(rufl_OK == rufl_split("Trinity", rufl_WEIGHT_500, 160,
			"!\xc2\xa0", 3, 25, &offset, &x));
	assert(3 == offset);
	assert(50 == x);

	/* Compute width of replacement character */
	assert(rufl_OK == rufl_width("Corpus", rufl_WEIGHT_500, 160,
			"\xef\xbf\xbd", 3, &width));
	assert(17 == width);
	assert(rufl_OK == rufl_width("Corpus", rufl_WEIGHT_500, 160,
			"\xf0\xa0\x80\xa5", 4, &width));
	assert(26 == width);

	/* Measure font bounding box */
	assert(rufl_OK == rufl_font_bbox("Corpus", rufl_WEIGHT_500, 160,
			&bbox));
	assert(0 == bbox.x0);
	assert(25 == bbox.x1);
	assert(0 == bbox.y0);
	assert(25 == bbox.y1);

	/* Trivial render */
	assert(rufl_OK == rufl_paint("Trinity", rufl_WEIGHT_500, 160,
			"!\xc2\xa0", 3, 0, 0, 0));

	rufl_dump_state(true);

	/* Obtain metrics for a glyph */
	assert(rufl_OK == rufl_glyph_metrics("Homerton", rufl_WEIGHT_500, 160,
			"!", 1, &x_bearing, &y_bearing,
			&mwidth, &mheight, &x_advance, &y_advance));
	assert(0 == x_bearing);
	assert(10000 == y_bearing);
	assert(10000 == mwidth);
	assert(10000 == mheight);
	assert(10000 == x_advance);
	assert(10000 == y_advance);

	rufl_quit();

	/* Reinit -- should load cache */
	assert(rufl_OK == rufl_init());
	assert(NULL == rufl_fm_error);
	assert(3 == rufl_family_list_entries);
	assert(NULL != rufl_family_menu);
	/* Done for real this time */
	rufl_quit();

	printf("PASS\n");

	return 0;
}
