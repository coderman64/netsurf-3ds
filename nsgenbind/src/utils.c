/* utility functions
 *
 * This file is part of nsgenbind.
 * Published under the MIT License,
 *                http://www.opensource.org/licenses/mit-license.php
 * Copyright 2015 Vincent Sanders <vince@netsurf-browser.org>
 */

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#include "options.h"
#include "utils.h"

/* exported function documented in utils.h */
char *genb_fpath(const char *fname)
{
        char *fpath;
        int fpathl;

        fpathl = strlen(options->outdirname) + strlen(fname) + 2;
        fpath = malloc(fpathl);
        snprintf(fpath, fpathl, "%s/%s", options->outdirname, fname);

        return fpath;
}

static char *genb_fpath_tmp(const char *fname)
{
        char *fpath;
        int fpathl;

        fpathl = strlen(options->outdirname) + strlen(fname) + 3;
        fpath = malloc(fpathl);
        snprintf(fpath, fpathl, "%s/%s.%d", options->outdirname, fname, getpid());

        return fpath;
}

/* exported function documented in utils.h */
FILE *genb_fopen(const char *fname, const char *mode)
{
        char *fpath;
        FILE *filef;

        if (options->dryrun) {
                fpath = strdup("/dev/null");
        } else {
                fpath = genb_fpath(fname);
        }

        filef = fopen(fpath, mode);
        if (filef == NULL) {
                fprintf(stderr, "Error: unable to open file %s (%s)\n",
                        fpath, strerror(errno));
                free(fpath);
                return NULL;
        }
        free(fpath);

        return filef;
}

/* exported function documented in utils.h */
FILE *genb_fopen_tmp(const char *fname)
{
        char *fpath;
        FILE *filef;

        if (options->dryrun) {
                fpath = strdup("/dev/null");
        } else {
                fpath = genb_fpath_tmp(fname);
        }

        filef = fopen(fpath, "w+");
        if (filef == NULL) {
                fprintf(stderr, "Error: unable to open file %s (%s)\n",
                        fpath, strerror(errno));
                free(fpath);
                return NULL;
        }
        free(fpath);

        return filef;
}

int genb_fclose_tmp(FILE *filef_tmp, const char *fname)
{
        char *fpath;
        char *tpath;
        FILE *filef;
        char tbuf[1024];
        char fbuf[1024];
        size_t trd;
        size_t frd;

        if (options->dryrun) {
                fclose(filef_tmp);
                return 0;
        }

        fpath = genb_fpath(fname);
        tpath = genb_fpath_tmp(fname);

        filef = fopen(fpath, "r");
        if (filef == NULL) {
                /* unable to open target file for comparison */

                fclose(filef_tmp); /*  close tmpfile */

                remove(fpath);
                rename(tpath, fpath);
        } else {
                rewind(filef_tmp);

                frd = fread(fbuf, 1, 1024, filef);
                while (frd != 0) {
                        trd = fread(tbuf, 1, frd, filef_tmp);
                        if ((trd != frd) ||
                            (memcmp(tbuf, fbuf, trd) != 0)) {
                                /* file doesnt match */
                                fclose(filef_tmp);
                                fclose(filef);

                                remove(fpath);
                                rename(tpath, fpath);

                                goto close_done;
                        }

                        frd = fread(fbuf, 1, 1024, filef);
                }

                /* was the same kill temporary file */
                fclose(filef_tmp);
                fclose(filef);
                remove(tpath);
        }

close_done:
        free(fpath);
        free(tpath);

        return 0;
}


#ifdef NEED_STRNDUP

char *strndup(const char *s, size_t n)
{
        size_t len;
        char *s2;

        for (len = 0; len != n && s[len]; len++)
                continue;

        s2 = malloc(len + 1);
        if (!s2)
                return 0;

        memcpy(s2, s, len);
        s2[len] = 0;
        return s2;
}

#endif
