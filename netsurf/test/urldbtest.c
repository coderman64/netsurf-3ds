/*
 * Copyright 2015 Vincent Sanders <vince@netsurf-browser.org>
 * Copyright 2011 John Mark Bell <jmb@netsurf-browser.org>
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
 * Test nsurl operations.
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <check.h>

#include <libwapcaplet/libwapcaplet.h>

#include "utils/corestrings.h"
#include "utils/log.h"
#include "utils/corestrings.h"
#include "utils/nsurl.h"
#include "utils/nsoption.h"
#include "netsurf/url_db.h"
#include "netsurf/cookie_db.h"
#include "netsurf/bitmap.h"
#include "content/urldb.h"
#include "desktop/gui_internal.h"
#include "desktop/cookie_manager.h"

/**
 * url database used as input to test sets
 */
const char *test_urldb_path = "test/data/urldb";
/**
 * url database used as output reference
 */
const char *test_urldb_out_path = "test/data/urldb-out";

/**
 * cookie database used as input
 */
const char *test_cookies_path = "test/data/cookies";
/**
 * cookie database used as output reference
 */
const char *test_cookies_out_path = "test/data/cookies-out";

const char *wikipedia_url = "http://www.wikipedia.org/";

struct netsurf_table *guit = NULL;


struct test_urls {
	const char* url;
	const char* title;
	const content_type type;
	const bool persistent;
};


#define NELEMS(x)  (sizeof(x) / sizeof((x)[0]))


/* Stubs */
nserror nslog_set_filter_by_options() { return NSERROR_OK; }

/**
 * generate test output filenames
 */
static char *testnam(char *out)
{
	static int count = 0;
	static char name[64];
	snprintf(name, 64, "/tmp/urldbtest%d-%d", getpid(), count);
	count++;
	return name;
}

/**
 * compare two files contents
 */
static int cmp(const char *f1, const char *f2)
{
	int res = 0;
	FILE *fp1;
	FILE *fp2;
	int ch1;
	int ch2;

	fp1 = fopen(f1, "r");
	if (fp1 == NULL) {
		return -1;
	}
	fp2 = fopen(f2, "r");
	if (fp2 == NULL) {
		fclose(fp1);
		return -1;
	}

	while (res == 0) {
		ch1 = fgetc(fp1);
		ch2 = fgetc(fp2);

		if (ch1 != ch2) {
			res = 1;
		}

		if (ch1 == EOF) {
			break;
		}
	}

	fclose(fp1);
	fclose(fp2);
	return res;
}

/*************** original test helpers ************/

bool cookie_manager_add(const struct cookie_data *data)
{
	return true;
}

void cookie_manager_remove(const struct cookie_data *data)
{
}

static nsurl *make_url(const char *url)
{
	nsurl *nsurl;
	if (nsurl_create(url, &nsurl) != NSERROR_OK) {
		NSLOG(netsurf, INFO, "failed creating nsurl");
		exit(1);
	}
	return nsurl;
}


static bool test_urldb_set_cookie(const char *header, const char *url,
		const char *referer)
{
	nsurl *r = NULL;
	nsurl *nsurl = make_url(url);
	bool ret;

	if (referer != NULL)
		r = make_url(referer);

	ret = urldb_set_cookie(header, nsurl, r);

	if (referer != NULL)
		nsurl_unref(r);
	nsurl_unref(nsurl);

	return ret;
}

static char *test_urldb_get_cookie(const char *url)
{
	nsurl *nsurl = make_url(url);
	char *ret;

	ret = urldb_get_cookie(nsurl, true);
	nsurl_unref(nsurl);

	return ret;
}


/*************************************************/

/* mock table callbacks */
static void destroy_bitmap(void *b)
{
}

struct gui_bitmap_table tst_bitmap_table = {
	.destroy = destroy_bitmap,
};

struct netsurf_table tst_table = {
	.bitmap = &tst_bitmap_table,
};

/** urldb create fixture */
static void urldb_create(void)
{
	nserror res;

	/* mock bitmap interface */
	guit = &tst_table;

	res = corestrings_init();
	ck_assert_int_eq(res, NSERROR_OK);
}

