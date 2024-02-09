/*
 * Copyright 2016 Michael Drake <tlsa@netsurf-browser.org>
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
 * Test time operations.
 */

#include <stdlib.h>
#include <check.h>

#include "utils/errors.h"
#include "utils/time.h"

#define NELEMS(x)  (sizeof(x) / sizeof((x)[0]))

struct test_string_pair {
	const char* test;
	const char* expected;
};

struct test_bad_string {
	const char* test;
	nserror res;
};

static const struct test_string_pair date_string_tests[] = {
	{
		.test     = "Thu, 01 Jan 1970 00:00:00 GMT",
		.expected = "Thu, 01 Jan 1970 00:00:00 GMT"
	},
	{
		.test     = "Thursday, 01 Jan 1970 00:00:00 GMT",
		.expected = "Thu, 01 Jan 1970 00:00:00 GMT"
	},
	{
		.test     = "Tue, 16 Feb 1999 19:45:12 GMT",
		.expected = "Tue, 16 Feb 1999 19:45:12 GMT"
	},
	{
		.test     = "Sunday, 16 Mar 1980 19:45:12 GMT",
		.expected = "Sun, 16 Mar 1980 19:45:12 GMT"
	},
	{
		.test     = "Sun, 16 Mar 1980 19:45:12 GMT",
		.expected = "Sun, 16 Mar 1980 19:45:12 GMT"
	},
	{
		.test     = "Tue, 16 Apr 2013 19:45:12 GMT",
		.expected = "Tue, 16 Apr 2013 19:45:12 GMT"
	},
	{
		.test     = "Tue, 16 May 2000 19:45:12 GMT",
		.expected = "Tue, 16 May 2000 19:45:12 GMT"
	},
	{
		.test     = "Tuesday, 12 Jun 2001 12:12:12 GMT",
		.expected = "Tue, 12 Jun 2001 12:12:12 GMT"
	},
	{
		.test     = "Tue, 12 Jun 2001 12:12:12 GMT",
		.expected = "Tue, 12 Jun 2001 12:12:12 GMT"
	},
	{
		.test     = "Thu, 16 Jul 2207 12:45:12 GMT",
		.expected = "Thu, 16 Jul 2207 12:45:12 GMT"
	},
	{
		.test     = "Thu, 16 Aug 2007 19:45:12 GMT",
		.expected = "Thu, 16 Aug 2007 19:45:12 GMT"
	},
	{
		.test     = "Tue, 16 Sep 3456 00:45:12 GMT",
		.expected = "Tue, 16 Sep 3456 00:45:12 GMT"
	},
	{
		.test     = "Sun, 16 Oct 1988 19:45:59 GMT",
		.expected = "Sun, 16 Oct 1988 19:45:59 GMT"
	},
	{
		.test     = "Tue, 16 Nov 1971 19:59:12 GMT",
		.expected = "Tue, 16 Nov 1971 19:59:12 GMT"
	},
	{
		.test     = "Friday, 16 Dec 1977 23:45:12 GMT",
		.expected = "Fri, 16 Dec 1977 23:45:12 GMT"
	},
	{
		.test     = "Fri, 16 Dec 1977 23:45:12 GMT",
		.expected = "Fri, 16 Dec 1977 23:45:12 GMT"
	},
	{
		.test     = "     16 Dec 1977 23:45:12 GMT",
		.expected = "Fri, 16 Dec 1977 23:45:12 GMT"
	},
	{
		.test     = "     16 Dec 1977 23:45    GMT",
		.expected = "Fri, 16 Dec 1977 23:45:00 GMT"
	},
	{
		.test     = "23:59 16 Dec 1977         GMT",
		.expected = "Fri, 16 Dec 1977 23:59:00 GMT"
	},
	{
		.test     = "23:59 16 Dec 1977         UTC",
		.expected = "Fri, 16 Dec 1977 23:59:00 GMT"
	},
	{
		.test     = "1977 GMT 23:59 16 Dec",
		.expected = "Fri, 16 Dec 1977 23:59:00 GMT"
	},
	{
		.test     = "1977 Dec GMT 16",
		.expected = "Fri, 16 Dec 1977 00:00:00 GMT"
	},
	{
		.test     = "1977 Dec 12",
		.expected = "Mon, 12 Dec 1977 00:00:00 GMT"
	},
	{
		.test     = "1977 12 Dec",
		.expected = "Mon, 12 Dec 1977 00:00:00 GMT"
	},
	{
		.test     = "Dec 1977 12",
		.expected = "Mon, 12 Dec 1977 00:00:00 GMT"
	},
	{
		.test     = "12 Dec 1977",
		.expected = "Mon, 12 Dec 1977 00:00:00 GMT"
	},
	{
		.test     = "12 Dec 77",
		.expected = "Mon, 12 Dec 1977 00:00:00 GMT"
	},
	{
		.test     = "12 77 Dec",
		.expected = "Mon, 12 Dec 1977 00:00:00 GMT"
	},
	{
		.test     = "77 12 Dec",
		.expected = "Mon, 12 Dec 1977 00:00:00 GMT"
	},
	{
		.test     = "12 12 Dec",
		.expected = "Wed, 12 Dec 2012 00:00:00 GMT"
	},
	{
		.test     = "5 12 Dec",
		.expected = "Wed, 05 Dec 2012 00:00:00 GMT"
	},
	{
		.test     = "12 5 Dec",
		.expected = "Mon, 12 Dec 2005 00:00:00 GMT"
	},
	{
		.test     = "12/5/Dec",
		.expected = "Mon, 12 Dec 2005 00:00:00 GMT"
	},
	{
		.test     = "Dec-12/2005/",
		.expected = "Mon, 12 Dec 2005 00:00:00 GMT"
	},
	{
		.test     = "12-5-Dec",
		.expected = "Mon, 12 Dec 2005 00:00:00 GMT"
	},
	{
		.test     = "2005-12-Dec",
		.expected = "Mon, 12 Dec 2005 00:00:00 GMT"
	},
	{
		.test     = "2005-Dec-12",
		.expected = "Mon, 12 Dec 2005 00:00:00 GMT"
	},
	{
		.test     = "2005-dec-12",
		.expected = "Mon, 12 Dec 2005 00:00:00 GMT"
	},
	{
		.test     = "2005-dEC-12",
		.expected = "Mon, 12 Dec 2005 00:00:00 GMT"
	},
	{
		.test     = "20051212",
		.expected = "Mon, 12 Dec 2005 00:00:00 GMT"
	},
	{
		.test     = "20051212 GMT",
		.expected = "Mon, 12 Dec 2005 00:00:00 GMT"
	},
	{
		.test     = "20051212 +0000",
		.expected = "Mon, 12 Dec 2005 00:00:00 GMT"
	},
	{
		.test     = "20051212 UTC",
		.expected = "Mon, 12 Dec 2005 00:00:00 GMT"
	},
	{
		.test     = "20051212     \n",
		.expected = "Mon, 12 Dec 2005 00:00:00 GMT"
	},
	{
		.test     = "20051212 00:00 UTC",
		.expected = "Mon, 12 Dec 2005 00:00:00 GMT"
	},
	{
		.test     = "00:00 20051212 UTC",
		.expected = "Mon, 12 Dec 2005 00:00:00 GMT"
	},
	{
		.test     = "00:00:59 20051212 UTC",
		.expected = "Mon, 12 Dec 2005 00:00:59 GMT"
	},
	{
		.test     = "00:00:60 20051212 UTC", /* leap second */
		.expected = "Mon, 12 Dec 2005 00:01:00 GMT"
	},
	{
		.test     = "Thu, 11 Aug 2016 08:47:30 GMT",
		.expected = "Thu, 11 Aug 2016 08:47:30 GMT"
	},
	{
		.test     = "Thu, 11 Aug 2016 08:47:30 GMT garbage",
		.expected = "Thu, 11 Aug 2016 08:47:30 GMT"
	},
	{
		.test     = "Thu, 11 Aug 2016 08:47:30 UTC",
		.expected = "Thu, 11 Aug 2016 08:47:30 GMT"
	},
	{
		.test     = "Thu, 11 Aug 2016 08:47:30 +0000",
		.expected = "Thu, 11 Aug 2016 08:47:30 GMT"
	},
	{
		.test     = "Thu, 11 Aug 2016 08:47:30 -0000",
		.expected = "Thu, 11 Aug 2016 08:47:30 GMT"
	},
	{
		.test     = "Thu, 11 Aug 2016 08:47:30 +0001",
		.expected = "Thu, 11 Aug 2016 08:46:30 GMT"
	},
	{
		.test     = "Thu, 11 Aug 2016 08:47:30 -0001",
		.expected = "Thu, 11 Aug 2016 08:48:30 GMT"
	},
	{
		.test     = "Thu, 11 Aug 2016 08:47:30 +0030",
		.expected = "Thu, 11 Aug 2016 08:17:30 GMT"
	},
	{
		.test     = "Thu, 11 Aug 2016 08:47:30 -0030",
		.expected = "Thu, 11 Aug 2016 09:17:30 GMT"
	},
	{
		.test     = "Thu, 11 Aug 2016 08:47:30 +0059",
		.expected = "Thu, 11 Aug 2016 07:48:30 GMT"
	},
	{
		.test     = "Thu, 11 Aug 2016 08:47:30 -0059",
		.expected = "Thu, 11 Aug 2016 09:46:30 GMT"
	},
	{
		.test     = "Thu, 11 Aug 2016 08:47:30 +0100",
		.expected = "Thu, 11 Aug 2016 07:47:30 GMT"
	},
	{
		.test     = "Thu, 11 Aug 2016 08:47:30 -0100",
		.expected = "Thu, 11 Aug 2016 09:47:30 GMT"
	},
	{
		.test     = "Thu, 11 Aug 2016 08:47:30 +1200",
		.expected = "Wed, 10 Aug 2016 20:47:30 GMT"
	},
	{
		.test     = "Thu, 11 Aug 2016 08:47:30 -1200",
		.expected = "Thu, 11 Aug 2016 20:47:30 GMT"
	},
	{
		.test     = "Thu, 11 Aug 2016 08:47:30 +0060",
		.expected = "Thu, 11 Aug 2016 07:47:30 GMT"
	},
	{
		.test     = "Thu, 11 Aug 2016 08:47:30 -0060",
		.expected = "Thu, 11 Aug 2016 09:47:30 GMT"
	},
	{
		.test     = "Thu, 11 Aug 2016 08:47:30 +0070",
		.expected = "Thu, 11 Aug 2016 07:37:30 GMT"
	},
	{
		.test     = "Thu, 11 Aug 2016 08:47:30 -0070",
		.expected = "Thu, 11 Aug 2016 09:57:30 GMT"
	},
	{
		.test     = "Thu, 11 Aug 2016 08:47:30 BST",
		.expected = "Thu, 11 Aug 2016 07:47:30 GMT"
	},
	{
		.test     = "14-Aug-2015 11:58:16 GMT",
		.expected = "Fri, 14 Aug 2015 11:58:16 GMT"
	},
};


