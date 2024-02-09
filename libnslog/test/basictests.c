/* test/basictests.c
 *
 * Basic tests for the test suite for libnslog
 *
 * Copyright 2009,2017 The NetSurf Browser Project
 *                Daniel Silverstone <dsilvers@netsurf-browser.org>
 */

#include <check.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#include "tests.h"

#ifndef UNUSED
#define UNUSED(x) (void)(x)
#endif

NSLOG_DEFINE_CATEGORY(test, "Top level test category");
NSLOG_DEFINE_SUBCATEGORY(test, sub, "Lower level test category");

static void *captured_render_context = NULL;
static nslog_entry_context_t captured_context = { 0 };
static char captured_rendered_message[4096] = { 0 };
static int captured_rendered_message_length = 0;
static int captured_message_count = 0;

static const char* anchor_context_1 = "1";

static void
nslog__test__render_function(void *_ctx, nslog_entry_context_t *ctx,
			     const char *fmt, va_list args)
{
	captured_context = *ctx;
	captured_render_context = _ctx;
	captured_rendered_message_length =
		vsnprintf(captured_rendered_message,
			  sizeof(captured_rendered_message),
			  fmt, args);
	captured_message_count++;
}

/**** The next set of tests need a fixture set ****/

static void
with_simple_context_setup(void)
{
	captured_render_context = NULL;
	memset(&captured_context, 0, sizeof(captured_context));
	memset(captured_rendered_message, 0, sizeof(captured_rendered_message));
	captured_rendered_message_length = 0;
	captured_message_count = 0;
	fail_unless(nslog_set_render_callback(
			    nslog__test__render_function,
			    (void *)anchor_context_1) == NSLOG_NO_ERROR,
		    "Unable to set up render callback");
}

static void
with_simple_context_teardown(void)
{
	nslog_cleanup();
}

START_TEST (test_nslog_trivial_corked_message)
{
	NSLOG(test, INFO, "Hello %s", "world");
	fail_unless(nslog_uncork() == NSLOG_NO_ERROR,
		    "Unable to uncork");
	fail_unless(captured_message_count == 1,
		    "Captured message count was wrong");
	fail_unless(captured_render_context == anchor_context_1,
		    "Captured context wasn't passed through");
	fail_unless(strcmp(captured_context.category->name, "test") == 0,
		    "Captured context category wasn't normalised");
	fail_unless(captured_context.category == &__nslog_category_test,
		    "Captured context category wasn't the one we wanted");
	fail_unless(captured_rendered_message_length == 11,
		    "Captured message wasn't correct length");
	fail_unless(strcmp(captured_rendered_message, "Hello world") == 0,
		    "Captured message wasn't correct");
	fail_unless(strcmp(captured_context.filename, "test/basictests.c") == 0,
		    "Captured message wasn't correct filename");
	fail_unless(strcmp(captured_context.funcname, __func__) == 0,
		    "Captured message wasn't correct function name");
}
END_TEST

START_TEST (test_nslog_trivial_uncorked_message)
{
	fail_unless(nslog_uncork() == NSLOG_NO_ERROR,
		    "Unable to uncork");
	fail_unless(captured_message_count == 0,
		    "Unusual, we had messages from before uncorking");
	NSLOG(test, INFO, "Hello %s", "world");
	fail_unless(captured_message_count == 1,
		    "Captured message count was wrong");
	fail_unless(captured_render_context == anchor_context_1,
		    "Captured context wasn't passed through");
	fail_unless(strcmp(captured_context.category->name, "test") == 0,
		    "Captured context category wasn't normalised");
	fail_unless(captured_context.category == &__nslog_category_test,
		    "Captured context category wasn't the one we wanted");
	fail_unless(captured_rendered_message_length == 11,
		    "Captured message wasn't correct length");
	fail_unless(strcmp(captured_rendered_message, "Hello world") == 0,
		    "Captured message wasn't correct");
	fail_unless(strcmp(captured_context.filename, "test/basictests.c") == 0,
		    "Captured message wasn't correct filename");
	fail_unless(strcmp(captured_context.funcname, __func__) == 0,
		    "Captured message wasn't correct function name");
}
END_TEST