/** urldb create pre-loaded db fixture */
static void urldb_create_loaded(void)
{
	nserror res;

	/* mock bitmap interface */
	guit = &tst_table;

	res = corestrings_init();
	ck_assert_int_eq(res, NSERROR_OK);

	res = urldb_load(test_urldb_path);
	ck_assert_int_eq(res, NSERROR_OK);

	urldb_load_cookies(test_cookies_path);
}

static void urldb_lwc_iterator(lwc_string *str, void *pw)
{
	int *scount = pw;

	NSLOG(netsurf, INFO, "[%3u] %.*s", str->refcnt,
	      (int)lwc_string_length(str), lwc_string_data(str));
	(*scount)++;
}


/** urldb teardown fixture with destroy */
static void urldb_teardown(void)
{
	int scount = 0;

	urldb_destroy();

	corestrings_fini();

	NSLOG(netsurf, INFO, "Remaining lwc strings:");
	lwc_iterate_strings(urldb_lwc_iterator, &scount);
	ck_assert_int_eq(scount, 0);
}





START_TEST(urldb_original_test)
{
	nsurl *url;
	nsurl *urlr;

	/* fragments */
	url = make_url("http://netsurf.strcprstskrzkrk.co.uk/path/to/resource.htm?a=b");
	ck_assert(urldb_add_url(url) == true);
	nsurl_unref(url);

	url = make_url("http://netsurf.strcprstskrzkrk.co.uk/path/to/resource.htm#zz?a=b");
	ck_assert(urldb_add_url(url) == true);
	nsurl_unref(url);

	url = make_url("http://netsurf.strcprstskrzkrk.co.uk/path/to/resource.htm#aa?a=b");
	ck_assert(urldb_add_url(url) == true);
	nsurl_unref(url);

	url = make_url("http://netsurf.strcprstskrzkrk.co.uk/path/to/resource.htm#yy?a=b");
	ck_assert(urldb_add_url(url) == true);
	nsurl_unref(url);


	/* set cookies on urls */
	url = make_url("http://www.minimarcos.org.uk/cgi-bin/forum/Blah.pl?,v=login,p=2");
	urldb_set_cookie("mmblah=foo; path=/; expires=Thur, 31-Dec-2099 00:00:00 GMT\r\n", url, NULL);
	nsurl_unref(url);

	url = make_url("http://www.minimarcos.org.uk/cgi-bin/forum/Blah.pl?,v=login,p=2");
	urldb_set_cookie("BlahPW=bar; path=/; expires=Thur, 31-Dec-2099 00:00:00 GMT\r\n", url, NULL);
	nsurl_unref(url);

	url = make_url("http://ccdb.cropcircleresearch.com/");
	urldb_set_cookie("details=foo|bar|Sun, 03-Jun-2007;expires=Mon, 24-Jul-2006 09:53:45 GMT\r\n", url, NULL);
	nsurl_unref(url);

	url = make_url("http://www.google.com/");
	urldb_set_cookie("PREF=ID=a:TM=b:LM=c:S=d; path=/; domain=.google.com\r\n", url, NULL);
	nsurl_unref(url);

	url = make_url("http://www.bbc.co.uk/");
	urldb_set_cookie("test=foo, bar, baz; path=/, quux=blah; path=/", url, NULL);
	nsurl_unref(url);

//	urldb_set_cookie("a=b; path=/; domain=.a.com", "http://a.com/", NULL);

	url = make_url("https://www.foo.com/blah/moose");
	urlr = make_url("https://www.foo.com/blah/moose");
	urldb_set_cookie("foo=bar;Path=/blah;Secure\r\n", url, urlr);
	nsurl_unref(url);
	nsurl_unref(urlr);

	url = make_url("https://www.foo.com/blah/wxyzabc");
	urldb_get_cookie(url, true);
	nsurl_unref(url);


	/* Valid path */
	ck_assert(test_urldb_set_cookie("name=value;Path=/\r\n", "http://www.google.com/", NULL));

	/* Valid path (non-root directory) */
	ck_assert(test_urldb_set_cookie("name=value;Path=/foo/bar/\r\n", "http://www.example.org/foo/bar/", NULL));

	/* Defaulted path */
	ck_assert(test_urldb_set_cookie("name=value\r\n", "http://www.example.org/foo/bar/baz/bat.html", NULL));
	ck_assert(test_urldb_get_cookie("http://www.example.org/foo/bar/baz/quux.htm") != NULL);

	/* Defaulted path with no non-leaf path segments */
	ck_assert(test_urldb_set_cookie("name=value\r\n", "http://no-non-leaf.example.org/index.html", NULL));
	ck_assert(test_urldb_get_cookie("http://no-non-leaf.example.org/page2.html") != NULL);
	ck_assert(test_urldb_get_cookie("http://no-non-leaf.example.org/") != NULL);

	/* Valid path (includes leafname) */
	ck_assert(test_urldb_set_cookie("name=value;Version=1;Path=/index.cgi\r\n", "http://example.org/index.cgi", NULL));
	ck_assert(test_urldb_get_cookie("http://example.org/index.cgi") != NULL);

	/* Valid path (includes leafname in non-root directory) */
	ck_assert(test_urldb_set_cookie("name=value;Path=/foo/index.html\r\n", "http://www.example.org/foo/index.html", NULL));
	/* Should _not_ match the above, as the leafnames differ */
	ck_assert(test_urldb_get_cookie("http://www.example.org/foo/bar.html") == NULL);

	/* Invalid path (contains different leafname) */
	ck_assert(test_urldb_set_cookie("name=value;Path=/index.html\r\n", "http://example.org/index.htm", NULL) == false);

	/* Invalid path (contains leafname in different directory) */
	ck_assert(test_urldb_set_cookie("name=value;Path=/foo/index.html\r\n", "http://www.example.org/bar/index.html", NULL) == false);

	/* Test partial domain match with IP address failing */
	ck_assert(test_urldb_set_cookie("name=value;Domain=.foo.org\r\n", "http://192.168.0.1/", NULL) == false);

	/* Test handling of non-domain cookie sent by server (domain part should
	 * be ignored) */
	ck_assert(test_urldb_set_cookie("foo=value;Domain=blah.com\r\n", "http://www.example.com/", NULL));
	ck_assert(strcmp(test_urldb_get_cookie("http://www.example.com/"), "foo=value") == 0);

	/* Test handling of domain cookie from wrong host (strictly invalid but
	 * required to support the real world) */
	ck_assert(test_urldb_set_cookie("name=value;Domain=.example.com\r\n", "http://foo.bar.example.com/", NULL));
	ck_assert(strcmp(test_urldb_get_cookie("http://www.example.com/"), "foo=value; name=value") == 0);

	/* Test presence of separators in cookie value */
	ck_assert(test_urldb_set_cookie("name=\"value=foo\\\\bar\\\\\\\";\\\\baz=quux\";Version=1\r\n", "http://www.example.org/", NULL));
	ck_assert(strcmp(test_urldb_get_cookie("http://www.example.org/"), "$Version=1; name=\"value=foo\\\\bar\\\\\\\";\\\\baz=quux\"") == 0);

	/* Test cookie with blank value */
	ck_assert(test_urldb_set_cookie("a=\r\n", "http://www.example.net/", NULL));
	ck_assert(strcmp(test_urldb_get_cookie("http://www.example.net/"), "a=") == 0);

	/* Test specification of multiple cookies in one header */
	ck_assert(test_urldb_set_cookie("a=b, foo=bar; Path=/\r\n", "http://www.example.net/", NULL));
	ck_assert(strcmp(test_urldb_get_cookie("http://www.example.net/"), "a=b; foo=bar") == 0);

	/* Test use of separators in unquoted cookie value */
	ck_assert(test_urldb_set_cookie("foo=moo@foo:blah?moar\\ text\r\n", "http://example.com/", NULL));
	ck_assert(strcmp(test_urldb_get_cookie("http://example.com/"), "foo=moo@foo:blah?moar\\ text; name=value") == 0);

	/* Test use of unnecessary quotes */
	ck_assert(test_urldb_set_cookie("foo=\"hello\";Version=1,bar=bat\r\n", "http://example.com/", NULL));
	ck_assert(strcmp(test_urldb_get_cookie("http://example.com/"), "foo=\"hello\"; bar=bat; name=value") == 0);

	/* Test domain matching in unverifiable transactions */
	ck_assert(test_urldb_set_cookie("foo=bar; domain=.example.tld\r\n", "http://www.foo.example.tld/", "http://bar.example.tld/"));
	ck_assert(strcmp(test_urldb_get_cookie("http://www.foo.example.tld/"), "foo=bar") == 0);

	/* Test expiry */
	ck_assert(test_urldb_set_cookie("foo=bar", "http://expires.com/", NULL));
	ck_assert(strcmp(test_urldb_get_cookie("http://expires.com/"), "foo=bar") == 0);
	ck_assert(test_urldb_set_cookie("foo=bar; expires=Thu, 01-Jan-1970 00:00:01 GMT\r\n", "http://expires.com/", NULL));
	ck_assert(test_urldb_get_cookie("http://expires.com/") == NULL);

	urldb_dump();
}
END_TEST

