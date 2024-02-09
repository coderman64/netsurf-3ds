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
 */

#include <assert.h>
#include <inttypes.h>

#include "utils/log.h"
#include "utils/messages.h"
#include "netsurf/mouse.h"
#include "desktop/global_history.h"

#include "atari/treeview.h"
#include "atari/history.h"
#include "atari/gemtk/gemtk.h"
#include "atari/res/netsurf.rsh"

extern GRECT desk_area;

struct atari_global_history_s atari_global_history;

/* Setup Atari Treeview Callbacks: */

static nserror
atari_global_history_init_phase2(struct core_window *cw,
				 struct core_window_callback_table *cb_t)
{
	NSLOG(netsurf, INFO, "cw %p", cw);
	return(global_history_init(cb_t, cw));
}

static void atari_global_history_finish(struct core_window *cw)
{
	NSLOG(netsurf, INFO, "cw %p", cw);
	global_history_fini();
}

static void atari_global_history_draw(struct core_window *cw, int x,
				      int y, struct rect *clip,
				      const struct redraw_context *ctx)
{
	global_history_redraw(x, y, clip, ctx);
}

static void atari_global_history_keypress(struct core_window *cw, uint32_t ucs4)
{
	NSLOG(netsurf, INFO, "ucs4: %"PRIu32, ucs4);
	global_history_keypress(ucs4);
}

static void
atari_global_history_mouse_action(struct core_window *cw,
				  browser_mouse_state mouse,
				  int x, int y)
{
	NSLOG(netsurf, INFO, "x:  %d, y: %d\n", x, y);

	global_history_mouse_action(mouse, x, y);
}

void atari_global_history_close(void)
{
	atari_treeview_close(atari_global_history.tv);
}


static short handle_event(GUIWIN *win, EVMULT_OUT *ev_out, short msg[8])
{
	short retval = 0;

	NSLOG(netsurf, INFO, "win %p", win);

	if (ev_out->emo_events & MU_MESAG) {
		switch (msg[0]) {
		case WM_CLOSED:
			atari_global_history_close();
			retval = 1;
			break;

		default: break;
		}
	}

	return(retval);
}

static struct atari_treeview_callbacks atari_global_history_treeview_callbacks = {
	.init_phase2 = atari_global_history_init_phase2,
	.finish = atari_global_history_finish,
	.draw = atari_global_history_draw,
	.keypress = atari_global_history_keypress,
	.mouse_action = atari_global_history_mouse_action,
	.gemtk_user_func = handle_event
};

void atari_global_history_init(void)
{
	if (atari_global_history.init == false) {
		if( atari_global_history.window == NULL ) {
			int flags = ATARI_TREEVIEW_WIDGETS;
			short handle = -1;
			OBJECT * tree = gemtk_obj_get_tree(TOOLBAR_HISTORY);
			assert( tree );

			handle = wind_create(flags, 0, 0, desk_area.g_w, desk_area.g_h);
			atari_global_history.window = gemtk_wm_add(handle, GEMTK_WM_FLAG_DEFAULTS, NULL);
			if( atari_global_history.window == NULL ) {
				gemtk_msg_box_show(GEMTK_MSG_BOX_ALERT,
						   "Failed to allocate History");
				return;
			}
			wind_set_str(handle, WF_NAME, (char*)messages_get("History"));
			gemtk_wm_set_toolbar(atari_global_history.window, tree, 0, 0);
			gemtk_wm_unlink(atari_global_history.window);

			atari_global_history.tv = atari_treeview_create(
				atari_global_history.window,
				&atari_global_history_treeview_callbacks,
				NULL, flags);

			if (atari_global_history.tv == NULL) {
				/* handle it properly, clean up previous allocs */
				NSLOG(netsurf, INFO,
				      "Failed to allocate treeview");
				return;
			}
		}
	}
	atari_global_history.init = true;
}

void atari_global_history_open(void)
{
	assert(atari_global_history.init);

	if (atari_global_history.init == false) {
		return;
	}

	if (atari_treeview_is_open(atari_global_history.tv) == false) {

		GRECT pos;
		pos.g_x = desk_area.g_w - desk_area.g_w / 4;
		pos.g_y = desk_area.g_y;
		pos.g_w = desk_area.g_w / 4;
		pos.g_h = desk_area.g_h;

		atari_treeview_open(atari_global_history.tv, &pos);
	} else {
		wind_set(gemtk_wm_get_handle(atari_global_history.window), WF_TOP, 1, 0, 0, 0);
	}
}


void atari_global_history_destroy(void)
{

	if ( atari_global_history.init == false) {
		return;
	}

	if ( atari_global_history.window != NULL ) {
		if (atari_treeview_is_open(atari_global_history.tv))
			atari_global_history_close();
		wind_delete(gemtk_wm_get_handle(atari_global_history.window));
		gemtk_wm_remove(atari_global_history.window);
		atari_global_history.window = NULL;
		atari_treeview_delete(atari_global_history.tv);
		atari_global_history.init = false;
	}
	NSLOG(netsurf, INFO, "done");
}

void atari_global_history_redraw(void)
{
	atari_treeview_redraw(atari_global_history.tv);
}