START_TEST (test_nslog_subcategory_name)
{
	fail_unless(nslog_uncork() == NSLOG_NO_ERROR,
		    "Unable to uncork");
	fail_unless(captured_message_count == 0,
		    "Unusual, we had messages from before uncorking");
	NSLOG(sub, INFO, "Hello %s", "world");
	fail_unless(captured_message_count == 1,
		    "Captured message count was wrong");
	fail_unless(captured_render_context == anchor_context_1,
		    "Captured context wasn't passed through");
	fail_unless(strcmp(captured_context.category->name, "test/sub") == 0,
		    "Captured context category wasn't normalised");
}
END_TEST

START_TEST (test_nslog_two_corked_messages)
{
	NSLOG(test, INFO, "First");
	NSLOG(sub, CRIT, "Second");
	fail_unless(nslog_uncork() == NSLOG_NO_ERROR,
		    "Unable to uncork");
	fail_unless(captured_message_count == 2,
		    "Captured message count was wrong");
	fail_unless(captured_render_context == anchor_context_1,
		    "Captured context wasn't passed through");
	fail_unless(strcmp(captured_context.category->name, "test/sub") == 0,
		    "Captured context category wasn't normalised");
	fail_unless(captured_context.category == &__nslog_category_sub,
		    "Captured context category wasn't the one we wanted");
	fail_unless(captured_rendered_message_length == 6,
		    "Captured message wasn't correct length");
	fail_unless(strcmp(captured_rendered_message, "Second") == 0,
		    "Captured message wasn't correct");
	fail_unless(strcmp(captured_context.filename, "test/basictests.c") == 0,
		    "Captured message wasn't correct filename");
	fail_unless(strcmp(captured_context.funcname, __func__) == 0,
		    "Captured message wasn't correct function name");
}
END_TEST

START_TEST (test_nslog_check_bad_level)
{
	fail_unless(strcmp(nslog_level_name((nslog_level)-1),
			   "**UNKNOWN**") == 0,
		    "Failed to fail to render bad level");
}
END_TEST

/**** The next set of tests need a fixture set for filters ****/

static nslog_filter_t *cat_test = NULL;
static nslog_filter_t *cat_another = NULL;
static nslog_filter_t *cat_test_sub = NULL;

static const char *anchor_context_2 = "2";

static void
with_simple_filter_context_setup(void)
{
	captured_render_context = NULL;
	memset(&captured_context, 0, sizeof(captured_context));
	memset(captured_rendered_message, 0, sizeof(captured_rendered_message));
	captured_rendered_message_length = 0;
	captured_message_count = 0;
	fail_unless(nslog_set_render_callback(
			    nslog__test__render_function,
			    (void *)anchor_context_2) == NSLOG_NO_ERROR,
		    "Unable to set up render callback");
	fail_unless(nslog_filter_category_new("test", &cat_test) == NSLOG_NO_ERROR,
		    "Unable to create a category filter for 'test'");
	fail_unless(nslog_filter_category_new("another", &cat_another) == NSLOG_NO_ERROR,
		    "Unable to create a category filter for 'another'");
	fail_unless(nslog_filter_category_new("test/sub", &cat_test_sub) == NSLOG_NO_ERROR,
		    "Unable to create a category filter for 'test/sub'");
}

static void
with_simple_filter_context_teardown(void)
{
	fail_unless(nslog_filter_set_active(NULL, NULL) == NSLOG_NO_ERROR,
		    "Unable to clear active filter");
	cat_test = nslog_filter_unref(cat_test);
	cat_another = nslog_filter_unref(cat_another);
	cat_test_sub = nslog_filter_unref(cat_test_sub);
	nslog_cleanup();
}