/**
 * test case comprised of tests historicaly found in netsurf
 *
 * These tests are carried forward from original open coded tests
 * found in the url database code.
 */
static TCase *urldb_original_case_create(void)
{
	TCase *tc;
	tc = tcase_create("Original_tests");

	/* ensure corestrings are initialised and finalised for every test */
	tcase_add_checked_fixture(tc,
				  urldb_create,
				  urldb_teardown);

	tcase_add_test(tc, urldb_original_test);

	return tc;
}


/**
 * add set and get tests
 */
static const struct test_urls add_set_get_tests[] = {
	{
		"http://intranet/",
		"foo",
		CONTENT_HTML,
		false
	}, /* from legacy tests */
	{
		"http:moodle.org",
		"buggy",
		CONTENT_HTML,
		false
	}, /* Mantis bug #993 */
	{
		"http://a_a/",
		"buggsy",
		CONTENT_HTML,
		false
	}, /* Mantis bug #993 */
	{
		"http://www2.2checkout.com/",
		"foobar",
		CONTENT_HTML,
		false
	}, /* Mantis bug #913 */
	{
		"http://2.bp.blogspot.com/_448y6kVhntg/TSekubcLJ7I/AAAAAAAAHJE/yZTsV5xT5t4/s1600/covers.jpg",
		"a more complex title",
		CONTENT_IMAGE,
		true
	}, /* Numeric subdomains */
	{
		"http://tree.example.com/this_url_has_a_ridiculously_long_path/made_up_from_a_number_of_inoranately_long_elments_some_of_well_over_forty/characters_in_length/the_whole_path_comes_out_well_in_excess_of_two_hundred_characters_in_length/this_is_intended_to_try_and_drive/the_serialisation_code_mad/foo.png",
		NULL,
		CONTENT_IMAGE,
		false
	},
	{
		"https://tree.example.com:8080/example.png",
		"fishy port       ",
		CONTENT_HTML,
		false
	},
	{
		"http://tree.example.com/bar.png",
		"\t     ",
		CONTENT_IMAGE,
		false
	}, /* silly title */
	{
		"http://[2001:db8:1f70::999:de8:7648:6e8]:100/",
		"ipv6 with port",
		CONTENT_TEXTPLAIN,
		false
	},
	{
		"file:///home/",
		NULL,
		CONTENT_HTML,
		false
	}, /* no title */
	{
		"http://foo@moose.com/",
		NULL,
		CONTENT_HTML,
		false
	}, /* Mantis bug #996 */
	{
		"http://a.xn--11b4c3d/a",
		"a title",
		CONTENT_HTML,
		false
	},
	{
		"https://smog.大众汽车/test",
		"unicode title 大众汽车",
		CONTENT_HTML,
		false
	},
};


