/*
 * Copyright 2020 Daniel Silverstone <dsilvers@netsurf-browser.org>
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
 * Tests for hashmap.
 *
 * In part, borrows from the corestrings tests
 */

#include "utils/config.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <check.h>
#include <limits.h>

#include <libwapcaplet/libwapcaplet.h>

#include "utils/nsurl.h"
#include "utils/corestrings.h"
#include "utils/hashmap.h"

#include "test/malloc_fig.h"

/* Low level fixtures */

static void
corestring_create(void)
{
	ck_assert(corestrings_init() == NSERROR_OK);
}

/**
 * iterator for any remaining strings in teardown fixture
 */
static void
netsurf_lwc_iterator(lwc_string *str, void *pw)
{
	fprintf(stderr,
		"[%3u] %.*s",
		str->refcnt,
		(int)lwc_string_length(str),
		lwc_string_data(str));
}

static void
corestring_teardown(void)
{
	corestrings_fini();

	lwc_iterate_strings(netsurf_lwc_iterator, NULL);
}

/* Infra */

static ssize_t keys;
static ssize_t values;

typedef struct {
	nsurl *key;
} hashmap_test_value_t;

static void *
key_clone(void *key)
{
	/* Pretend cloning costs memory so that it can fail for
	 * testing error return pathways
	 */
	void *temp = malloc(1);
	if (temp == NULL) return NULL;
	free(temp);
	/* In reality we just ref the nsurl */
	keys++;
	return nsurl_ref((nsurl *)key);
}

static void
key_destroy(void *key)
{
	keys--;
	nsurl_unref((nsurl *)key);
}

static uint32_t
key_hash(void *key)
{
	/* Deliberately bad hash.
	 * returns 0, 1, 2, or 3 to force bucket chaining
	 */
	return nsurl_hash((nsurl *)key) & 3;
}

static bool
key_eq(void *key1, void *key2)
{
	return nsurl_compare((nsurl *)key1, (nsurl*)key2, NSURL_COMPLETE);
}

static void *
value_alloc(void *key)
{
	hashmap_test_value_t *ret = malloc(sizeof(hashmap_test_value_t));

	if (ret == NULL)
		return NULL;

	ret->key = (nsurl *)key;
	
	values++;

	return ret;
}

static void
value_destroy(void *value)
{
	hashmap_test_value_t *val = value;
	
	/* Do nothing for now */
	
	free(val);
	values--;
}

static hashmap_parameters_t test_params = {
	.key_clone = key_clone,
	.key_hash = key_hash,
	.key_eq = key_eq,
	.key_destroy = key_destroy,
	.value_alloc = value_alloc,
	.value_destroy = value_destroy,
};

/* Iteration helpers */

static size_t iteration_counter = 0;
static size_t iteration_stop = 0;
static char iteration_ctx = 0;

static bool
hashmap_test_iterator_cb(void *key, void *value, void *ctx)
{
	ck_assert(ctx == &iteration_ctx);
	iteration_counter++;
	return iteration_counter == iteration_stop;
}

/* Fixtures for basic tests */

static hashmap_t *test_hashmap = NULL;

static void
basic_fixture_create(void)
{
	corestring_create();

	test_hashmap = hashmap_create(&test_params);

	ck_assert(test_hashmap != NULL);
	ck_assert_int_eq(keys, 0);
	ck_assert_int_eq(values, 0);
}

static void
basic_fixture_teardown(void)
{
	hashmap_destroy(test_hashmap);
	test_hashmap = NULL;

	ck_assert_int_eq(keys, 0);
	ck_assert_int_eq(values, 0);
	
	corestring_teardown();
}

/* basic api tests */

START_TEST(empty_hashmap_create_destroy)
{
	ck_assert_int_eq(hashmap_count(test_hashmap), 0);
}
END_TEST

START_TEST(check_not_present)
{
	/* We're checking for a key which should not be present */
	ck_assert(hashmap_lookup(test_hashmap, corestring_nsurl_about_blank) == NULL);
}
END_TEST