START_TEST (test_nslog_simple_filter_corked_message)
{
	NSLOG(test, INFO, "Hello world");
	fail_unless(nslog_filter_set_active(cat_test, NULL) == NSLOG_NO_ERROR,
		    "Unable to set active filter to cat:test");
	fail_unless(nslog_uncork() == NSLOG_NO_ERROR,
		    "Unable to uncork");
	fail_unless(captured_message_count == 1,
		    "Captured message count was wrong");
	fail_unless(captured_render_context == anchor_context_2,
		    "Captured context wasn't passed through");
	fail_unless(strcmp(captured_context.category->name, "test") == 0,
		    "Captured context category wasn't normalised");
	fail_unless(captured_context.category == &__nslog_category_test,
		    "Captured context category wasn't the one we wanted");
	fail_unless(captured_rendered_message_length == 11,
		    "Captured message wasn't correct length");
	fail_unless(strcmp(captured_rendered_message, "Hello world") == 0,
		    "Captured message wasn't correct");
	fail_unless(strcmp(captured_context.filename, "test/basictests.c") == 0,
		    "Captured message wasn't correct filename");
	fail_unless(strcmp(captured_context.funcname, __func__) == 0,
		    "Captured message wasn't correct function name");

}
END_TEST

START_TEST (test_nslog_simple_filter_uncorked_message)
{
	fail_unless(nslog_filter_set_active(cat_test, NULL) == NSLOG_NO_ERROR,
		    "Unable to set active filter to cat:test");
	fail_unless(nslog_uncork() == NSLOG_NO_ERROR,
		    "Unable to uncork");
	NSLOG(test, INFO, "Hello world");
	fail_unless(captured_message_count == 1,
		    "Captured message count was wrong");
	fail_unless(captured_render_context == anchor_context_2,
		    "Captured context wasn't passed through");
	fail_unless(strcmp(captured_context.category->name, "test") == 0,
		    "Captured context category wasn't normalised");
	fail_unless(captured_context.category == &__nslog_category_test,
		    "Captured context category wasn't the one we wanted");
	fail_unless(captured_rendered_message_length == 11,
		    "Captured message wasn't correct length");
	fail_unless(strcmp(captured_rendered_message, "Hello world") == 0,
		    "Captured message wasn't correct");
	fail_unless(strcmp(captured_context.filename, "test/basictests.c") == 0,
		    "Captured message wasn't correct filename");
	fail_unless(strcmp(captured_context.funcname, __func__) == 0,
		    "Captured message wasn't correct function name");

}
END_TEST

START_TEST (test_nslog_simple_filter_subcategory_message)
{
	fail_unless(nslog_filter_set_active(cat_test, NULL) == NSLOG_NO_ERROR,
		    "Unable to set active filter to cat:test");
	fail_unless(nslog_uncork() == NSLOG_NO_ERROR,
		    "Unable to uncork");
	NSLOG(sub, INFO, "Hello world");
	fail_unless(captured_message_count == 1,
		    "Captured message count was wrong");
	fail_unless(captured_render_context == anchor_context_2,
		    "Captured context wasn't passed through");
	fail_unless(strcmp(captured_context.category->name, "test/sub") == 0,
		    "Captured context category wasn't normalised");
	fail_unless(captured_context.category == &__nslog_category_sub,
		    "Captured context category wasn't the one we wanted");
	fail_unless(captured_rendered_message_length == 11,
		    "Captured message wasn't correct length");
	fail_unless(strcmp(captured_rendered_message, "Hello world") == 0,
		    "Captured message wasn't correct");
	fail_unless(strcmp(captured_context.filename, "test/basictests.c") == 0,
		    "Captured message wasn't correct filename");
	fail_unless(strcmp(captured_context.funcname, __func__) == 0,
		    "Captured message wasn't correct function name");

}
END_TEST

START_TEST (test_nslog_simple_filter_out_subcategory_message)
{
	fail_unless(nslog_filter_set_active(cat_test_sub, NULL) == NSLOG_NO_ERROR,
		    "Unable to set active filter to cat:test/sub");
	fail_unless(nslog_uncork() == NSLOG_NO_ERROR,
		    "Unable to uncork");
	NSLOG(test, INFO, "Hello world");
	fail_unless(captured_message_count == 0,
		    "Captured message count was wrong");
}
END_TEST