/**
 * add set and get test
 */
START_TEST(urldb_add_set_get_test)
{
	nserror err;
	nsurl *url;
	nsurl *res_url;
	const struct url_data *data;
	const struct test_urls *tst = &add_set_get_tests[_i];

	/* not testing create, this should always succeed */
	err = nsurl_create(tst->url, &url);
	ck_assert(err == NSERROR_OK);

	/* add the url to the database */
	ck_assert(urldb_add_url(url) == true);

	/* set title */
	err = urldb_set_url_title(url, tst->title);
	ck_assert(err == NSERROR_OK);

	err = urldb_set_url_content_type(url, tst->type);
	ck_assert(err == NSERROR_OK);

	/* retrieve the url from the database and check it matches */
	res_url = urldb_get_url(url);
	ck_assert(res_url != NULL);
	ck_assert(nsurl_compare(url, res_url, NSURL_COMPLETE) == true);

	/* retrieve url data and check title matches */
	data = urldb_get_url_data(url);
	ck_assert(data != NULL);

	/* ensure title matches */
	if (tst->title != NULL) {
		ck_assert_str_eq(data->title, tst->title);
	} else {
		ck_assert(data->title == NULL);
	}

	/* ensure type matches */
	ck_assert(data->type == tst->type);

	/* release test url */
	nsurl_unref(url);
}
END_TEST