START_TEST(insert_works)
{
        hashmap_test_value_t *value = hashmap_insert(test_hashmap, corestring_nsurl_about_blank);
	ck_assert(value != NULL);
	ck_assert(value->key == corestring_nsurl_about_blank);
	ck_assert_int_eq(hashmap_count(test_hashmap), 1);
}
END_TEST

START_TEST(remove_not_present)
{
	ck_assert(hashmap_remove(test_hashmap, corestring_nsurl_about_blank) == false);
}
END_TEST

START_TEST(insert_then_remove)
{
        hashmap_test_value_t *value = hashmap_insert(test_hashmap, corestring_nsurl_about_blank);
	ck_assert(value != NULL);
	ck_assert(value->key == corestring_nsurl_about_blank);
	ck_assert_int_eq(keys, 1);
	ck_assert_int_eq(values, 1);
	ck_assert_int_eq(hashmap_count(test_hashmap), 1);
	ck_assert(hashmap_remove(test_hashmap, corestring_nsurl_about_blank) == true);
	ck_assert_int_eq(keys, 0);
	ck_assert_int_eq(values, 0);
	ck_assert_int_eq(hashmap_count(test_hashmap), 0);
}
END_TEST

START_TEST(insert_then_lookup)
{
        hashmap_test_value_t *value = hashmap_insert(test_hashmap, corestring_nsurl_about_blank);
	ck_assert(value != NULL);
	ck_assert(value->key == corestring_nsurl_about_blank);
	ck_assert(hashmap_lookup(test_hashmap, corestring_nsurl_about_blank) == value);
}
END_TEST

START_TEST(iterate_empty)
{
	iteration_stop = iteration_counter = 0;
	ck_assert(hashmap_iterate(test_hashmap, hashmap_test_iterator_cb, &iteration_ctx) == false);
	ck_assert_int_eq(iteration_counter, 0);
}
END_TEST

START_TEST(iterate_one)
{
	iteration_stop = iteration_counter = 0;
        hashmap_test_value_t *value = hashmap_insert(test_hashmap, corestring_nsurl_about_blank);
	ck_assert(value != NULL);
	ck_assert(hashmap_iterate(test_hashmap, hashmap_test_iterator_cb, &iteration_ctx) == false);
	ck_assert_int_eq(iteration_counter, 1);
}
END_TEST

START_TEST(iterate_one_and_stop)
{
	iteration_stop = 1;
	iteration_counter = 0;
        hashmap_test_value_t *value = hashmap_insert(test_hashmap, corestring_nsurl_about_blank);
	ck_assert(value != NULL);
	ck_assert(hashmap_iterate(test_hashmap, hashmap_test_iterator_cb, &iteration_ctx) == true);
	ck_assert_int_eq(iteration_counter, 1);
}
END_TEST

static TCase *basic_api_case_create(void)
{
	TCase *tc;
	tc = tcase_create("Basic API");
	
	tcase_add_unchecked_fixture(tc,
				    basic_fixture_create,
				    basic_fixture_teardown);
	
	tcase_add_test(tc, empty_hashmap_create_destroy);
	tcase_add_test(tc, check_not_present);
	tcase_add_test(tc, insert_works);
	tcase_add_test(tc, remove_not_present);
	tcase_add_test(tc, insert_then_remove);
	tcase_add_test(tc, insert_then_lookup);
	
	tcase_add_test(tc, iterate_empty);
	tcase_add_test(tc, iterate_one);
	tcase_add_test(tc, iterate_one_and_stop);
	
	return tc;
}

/* Chain verification test suite */

typedef struct {
	const char *url;
	nsurl *nsurl;
} case_pair;

/* The hobbled hash has only 4 values
 * By having at least 12 test cases, we can be confident that
 * at worst they'll all be on one chain, but at best there'll
 * be four chains of 3 entries which means we should be able
 * to validate prevptr and next in all cases.
 */
