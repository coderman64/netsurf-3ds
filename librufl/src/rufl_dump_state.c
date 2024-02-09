/*
 * This file is part of RUfl
 * Licensed under the MIT License,
 *                http://www.opensource.org/licenses/mit-license
 * Copyright 2006 James Bursa <james@semichrome.net>
 */

#include <stdio.h>
#include "rufl_internal.h"


static void rufl_dump_character_set_list(
		const struct rufl_character_set *charset);
static void rufl_dump_unicode_map(struct rufl_unicode_map *umap);


/**
 * Dump the internal library state to stdout.
 */

void rufl_dump_state(bool verbose)
{
	unsigned int i, j;

	printf("rufl_font_list:\n");
	for (i = 0; i != rufl_font_list_entries; i++) {
		printf("  %u \"%s\"\n", i, rufl_font_list[i].identifier);
		if (rufl_font_list[i].charset) {
			printf("    ");
			rufl_dump_character_set_list(rufl_font_list[i].charset);
			printf("\n");
		} else {
			printf("    (no charset table)\n");
		}
		if (verbose && rufl_font_list[i].umap) {
			for (j = 0; j < rufl_font_list[i].num_umaps; j++) {
				struct rufl_unicode_map *map =
						rufl_font_list[i].umap + j;

				printf("    ");
				rufl_dump_unicode_map(map);
				printf("\n");
			}
                }
	}

	printf("rufl_family_list:\n");
	for (i = 0; i != rufl_family_list_entries; i++) {
		printf("  %u \"%s\"\n", i, rufl_family_list[i]);
		for (j = 0; j != 9; j++) {
			struct rufl_family_map_entry *e = &rufl_family_map[i];
			printf("    %u ", j);
			if (e->font[j][0] == NO_FONT)
				printf("- ");
			else
				printf("\"%s\" ", rufl_font_list[e->font[j][0]].
						identifier);
			if (e->font[j][1] == NO_FONT)
				printf("- ");
			else
				printf("\"%s\" ", rufl_font_list[e->font[j][1]].
						identifier);
			printf("\n");
		}
	}

	printf("rufl_substitution_table:\n");
	rufl_substitution_table_dump();
}


/**
 * Dump a representation of a character set to stdout.
 *
 * \param  charset  character set to print
 */

static void rufl_dump_character_set(const struct rufl_character_set *charset)
{
	unsigned int u, t, plane = PLANE_ID(charset->metadata) << 16;

	u = 0;
	while (u != 0x10000) {
		while (u != 0x10000 &&
				!rufl_character_set_test(charset, plane + u))
			u++;
		if (u != 0x10000) {
			if (!rufl_character_set_test(charset, plane + u + 1)) {
				printf("%x ", plane + u);
				u++;
			} else {
				t = u;
				while (u != 0x10000 && rufl_character_set_test(
							charset, plane + u))
					u++;
				printf("%x-%x ", plane + t, plane + u - 1);
			}
		}
	}
}

/**
 * Dump a representation of a character set list to stdout.
 *
 * \param  charset  character set to print
 */

void rufl_dump_character_set_list(const struct rufl_character_set *charset)
{
	while (EXTENSION_FOLLOWS(charset->metadata)) {
		rufl_dump_character_set(charset);
		charset = (void *)(((uint8_t *)charset) +
				PLANE_SIZE(charset->metadata));
	}
	rufl_dump_character_set(charset);
}

/**
 * Dump a representation of a unicode map to stdout.
 *
 * \param  umap  unicode map to print
 */

void rufl_dump_unicode_map(struct rufl_unicode_map *umap)
{
	unsigned int i;

	if (umap->encoding)
		printf("%s: ", umap->encoding);

	for (i = 0; i != umap->entries; i++)
		printf("%x:%x ", umap->map[i].u, umap->map[i].c);
}
