/*
 * This file is part of RUfl
 * Licensed under the MIT License,
 *                http://www.opensource.org/licenses/mit-license
 * Copyright 2005 James Bursa <james@semichrome.net>
 */

#include "rufl_internal.h"


/**
 * Test if a character set contains a character.
 *
 * \param  charset  character set
 * \param  u        Unicode codepoint
 * \return  true if present, false if absent
 */

bool rufl_character_set_test(const struct rufl_character_set *charset,
		uint32_t u)
{
	unsigned int plane = u >> 16;
	unsigned int block = (u >> 8) & 0xff;
	unsigned int byte = (u >> 3) & 31;
	unsigned int bit = u & 7;

	if (17 <= plane)
		return false;

	/* Look for the plane we want */
	while (PLANE_ID(charset->metadata) != plane &&
			EXTENSION_FOLLOWS(charset->metadata)) {
		charset = (void *)(((uint8_t *)charset) +
				PLANE_SIZE(charset->metadata));
	}
	if (PLANE_ID(charset->metadata) != plane)
		return false;

	if (charset->index[block] == BLOCK_EMPTY)
		return false;
	else if (charset->index[block] == BLOCK_FULL)
		return true;
	else {
		uint8_t z = charset->block[charset->index[block]][byte];
		return z & (1 << bit);
	}
}