static case_pair chain_pairs[] = {
	{ "https://www.google.com/", NULL },
	{ "https://www.google.co.uk/", NULL },
	{ "https://www.netsurf-browser.org/", NULL },
	{ "http://www.google.com/", NULL },
	{ "http://www.google.co.uk/", NULL },
	{ "http://www.netsurf-browser.org/", NULL },
	{ "file:///tmp/test.html", NULL },
	{ "file:///tmp/inner.html", NULL },
	{ "about:blank", NULL },
	{ "about:welcome", NULL },
	{ "about:testament", NULL },
	{ "resources:default.css", NULL },
	{ NULL, NULL }
};

static void
chain_fixture_create(void)
{
	case_pair *chain_case = chain_pairs;
	basic_fixture_create();
	
	while (chain_case->url != NULL) {
		ck_assert(nsurl_create(chain_case->url, &chain_case->nsurl) == NSERROR_OK);
		chain_case++;
	}
	
}

static void
chain_fixture_teardown(void)
{
	case_pair *chain_case = chain_pairs;
	
	while (chain_case->url != NULL) {
		nsurl_unref(chain_case->nsurl);
		chain_case->nsurl = NULL;
		chain_case++;
	}
	
	basic_fixture_teardown();
}

START_TEST(chain_add_remove_all)
{
	case_pair *chain_case;
	
	for (chain_case = chain_pairs;
	     chain_case->url != NULL;
	     chain_case++) {
		ck_assert(hashmap_lookup(test_hashmap, chain_case->nsurl) == NULL);
		ck_assert(hashmap_insert(test_hashmap, chain_case->nsurl) != NULL);
		ck_assert(hashmap_lookup(test_hashmap, chain_case->nsurl) != NULL);
		ck_assert(hashmap_remove(test_hashmap, chain_case->nsurl) == true);
	}

	ck_assert_int_eq(keys, 0);
	ck_assert_int_eq(values, 0);
}
END_TEST

START_TEST(chain_add_all_remove_all)
{
	case_pair *chain_case;
	
	for (chain_case = chain_pairs;
	     chain_case->url != NULL;
	     chain_case++) {
		ck_assert(hashmap_lookup(test_hashmap, chain_case->nsurl) == NULL);
		ck_assert(hashmap_insert(test_hashmap, chain_case->nsurl) != NULL);
	}

	for (chain_case = chain_pairs;
	     chain_case->url != NULL;
	     chain_case++) {
		ck_assert(hashmap_remove(test_hashmap, chain_case->nsurl) == true);
	}

	ck_assert_int_eq(keys, 0);
	ck_assert_int_eq(values, 0);
}
END_TEST

START_TEST(chain_add_all_twice_remove_all)
{
	case_pair *chain_case;
	
	for (chain_case = chain_pairs;
	     chain_case->url != NULL;
	     chain_case++) {
		ck_assert(hashmap_lookup(test_hashmap, chain_case->nsurl) == NULL);
		ck_assert(hashmap_insert(test_hashmap, chain_case->nsurl) != NULL);
	}

	for (chain_case = chain_pairs;
	     chain_case->url != NULL;
	     chain_case++) {
		ck_assert(hashmap_lookup(test_hashmap, chain_case->nsurl) != NULL);
		ck_assert(hashmap_insert(test_hashmap, chain_case->nsurl) != NULL);
	}

	for (chain_case = chain_pairs;
	     chain_case->url != NULL;
	     chain_case++) {
		ck_assert(hashmap_remove(test_hashmap, chain_case->nsurl) == true);
	}

	ck_assert_int_eq(keys, 0);
	ck_assert_int_eq(values, 0);
}
END_TEST

