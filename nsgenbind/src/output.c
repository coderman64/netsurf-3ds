/* generated output implementation
 *
 * This file is part of nsgenbind.
 * Licensed under the MIT License,
 *                http://www.opensource.org/licenses/mit-license.php
 * Copyright 2019 Vincent Sanders <vince@netsurf-browser.org>
 */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"
#include "output.h"

struct opctx {
    char *filename;
    FILE *outf;
    unsigned int lineno;
};

int output_open(const char *filename, struct opctx **opctx_out)
{
        struct opctx *opctx;

        opctx = malloc(sizeof(struct opctx));
        if (opctx == NULL) {
                return -1;
        }

        opctx->filename = strdup(filename);
        if (opctx->filename == NULL) {
                free(opctx);
                return -1;
        }

        /* open output file */
        opctx->outf = genb_fopen_tmp(opctx->filename);
        if (opctx->outf == NULL) {
                free(opctx->filename);
                free(opctx);
                return -1;
        }

        opctx->lineno = 2;
        *opctx_out = opctx;

        return 0;
}

int output_close(struct opctx *opctx)
{
        int res;
        res = genb_fclose_tmp(opctx->outf, opctx->filename);
        free(opctx->filename);
        free(opctx);
        return res;
}

/**
 * buffer to hold formatted output so newlines can be counted
 */
static char output_buffer[128*1024];

int outputf(struct opctx *opctx, const char *fmt, ...)
{
        va_list ap;
        int res;
        int idx;

        va_start(ap, fmt);
        res = vsnprintf(output_buffer, sizeof(output_buffer), fmt, ap);
        va_end(ap);

        /* account for newlines in output */
        for (idx = 0; idx < res; idx++) {
                if (output_buffer[idx] == '\n') {
                        opctx->lineno++;
                }
        }

        fwrite(output_buffer, 1, res, opctx->outf);

        return res;
}

int outputc(struct opctx *opctx, int c)
{
        if (c == '\n') {
                opctx->lineno++;
        }
        fputc(c, opctx->outf);

        return 0;
}

int output_line(struct opctx *opctx)
{
        int res;
        res = fprintf(opctx->outf,
                      "#line %d \"%s\"\n",
                      opctx->lineno, opctx->filename);
        opctx->lineno++;
        return res;
}
