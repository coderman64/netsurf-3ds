#include <ftw.h>
#include <stdio.h>
#include <unistd.h>

#include "rufl.h"

#include "harness.h"
#include "testutils.h"

static char template[] = "/tmp/ucsinitXXXXXX";
static const char *ptmp = NULL;

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
	if (ptmp == NULL)
		return;

	nftw(ptmp, ftw_cb, FOPEN_MAX, FTW_DEPTH | FTW_MOUNT | FTW_PHYS);
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

	UNUSED(argc);
	UNUSED(argv);

	ptmp = mkdtemp(template);
	assert(NULL != ptmp);
	atexit(cleanup);
	chdir(ptmp);

	rufl_test_harness_init(380, true, true);

	assert(rufl_OK == rufl_init());
	assert(NULL == rufl_fm_error);
	assert(3 == rufl_family_list_entries);
	assert(NULL != rufl_family_menu);

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

	rufl_dump_state(true);

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
