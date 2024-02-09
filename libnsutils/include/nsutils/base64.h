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
 * Base64 encoding and decoding interface.
 */

#ifndef NSUTILS_BASE64_H_
#define NSUTILS_BASE64_H_

#include <inttypes.h>

#include <nsutils/errors.h>

/**
 * Base 64 encode data.
 *
 * allocate a buffer and encode source data into it using the Base64 encoding.
 *
 * \param input The source data to encode.
 * \param input_length The length of the source data.
 * \param output The buffer to recive data into, the caller must free.
 * \param output_length The length of data placed in \a output
 * \return NSUERROR_OK on success and \a output updated else error code.
 */
nsuerror nsu_base64_encode_alloc(const uint8_t *input,
                                 size_t input_length,
                                 uint8_t **output,
                                 size_t *output_length);

/**
 * Base 64 encode data.
 *
 * encode source data into buffer it using the Base64 encoding.
 *
 * \param input The source data to encode.
 * \param input_length The length of the source data.
 * \param output The output buffer
 * \param output_length The size of the \a output buffer on entry updated with length written on exit.
 * \return NSUERROR_OK on success and \a output_length updated else error code.
 */
nsuerror nsu_base64_encode(const uint8_t *input,
                           size_t input_length,
                           uint8_t *output,
                           size_t *output_length);

/**
 * Base 64 encode data.
 *
 * allocate a buffer and encode source data into it using the base64 URL safe
 *   encoding.
 *
 * \param input The source data to encode.
 * \param input_length The length of the source data.
 * \param output The buffer to recive data into, the caller must free.
 * \param output_length The length of data placed in \a output
 * \return NSERROR_OK on success and \a output updated else error code.
 */
nsuerror nsu_base64_encode_alloc_url(const uint8_t *input,
                                     size_t input_length,
                                     uint8_t **output,
                                     size_t *output_length);


/**
 * Base 64 encode data.
 *
 * encode source data into buffer using the Base64 URL safe encoding.
 *
 * \param input The source data to encode.
 * \param input_length The length of the source data.
 * \param output The output buffer
 * \param output_length The size of the \a output buffer on entry updated with length written on exit.
 * \return NSUERROR_OK on success and \a output_length updated else error code.
 */
nsuerror nsu_base64_encode_url(const uint8_t *input,
                           size_t input_length,
                           uint8_t *output,
                           size_t *output_length);

/**
 * Base 64 decode data.
 *
 * allocate a buffer and decode source data into it using the Base64 encoding.
 *
 * \param input The source data to decode.
 * \param input_length The length of the source data.
 * \param output The buffer to recive data into, the caller must free.
 * \param output_length The length of data placed in \a output
 * \return NSERROR_OK on success and \a output updated else error code.
 */
nsuerror nsu_base64_decode_alloc(const uint8_t *input,
                                 size_t input_length,
                                 uint8_t **output,
                                 size_t *output_length);

/**
 * Base 64 decode data.
 *
 * Allocate a buffer and decode source data into it using the Base64 URL safe
 *   encoding.
 *
 * \param input The source data to decode.
 * \param input_length The length of the source data.
 * \param output The buffer to recive data into, the caller must free.
 * \param output_length The length of data placed in \a output
 * \return NSERROR_OK on success and \a output updated else error code.
 */
nsuerror nsu_base64_decode_alloc_url(const uint8_t *input,
                                     size_t input_length,
                                     uint8_t **output,
                                     size_t *output_length);

#endif