START_TEST (test_nslog_basic_filter_sprintf)
{
	char *ct = nslog_filter_sprintf(cat_test);
	fail_unless(ct != NULL, "Unable to sprintf");
	fail_unless(strcmp(ct, "cat:test") == 0,
		    "Printed category test is wrong");
	free(ct);
	ct = nslog_filter_sprintf(cat_another);
	fail_unless(ct != NULL, "Unable to sprintf");
	fail_unless(strcmp(ct, "cat:another") == 0,
		    "Printed category another is wrong");
	free(ct);
}
END_TEST

START_TEST (test_nslog_parse_and_sprintf)
{
	nslog_filter_t *filt = NULL;
	fail_unless(nslog_filter_from_text("cat:test", &filt) == NSLOG_NO_ERROR,
		    "Unable to parse cat:test");
	fail_unless(filt != NULL,
		    "Strange, despite parsing okay, filt was NULL");
	char *ct = nslog_filter_sprintf(filt);
	nslog_filter_unref(filt);
	fail_unless(strcmp(ct, "cat:test") == 0,
		    "Printed parsed cat:test not right");
	free(ct);
}
END_TEST

START_TEST (test_nslog_parse_and_sprintf_all_levels)
{
	nslog_filter_t *filt = NULL;
	const char *input =
		"((((((lvl:DEEPDEBUG || lvl:DEBUG) || lvl:VERBOSE) || lvl:INFO) || lvl:WARNING) || lvl:ERROR) || lvl:CRITICAL)";
	fail_unless(nslog_filter_from_text(input, &filt) == NSLOG_NO_ERROR,
		    "Unable to parse all level test");
	fail_unless(filt != NULL,
		    "Strange, despite parsing okay, filt was NULL");
	char *ct = nslog_filter_sprintf(filt);
	nslog_filter_unref(filt);
	fail_unless(strcmp(ct, input) == 0,
		    "Printed parsed all-level not right");
	free(ct);
}
END_TEST

START_TEST (test_nslog_parse_and_sprintf_all_kinds)
{
	nslog_filter_t *filt = NULL;
	const char *input =
		"!((((lvl:WARNING || cat:test) && file:foo) ^ dir:bar) || func:baz)";
	fail_unless(nslog_filter_from_text(input, &filt) == NSLOG_NO_ERROR,
		    "Unable to parse all kind test");
	fail_unless(filt != NULL,
		    "Strange, despite parsing okay, filt was NULL");
	char *ct = nslog_filter_sprintf(filt);
	nslog_filter_unref(filt);
	fail_unless(strcmp(ct, input) == 0,
		    "Printed parsed all-kind not right");
	free(ct);
}
END_TEST

/**** The next set of tests need a fixture set for a variety of filters ****/

static const char *anchor_context_3 = "3";

static void
with_trivial_filter_context_setup(void)
{
	captured_render_context = NULL;
	memset(&captured_context, 0, sizeof(captured_context));
	memset(captured_rendered_message, 0, sizeof(captured_rendered_message));
	captured_rendered_message_length = 0;
	captured_message_count = 0;
	fail_unless(nslog_set_render_callback(
			    nslog__test__render_function,
			    (void *)anchor_context_3) == NSLOG_NO_ERROR,
		    "Unable to set up render callback");
}

static void
with_trivial_filter_context_teardown(void)
{
	fail_unless(nslog_filter_set_active(NULL, NULL) == NSLOG_NO_ERROR,
		    "Unable to clear active filter");
	nslog_cleanup();
}

START_TEST (test_nslog_filter_filename)
{
	nslog_filter_t *filter;
	fail_unless(nslog_filter_filename_new("basictests.c", &filter) == NSLOG_NO_ERROR,
		    "Unable to create filename filter");
	fail_unless(nslog_filter_set_active(filter, NULL) == NSLOG_NO_ERROR,
		    "Unable to set active filter to file:basictests.c");
	fail_unless(nslog_uncork() == NSLOG_NO_ERROR,
		    "Unable to uncork");
	NSLOG(test, WARN, "Hello");
	fail_unless(captured_message_count == 1,
		    "Captured message count was wrong");
	fail_unless(captured_render_context == anchor_context_3,
		    "Captured context wasn't passed through");
	fail_unless(captured_rendered_message_length == 5,
		    "Captured message wasn't correct length");
	fail_unless(strcmp(captured_rendered_message, "Hello") == 0,
		    "Mesage wasn't as expected");
	filter = nslog_filter_unref(filter);
}
END_TEST

