/* test/testmain.c
 *
 * Core of the test suite for libnslog
 *
 * Copyright 2009, 2017 The NetSurf Browser Project
 *                Daniel Silverstone <dsilvers@netsurf-browser.org>
 */

#include <check.h>
#include <stdlib.h>

#include "tests.h"

#ifndef UNUSED
#define UNUSED(x) ((x) = (x))
#endif

int
main(int argc, char **argv)
{
        int number_failed = 0;
        SRunner *sr;

	UNUSED(argc);
	UNUSED(argv);

        sr = srunner_create(suite_create("Test suite for libnslog"));

        nslog_basic_suite(sr);

        srunner_set_fork_status(sr, CK_FORK);
        srunner_run_all(sr, CK_ENV);
        number_failed = srunner_ntests_failed(sr);

        srunner_free(sr);

        return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
