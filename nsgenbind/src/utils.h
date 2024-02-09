/* utility helpers
 *
 * This file is part of nsnsgenbind.
 * Licensed under the MIT License,
 *                http://www.opensource.org/licenses/mit-license.php
 * Copyright 2015 Vincent Sanders <vince@netsurf-browser.org>
 */

#ifndef nsgenbind_utils_h
#define nsgenbind_utils_h

/**
 * get a pathname with the output prefix prepended
 *
 * \param fname leaf filename.
 * \return full prefixed path to file caller must free
 */
char *genb_fpath(const char *fname);

/**
 * Open file allowing for output path prefix
 */
FILE *genb_fopen(const char *fname, const char *mode);

/**
 * Open file allowing for output path prefix
 *
 * file is opened for reading/writing with a temporary suffix allowing for the
 * matching close call to check the output is different before touching the
 * target file.
 */
FILE *genb_fopen_tmp(const char *fname);

/**
 * Close file opened with genb_fopen
 */
int genb_fclose_tmp(FILE *filef, const char *fname);

#if defined(__APPLE__) || defined(_WIN32)
#define NEED_STRNDUP 1
char *strndup(const char *s, size_t n);
#endif

#ifndef SLEN
/* Calculate length of a string constant */
#define SLEN(s) (sizeof((s)) - 1) /* -1 for '\0' */
#endif

#ifndef UNUSED
#define UNUSED(x) ((void)(x))
#endif

#endif
