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
 * Base64 encoding and decoding implementation.
 *
 * Implements RFC4648 (https://tools.ietf.org/html/rfc4648)
 */

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "nsutils/base64.h"

static const uint8_t b64_decoding_table[256] = {
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /*  0 -  7 */
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /*  8 -  F */
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /* 10 - 17 */
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /* 18 - 1F */
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /* 20 - 27 */
    0xff, 0xff, 0xff,   62, 0xff, 0xff, 0xff,   63, /* 28 - 2F */
      52,   53,   54,   55,   56,   57,   58,   59, /* 30 - 37 */
      60,   61, 0xff, 0xff, 0xff,   64, 0xff, 0xff, /* 38 - 3F */
    0xff,    0,    1,    2,    3,    4,    5,    6, /* 40 - 47 */
       7,    8,    9,   10,   11,   12,   13,   14, /* 48 - 4F */
      15,   16,   17,   18,   19,   20,   21,   22, /* 50 - 57 */
      23,   24,   25, 0xff, 0xff, 0xff, 0xff, 0xff, /* 58 - 5F */
    0xff,   26,   27,   28,   29,   30,   31,   32, /* 60 - 67 */
      33,   34,   35,   36,   37,   38,   39,   40, /* 68 - 6F */
      41,   42,   43,   44,   45,   46,   47,   48, /* 70 - 77 */
      49,   50,   51, 0xff, 0xff, 0xff, 0xff, 0xff, /* 78 - 7F */
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /* 80 - 87 */
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /* 88 - 8F */
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /* 90 - 97 */
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /* 98 - 9F */
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /* A0 - A7 */
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /* A8 - AF */
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /* B0 - B7 */
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /* B8 - BF */
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /* C0 - C7 */
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /* C8 - CF */
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /* D0 - D7 */
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /* D8 - DF */
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /* E0 - E7 */
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /* E8 - EF */
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /* F0 - F7 */
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /* F8 - FF */
};
static const uint8_t b64_encoding_table[] = {
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
    'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
    'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
    'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
    'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
    'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
    'w', 'x', 'y', 'z', '0', '1', '2', '3',
    '4', '5', '6', '7', '8', '9', '+', '/'};
static const uint8_t b64url_decoding_table[256] = {
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /*  0 -  7 */
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /*  8 -  F */
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /* 10 - 17 */
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /* 18 - 1F */
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /* 20 - 27 */
    0xff, 0xff, 0xff, 0xff, 0xff,   62, 0xff, 0xff, /* 28 - 2F */
      52,   53,   54,   55,   56,   57,   58,   59, /* 30 - 37 */
      60,   61, 0xff, 0xff, 0xff,   64, 0xff, 0xff, /* 38 - 3F */
    0xff,    0,    1,    2,    3,    4,    5,    6, /* 40 - 47 */
       7,    8,    9,   10,   11,   12,   13,   14, /* 48 - 4F */
      15,   16,   17,   18,   19,   20,   21,   22, /* 50 - 57 */
      23,   24,   25, 0xff, 0xff, 0xff, 0xff,   63, /* 58 - 5F */
    0xff,   26,   27,   28,   29,   30,   31,   32, /* 60 - 67 */
      33,   34,   35,   36,   37,   38,   39,   40, /* 68 - 6F */
      41,   42,   43,   44,   45,   46,   47,   48, /* 70 - 77 */
      49,   50,   51, 0xff, 0xff, 0xff, 0xff, 0xff, /* 78 - 7F */
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /* 80 - 87 */
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /* 88 - 8F */
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /* 90 - 97 */
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /* 98 - 9F */
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /* A0 - A7 */
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /* A8 - AF */
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /* B0 - B7 */
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /* B8 - BF */
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /* C0 - C7 */
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /* C8 - CF */
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /* D0 - D7 */
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /* D8 - DF */
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /* E0 - E7 */
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /* E8 - EF */
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /* F0 - F7 */
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /* F8 - FF */
};
static const uint8_t b64url_encoding_table[] = {
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
    'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
    'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
    'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
    'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
    'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
    'w', 'x', 'y', 'z', '0', '1', '2', '3',
    '4', '5', '6', '7', '8', '9', '-', '_'};
static unsigned int mod_table[] = {0, 2, 1};




/**
 * Base 64 encode data using a given encoding table.
 *
 * encode source data into buffer using the Base64 encoding.
 *
 * \param input The source data to encode.
 * \param input_length The length of the source data.
 * \param output The buffer to recive data into,
 * \param output The output buffer
 * \param output_length The size of the \a output buffer on entry updated with
 *                        length written on exit.
 * \return NSUERROR_OK on success and \a output_length updated else error code.
 */
static nsuerror
table_encode(const uint8_t *encoding_table,
             const uint8_t *input,
             size_t input_length,
             uint8_t *encoded,
             size_t *output_length)
{
        size_t encoded_len;
        size_t i; /* input index */
        size_t j; /* output index */

        encoded_len = 4 * ((input_length + 2) / 3);

        if (encoded_len > *output_length) {
                /* output buffer is too small */
                return NSUERROR_NOSPACE;
        }

        for (i = 0, j = 0; i < input_length;) {

                uint32_t octet_a = i < input_length ? input[i++] : 0;
                uint32_t octet_b = i < input_length ? input[i++] : 0;
                uint32_t octet_c = i < input_length ? input[i++] : 0;

                uint32_t triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;

                encoded[j++] = encoding_table[(triple >> 3 * 6) & 0x3F];
                encoded[j++] = encoding_table[(triple >> 2 * 6) & 0x3F];
                encoded[j++] = encoding_table[(triple >> 1 * 6) & 0x3F];
                encoded[j++] = encoding_table[(triple >> 0 * 6) & 0x3F];
        }

        for (i = 0; i < mod_table[input_length % 3]; i++) {
                encoded[encoded_len - 1 - i] = '=';
        }

        *output_length = encoded_len;

        return NSUERROR_OK;
}



/**
 * Base 64 encode data using a given encoding table.
 *
 * allocate a buffer and encode source data into it using the Base64 encoding.
 *
 * \param input The source data to encode.
 * \param input_length The length of the source data.
 * \param output The buffer to recive data into, the caller must free.
 * \param output_length The length of data placed in \a output
 * \return NSERROR_OK on success and \a output updated else error code.
 */
static nsuerror
table_encode_alloc(const uint8_t *encoding_table,
                   const uint8_t *input,
                   size_t input_length,
                   uint8_t **output,
                   size_t *output_length)
{
        uint8_t *encoded;
        size_t encoded_len;
        nsuerror res;

        encoded_len = 4 * ((input_length + 2) / 3);

        encoded = malloc(encoded_len);
        if (encoded == NULL) {
                return NSUERROR_NOMEM;
        }

        res = table_encode(encoding_table,
                           input,
                           input_length,
                           encoded,
                           &encoded_len);
        if (res != NSUERROR_OK) {
                free(encoded);
        } else {
                *output = encoded;
                *output_length = encoded_len;
        }
        return res;
}

/*
 * standard base64 encoding
 *
 * exported interface documented in nsutils/base64.h
 */
nsuerror
nsu_base64_encode_alloc(const uint8_t *input,
                        size_t input_length,
                        uint8_t **output,
                        size_t *output_length)
{
        return table_encode_alloc(b64_encoding_table,
                                  input,
                                  input_length,
                                  output,
                                  output_length);
}


/*
 * url base64 encoding
 *
 * exported interface documented in nsutils/base64.h
 */
nsuerror
nsu_base64_encode_alloc_url(const uint8_t *input,
                            size_t input_length,
                            uint8_t **output,
                            size_t *output_length)
{
        return table_encode_alloc(b64url_encoding_table,
                                  input,
                                  input_length,
                                  output,
                                  output_length);
}


/*
 * standard base64 encoding
 *
 * exported interface documented in nsutils/base64.h
 */
nsuerror
nsu_base64_encode(const uint8_t *input,
                        size_t input_length,
                        uint8_t *output,
                        size_t *output_length)
{
        return table_encode(b64_encoding_table,
                                  input,
                                  input_length,
                                  output,
                                  output_length);
}


/*
 * url base64 encoding
 *
 * exported interface documented in nsutils/base64.h
 */
nsuerror
nsu_base64_encode_url(const uint8_t *input,
                            size_t input_length,
                            uint8_t *output,
                            size_t *output_length)
{
        return table_encode(b64url_encoding_table,
                            input,
                            input_length,
                            output,
                            output_length);
}


/**
 * Base 64 decode data with a given decoding table.
 *
 * Allocate a buffer and decode source data into it using the Base64 decoding
 * table.
 *
 * \param input The source data to decode.
 * \param input_length The length of the source data.
 * \param output The buffer to recive data into, the caller must free.
 * \param output_length The length of data placed in \a output
 * \return NSERROR_OK on success and \a output updated else error code.
 */
static nsuerror
base64_decode_alloc(const uint8_t *decoding_table,
                    const uint8_t *input,
                    size_t input_length,
                    uint8_t **output,
                    size_t *output_length)
{
        uint8_t *decoded;
        size_t decoded_len;
        size_t idx;
        size_t opidx;
        uint8_t sextet[4];
        int sextet_idx;

        decoded_len = ((input_length + 3) / 4) * 3;
        if (input[input_length - 1] == '=') (decoded_len)--;
        if (input[input_length - 2] == '=') (decoded_len)--;

        decoded = malloc(decoded_len);
        if (decoded == NULL) {
                return NSUERROR_NOMEM;
        }

        sextet_idx = 0;
        idx = 0;
        opidx = 0;
        while (idx < input_length) {
                sextet[sextet_idx] = decoding_table[input[idx++]];
                if (sextet[sextet_idx] >= 64) {
                        /* not in encoding set */
                        if (sextet[sextet_idx] == 64) {
                                break; /* pad recived - input complete */
                        }
                } else {
                        sextet_idx++;
                        if (sextet_idx == 4) {
                                if (opidx >= (decoded_len - 3)) {
                                        break; /* insufficient output buffer space */
                                }
                                decoded[opidx++] = (sextet[0] << 2) | (sextet[1] >> 4);
                                decoded[opidx++] = (sextet[1] << 4) | (sextet[2] >> 2);
                                decoded[opidx++] = (sextet[2] << 6) | (sextet[3]);

                                sextet_idx = 0;
                        }
                }
        }

        /* deal with any remaining recived bytes ensuring output buffer is not overrun */
        switch (sextet_idx) {
        case 1:
                if (opidx < decoded_len) {
                        decoded[opidx++] = (sextet[0] << 2);
                }
                break;

        case 2:
                if (opidx < decoded_len) {
                        decoded[opidx++] = (sextet[0] << 2) | (sextet[1] >> 4);
                }
                if (opidx < decoded_len) {
                        decoded[opidx++] = (sextet[1] << 4);
                }
                break;

        case 3:
                if (opidx < decoded_len) {
                        decoded[opidx++] = (sextet[0] << 2) | (sextet[1] >> 4);
                }
                if (opidx < decoded_len) {
                        decoded[opidx++] = (sextet[1] << 4) | (sextet[2] >> 2);
                }
                if (opidx < decoded_len) {
                        decoded[opidx++] = (sextet[2] << 6);
                }
                break;

        case 4:
                if (opidx < decoded_len) {
                        decoded[opidx++] = (sextet[0] << 2) | (sextet[1] >> 4);
                }
                if (opidx < decoded_len) {
                        decoded[opidx++] = (sextet[1] << 4) | (sextet[2] >> 2);
                }
                if (opidx < decoded_len) {
                        decoded[opidx++] = (sextet[2] << 6) | (sextet[3]);
                }
                break;
        }

        *output = decoded;
        *output_length = opidx;

        return NSUERROR_OK;
}


/*
 * standard base64 decoding
 *
 * exported interface documented in nsutils/base64.h
 */
nsuerror
nsu_base64_decode_alloc(const uint8_t *input,
                        size_t input_length,
                        uint8_t **output,
                        size_t *output_length)
{
        return base64_decode_alloc(b64_decoding_table,
                                   input,
                                   input_length,
                                   output,
                                   output_length);
}


/*
 * standard base64 decoding
 *
 * exported interface documented in nsutils/base64.h
 */
nsuerror
nsu_base64_decode_alloc_url(const uint8_t *input,
                            size_t input_length,
                            uint8_t **output,
                            size_t *output_length)
{
        return base64_decode_alloc(b64url_decoding_table,
                                   input,
                                   input_length,
                                   output,
                                   output_length);
}