START_TEST (test_nslog_filter_full_filename)
{
	nslog_filter_t *filter;
	fail_unless(nslog_filter_filename_new("test/basictests.c", &filter) == NSLOG_NO_ERROR,
		    "Unable to create filename filter");
	fail_unless(nslog_filter_set_active(filter, NULL) == NSLOG_NO_ERROR,
		    "Unable to set active filter to file:test/basictests.c");
	fail_unless(nslog_uncork() == NSLOG_NO_ERROR,
		    "Unable to uncork");
	NSLOG(test, WARN, "Hello");
	fail_unless(captured_message_count == 1,
		    "Captured message count was wrong");
	fail_unless(captured_render_context == anchor_context_3,
		    "Captured context wasn't passed through");
	fail_unless(captured_rendered_message_length == 5,
		    "Captured message wasn't correct length");
	fail_unless(strcmp(captured_rendered_message, "Hello") == 0,
		    "Mesage wasn't as expected");
	filter = nslog_filter_unref(filter);
}
END_TEST

START_TEST (test_nslog_filter_out_filename)
{
	nslog_filter_t *filter;
	fail_unless(nslog_filter_filename_new("testmain.c", &filter) == NSLOG_NO_ERROR,
		    "Unable to create filename filter");
	fail_unless(nslog_filter_set_active(filter, NULL) == NSLOG_NO_ERROR,
		    "Unable to set active filter to file:testmain.c");
	fail_unless(nslog_uncork() == NSLOG_NO_ERROR,
		    "Unable to uncork");
	NSLOG(test, WARN, "Hello");
	fail_unless(captured_message_count == 0,
		    "Captured message count was wrong");
	filter = nslog_filter_unref(filter);
}
END_TEST

START_TEST (test_nslog_filter_level)
{
	nslog_filter_t *filter;
	fail_unless(nslog_filter_level_new(NSLOG_LEVEL_WARN, &filter) == NSLOG_NO_ERROR,
		    "Unable to create level filter");
	fail_unless(nslog_filter_set_active(filter, NULL) == NSLOG_NO_ERROR,
		    "Unable to set active filter to lvl:WARN");
	fail_unless(nslog_uncork() == NSLOG_NO_ERROR,
		    "Unable to uncork");
	NSLOG(test, WARN, "Hello");
	fail_unless(captured_message_count == 1,
		    "Captured message count was wrong");
	fail_unless(captured_render_context == anchor_context_3,
		    "Captured context wasn't passed through");
	fail_unless(captured_rendered_message_length == 5,
		    "Captured message wasn't correct length");
	fail_unless(strcmp(captured_rendered_message, "Hello") == 0,
		    "Mesage wasn't as expected");
	filter = nslog_filter_unref(filter);
}
END_TEST

START_TEST (test_nslog_filter_out_level)
{
	nslog_filter_t *filter;
	fail_unless(nslog_filter_level_new(NSLOG_LEVEL_ERR, &filter) == NSLOG_NO_ERROR,
		    "Unable to create filename filter");
	fail_unless(nslog_filter_set_active(filter, NULL) == NSLOG_NO_ERROR,
		    "Unable to set active filter to lvl:ERR");
	fail_unless(nslog_uncork() == NSLOG_NO_ERROR,
		    "Unable to uncork");
	NSLOG(test, WARN, "Hello");
	fail_unless(captured_message_count == 0,
		    "Captured message count was wrong");
	filter = nslog_filter_unref(filter);
}
END_TEST

