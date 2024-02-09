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
 * Tests for user option processing
 */

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <check.h>

#include "utils/errors.h"
#include "utils/log.h"
#include "utils/nsoption.h"

#ifndef TESTROOT
#define TESTROOT "/tmp"
#endif

const char *test_choices_path = "test/data/Choices";
const char *test_choices_short_path = "test/data/Choices-short";
const char *test_choices_all_path = "test/data/Choices-all";
const char *test_choices_full_path = "test/data/Choices-full";
const char *test_choices_missing_path = "test/data/Choices-missing";

/* Stubs */
nserror nslog_set_filter_by_options() { return NSERROR_OK; }

/**
 * generate test output filenames
 */
static char *testnam(char *out)
{
	static int count = 0;
	static char name[64];
	int pid;
	pid=getpid();
	snprintf(name, 64, TESTROOT"/nsoptiontest%d%d", pid, count);
	count++;
	return name;
}

static nserror gui_options_init_defaults(struct nsoption_s *defaults)
{
	/* Set defaults for absent option strings */
	nsoption_setnull_charp(ca_bundle, strdup("NetSurf:Resources.ca-bundle"));
	nsoption_setnull_charp(cookie_file, strdup("NetSurf:Cookies"));
	nsoption_setnull_charp(cookie_jar, strdup("Cookies"));

	if (nsoption_charp(ca_bundle) == NULL ||
	    nsoption_charp(cookie_file) == NULL ||
	    nsoption_charp(cookie_jar) == NULL) {
		return NSERROR_BAD_PARAMETER;
	}
	return NSERROR_OK;
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

/** option create fixture */
static void nsoption_create(void)
{
	nserror res;

	res = nsoption_init(NULL, NULL, NULL);
	ck_assert_int_eq(res, NSERROR_OK);
}

/** option create fixture for format case */
static void nsoption_format_create(void)
{
	nserror res;

	res = nsoption_init(NULL, NULL, NULL);
	ck_assert_int_eq(res, NSERROR_OK);

	/* read from file */
	res = nsoption_read(test_choices_path, NULL);
	ck_assert_int_eq(res, NSERROR_OK);
}

/** option teardown fixture */
static void nsoption_teardown(void)
{
	nserror res;

	res = nsoption_finalise(NULL, NULL);
	ck_assert_int_eq(res, NSERROR_OK);
}


/**
 * Test full options session from start to finish
 */
START_TEST(nsoption_session_test)
{
	nserror res;
	int argc = 2;
	char arg1[] = "nsoption";
	char arg2[] = "--http_proxy_host=fooo";
	char *argv[] = { arg1, arg2, NULL};
	char *outnam;

	res = nsoption_init(gui_options_init_defaults, NULL, NULL);
	ck_assert_int_eq(res, NSERROR_OK);

	/* read from file */
	res = nsoption_read(test_choices_path, NULL);
	ck_assert_int_eq(res, NSERROR_OK);

	/* overlay commandline */
	res = nsoption_commandline(&argc, &argv[0], NULL);
	ck_assert_int_eq(res, NSERROR_OK);

	/* change a string option */
	nsoption_set_charp(http_proxy_host, strdup("bar"));

	/* change an uint option */
	nsoption_set_uint(disc_cache_size, 42);

	/* change a colour */
	nsoption_set_colour(sys_colour_ActiveBorder, 0x00d0000d);

	/* write options out */
	outnam = testnam(NULL);
	res = nsoption_write(outnam, NULL, NULL);
	ck_assert_int_eq(res, NSERROR_OK);

	/* check for the correct answer */
	ck_assert_int_eq(cmp(outnam, test_choices_full_path), 0);

	/* remove test output */
	unlink(outnam);

	res = nsoption_finalise(NULL, NULL);
	ck_assert_int_eq(res, NSERROR_OK);

}
END_TEST

static TCase *nsoption_session_case_create(void)
{
	TCase *tc;
	tc = tcase_create("Full session");

	tcase_add_test(tc, nsoption_session_test);

	return tc;
}



struct format_test_vec_s {
	int opt_idx;
	const char *res_html;
	const char *res_text;
};

struct format_test_vec_s format_test_vec[] = {
	{
		NSOPTION_http_proxy,
		"<tr><th>http_proxy</th><td>boolean</td><td>default</td><td>false</td></tr>",
		"http_proxy:0"
	},
	{
		NSOPTION_enable_javascript,
		"<tr><th>enable_javascript</th><td>boolean</td><td>user</td><td>true</td></tr>",
		"enable_javascript:1"
	},
	{
		NSOPTION_http_proxy_port,
		"<tr><th>http_proxy_port</th><td>integer</td><td>default</td><td>8080</td></tr>",
		"http_proxy_port:8080"
	},
	{
		NSOPTION_http_proxy_host,
		"<tr><th>http_proxy_host</th><td>string</td><td>default</td><td><span class=\"null-content\">NULL</span></td></tr>",
		"http_proxy_host:"
	},
	{
		NSOPTION_cookie_file,
		"<tr><th>cookie_file</th><td>string</td><td>user</td><td>/home/vince/.netsurf/Cookies</td></tr>",
		"cookie_file:/home/vince/.netsurf/Cookies"
	},
	{
		NSOPTION_disc_cache_size,
		"<tr><th>disc_cache_size</th><td>unsigned integer</td><td>default</td><td>1073741824</td></tr>",
		"disc_cache_size:1073741824"
	},
	{
		NSOPTION_sys_colour_ActiveBorder,
		"<tr><th>sys_colour_ActiveBorder</th><td>colour</td><td>default</td><td><span style=\"font-family:Monospace;\">#D3D3D3</span> <span style=\"background-color: #d3d3d3; border: 1px solid #000000; display: inline-block; width: 1em; height: 1em;\"></span></td></tr>",
		"sys_colour_ActiveBorder:d3d3d3"
	},
};

/**
 * Test formatting of html output
 */
START_TEST(nsoption_format_html_test)
{
	int ret;
	char buffer[1024];
	struct format_test_vec_s *tst = &format_test_vec[_i];

	ret = nsoption_snoptionf(buffer, sizeof buffer, tst->opt_idx,
		"<tr><th>%k</th><td>%t</td><td>%p</td><td>%V</td></tr>");
	ck_assert_int_gt(ret, 0);
	ck_assert_str_eq(buffer, tst->res_html);
}
END_TEST

/**
 * Test formatting of text output
 */
START_TEST(nsoption_format_text_test)
{
	int ret;
	char buffer[1024];
	struct format_test_vec_s *tst = &format_test_vec[_i];

	ret = nsoption_snoptionf(buffer, sizeof buffer, tst->opt_idx,
				 "%k:%v");
	ck_assert_int_gt(ret, 0);
	ck_assert_str_eq(buffer, tst->res_text);
}
END_TEST

#define NELEMS(x)  (sizeof(x) / sizeof((x)[0]))

static TCase *nsoption_format_case_create(void)
{
	TCase *tc;
	tc = tcase_create("Formatted output");

	/* ensure options are initialised and finalised for every test */
	tcase_add_unchecked_fixture(tc,
				    nsoption_format_create,
				    nsoption_teardown);

	tcase_add_loop_test(tc,
			    nsoption_format_html_test,
			    0, NELEMS(format_test_vec));

	tcase_add_loop_test(tc,
			    nsoption_format_text_test,
			    0, NELEMS(format_test_vec));

	return tc;
}


/**
 * Test dumping option file
 */
START_TEST(nsoption_dump_test)
{
	nserror res;
	char *outnam;
	FILE *fp;

	res = nsoption_read(test_choices_path, NULL);
	ck_assert_int_eq(res, NSERROR_OK);

	outnam = testnam(NULL);

	fp = fopen(outnam, "w");
	res = nsoption_dump(fp, NULL);
	fclose(fp);

	ck_assert_int_eq(res, NSERROR_OK);

	ck_assert_int_eq(cmp(outnam, test_choices_all_path), 0);

	unlink(outnam);
}
END_TEST

/**
 * Test writing option file
 */
START_TEST(nsoption_write_test)
{
	nserror res;
	char *outnam;

	res = nsoption_read(test_choices_path, NULL);
	ck_assert_int_eq(res, NSERROR_OK);

	outnam = testnam(NULL);

	res = nsoption_write(outnam, NULL, NULL);
	ck_assert_int_eq(res, NSERROR_OK);

	ck_assert_int_eq(cmp(outnam, test_choices_short_path), 0);

	unlink(outnam);
}
END_TEST

/**
 * Test reading option file
 */
START_TEST(nsoption_read_test)
{
	nserror res;
	res = nsoption_read(test_choices_path, NULL);
	ck_assert_int_eq(res, NSERROR_OK);

	ck_assert(nsoption_charp(homepage_url) != NULL);
	ck_assert_str_eq(nsoption_charp(homepage_url), "about:welcome");
}
END_TEST


/**
 * Test reading missing option file
 */
START_TEST(nsoption_read_missing_test)
{
	nserror res;
	res = nsoption_read(test_choices_missing_path, NULL);
	ck_assert_int_eq(res, NSERROR_NOT_FOUND);
}
END_TEST

/**
 * Test commandline string value setting
 */
START_TEST(nsoption_commandline_test)
{
	nserror res;
	int argc = 4;
	char arg1[] = "nsoption";
	char arg2[] = "--http_proxy_host=fooo";
	char arg3[] = "--http_proxy_port";
	char arg4[] = "not-option";
	char *argv[] = { arg1, arg2, arg3, arg4, NULL};

	/* commandline */
	res = nsoption_commandline(&argc, &argv[0], NULL);
	ck_assert_int_eq(res, NSERROR_OK);

	ck_assert(nsoption_charp(http_proxy_host) != NULL);
	ck_assert_str_eq(nsoption_charp(http_proxy_host), "fooo");
}
END_TEST

static TCase *nsoption_case_create(void)
{
	TCase *tc;
	tc = tcase_create("File operations");

	/* ensure options are initialised and finalised for every test */
	tcase_add_unchecked_fixture(tc,
				    nsoption_create,
				    nsoption_teardown);

	tcase_add_test(tc, nsoption_commandline_test);
	tcase_add_test(tc, nsoption_read_test);
	tcase_add_test(tc, nsoption_read_missing_test);
	tcase_add_test(tc, nsoption_write_test);
	tcase_add_test(tc, nsoption_dump_test);

	return tc;
}


/**
 * Test finalisation without init
 */
START_TEST(nsoption_api_fini_no_init_test)
{
	nserror res;

	/* attempt to finalise without init */
	res = nsoption_finalise(NULL, NULL);
	ck_assert_int_eq(res, NSERROR_BAD_PARAMETER);
}
END_TEST

/**
 * Test read without path
 */
START_TEST(nsoption_api_read_no_path_test)
{
	nserror res;

	/* read with no path or init */
	res = nsoption_read(NULL, NULL);
	ck_assert_int_eq(res, NSERROR_BAD_PARAMETER);
}
END_TEST

/**
 * Test read without init
 */
START_TEST(nsoption_api_read_no_init_test)
{
	nserror res;

	/* read with path but no init */
	res = nsoption_read(test_choices_path, NULL);
	ck_assert_int_eq(res, NSERROR_BAD_PARAMETER);
}
END_TEST

/**
 * Test write without path
 */
START_TEST(nsoption_api_write_no_path_test)
{
	nserror res;

	/* write with no path or init */
	res = nsoption_write(NULL, NULL, NULL);
	ck_assert_int_eq(res, NSERROR_BAD_PARAMETER);
}
END_TEST

/**
 * Test write without init
 */
START_TEST(nsoption_api_write_no_init_test)
{
	nserror res;

	/* write with path but no init */
	res = nsoption_write(test_choices_path, NULL, NULL);
	ck_assert_int_eq(res, NSERROR_BAD_PARAMETER);

}
END_TEST

/**
 * Test dump without path
 */
START_TEST(nsoption_api_dump_no_path_test)
{
	nserror res;

	/* write with no path or init */
	res = nsoption_dump(NULL, NULL);
	ck_assert_int_eq(res, NSERROR_BAD_PARAMETER);
}
END_TEST

/**
 * Test dump without init
 */
START_TEST(nsoption_api_dump_no_init_test)
{
	nserror res;
	FILE *outf;

	outf = tmpfile();
	ck_assert(outf != NULL);

	/* write with path but no init */
	res = nsoption_dump(outf, NULL);
	ck_assert_int_eq(res, NSERROR_BAD_PARAMETER);

	fclose(outf);
}
END_TEST

/**
 * Test commandline without args
 */
START_TEST(nsoption_api_commandline_no_args_test)
{
	nserror res;
	int argc = 2;
	char arg1[] = "nsoption";
	char arg2[] = "--http_proxy_host=fooo";
	char *argv[] = { arg1, arg2, NULL};

	/* commandline with no argument count or init */
	res = nsoption_commandline(NULL, &argv[0], NULL);
	ck_assert_int_eq(res, NSERROR_BAD_PARAMETER);

	/* commandline with no argument vector or init */
	res = nsoption_commandline(&argc, NULL, NULL);
	ck_assert_int_eq(res, NSERROR_BAD_PARAMETER);
}
END_TEST

/**
 * Test commandline without init
 */
START_TEST(nsoption_api_commandline_no_init_test)
{
	nserror res;
	int argc = 2;
	char arg1[] = "nsoption";
	char arg2[] = "--http_proxy_host=fooo";
	char *argv[] = { arg1, arg2, NULL};

	/* write with path but no init */
	res = nsoption_commandline(&argc, &argv[0], NULL);
	ck_assert_int_eq(res, NSERROR_BAD_PARAMETER);
}
END_TEST


/**
 * Test default initialisation and repeated finalisation
 */
START_TEST(nsoption_api_fini_twice_test)
{
	nserror res;
	res = nsoption_init(NULL, NULL, NULL);
	ck_assert_int_eq(res, NSERROR_OK);

	res = nsoption_finalise(NULL, NULL);
	ck_assert_int_eq(res, NSERROR_OK);

	res = nsoption_finalise(NULL, NULL);
	ck_assert_int_eq(res, NSERROR_BAD_PARAMETER);
}
END_TEST


/**
 * Test default initialisation and finalisation
 */
START_TEST(nsoption_api_init_def_test)
{
	nserror res;
	res = nsoption_init(NULL, NULL, NULL);
	ck_assert_int_eq(res, NSERROR_OK);

	res = nsoption_finalise(NULL, NULL);
	ck_assert_int_eq(res, NSERROR_OK);
}
END_TEST

/**
 * Test default initialisation and finalisation with parameters
 */
START_TEST(nsoption_api_init_param_test)
{
	nserror res;
	res = nsoption_init(NULL, &nsoptions, &nsoptions_default);
	ck_assert_int_eq(res, NSERROR_OK);

	res = nsoption_finalise(nsoptions, nsoptions_default);
	ck_assert_int_eq(res, NSERROR_OK);
}
END_TEST

static nserror failing_init_cb(struct nsoption_s *defaults)
{
	return NSERROR_INIT_FAILED;
}

/**
 * Test default initialisation with failing callback
 */
START_TEST(nsoption_api_init_failcb_test)
{
	nserror res;
	res = nsoption_init(failing_init_cb, NULL, NULL);
	ck_assert_int_eq(res, NSERROR_INIT_FAILED);
}
END_TEST

/**
 * Test snoptionf format
 */
START_TEST(nsoption_api_snoptionf_badfmt_test)
{
	int ret;
	ret = nsoption_snoptionf(NULL, 0, -1, NULL);
	ck_assert_int_eq(ret, -1);
}
END_TEST

/**
 * Test snoptionf range
 */
START_TEST(nsoption_api_snoptionf_param_test)
{
	int ret;

	ret = nsoption_snoptionf(NULL, 0, NSOPTION_LISTEND, "");
	ck_assert_int_eq(ret, -1);
}
END_TEST

/**
 * Test snoptionf with no initialisation
 */
START_TEST(nsoption_api_snoptionf_no_init_test)
{
	int ret;
	ret = nsoption_snoptionf(NULL, 0, 0, "");
	ck_assert_int_eq(ret, -1);
}
END_TEST


static TCase *nsoption_api_case_create(void)
{
	TCase *tc;
	tc = tcase_create("API checks");

	tcase_add_test(tc, nsoption_api_fini_no_init_test);
	tcase_add_test(tc, nsoption_api_read_no_path_test);
	tcase_add_test(tc, nsoption_api_read_no_init_test);
	tcase_add_test(tc, nsoption_api_write_no_path_test);
	tcase_add_test(tc, nsoption_api_write_no_init_test);
	tcase_add_test(tc, nsoption_api_dump_no_path_test);
	tcase_add_test(tc, nsoption_api_dump_no_init_test);
	tcase_add_test(tc, nsoption_api_commandline_no_args_test);
	tcase_add_test(tc, nsoption_api_commandline_no_init_test);
	tcase_add_test(tc, nsoption_api_init_def_test);
	tcase_add_test(tc, nsoption_api_fini_twice_test);
	tcase_add_test(tc, nsoption_api_init_param_test);
	tcase_add_test(tc, nsoption_api_init_failcb_test);
	tcase_add_test(tc, nsoption_api_snoptionf_no_init_test);
	tcase_add_test(tc, nsoption_api_snoptionf_badfmt_test);
	tcase_add_test(tc, nsoption_api_snoptionf_param_test);

	return tc;
}


static Suite *nsoption_suite_create(void)
{
	Suite *s;
	s = suite_create("User options");

	suite_add_tcase(s, nsoption_api_case_create());
	suite_add_tcase(s, nsoption_case_create());
	suite_add_tcase(s, nsoption_format_case_create());
	suite_add_tcase(s, nsoption_session_case_create());

	return s;
}

int main(int argc, char **argv)
{
	int number_failed;
	SRunner *sr;

	sr = srunner_create(nsoption_suite_create());

	srunner_run_all(sr, CK_ENV);

	number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);

	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
