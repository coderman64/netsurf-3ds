/*
 * This file is part of RUfl
 * Licensed under the MIT License,
 *                http://www.opensource.org/licenses/mit-license
 * Copyright 2006 James Bursa <james@semichrome.net>
 */

#include <stdlib.h>
#include <oslib/font.h>
#include "rufl_internal.h"


/**
 * Free all resources used by the library.
 */

void rufl_quit(void)
{
	unsigned int i;

	if (!rufl_font_list)
		return;

	for (i = 0; i != rufl_font_list_entries; i++) {
		free(rufl_font_list[i].identifier);
		free(rufl_font_list[i].charset);
		if (rufl_font_list[i].umap != NULL) {
			size_t j;
			for (j = 0; j != rufl_font_list[i].num_umaps; j++) {
				free((rufl_font_list[i].umap + j)->encoding);
			}
			free(rufl_font_list[i].umap);
		}
	}
	free(rufl_font_list);
	rufl_font_list = NULL;
	rufl_font_list_entries = 0;

	for (i = 0; i != rufl_family_list_entries; i++)
		free((void *) rufl_family_list[i]);
	free(rufl_family_list);
	free(rufl_family_map);
	rufl_family_map = NULL;
	rufl_family_list = NULL;
	rufl_family_list_entries = 0;

	for (i = 0; i != rufl_CACHE_SIZE; i++) {
		if (rufl_cache[i].font != rufl_CACHE_NONE) {
			xfont_lose_font(rufl_cache[i].f);
			rufl_cache[i].font = rufl_CACHE_NONE;
		}
	}
	rufl_cache_time = 0;

        free(rufl_family_menu);
        rufl_family_menu = NULL;

	rufl_substitution_table_fini();
}
