/*
 * This file is part of LibNSUtils.
 * Licensed under the MIT License,
 *                http://www.opensource.org/licenses/mit-license.php
 * Copyright 2009 John-Mark Bell <jmb@netsurf-browser.org>
 */

/**
 * \file
 * Endianness helper functions.
 */

#ifndef NSUTILS_ENDIAN_H__
#define NSUTILS_ENDIAN_H__

#include <inttypes.h>
#include <stdbool.h>

/**
 * Detect if the host is little endian.
 *
 * \return True if the host is little endian; false otherwise.
 */
static inline bool endian_host_is_le(void)
{
	const uint16_t test = 1;

	return ((const uint8_t *) &test)[0];
}

/**
 * Swap the endianness of a 32bit value.
 *
 * \param val Value to swap endianness of
 * \return Endian-swapped value
 */
static inline uint32_t endian_swap(uint32_t val)
{
	return ((val & 0xff000000) >> 24) | ((val & 0x00ff0000) >> 8) |
		((val & 0x0000ff00) << 8) | ((val & 0x000000ff) << 24);
}

/**
 * Convert a host-endian 32bit value to its big-endian equivalent.
 *
 * \param host Value to convert
 * \return Big endian value
 */
static inline uint32_t endian_host_to_big(uint32_t host)
{
	if (endian_host_is_le())
		return endian_swap(host);

	return host;
}

/**
 * Convert a big-endian 32bit value to its host-endian equivalent.
 *
 * \param host Value to convert
 * \return Host endian value
 */
static inline uint32_t endian_big_to_host(uint32_t big)
{
	if (endian_host_is_le())
		return endian_swap(big);

	return big;
}

#endif