START_TEST (test_nslog_filter_dirname)
{
	nslog_filter_t *filter;
	fail_unless(nslog_filter_dirname_new("test", &filter) == NSLOG_NO_ERROR,
		    "Unable to create level filter");
	fail_unless(nslog_filter_set_active(filter, NULL) == NSLOG_NO_ERROR,
		    "Unable to set active filter to dir:test");
	fail_unless(nslog_uncork() == NSLOG_NO_ERROR,
		    "Unable to uncork");
	NSLOG(test, WARN, "Hello");
	fail_unless(captured_message_count == 1,
		    "Captured message count was wrong");
	fail_unless(captured_render_context == anchor_context_3,
		    "Captured context wasn't passed through");
	fail_unless(captured_rendered_message_length == 5,
		    "Captured message wasn't correct length");
	fail_unless(strcmp(captured_rendered_message, "Hello") == 0,
		    "Mesage wasn't as expected");
	filter = nslog_filter_unref(filter);
}
END_TEST

START_TEST (test_nslog_filter_out_dirname)
{
	nslog_filter_t *filter;
	fail_unless(nslog_filter_dirname_new("src", &filter) == NSLOG_NO_ERROR,
		    "Unable to create filename filter");
	fail_unless(nslog_filter_set_active(filter, NULL) == NSLOG_NO_ERROR,
		    "Unable to set active filter to dir:src");
	fail_unless(nslog_uncork() == NSLOG_NO_ERROR,
		    "Unable to uncork");
	NSLOG(test, WARN, "Hello");
	fail_unless(captured_message_count == 0,
		    "Captured message count was wrong");
	filter = nslog_filter_unref(filter);
}
END_TEST

START_TEST (test_nslog_filter_funcname)
{
	nslog_filter_t *filter;
	fail_unless(nslog_filter_funcname_new(__func__, &filter) == NSLOG_NO_ERROR,
		    "Unable to create level filter");
	fail_unless(nslog_filter_set_active(filter, NULL) == NSLOG_NO_ERROR,
		    "Unable to set active filter to dir:test");
	fail_unless(nslog_uncork() == NSLOG_NO_ERROR,
		    "Unable to uncork");
	NSLOG(test, WARN, "Hello");
	fail_unless(captured_message_count == 1,
		    "Captured message count was wrong");
	fail_unless(captured_render_context == anchor_context_3,
		    "Captured context wasn't passed through");
	fail_unless(captured_rendered_message_length == 5,
		    "Captured message wasn't correct length");
	fail_unless(strcmp(captured_rendered_message, "Hello") == 0,
		    "Mesage wasn't as expected");
	filter = nslog_filter_unref(filter);
}
END_TEST

START_TEST (test_nslog_filter_out_funcname)
{
	nslog_filter_t *filter;
	fail_unless(nslog_filter_funcname_new("test_nslog_filter_funcname", &filter) == NSLOG_NO_ERROR,
		    "Unable to create filename filter");
	fail_unless(nslog_filter_set_active(filter, NULL) == NSLOG_NO_ERROR,
		    "Unable to set active filter to dir:src");
	fail_unless(nslog_uncork() == NSLOG_NO_ERROR,
		    "Unable to uncork");
	NSLOG(test, WARN, "Hello");
	fail_unless(captured_message_count == 0,
		    "Captured message count was wrong");
	filter = nslog_filter_unref(filter);
}
END_TEST

START_TEST (test_nslog_complex_filter1)
{
	nslog_filter_t *filter;
	fail_unless(nslog_filter_from_text("(lvl:WARN || (lvl:DEBUG && cat:test/sub))", &filter) == NSLOG_NO_ERROR,
		    "Unable to create filename filter");
	fail_unless(nslog_filter_set_active(filter, NULL) == NSLOG_NO_ERROR,
		    "Unable to set active filter to dir:src");
	fail_unless(nslog_uncork() == NSLOG_NO_ERROR,
		    "Unable to uncork");
	NSLOG(sub, WARN, "Hello");
	fail_unless(captured_message_count == 1,
		    "Captured message count was wrong (1)");
	NSLOG(test, DEBUG, "Hello");
	fail_unless(captured_message_count == 1,
		    "Captured message count was wrong (2)");
	NSLOG(sub, DEBUG, "Hello");
	fail_unless(captured_message_count == 2,
		    "Captured message count was wrong (3)");
	filter = nslog_filter_unref(filter);
}
END_TEST

