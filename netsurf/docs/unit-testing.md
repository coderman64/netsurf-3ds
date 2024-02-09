NetSurf Unit Testing
====================

[TOC]

# Overview

NetSurf has unit tests integrated in the test directory. These tests
use the check unit test framework for C [1].

The tests are in a logical hierachy of "suite", "case" and individual
"test". Historicaly we have split suites of tests into separate test
programs although the framework does not madate this and some test
programs contain more than one suite.


# Execution

The test programs are executed by using the standard "test" target
from the top level make invocation. The "coverage" target additionally
generates code coverage reports allowing visibility on how much of a
code module is being exercised.

The check library must be installed to run the tests and the CI system
automatically executes all enabled tests and generates coverage
reports for each commit.

# Adding tests

The test/Makefile defines each indiviadual test program that should be
built and executed in the TESTS variable.

The test program source files are defined in a xxx_SRCS variable and
the make rules will then ensure the target program is built and
executed.

Each individual test program requires a main function which creates
one (or more) suites. The suites are added to a test runner and then
executed and the results reported.

    int main(int argc, char **argv)
    {
    	int number_failed;
    	SRunner *sr;
    
    	sr = srunner_create(foo_suite_create());
    	//srunner_add_suite(sr, bar_suite_create());
    
    	srunner_run_all(sr, CK_ENV);
    
    	number_failed = srunner_ntests_failed(sr);
    	srunner_free(sr);
    
    	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
    }

Suite creation is done with a sub function to logically split suite
code into sub modules. Each suite has test cases added to it.

    Suite *foo_suite_create(void)
    {
    	Suite *s;
    	s = suite_create("foo");
    
    	suite_add_tcase(s, baz_case_create());
    	suite_add_tcase(s, qux_case_create());
    
    	return s;
    }

Test cases include the actual tests to be performed within each case.

    TCase *baz_case_create(void)
    {
    	TCase *tc;
    	tc = tcase_create("Baz");
    
    	tcase_add_test(tc, xxyz_test);
    	tcase_add_test(tc, zzyx_test);
    
    	return tc;
    }

A test case may optionally have a fixture which is code that is
executed before and after each test case. Unchecked fixtures are
executed once before the test process forks for each test whereas
checked fixtures are executed for each and every test.

    static void fixture_setup(void)
    {
    }
    
    static void fixture_teardown(void)
    {
    }
    
    TCase *qux_case_create(void)
    {
    	TCase *tc;
    
    	/* Matching entry tests */
    	tc = tcase_create("Match");
    
    	tcase_add_checked_fixture(tc,
    				  fixture_setup,
    				  fixture_teardown);
    
    	tcase_add_test(tc, zzz_test);
    
    	return tc;
    }

Additionally test cases can contain tests executed in a loop. The test
recives a single integer as a parameter named _i which iterates
between values specified in the case setup.

    TCase *baz_case_create(void)
    {
    	TCase *tc;
    	tc = tcase_create("Baz");
    
    	tcase_add_loop_test(tc, looping_test, 0, 5);
    
    	return tc;
    }

It is also possible to create tests which will generate a signal. The
most commonly used of these is to check asserts in API calls.

    TCase *baz_case_create(void)
    {
    	TCase *tc;
    	tc = tcase_create("Baz");
    
    	tcase_add_test_raise_signal(tc, assert_test, 6);
    
    	return tc;
    }


Actual test code is self contained in a function which uses the
ck_assert macros to test results. The check framework requires each
test to use the START_TEST and END_TEST macros when definig them.

    /**
     * url access leaf test
     */
    START_TEST(nsurl_access_leaf_test)
    {
    	nserror err;
    	nsurl *res_url;
    	const struct test_triplets *tst = &access_tests[_i];
    
    	/* not testing create, this should always succeed */
    	err = nsurl_create(tst->test1, &res_url);
    	ck_assert(err == NSERROR_OK);
    
    	ck_assert_str_eq(nsurl_access_leaf(res_url), tst->res);
    
    	nsurl_unref(res_url);
    }
    END_TEST


[1] https://libcheck.github.io/check/
