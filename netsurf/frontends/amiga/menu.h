/*
 * Copyright 2017 Chris Young <chris@unsatisfactorysoftware.co.uk>
 *
 * This file is part of NetSurf, http://www.netsurf-browser.org/
 *
 * NetSurf is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * NetSurf is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef AMIGA_MENU_H
#define AMIGA_MENU_H

#include <exec/types.h>
#include <intuition/intuition.h>
#include <libraries/gadtools.h>

struct ami_menu_data {
	char *restrict menulab;
	Object *restrict menuobj;
	char *restrict menukey;
	char *restrict menuicon;
	struct Hook menu_hook;
	UBYTE menutype;
	UWORD flags;
};

/** empty space */
#define NSA_SPACE "blankspace.png"

/* cleanup */
void ami_menu_free_glyphs(void);

/* generic menu alloc/free/layout */
void ami_menu_alloc_item(struct ami_menu_data **md, int num, UBYTE type,
			const char *restrict label, const char *restrict key, const char *restrict icon,
			void *restrict func, void *restrict hookdata, UWORD flags);
struct Menu *ami_menu_layout(struct ami_menu_data **md, int max);
void ami_menu_free_menu(struct ami_menu_data **md, int max, struct Menu *imenu);
void ami_menu_free_lab_item(struct ami_menu_data **md, int i);

/* refresh a menu's children */
void ami_menu_refresh(struct Menu *menu, struct ami_menu_data **md, int menu_item, int max,
	nserror (*cb)(struct ami_menu_data **md));

/**
 * Get the selected state of a menu item
 */
bool ami_menu_get_selected(struct Menu *menu, struct IntuiMessage *msg);
#endif

