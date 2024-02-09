/*
 * Copyright 2015 Vincent Sanders <vince@netsurf-browser.org>
 * Copyright 2013 Rob Kendrick <rjek@netsurf-browser.org>
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
 * Test bloom filter operations.
 *
 * Implementation taken from original test rig in bloom filter code
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <check.h>

#include "utils/bloom.h"

#define BLOOM_SIZE 8192
#define FALSE_POSITIVE_RATE 15 /* acceptable false positive percentage rate */

static struct bloom_filter *dict_bloom;

/* Fixtures */


/**
 * create dictionary bloom
 *
 * bloom constructed from the first BLOOM_SIZE entries of the
 * dictionary
 */
static void dict_bloom_create(void)
{
	FILE *dictf;
	char buf[BUFSIZ];
	int i;

	dictf = fopen("/usr/share/dict/words", "r");
	ck_assert(dictf != NULL);

	dict_bloom = bloom_create(BLOOM_SIZE);
	ck_assert(dict_bloom != NULL);

	for (i = 0; i < BLOOM_SIZE; i++) {
		fscanf(dictf, "%s", buf);
		bloom_insert_str(dict_bloom, buf, strlen(buf));
	}

	fclose(dictf);
}

static void dict_bloom_teardown(void)
{
	bloom_destroy(dict_bloom);
}

/* Tests */

/**
 * Test blooom filter creation
 *
 * Create a bloom filter, add a single entry and test for presece and
 * absence of that entry (single entry cannot have false positives).
 *
 */
START_TEST(bloom_create_test)
{
	struct bloom_filter *b;
	b = bloom_create(BLOOM_SIZE);
	ck_assert(b != NULL);

	bloom_insert_str(b, "NetSurf", 7);
	ck_assert(bloom_search_str(b, "NetSurf", 7));
	ck_assert(!bloom_search_str(b, "NotSurf", 7));

	ck_assert(bloom_items(b) == 1);

	bloom_destroy(b);
}
END_TEST

/**
 * insert empty string test
 */
START_TEST(bloom_insert_empty_str_test)
{
	struct bloom_filter *b;
	b = bloom_create(BLOOM_SIZE);
	ck_assert(b != NULL);

	bloom_insert_str(b, NULL, 7);

	ck_assert(bloom_items(b) == 1);

	bloom_destroy(b);
}
END_TEST


/**
 * Basic API creation test case
 */
static TCase *bloom_api_case_create(void)
{
	TCase *tc;

	tc = tcase_create("Creation");

	tcase_add_test(tc, bloom_create_test);
	tcase_add_test(tc, bloom_insert_empty_str_test);

	return tc;
}


START_TEST(bloom_match_test)
{
	FILE *dictf;
	char buf[BUFSIZ];
	int i;

	dictf = fopen("/usr/share/dict/words", "r");
	ck_assert(dictf != NULL);

	for (i = 0; i < BLOOM_SIZE; i++) {
		fscanf(dictf, "%s", buf);
		ck_assert(bloom_search_str(dict_bloom, buf, strlen(buf)));
	}
	fclose(dictf);
}
END_TEST


/**
 * Matching entry test case
 */
static TCase *bloom_match_case_create(void)
{
	TCase *tc;

	tc = tcase_create("Match");

	tcase_add_checked_fixture(tc,
				  dict_bloom_create,
				  dict_bloom_teardown);

	tcase_add_test(tc, bloom_match_test);

	return tc;
}


START_TEST(bloom_falsepositive_test)
{
	FILE *dictf;
	char buf[BUFSIZ];
	int i;
	int false_positives = 0;

	dictf = fopen("/usr/share/dict/words", "r");
	ck_assert(dictf != NULL);

	/* skip elements known presnent */
	for (i = 0; i < BLOOM_SIZE; i++) {
		fscanf(dictf, "%s", buf);
	}

	/* false positives are possible we are checking for low rate */
	for (i = 0; i < BLOOM_SIZE; i++) {
		fscanf(dictf, "%s", buf);
		if (bloom_search_str(dict_bloom, buf, strlen(buf)) == true)
			false_positives++;
	}
	fclose(dictf);

	printf("false positive rate %d%%/%d%%\n",
	       (false_positives * 100)/BLOOM_SIZE,
		FALSE_POSITIVE_RATE);
	ck_assert(false_positives < ((BLOOM_SIZE * FALSE_POSITIVE_RATE) / 100));
}
END_TEST


/**
 * Not matching test case
 */
static TCase *bloom_rate_case_create(void)
{
	TCase *tc;

	tc = tcase_create("False positive rate");

	tcase_add_checked_fixture(tc,
				  dict_bloom_create,
				  dict_bloom_teardown);

	tcase_add_test(tc, bloom_falsepositive_test);

	return tc;
}


static Suite *bloom_suite(void)
{
	Suite *s;
	s = suite_create("Bloom filter");

	suite_add_tcase(s, bloom_api_case_create());
	suite_add_tcase(s, bloom_match_case_create());
	suite_add_tcase(s, bloom_rate_case_create());

	return s;
}

int main(int argc, char **argv)
{
	int number_failed;
	Suite *s;
	SRunner *sr;

	s = bloom_suite();

	sr = srunner_create(s);
	srunner_run_all(sr, CK_ENV);

	number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);

	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
