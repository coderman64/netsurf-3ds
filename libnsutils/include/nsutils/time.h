/*
 * Copyright 2014 Vincent Sanders <vince@netsurf-browser.org>
 *
 * This file is part of libnsutils.
 *
 * Licensed under the MIT License,
 *                http://www.opensource.org/licenses/mit-license.php
 */

/**
 * \file
 * Time related operations.
 */

#ifndef NSUTILS_TIME_H_
#define NSUTILS_TIME_H_

#include <inttypes.h>

#include <nsutils/errors.h>

/**
 * Get a monotonically incrementing number of milliseconds.
 *
 * Obtain a count of elapsed time in milliseconds from an arbitrary point in
 * time. Unlike gettimeofday this will continue linearly across time setting
 * and not go backwards.
 *
 * \param current The current value of the counter.
 * \return NSERROR_OK on success else error code.
 */
nsuerror nsu_getmonotonic_ms(uint64_t *current);
 
#endif