START_TEST (test_nslog_complex_filter2)
{
	nslog_filter_t *filter;
	fail_unless(nslog_filter_from_text("!(lvl:WARN ^ cat:test/sub)", &filter) == NSLOG_NO_ERROR,
		    "Unable to create filename filter");
	fail_unless(nslog_filter_set_active(filter, NULL) == NSLOG_NO_ERROR,
		    "Unable to set active filter to dir:src");
	fail_unless(nslog_uncork() == NSLOG_NO_ERROR,
		    "Unable to uncork");
	NSLOG(sub, WARN, "Hello");
	fail_unless(captured_message_count == 1,
		    "Captured message count was wrong (1)");
	NSLOG(test, WARN, "Hello");
	fail_unless(captured_message_count == 1,
		    "Captured message count was wrong (2)");
	NSLOG(test, DEBUG, "Hello");
	fail_unless(captured_message_count == 2,
		    "Captured message count was wrong (3)");
	filter = nslog_filter_unref(filter);
}
END_TEST

/**** And the suites are set up here ****/

void
nslog_basic_suite(SRunner *sr)
{
        Suite *s = suite_create("libnslog: Basic tests");
        TCase *tc_basic = NULL;

	tc_basic = tcase_create("Simple log checks, no filters");

        tcase_add_checked_fixture(tc_basic, with_simple_context_setup,
                                  with_simple_context_teardown);
        tcase_add_test(tc_basic, test_nslog_trivial_corked_message);
        tcase_add_test(tc_basic, test_nslog_trivial_uncorked_message);
	tcase_add_test(tc_basic, test_nslog_subcategory_name);
	tcase_add_test(tc_basic, test_nslog_two_corked_messages);
	tcase_add_test(tc_basic, test_nslog_check_bad_level);
        suite_add_tcase(s, tc_basic);

        tc_basic = tcase_create("Simple filter checks");

        tcase_add_checked_fixture(tc_basic, with_simple_filter_context_setup,
                                  with_simple_filter_context_teardown);
        tcase_add_test(tc_basic, test_nslog_simple_filter_corked_message);
        tcase_add_test(tc_basic, test_nslog_simple_filter_uncorked_message);
        tcase_add_test(tc_basic, test_nslog_simple_filter_subcategory_message);
        tcase_add_test(tc_basic, test_nslog_simple_filter_out_subcategory_message);
        tcase_add_test(tc_basic, test_nslog_basic_filter_sprintf);
        tcase_add_test(tc_basic, test_nslog_parse_and_sprintf);
        tcase_add_test(tc_basic, test_nslog_parse_and_sprintf_all_levels);
	tcase_add_test(tc_basic, test_nslog_parse_and_sprintf_all_kinds);
        suite_add_tcase(s, tc_basic);

	tc_basic = tcase_create("Trivial, varied, filter checks");
	tcase_add_checked_fixture(tc_basic, with_trivial_filter_context_setup,
				  with_trivial_filter_context_teardown);
	tcase_add_test(tc_basic, test_nslog_filter_filename);
	tcase_add_test(tc_basic, test_nslog_filter_full_filename);
	tcase_add_test(tc_basic, test_nslog_filter_out_filename);
	tcase_add_test(tc_basic, test_nslog_filter_level);
	tcase_add_test(tc_basic, test_nslog_filter_out_level);
	tcase_add_test(tc_basic, test_nslog_filter_dirname);
	tcase_add_test(tc_basic, test_nslog_filter_out_dirname);
	tcase_add_test(tc_basic, test_nslog_filter_funcname);
	tcase_add_test(tc_basic, test_nslog_filter_out_funcname);
	tcase_add_test(tc_basic, test_nslog_complex_filter1);
	tcase_add_test(tc_basic, test_nslog_complex_filter2);
	suite_add_tcase(s, tc_basic);

        srunner_add_suite(sr, s);
}
