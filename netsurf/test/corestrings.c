/*
 * Copyright 2016 Vincent Sanders <vince@netsurf-browser.org>
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
 * Tests for corestrings.
 */

#include "utils/config.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <check.h>
#include <limits.h>

#include "utils/corestrings.h"

#include "test/malloc_fig.h"

/**
 * The number of corestrings.
 *
 * This is used to test all the out of memory paths in initialisation.
 */
#define CORESTRING_TEST_COUNT 488

START_TEST(corestrings_test)
{
	nserror ires;
	nserror res;

	malloc_limit(_i);

	ires = corestrings_init();
	res = corestrings_fini();

	malloc_limit(UINT_MAX);
	
	if (_i < CORESTRING_TEST_COUNT) {
		ck_assert_int_eq(ires, NSERROR_NOMEM);
	} else {
		ck_assert_int_eq(ires, NSERROR_OK);
	}
	ck_assert_int_eq(res, NSERROR_OK);
}
END_TEST


static TCase *corestrings_case_create(void)
{
	TCase *tc;
	tc = tcase_create("corestrings");

	tcase_add_loop_test(tc, corestrings_test, 0, CORESTRING_TEST_COUNT + 1);

	return tc;
}


/*
 * corestrings test suite creation
 */
static Suite *corestrings_suite_create(void)
{
	Suite *s;
	s = suite_create("Corestrings API");

	suite_add_tcase(s, corestrings_case_create());

	return s;
}

int main(int argc, char **argv)
{
	int number_failed;
	SRunner *sr;

	sr = srunner_create(corestrings_suite_create());

	srunner_run_all(sr, CK_ENV);

	number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);

	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
