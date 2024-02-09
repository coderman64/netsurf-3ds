/*
 * Copyright 2005 Adrian Lees <adrianl@users.sourceforge.net>
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

#ifndef _NETSURF_RISCOS_QUERY_H
#define _NETSURF_RISCOS_QUERY_H

#include <stdbool.h>
#include "oslib/wimp.h"

enum query_response {
  QUERY_CONTINUE,
  QUERY_YES,
  QUERY_NO,
  QUERY_ESCAPE
};

typedef int query_id;

#define QUERY_INVALID ((query_id)-1)

typedef struct
{
	void (*confirm)(query_id id, enum query_response res, void *pw);
	void (*cancel)(query_id, enum query_response res, void *pw);
} query_callback;


query_id query_user_xy(const char *query, const char *detail,
	const query_callback *cb, void *pw, const char *yes, const char *no,
	int x, int y);
void ro_gui_query_init(void);
void ro_gui_query_window_bring_to_front(query_id id);

query_id query_user(const char *query, const char *detail,
	const query_callback *cb, void *pw, const char *yes, const char *no);
void query_close(query_id);

#endif
