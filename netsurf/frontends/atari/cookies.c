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

#include "netsurf/inttypes.h"
#include "utils/log.h"
#include "utils/messages.h"
#include "netsurf/mouse.h"
#include "netsurf/plotters.h"
#include "desktop/cookie_manager.h"

#include "atari/treeview.h"
#include "atari/cookies.h"
#include "atari/gemtk/gemtk.h"
#include "atari/res/netsurf.rsh"

extern GRECT desk_area;

struct atari_cookie_manager_s atari_cookie_manager;


/* Setup Atari Treeview Callbacks: */
static nserror atari_cookie_manager_init_phase2(struct core_window *cw,
				struct core_window_callback_table * default_callbacks);
static void atari_cookie_manager_finish(struct core_window *cw);
static void atari_cookie_manager_keypress(struct core_window *cw,
												uint32_t ucs4);
static void atari_cookie_manager_mouse_action(struct core_window *cw,
												browser_mouse_state mouse,
												int x, int y);
static void atari_cookie_manager_draw(struct core_window *cw, int x,
											int y, struct rect *clip,
											const struct redraw_context *ctx);
static short handle_event(GUIWIN *win, EVMULT_OUT *ev_out, short msg[8]);

static struct atari_treeview_callbacks atari_cookie_manager_treeview_callbacks = {
	.init_phase2 = atari_cookie_manager_init_phase2,
	.finish = atari_cookie_manager_finish,
	.draw = atari_cookie_manager_draw,
	.keypress = atari_cookie_manager_keypress,
	.mouse_action = atari_cookie_manager_mouse_action,
	.gemtk_user_func = handle_event
};


static nserror
atari_cookie_manager_init_phase2(struct core_window *cw,
                                 struct core_window_callback_table *cb_t)
{
	NSLOG(netsurf, INFO, "cw %p", cw);
	return(cookie_manager_init(cb_t, cw));
}


static void
atari_cookie_manager_finish(struct core_window *cw)
{
	NSLOG(netsurf, INFO, "cw %p", cw);
	cookie_manager_fini();
}


static void
atari_cookie_manager_draw(struct core_window *cw,
                          int x, int y,
                          struct rect *clip,
                          const struct redraw_context *ctx)
{
	cookie_manager_redraw(x, y, clip, ctx);
}


static void
atari_cookie_manager_keypress(struct core_window *cw, uint32_t ucs4)
{
	NSLOG(netsurf, INFO, "ucs4: %"PRIu32, ucs4);
	cookie_manager_keypress(ucs4);
}


static void
atari_cookie_manager_mouse_action(struct core_window *cw,
                                  browser_mouse_state mouse,
                                  int x, int y)
{
	cookie_manager_mouse_action(mouse, x, y);
}



static short handle_event(GUIWIN *win, EVMULT_OUT *ev_out, short msg[8])
{
	short retval = 0;

	NSLOG(netsurf, INFO, "win %p", win);

	if (ev_out->emo_events & MU_MESAG) {
		switch (msg[0]) {

			case WM_TOOLBAR:
				NSLOG(netsurf, INFO, "WM_TOOLBAR");
			break;

			case WM_CLOSED:
				atari_cookie_manager_close();
				retval = 1;
			break;

			default: break;
		}
	}

	return(retval);
}

void atari_cookie_manager_init(void)
{
	if (atari_cookie_manager.init == false) {

		if (atari_cookie_manager.window == NULL) {
			int flags = ATARI_TREEVIEW_WIDGETS;
			short handle = -1;
			OBJECT * tree = gemtk_obj_get_tree(TOOLBAR_COOKIES);
			assert( tree );

			handle = wind_create(flags, 0, 0, desk_area.g_w, desk_area.g_h);
			atari_cookie_manager.window = gemtk_wm_add(handle,
												GEMTK_WM_FLAG_DEFAULTS, NULL);
			if( atari_cookie_manager.window == NULL ) {
				gemtk_msg_box_show(GEMTK_MSG_BOX_ALERT,
									"Failed to allocate Treeview:\nCookies");
				return;
			}
			wind_set_str(handle, WF_NAME, (char*)messages_get("Cookies"));
			gemtk_wm_set_toolbar(atari_cookie_manager.window, tree, 0, 0);
			gemtk_wm_unlink(atari_cookie_manager.window);

			atari_cookie_manager.tv = atari_treeview_create(
										atari_cookie_manager.window,
										&atari_cookie_manager_treeview_callbacks,
										NULL, flags);

			if (atari_cookie_manager.tv == NULL) {
				/* handle it properly, clean up previous allocs */
				NSLOG(netsurf, INFO,
				      "Failed to allocate treeview");
				return;
			}

		} else {

		}
	}
	atari_cookie_manager.init = true;
}

void atari_cookie_manager_open(void)
{
	assert(atari_cookie_manager.init);

	if (atari_treeview_is_open(atari_cookie_manager.tv) == false) {

	    GRECT pos;
	    pos.g_x = desk_area.g_w - desk_area.g_w / 4;
	    pos.g_y = desk_area.g_y;
	    pos.g_w = desk_area.g_w / 4;
	    pos.g_h = desk_area.g_h;

		atari_treeview_open(atari_cookie_manager.tv, &pos);
	} else {
		wind_set(gemtk_wm_get_handle(atari_cookie_manager.window), WF_TOP, 1, 0,
				0, 0);
	}
}


void atari_cookie_manager_close(void)
{
	atari_treeview_close(atari_cookie_manager.tv);
}


void atari_cookie_manager_destroy(void)
{
	if( atari_cookie_manager.init == false) {
		return;
	}
	if( atari_cookie_manager.window != NULL ) {
		if (atari_treeview_is_open(atari_cookie_manager.tv))
			atari_cookie_manager_close();
		wind_delete(gemtk_wm_get_handle(atari_cookie_manager.window));
		gemtk_wm_remove(atari_cookie_manager.window);
		atari_cookie_manager.window = NULL;
		atari_treeview_delete(atari_cookie_manager.tv);
		atari_cookie_manager.init = false;
	}
	NSLOG(netsurf, INFO, "done");

}


void atari_cookie_manager_redraw(void)
{
	atari_treeview_redraw(atari_cookie_manager.tv);
}
