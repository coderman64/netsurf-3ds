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
 * \brief Interface to utility string handling.
 */

#ifndef _NETSURF_UTILS_STRING_H_
#define _NETSURF_UTILS_STRING_H_

#include <stdlib.h>
#include <stdarg.h>

#include "utils/errors.h"


/**
 * Replace consecutive whitespace with a single space.
 *
 * @todo determine if squash_whitespace utf-8 safe and that it needs to be
 *
 * \param  s  source string
 * \return  heap allocated result, or NULL on memory exhaustion
 */
char *squash_whitespace(const char * s);


/**
 * Converts NUL terminated UTF-8 encoded string s containing zero or more
 * spaces (char 32) or TABs (char 9) to non-breaking spaces
 * (0xC2 + 0xA0 in UTF-8 encoding).
 *
 * Caller needs to free() result.  Returns NULL in case of error.  No
 * checking is done on validness of the UTF-8 input string.
 */
char *cnv_space2nbsp(const char *s);


/**
 * Create a human readable representation of a size in bytes.
 *
 * Does a simple conversion which assumes the user speaks English.
 * The buffer returned is one of three static ones so may change each
 * time this call is made.  Don't store the buffer for later use.
 * It's done this way for convenience and to fight possible memory
 * leaks, it is not necessarily pretty.
 *
 * @param bytesize The size in bytes.
 * @return A human readable string representing the size.
 */
char *human_friendly_bytesize(unsigned long long int bytesize);


/**
 * Generate a string from one or more component elements separated with
 * a single value.
 *
 * This is similar in intent to the perl join function creating a
 * single delimited string from an array of several.
 *
 * @note If a string is allocated it must be freed by the caller.
 *
 * @param[in,out] str pointer to string pointer if this is NULL enough
 *                    storage will be allocated for the complete path.
 * @param[in,out] size The size of the space available if \a str not
 *                     NULL on input and if not NULL set to the total
 *                     output length on output.
 * @param[in] sep The character to separate the elements with.
 * @param[in] nelm The number of elements up to a maximum of 16.
 * @param[in] ap The elements of the path as string pointers.
 * @return NSERROR_OK and the complete path is written to str or error
 *         code on failure.
 */
nserror vsnstrjoin(char **str, size_t *size, char sep, size_t nelm, va_list ap);


/**
 * Generate a string from one or more component elements separated with
 * a single value.
 *
 * This is similar in intent to the Perl join function creating a
 * single delimited string from an array of several.
 *
 * @note If a string is allocated it must be freed by the caller.
 *
 * @param[in,out] str pointer to string pointer if this is NULL enough
 *                    storage will be allocated for the complete path.
 * @param[in,out] size The size of the space available if \a str not
 *                     NULL on input and if not NULL set to the total
 *                     output length on output.
 * @param[in] sep The character to separate the elements with.
 * @param[in] nelm The number of elements up to a maximum of 16.
 * @param[in] ... The elements of the path as string pointers.
 * @return NSERROR_OK and the complete path is written to str or error
 *         code on failure.
 */
nserror snstrjoin(char **str, size_t *size, char sep, size_t nelm, ...);

#endif