/**
 * test cases that simply add and then get a url
 *
 * these tests exercise the adding and retrival of urls verifying the
 * data added.
 */
static TCase *urldb_add_get_case_create(void)
{
	TCase *tc;
	tc = tcase_create("Add Get tests");

	/* ensure corestrings are initialised and finalised for every test */
	tcase_add_checked_fixture(tc,
				  urldb_create,
				  urldb_teardown);

	tcase_add_loop_test(tc,
			    urldb_add_set_get_test,
			    0, NELEMS(add_set_get_tests));

	return tc;
}

/**
 * Session basic test case
 *
 * The databases are loaded and saved with no manipulation
 *
 * \warning This test will fail when 32bit time_t wraps in 2038 as the
 *           cookie database expiry field is limited to that size.
 */
START_TEST(urldb_session_test)
{
	nserror res;
	char *outnam;

	/* writing output requires options initialising */
	res = nsoption_init(NULL, NULL, NULL);
	ck_assert_int_eq(res, NSERROR_OK);

	res = urldb_load(test_urldb_path);
	ck_assert_int_eq(res, NSERROR_OK);

	urldb_load_cookies(test_cookies_path);

	/* write database out */
	outnam = testnam(NULL);
	res = urldb_save(outnam);
	ck_assert_int_eq(res, NSERROR_OK);

	/* check the url database file written and the test file match */
	ck_assert_int_eq(cmp(outnam, test_urldb_out_path), 0);

	/* remove test output */
	unlink(outnam);

	/* write cookies out */
	outnam = testnam(NULL);
	urldb_save_cookies(outnam);

	/* check the cookies file written and the test file match */
	ck_assert_int_eq(cmp(outnam, test_cookies_out_path), 0);

	/* remove test output */
	unlink(outnam);

	/* finalise options */
	res = nsoption_finalise(NULL, NULL);
	ck_assert_int_eq(res, NSERROR_OK);

}
END_TEST

/**
 * Session more extensive test case
 *
 * The databases are loaded and saved with a host and paths added
 */
START_TEST(urldb_session_add_test)
{
	nserror res;
	char *outnam;
	nsurl *url;
	unsigned int t;

	/* writing output requires options initialising */
	res = nsoption_init(NULL, NULL, NULL);
	ck_assert_int_eq(res, NSERROR_OK);

	res = urldb_load(test_urldb_path);
	ck_assert_int_eq(res, NSERROR_OK);

	urldb_load_cookies(test_cookies_path);

	/* add to db */
	for (t = 0; t < NELEMS(add_set_get_tests); t++) {
		const struct test_urls *tst = &add_set_get_tests[t];

		/* not testing url creation, this should always succeed */
		res = nsurl_create(tst->url, &url);
		ck_assert_int_eq(res, NSERROR_OK);

		/* add the url to the database */
		ck_assert(urldb_add_url(url) == true);

		/* set title */
		res = urldb_set_url_title(url, tst->title);
		ck_assert(res == NSERROR_OK);

		/* update the visit time so it gets serialised */
		if (tst->persistent) {
			res = urldb_set_url_persistence(url, true);
		} else {
			res = urldb_update_url_visit_data(url);
		}
		ck_assert_int_eq(res, NSERROR_OK);

		nsurl_unref(url);
	}

	/* write database out */
	outnam = testnam(NULL);
	res = urldb_save(outnam);
	ck_assert_int_eq(res, NSERROR_OK);

	/* remove urldb test output */
	unlink(outnam);

	/* write cookies out */
	outnam = testnam(NULL);
	urldb_save_cookies(outnam);

	/* remove cookies test output */
	unlink(outnam);

	/* finalise options */
	res = nsoption_finalise(NULL, NULL);
	ck_assert_int_eq(res, NSERROR_OK);

}
END_TEST

/**
 * Test case to check entire session
 *
 * These tests define a session as loading a url database and cookie
 * database and then saving them back to disc.
 */
