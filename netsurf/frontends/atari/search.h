/*
 * Copyright 2013 Ole Loots <ole@monochrom.net>
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
 *
 */

#ifndef NS_ATARI_SEARCH_H
#define NS_ATARI_SEARCH_H

#define SEARCH_MAX_SLEN 24

struct gui_window;
struct browser_window;

#include "desktop/search.h"

struct s_search_form_state {
	char text[32];
	uint32_t flags;
	bool back_avail;
};

struct s_search_form_session {
	struct gui_window *g;
	struct s_search_form_state state;
};

typedef struct s_search_form_session * SEARCH_FORM_SESSION;

struct s_search_form_session * nsatari_search_session_create(OBJECT * obj,
		struct gui_window *gw);

extern struct gui_search_table *atari_search_table;

void nsatari_search_session_destroy(struct s_search_form_session *s);
void nsatari_search_perform(struct s_search_form_session *s, OBJECT *obj,
		search_flags_t f);
void nsatari_search_restore_form( struct s_search_form_session *s, OBJECT *obj);

#endif
