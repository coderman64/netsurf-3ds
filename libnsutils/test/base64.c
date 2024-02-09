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
 *
 * Base64 test program. Reads data from stdin and en/de codes it to/from base64
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include <nsutils/base64.h>

int main(int argc, char**argv)
{
        uint8_t *buffer;
        size_t buffer_len=0;
        uint8_t *output;
        size_t output_len;
        int opt;
        int decode = 0;
        int url = 0;


        while ((opt = getopt(argc, argv, "du")) != -1) {
                switch (opt) {
                case 'd':
                        decode = 1;
                        break;
                case 'u':
                        url = 1;
                        break;

                default: /* '?' */
                        fprintf(stderr, "Usage: %s [-d] [-u]\n", argv[0]);
                        exit(EXIT_FAILURE);

                }
        }

        if (scanf("%1024mc%n", &buffer, (int *)&buffer_len) < 1) {
                return 1;
        }


        if (decode) {
                /* decode */
                if (url) {
                        nsu_base64_decode_alloc_url(buffer, buffer_len, &output, &output_len);
                } else {

                        nsu_base64_decode_alloc(buffer, buffer_len, &output, &output_len);
                }
        } else {
                /* encode */
                if (url) {
                        nsu_base64_encode_alloc_url(buffer, buffer_len, &output, &output_len);
                } else {
                        nsu_base64_encode_alloc(buffer, buffer_len, &output, &output_len);
                }
        }

        if (output != NULL) {
                printf("%.*s", (int)output_len, output);
                free(output);
        }

        free(buffer);

        return 0;
}