static TCase *urldb_session_case_create(void)
{
	TCase *tc;
	tc = tcase_create("Full_session");

	/* ensure corestrings are initialised and finalised for every test */
	tcase_add_checked_fixture(tc,
				  urldb_create,
				  urldb_teardown);

	tcase_add_test(tc, urldb_session_test);
	tcase_add_test(tc, urldb_session_add_test);

	return tc;
}

static int cb_count;

static bool urldb_iterate_entries_cb(nsurl *url, const struct url_data *data)
{
	NSLOG(netsurf, INFO, "url: %s", nsurl_access(url));
	/* fprintf(stderr, "url:%s\ntitle:%s\n\n",nsurl_access(url), data->title); */
	cb_count++;
	return true;
}

START_TEST(urldb_iterate_entries_test)
{
	urldb_iterate_entries(urldb_iterate_entries_cb);
}
END_TEST

/**
 * iterate through partial matches
 */
START_TEST(urldb_iterate_partial_www_test)
{
	cb_count = 0;
	urldb_iterate_partial("www", urldb_iterate_entries_cb);
	ck_assert_int_eq(cb_count, 7);

}
END_TEST

/**
 * iterate through partial matches
 */
START_TEST(urldb_iterate_partial_nomatch_test)
{
	cb_count = 0;
	urldb_iterate_partial("/", urldb_iterate_entries_cb);
	ck_assert_int_eq(cb_count, 0);

}
END_TEST

/**
 * iterate through partial matches
 */
START_TEST(urldb_iterate_partial_add_test)
{
	nsurl *url;

	cb_count = 0;
	urldb_iterate_partial("wikipedia", urldb_iterate_entries_cb);
	ck_assert_int_eq(cb_count, 0);

	url = make_url(wikipedia_url);
	urldb_add_url(url);
	nsurl_unref(url);

	cb_count = 0;
	urldb_iterate_partial("wikipedia", urldb_iterate_entries_cb);
	ck_assert_int_eq(cb_count, 1);
}
END_TEST

/**
 * iterate through partial matches
 */
START_TEST(urldb_iterate_partial_path_test)
{

	cb_count = 0;
	urldb_iterate_partial("en.wikipedia.org/wiki", urldb_iterate_entries_cb);
	ck_assert_int_eq(cb_count, 2);
}
END_TEST

/**
 * iterate through partial matches of numeric v4 address
 */
START_TEST(urldb_iterate_partial_numeric_v4_test)
{
	nsurl *url;

	cb_count = 0;
	urldb_iterate_partial("192.168.7.1/", urldb_iterate_entries_cb);
	ck_assert_int_eq(cb_count, 0);

	url = make_url("http://192.168.7.1/index.html");
	urldb_add_url(url);
	nsurl_unref(url);

	cb_count = 0;
	urldb_iterate_partial("192.168.7.1/", urldb_iterate_entries_cb);
	ck_assert_int_eq(cb_count, 1);
}
END_TEST


/**
 * iterate through partial matches of numeric v6 address
 */
START_TEST(urldb_iterate_partial_numeric_v6_test)
{
	nsurl *url;

	cb_count = 0;
	urldb_iterate_partial("[2001:db8:1f70::999:de8:7648:6e8]",
			      urldb_iterate_entries_cb);
	ck_assert_int_eq(cb_count, 0);

	url = make_url("http://[2001:db8:1f70::999:de8:7648:6e8]/index.html");
	urldb_add_url(url);
	nsurl_unref(url);

	cb_count = 0;
	urldb_iterate_partial("[2001:db8:1f70::999:de8:7648:6e8]/index.wrong",
			      urldb_iterate_entries_cb);
	ck_assert_int_eq(cb_count, 0);

	cb_count = 0;
	urldb_iterate_partial("[2001:db8:1f70::999:de8:7648:6e8]",
			      urldb_iterate_entries_cb);
	ck_assert_int_eq(cb_count, 1);

	cb_count = 0;
	urldb_iterate_partial("[2001:db8:1f70::999:de8:7648:6e8]/in",
			      urldb_iterate_entries_cb);
	ck_assert_int_eq(cb_count, 1);

	/* double path separators are ignored */
	cb_count = 0;
	urldb_iterate_partial("[2001:db8:1f70::999:de8:7648:6e8]//index.html",
			      urldb_iterate_entries_cb);
	ck_assert_int_eq(cb_count, 1);

	/* bad ipv6 address inet_pton should fail with this */
	cb_count = 0;
	urldb_iterate_partial("[2001::1f70::999::7648:8]",
			      urldb_iterate_entries_cb);
	ck_assert_int_eq(cb_count, 0);

}
END_TEST