static const struct test_bad_string date_bad_string_tests[] = {
	{
		.test = NULL,
		.res  = NSERROR_BAD_PARAMETER
	},
	{
		.test = "",
		.res  = NSERROR_INVALID
	},
	{
		.test = "Th",
		.res  = NSERROR_INVALID
	},
	{
		.test = "5",
		.res  = NSERROR_INVALID
	},
	{
		.test = "5",
		.res  = NSERROR_INVALID
	},
	{
		.test = "dsflihs9l84toswuhfsif74f",
		.res  = NSERROR_INVALID
	},
	{
		.test = "Foosday, 16 Dec 1977 23:45:12 GMT",
		.res  = NSERROR_INVALID
	},
	{
		.test = "20051212 garbage",
		.res  = NSERROR_INVALID
	},
};

/**
 * Date string comparason test
 */
START_TEST(date_string_compare)
{
	nserror res;
	time_t time_out;
	const struct test_string_pair *t = &date_string_tests[_i];

	res = nsc_strntimet(t->test, strlen(t->test), &time_out);
	ck_assert(res == NSERROR_OK);
	ck_assert_str_eq(rfc1123_date(time_out), t->expected);
}
END_TEST

/**
 * Date string conversion bad data test
 */
START_TEST(date_bad_string)
{
	nserror res;
	time_t time_out;
	const struct test_bad_string *t = &date_bad_string_tests[_i];

	res = nsc_strntimet(t->test,
			t->test != NULL ? strlen(t->test) : 0,
			&time_out);
	ck_assert(res != NSERROR_OK);
	ck_assert(res == t->res);
}
END_TEST


/* suite generation */
static Suite *time_suite(void)
{
	Suite *s;
	TCase *tc_date_string_compare;
	TCase *tc_date_bad_string;

	s = suite_create("time");

	/* date parsing: string comparason */
	tc_date_string_compare = tcase_create(
			"date string to time_t");

	/* date parsing: bad string handling */
	tc_date_bad_string = tcase_create(
			"date string to time_t (bad input)");

	tcase_add_loop_test(tc_date_string_compare,
			    date_string_compare,
			    0, NELEMS(date_string_tests));
	suite_add_tcase(s, tc_date_string_compare);

	tcase_add_loop_test(tc_date_bad_string,
			    date_bad_string,
			    0, NELEMS(date_bad_string_tests));
	suite_add_tcase(s, tc_date_bad_string);

	return s;
}

int main(int argc, char **argv)
{
	int number_failed;
	Suite *s;
	SRunner *sr;

	s = time_suite();

	sr = srunner_create(s);
	srunner_run_all(sr, CK_ENV);

	number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);

	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
