/*
 * Copyright 2016 Vincent Sanders <vince@netsurf-browser.org>
 *
 * This file is part of libnspsl
 *
 * Licensed under the MIT License,
 *                http://www.opensource.org/licenses/mit-license.php
 */

/**
 * \file
 *
 * psl test program. first argument is checked against being a public suffix.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <nspsl.h>

int main(int argc, char**argv)
{
    const char *output;
    size_t output_len;


    if (argc == 2) {
        output = nspsl_getpublicsuffix(argv[1]);
    } else {
        fprintf(stderr, "Usage: %s data", argv[0]);
        return 1;
    }
    
    if (output != NULL) {
        output_len = strlen(output);
        printf("%.*s\n", (int)output_len, output);
    } else {
        printf("null\n");
    }

    return 0;
}
