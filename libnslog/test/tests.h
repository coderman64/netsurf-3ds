/* test/tests.h
 *
 * Set of test suites for libnslog
 *
 * Copyright 2009,2017 The NetSurf Browser Project
 *                Daniel Silverstone <dsilvers@netsurf-browser.org>
 */

#ifndef nslog_tests_h_
#define nslog_tests_h_

#include <assert.h>
#include <signal.h>

#include <check.h>

#include "nslog/nslog.h"

extern void nslog_basic_suite(SRunner *);

#endif /* nslog_tests_h_ */
