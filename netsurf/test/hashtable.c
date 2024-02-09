/*
 * Copyright 2015 Vincent Sanders <vince@netsurf-browser.org>
 * Copyright 2006 Rob Kendrick <rjek@netsurf-browser.org>
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
 * Test hash table operations.
 *
 * Implementation taken from original test rig in bloom filter code
 */

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <check.h>

#include "utils/errors.h"
#include "utils/hashtable.h"

/* Limit for hash table tests which use /usr/share/dict/words */
#define DICT_TEST_WORD_COUNT 100000

#define NELEMS(x)  (sizeof(x) / sizeof((x)[0]))

struct test_pairs {
	const char* test;
	const char* res;
};

static struct hash_table *match_hash_a;
static struct hash_table *match_hash_b;

static struct hash_table *dict_hash;

static const struct test_pairs match_tests[] = {
	{ "cow", "moo" },
	{ "pig", "oink" },
	{ "chicken", "cluck" },
	{ "dog", "woof" },
	{ "sheep", "baaa" },
};

/* Fixtures */

static void match_hashtable_create(void)
{
	unsigned int idx;

	match_hash_a = hash_create(79);
	ck_assert(match_hash_a != NULL);

	match_hash_b = hash_create(103);
	ck_assert(match_hash_b != NULL);

	for (idx = 0; idx < NELEMS(match_tests); idx++) {
		hash_add(match_hash_a,
			 match_tests[idx].test,
			 match_tests[idx].res);
		hash_add(match_hash_b,
			 match_tests[idx].res,
			 match_tests[idx].test);
	}
}

static void match_hashtable_teardown(void)
{
	hash_destroy(match_hash_a);
	hash_destroy(match_hash_b);
}


/**
 * create dictionary hashtable
 *
 * hashtable constructed from the odd/even rows of the
 * dictionary
 */
static void dict_hashtable_create(int dict_hash_size)
{
	FILE *dictf;
	char keybuf[BUFSIZ], valbuf[BUFSIZ];
	uint32_t counter = 0;

	dictf = fopen("/usr/share/dict/words", "r");
	ck_assert(dictf != NULL);

	dict_hash = hash_create(dict_hash_size);
	ck_assert(dict_hash != NULL);

	while (!feof(dictf)) {
		fscanf(dictf, "%s", keybuf);
		fscanf(dictf, "%s", valbuf);
		hash_add(dict_hash, keybuf, valbuf);
		if (counter++ > DICT_TEST_WORD_COUNT) break;
	}

	fclose(dictf);
}

static void dicts_hashtable_create(void)
{
	dict_hashtable_create(1031);
}

static void dictl_hashtable_create(void)
{
	dict_hashtable_create(7919);
}

static void dict_hashtable_teardown(void)
{
	hash_destroy(dict_hash);
}

/* Tests */

/**
 * Test hash table creation
 *
 * Create a hash table, add a single entry and test for value retival
 * from key.
 *
 */
START_TEST(hashtable_create_test)
{
	struct hash_table *ht;

	ht = hash_create(42);
	ck_assert(ht != NULL);

	hash_destroy(ht);
}
END_TEST

/**
 * Test hash table simple operation
 *
 * Create a hash table, add a single entry and test for failed retival
 * from not present key.
 *
 */
START_TEST(hashtable_negative_test)
{
	struct hash_table *ht;
	bool added;
	const char *res;

	/* create hash */
	ht = hash_create(42);
	ck_assert(ht != NULL);

	/* add entry */
	added = hash_add(ht, "cow", "moo");
	ck_assert(added == true);

	res = hash_get(ht, "sheep");
	ck_assert(res == NULL);

	hash_destroy(ht);
}
END_TEST

/**
 * Test hash table simple operation
 *
 * Create a hash table, add a single entry and test for sucessful
 * retrival of key.
 *
 */
START_TEST(hashtable_positive_test)
{
	struct hash_table *ht;
	bool added;
	const char *res;

	/* create hash */
	ht = hash_create(42);
	ck_assert(ht != NULL);

	/* add entry */
	added = hash_add(ht, "cow", "moo");
	ck_assert(added == true);

	res = hash_get(ht, "cow");
	ck_assert(res != NULL);
	ck_assert_str_eq(res, "moo");

	hash_destroy(ht);
}
END_TEST


START_TEST(hashtable_matcha_test)
{
	const char *res;

	res = hash_get(match_hash_a, match_tests[_i].test);
	ck_assert(res != NULL);
	ck_assert_str_eq(res, match_tests[_i].res);
}
END_TEST


START_TEST(hashtable_matchb_test)
{
	const char *res;

	res = hash_get(match_hash_b, match_tests[_i].res);
	ck_assert(res != NULL);
	ck_assert_str_eq(res, match_tests[_i].test);
}
END_TEST


START_TEST(hashtable_dict_test)
{
	FILE *dictf;
	char keybuf[BUFSIZ], valbuf[BUFSIZ];
	const char *res;
	uint32_t counter = 0;

	dictf = fopen("/usr/share/dict/words", "r");
	ck_assert(dictf != NULL);

	while (!feof(dictf)) {
		fscanf(dictf, "%s", keybuf);
		fscanf(dictf, "%s", valbuf);

		res = hash_get(dict_hash, keybuf);
		ck_assert(res != NULL);
		ck_assert_str_eq(res, valbuf);
		if (counter++ > DICT_TEST_WORD_COUNT) break;
	}

	fclose(dictf);
}
END_TEST

/* Suite */

static Suite *hashtable_suite(void)
{
	Suite *s;
	TCase *tc_create;
	TCase *tc_match;
	TCase *tc_dict_s;
	TCase *tc_dict_l;

	s = suite_create("hash table filter");

	/* Core API */
	tc_create = tcase_create("Core");

	tcase_add_test(tc_create, hashtable_create_test);
	tcase_add_test(tc_create, hashtable_negative_test);
	tcase_add_test(tc_create, hashtable_positive_test);

	suite_add_tcase(s, tc_create);

	/* Matching entry tests */
	tc_match = tcase_create("Match");

	tcase_add_checked_fixture(tc_match,
				  match_hashtable_create,
				  match_hashtable_teardown);

	tcase_add_loop_test(tc_match,
			    hashtable_matcha_test,
			    0, NELEMS(match_tests));
	tcase_add_loop_test(tc_match,
			    hashtable_matchb_test,
			    0, NELEMS(match_tests));

	suite_add_tcase(s, tc_match);

	/* small table dictionary test */
	tc_dict_s = tcase_create("small table dictionary");
	tcase_add_checked_fixture(tc_dict_s,
				  dicts_hashtable_create,
				  dict_hashtable_teardown);

	tcase_add_test(tc_dict_s, hashtable_dict_test);

	suite_add_tcase(s, tc_dict_s);


	/* large table dictionary test */
	tc_dict_l = tcase_create("large table dictionary");
	tcase_add_checked_fixture(tc_dict_l,
				  dictl_hashtable_create,
				  dict_hashtable_teardown);

	tcase_add_test(tc_dict_l, hashtable_dict_test);

	suite_add_tcase(s, tc_dict_l);

	return s;
}

int main(int argc, char **argv)
{
	int number_failed;
	Suite *s;
	SRunner *sr;

	s = hashtable_suite();

	sr = srunner_create(s);
	srunner_run_all(sr, CK_ENV);

	number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);

	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
