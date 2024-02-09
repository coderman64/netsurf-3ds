/* generated output handlers
 *
 * This file is part of nsgenbind.
 * Licensed under the MIT License,
 *                http://www.opensource.org/licenses/mit-license.php
 * Copyright 2012 Vincent Sanders <vince@netsurf-browser.org>
 */

#ifndef nsgenbind_output_h
#define nsgenbind_output_h

struct opctx;

/**
 * open output file
 *
 * opens output file and creates output context
 *
 * \param filename The filename of the file to output
 * \param opctx_out The resulting output context
 * \return 0 on success and opctx_out updated else -1
 */
int output_open(const char *filename, struct opctx **opctx_out);

/**
 * close output file and free context
 */
int output_close(struct opctx *opctx);

/**
 * output formatted data to file
 */
int outputf(struct opctx *opctx, const char *fmt, ...) __attribute__ ((format (printf, 2, 3)));

int outputc(struct opctx *opctx, int c);

/**
 * generate c comment with line directive for current outut context
 */
int output_line(struct opctx *opctx);

#endif