START_TEST(urldb_auth_details_test)
{
	nsurl *url;
	const char *res;
	const char *auth = "mooooo";

	url = make_url(wikipedia_url);
	urldb_set_auth_details(url, "tree", auth);

	res = urldb_get_auth_details(url, "tree");
	ck_assert_str_eq(res, auth);

	nsurl_unref(url);
}
END_TEST


START_TEST(urldb_cert_permissions_test)
{
	nsurl *url;
	bool permit;

	url = make_url(wikipedia_url);

	urldb_set_cert_permissions(url, true); /* permit invalid certs for url */

	permit = urldb_get_cert_permissions(url);

	ck_assert(permit == true);

	urldb_set_cert_permissions(url, false); /* do not permit invalid certs for url */

	permit = urldb_get_cert_permissions(url);

	ck_assert(permit == false);

	nsurl_unref(url);
}
END_TEST

START_TEST(urldb_update_visit_test)
{
	nsurl *url;

	url = make_url(wikipedia_url);

	urldb_update_url_visit_data(url);

	urldb_add_url(url);

	urldb_update_url_visit_data(url);
	/** \todo test needs to check results */

	nsurl_unref(url);
}
END_TEST

START_TEST(urldb_reset_visit_test)
{
	nsurl *url;

	url = make_url(wikipedia_url);

	urldb_reset_url_visit_data(url);

	urldb_add_url(url);

	urldb_reset_url_visit_data(url);
	/** \todo test needs to check results */

	nsurl_unref(url);
}
END_TEST

START_TEST(urldb_persistence_test)
{
	nsurl *url;

	url = make_url(wikipedia_url);

	urldb_set_url_persistence(url, true);

	urldb_add_url(url);

	urldb_set_url_persistence(url, true);

	urldb_set_url_persistence(url, false);
	/** \todo test needs to check results */

	nsurl_unref(url);
}
END_TEST


static TCase *urldb_case_create(void)
{
	TCase *tc;
	tc = tcase_create("General");

	/* ensure corestrings are initialised and finalised for every test */
	tcase_add_checked_fixture(tc,
				  urldb_create_loaded,
				  urldb_teardown);

	tcase_add_test(tc, urldb_iterate_entries_test);
	tcase_add_test(tc, urldb_iterate_partial_www_test);
	tcase_add_test(tc, urldb_iterate_partial_nomatch_test);
	tcase_add_test(tc, urldb_iterate_partial_add_test);
	tcase_add_test(tc, urldb_iterate_partial_path_test);
	tcase_add_test(tc, urldb_iterate_partial_numeric_v4_test);
	tcase_add_test(tc, urldb_iterate_partial_numeric_v6_test);
	tcase_add_test(tc, urldb_auth_details_test);
	tcase_add_test(tc, urldb_cert_permissions_test);
	tcase_add_test(tc, urldb_update_visit_test);
	tcase_add_test(tc, urldb_reset_visit_test);
	tcase_add_test(tc, urldb_persistence_test);

	return tc;
}


static bool urldb_iterate_cookies_cb(const struct cookie_data *data)
{
	NSLOG(netsurf, INFO, "%p", data);
	/* fprintf(stderr, "domain:%s\npath:%s\nname:%s\n\n",data->domain, data->path, data->name);*/
	return true;
}

START_TEST(urldb_iterate_cookies_test)
{
	urldb_iterate_cookies(urldb_iterate_cookies_cb);
}
END_TEST

START_TEST(urldb_cookie_create_test)
{
	/* Valid path (includes leafname) */
	const char *cookie_hdr = "name=value;Version=1;Path=/index.cgi\r\n";
	const char *cookie = "$Version=1; name=value; $Path=\"/index.cgi\"";
	char *cdata; /* cookie data */

	ck_assert(test_urldb_set_cookie(cookie_hdr, "http://example.org/index.cgi", NULL));
	cdata = test_urldb_get_cookie("http://example.org/index.cgi");
	ck_assert_str_eq(cdata, cookie);

}
END_TEST