START_TEST(chain_add_all_twice_remove_all_iterate)
{
	case_pair *chain_case;
	size_t chain_count = 0;
	
	for (chain_case = chain_pairs;
	     chain_case->url != NULL;
	     chain_case++) {
		ck_assert(hashmap_lookup(test_hashmap, chain_case->nsurl) == NULL);
		ck_assert(hashmap_insert(test_hashmap, chain_case->nsurl) != NULL);
		chain_count++;
	}
	
	iteration_counter = 0;
	iteration_stop = 0;
	ck_assert(hashmap_iterate(test_hashmap, hashmap_test_iterator_cb, &iteration_ctx) == false);
	ck_assert_int_eq(iteration_counter, chain_count);
	
	for (chain_case = chain_pairs;
	     chain_case->url != NULL;
	     chain_case++) {
		ck_assert(hashmap_lookup(test_hashmap, chain_case->nsurl) != NULL);
		ck_assert(hashmap_insert(test_hashmap, chain_case->nsurl) != NULL);
	}

	iteration_counter = 0;
	iteration_stop = 0;
	ck_assert(hashmap_iterate(test_hashmap, hashmap_test_iterator_cb, &iteration_ctx) == false);
	ck_assert_int_eq(iteration_counter, chain_count);
	ck_assert_int_eq(hashmap_count(test_hashmap), chain_count);
	
	iteration_counter = 0;
	iteration_stop = chain_count;
	ck_assert(hashmap_iterate(test_hashmap, hashmap_test_iterator_cb, &iteration_ctx) == true);
	ck_assert_int_eq(iteration_counter, chain_count);
	
	for (chain_case = chain_pairs;
	     chain_case->url != NULL;
	     chain_case++) {
		ck_assert(hashmap_remove(test_hashmap, chain_case->nsurl) == true);
	}

	iteration_counter = 0;
	iteration_stop = chain_count;
	ck_assert(hashmap_iterate(test_hashmap, hashmap_test_iterator_cb, &iteration_ctx) == false);
	ck_assert_int_eq(iteration_counter, 0);
	
	ck_assert_int_eq(keys, 0);
	ck_assert_int_eq(values, 0);
	ck_assert_int_eq(hashmap_count(test_hashmap), 0);
}
END_TEST

#define CHAIN_TEST_MALLOC_COUNT_MAX 60

START_TEST(chain_add_all_remove_all_alloc)
{
	bool failed = false;
	case_pair *chain_case;
		
	malloc_limit(_i);

	for (chain_case = chain_pairs;
	     chain_case->url != NULL;
	     chain_case++) {
		if (hashmap_insert(test_hashmap, chain_case->nsurl) == NULL) {
			failed = true;
		}
	}

	for (chain_case = chain_pairs;
	     chain_case->url != NULL;
	     chain_case++) {
		if (hashmap_insert(test_hashmap, chain_case->nsurl) == NULL) {
			failed = true;
		}
	}

	for (chain_case = chain_pairs;
	     chain_case->url != NULL;
	     chain_case++) {
		hashmap_remove(test_hashmap, chain_case->nsurl);
	}

	malloc_limit(UINT_MAX);
	
	ck_assert_int_eq(keys, 0);
	ck_assert_int_eq(values, 0);
	
	if (_i < CHAIN_TEST_MALLOC_COUNT_MAX) {
		ck_assert(failed);
	} else {
		ck_assert(!failed);
	}
	
}
END_TEST

static TCase *chain_case_create(void)
{
	TCase *tc;
	tc = tcase_create("Bucket Chain tests");
	
	tcase_add_unchecked_fixture(tc,
				    chain_fixture_create,
				    chain_fixture_teardown);
	
	tcase_add_test(tc, chain_add_remove_all);
	tcase_add_test(tc, chain_add_all_remove_all);
	tcase_add_test(tc, chain_add_all_twice_remove_all);
	tcase_add_test(tc, chain_add_all_twice_remove_all_iterate);

	tcase_add_loop_test(tc, chain_add_all_remove_all_alloc, 0, CHAIN_TEST_MALLOC_COUNT_MAX + 1);
	
	return tc;
}

/*
 * hashmap test suite creation
 */
static Suite *hashmap_suite_create(void)
{
	Suite *s;
	s = suite_create("Hashmap");

	suite_add_tcase(s, basic_api_case_create());
	suite_add_tcase(s, chain_case_create());

	return s;
}

int main(int argc, char **argv)
{
	int number_failed;
	SRunner *sr;

	sr = srunner_create(hashmap_suite_create());

	srunner_run_all(sr, CK_ENV);

	number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);

	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
