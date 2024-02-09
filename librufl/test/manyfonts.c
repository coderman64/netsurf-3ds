/*
 * XXX: This test currently needs the following patch to be valid.
 * We need a way of ensuring the library chooses the direct substitution
 * table format (which basically means we need to flood a plane with glyphs)
 *
 * diff --git a/src/rufl_substitution_table.c b/src/rufl_substitution_table.c
 * index f5de7d8..2b58b72 100644
 * --- a/src/rufl_substitution_table.c
 * +++ b/src/rufl_substitution_table.c
 * @@ -1019,7 +1019,7 @@ static rufl_code create_substitution_table_for_plane(unsigned int plane)
 *                         table_entries, blocks_used);
 *         chd_size = rufl_substitution_table_estimate_size_chd(
 *                         table_entries, blocks_used);
 * -       if (direct_size <= chd_size) {
 * +       if (1 || direct_size <= chd_size) {
 *                 result = direct(table, table_entries, blocks_used,
 *                                 block_histogram,
 *                                 &rufl_substitution_table[plane]);
 */

#include <ftw.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "rufl.h"

#include "harness.h"
#include "testutils.h"

static char template[] = "/tmp/manyfontsXXXXXX";
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
	char *names[300];
	int x;

	UNUSED(argc);
	UNUSED(argv);

	ptmp = mkdtemp(template);
	assert(NULL != ptmp);
	atexit(cleanup);
	chdir(ptmp);

	rufl_test_harness_init(380, true, true);

	for (x = 0; x < 300; x++) {
		char buf[64];
		sprintf(buf, "Font%03d", x);
		names[x] = strdup(buf);
		rufl_test_harness_register_font(names[x]);
	}

	assert(rufl_OK == rufl_init());
	assert(NULL == rufl_fm_error);
	assert(303 == rufl_family_list_entries);
	assert(NULL != rufl_family_menu);

	rufl_dump_state(true);

	rufl_quit();

	/* Reinit -- should load cache */
	assert(rufl_OK == rufl_init());
	assert(NULL == rufl_fm_error);
	assert(303 == rufl_family_list_entries);
	assert(NULL != rufl_family_menu);
	/* Done for real this time */
	rufl_quit();

	for (x = 0; x < 300; x++) {
		free(names[x]);
	}

	printf("PASS\n");

	return 0;
}