START_TEST(urldb_cookie_delete_test)
{
	/* Valid path (includes leafname) */
	const char *cookie_hdr = "name=value;Version=1;Path=/index.cgi\r\n";
	const char *cookie = "$Version=1; name=value; $Path=\"/index.cgi\"";
	char *cdata; /* cookie data */

	ck_assert(test_urldb_set_cookie(cookie_hdr, "http://example.org/index.cgi", NULL));
	cdata = test_urldb_get_cookie("http://example.org/index.cgi");
	ck_assert_str_eq(cdata, cookie);

	urldb_delete_cookie("example.org", "/index.cgi", "name");

	cdata = test_urldb_get_cookie("http://example.org/index.cgi");
	ck_assert(cdata == NULL);

}
END_TEST

/**
 * Test case for urldb cookie management
 */
static TCase *urldb_cookie_case_create(void)
{
	TCase *tc;
	tc = tcase_create("Cookies");

	/* ensure corestrings are initialised and finalised for every test */
	tcase_add_checked_fixture(tc,
				  urldb_create_loaded,
				  urldb_teardown);

	tcase_add_test(tc, urldb_cookie_create_test);
	tcase_add_test(tc, urldb_iterate_cookies_test);
	tcase_add_test(tc, urldb_cookie_delete_test);

	return tc;
}


/**
 * Test urldb_add_url asserting on NULL.
 */
START_TEST(urldb_api_add_url_assert_test)
{
	bool res;
	res = urldb_add_url(NULL);
	ck_assert(res == true);
}
END_TEST


/**
 * Test urldb find failing for differing bad url.
 */
START_TEST(urldb_api_url_find_test)
{
	nsurl *url;
	nserror res;

	urldb_create();

	/* search for a url with mailto scheme */
	res = nsurl_create("mailto:", &url);
	ck_assert_int_eq(res, NSERROR_OK);

	res = urldb_set_url_persistence(url, true);
	ck_assert_int_eq(res, NSERROR_NOT_FOUND);

	nsurl_unref(url);

	/* search for a url with odd scheme and no host */
	res = nsurl_create("fish:///", &url);
	ck_assert_int_eq(res, NSERROR_OK);
	ck_assert(nsurl_has_component(url, NSURL_HOST) == false);

	res = urldb_set_url_title(url, NULL);
	ck_assert_int_eq(res, NSERROR_NOT_FOUND);

	nsurl_unref(url);

	/* search for a url with not found url  */
	res = nsurl_create("http://no.example.com/", &url);
	ck_assert_int_eq(res, NSERROR_OK);
	ck_assert(nsurl_has_component(url, NSURL_HOST) == true);

	res = urldb_set_url_persistence(url, true);
	ck_assert_int_eq(res, NSERROR_NOT_FOUND);

	nsurl_unref(url);
}
END_TEST

/**
 * test url database finalisation without initialisation.
 */
START_TEST(urldb_api_destroy_no_init_test)
{
	urldb_destroy();
}
END_TEST


/**
 * test case for urldb API including error returns and asserts
 */
static TCase *urldb_api_case_create(void)
{
	TCase *tc;
	tc = tcase_create("API_checks");

	tcase_add_test_raise_signal(tc,
				    urldb_api_add_url_assert_test,
				    6);

	tcase_add_test(tc, urldb_api_url_find_test);

	tcase_add_test(tc, urldb_api_destroy_no_init_test);


	return tc;
}

/**
 * Test suite for url database
 */
static Suite *urldb_suite_create(void)
{
	Suite *s;
	s = suite_create("URLDB");

	suite_add_tcase(s, urldb_api_case_create());
	suite_add_tcase(s, urldb_add_get_case_create());
	suite_add_tcase(s, urldb_session_case_create());
	suite_add_tcase(s, urldb_case_create());
	suite_add_tcase(s, urldb_cookie_case_create());
	suite_add_tcase(s, urldb_original_case_create());

	return s;
}

int main(int argc, char **argv)
{
	int number_failed;
	SRunner *sr;

	sr = srunner_create(urldb_suite_create());

	srunner_run_all(sr, CK_ENV);

	number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);

	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
