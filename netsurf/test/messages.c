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
 * Tests for message processing
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
#include "utils/messages.h"

#include "test/message_data_inline.h"

#define NELEMS(x)  (sizeof(x) / sizeof((x)[0]))

const char *test_messages_path = "test/data/Messages";

struct message_test_vec_s {
	int test;
	const char *res;
};

struct message_test_vec_s message_errorcode_test_vec[] = {
	{ NSERROR_OK, "OK" },
	{ NSERROR_NOMEM, "NetSurf is running out of memory. Please free some memory and try again." },
	{ NSERROR_NO_FETCH_HANDLER, "NoHandler" },
	{ NSERROR_NOT_FOUND, "Not found" },
	{ NSERROR_SAVE_FAILED, "SaveFailed" },
	{ NSERROR_CLONE_FAILED, "CloneFailed" },
	{ NSERROR_INIT_FAILED, "InitFailed" },
	{ NSERROR_BAD_ENCODING, "BadEncoding" },
	{ NSERROR_NEED_DATA, "NeedData" },
	{ NSERROR_ENCODING_CHANGE, "EncodingChanged" },
	{ NSERROR_BAD_PARAMETER, "BadParameter" },
	{ NSERROR_INVALID, "Invalid" },
	{ NSERROR_BOX_CONVERT, "BoxConvert" },
	{ NSERROR_STOPPED, "Stopped" },
	{ NSERROR_DOM, "Parsing the document failed." },
	{ NSERROR_CSS, "Error processing CSS" },
	{ NSERROR_CSS_BASE, "Base stylesheet failed to load" },
	{ NSERROR_BAD_URL, "BadURL" },
	{ NSERROR_UNKNOWN, "Unknown" },
};

START_TEST(messages_errorcode_test)
{
	const char *res_str;
	const struct message_test_vec_s *tst = &message_errorcode_test_vec[_i];
	nserror res;

	res = messages_add_from_inline(test_data_Messages,
				       test_data_Messages_len);
	ck_assert_int_eq(res, NSERROR_OK);

	res_str = messages_get_errorcode(tst->test);

	/* ensure result data is correct */
	ck_assert_str_eq(res_str, tst->res);

	/* cleanup */
	messages_destroy();
}
END_TEST

START_TEST(message_inline_load_test)
{
	nserror res;
	res = messages_add_from_inline(test_data_Messages,
				       test_data_Messages_len);
	ck_assert_int_eq(res, NSERROR_OK);

	/* cleanup */
	messages_destroy();
}
END_TEST

START_TEST(message_file_load_test)
{
	nserror res;
	res = messages_add_from_file(test_messages_path);
	ck_assert_int_eq(res, NSERROR_OK);

	/* cleanup */
	messages_destroy();
}
END_TEST

START_TEST(message_get_buff_test)
{
	nserror res;
	char *buf;
	res = messages_add_from_inline(test_data_Messages,
				       test_data_Messages_len);
	ck_assert_int_eq(res, NSERROR_OK);

	buf = messages_get_buff("DefinitelyNotAKey");
	ck_assert(buf == NULL);

	buf = messages_get_buff("NoMemory");
	ck_assert_str_eq(buf, "NetSurf is running out of memory. Please free some memory and try again.");
	free(buf);

	/* cleanup */
	messages_destroy();
}
END_TEST


static TCase *message_session_case_create(void)
{
	TCase *tc;
	tc = tcase_create("Message adding");

	tcase_add_test(tc, message_file_load_test);
	tcase_add_test(tc, message_inline_load_test);
	tcase_add_loop_test(tc, messages_errorcode_test,
			    0, NELEMS(message_errorcode_test_vec));
	tcase_add_test(tc, message_get_buff_test);

	return tc;
}


static Suite *message_suite_create(void)
{
	Suite *s;
	s = suite_create("message");

	suite_add_tcase(s, message_session_case_create());

	return s;
}

int main(int argc, char **argv)
{
	int number_failed;
	SRunner *sr;

	sr = srunner_create(message_suite_create());

	srunner_run_all(sr, CK_ENV);

	number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);

	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
