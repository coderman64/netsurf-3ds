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

/**
 * \file
 * Implementation of Amiga local history using core windows.
 */

#include <stdint.h>
#include <stdlib.h>

#include <proto/intuition.h>

#include <classes/window.h>
#include <gadgets/button.h>
#include <gadgets/layout.h>
#include <gadgets/scroller.h>
#include <gadgets/space.h>
#include <images/label.h>

#include <intuition/icclass.h>
#include <reaction/reaction_macros.h>

#include "utils/log.h"
#include "netsurf/keypress.h"
#include "netsurf/plotters.h"
#include "desktop/local_history.h"
#include "utils/messages.h"
#include "utils/nsoption.h"
#include "utils/nsurl.h"

#include "amiga/corewindow.h"
#include "amiga/gui.h"
#include "amiga/libs.h"
#include "amiga/history_local.h"
#include "amiga/utf8.h"


/**
 * Amiga local history viewing window context
 */
struct ami_history_local_window {
	/** Amiga core window context */
	struct ami_corewindow core;

	/** Amiga GUI stuff */
	struct gui_window *gw;

	/** local history viewer context data */
	struct local_history_session *session;
};

static struct ami_history_local_window *history_local_window = NULL;

/**
 * destroy a previously created local history view
 */
nserror
ami_history_local_destroy(struct ami_history_local_window *history_local_win)
{
	nserror res;

	if (history_local_win == NULL) {
		return NSERROR_OK;
	}

	res = local_history_fini(history_local_win->session);
	if (res == NSERROR_OK) {
		ami_gui_set_history_window(history_local_win->gw, NULL);
		res = ami_corewindow_fini(&history_local_win->core); /* closes the window for us */
		history_local_window = NULL;
	}
	return res;
}

static void ami_history_local_destroy_cw(struct ami_corewindow *ami_cw)
{
	struct ami_history_local_window *history_local_win;
	/* technically degenerate container of */
	history_local_win = (struct ami_history_local_window *)ami_cw;

	ami_history_local_destroy(history_local_win);
}

/**
 * callback for mouse action for local history on core window
 *
 * \param ami_cw The Amiga core window structure.
 * \param mouse_state netsurf mouse state on event
 * \param x location of event
 * \param y location of event
 * \return NSERROR_OK on success otherwise apropriate error code
 */
static nserror
ami_history_local_mouse(struct ami_corewindow *ami_cw,
					browser_mouse_state mouse_state,
					int x, int y)
{
	struct ami_history_local_window *history_local_win;
	/* technically degenerate container of */
	history_local_win = (struct ami_history_local_window *)ami_cw;

	nsurl *url;

	if(local_history_get_url(history_local_win->session, x, y, &url) == NSERROR_OK) {
		if (url == NULL) {
			SetGadgetAttrs(
				(struct Gadget *)ami_cw->objects[GID_CW_DRAW],
				ami_cw->win,
				NULL,
				GA_HintInfo,
				NULL,
				TAG_DONE);
		} else {
			SetGadgetAttrs(
				(struct Gadget *)ami_cw->objects[GID_CW_DRAW],
				ami_cw->win,
				NULL,
				GA_HintInfo,
				nsurl_access(url),
				TAG_DONE);
			nsurl_unref(url);
		}
	}

	local_history_mouse_action(history_local_win->session, mouse_state, x, y);

	return NSERROR_OK;
}

/**
 * callback for keypress for local history on core window
 *
 * \param ami_cw The Amiga core window structure.
 * \param nskey The netsurf key code
 * \return NSERROR_OK on success otherwise apropriate error code
 */
static nserror
ami_history_local_key(struct ami_corewindow *ami_cw, uint32_t nskey)
{
	struct ami_history_local_window *history_local_win;

	/* technically degenerate container of */
	history_local_win = (struct ami_history_local_window *)ami_cw;

	if (local_history_keypress(history_local_win->session, nskey)) {
			return NSERROR_OK;
	}
	return NSERROR_NOT_IMPLEMENTED;
}

/**
 * callback on draw event for certificate verify on core window
 *
 * \param ami_cw The Amiga core window structure.
 * \param x the x coordinate to draw
 * \param y the y coordinate to draw
 * \param r The rectangle of the window that needs updating.
 * \param ctx The drawing context
 * \return NSERROR_OK on success otherwise apropriate error code
 */
static nserror
ami_history_local_draw(struct ami_corewindow *ami_cw, int x, int y, struct rect *r, struct redraw_context *ctx)
{
	struct ami_history_local_window *history_local_win;

	/* technically degenerate container of */
	history_local_win = (struct ami_history_local_window *)ami_cw;

	//ctx->plot->clip(ctx, r); //??
	local_history_redraw(history_local_win->session, x, y, r, ctx);

	return NSERROR_OK;
}

static nserror
ami_history_local_create_window(struct ami_history_local_window *history_local_win)
{
	struct ami_corewindow *ami_cw = (struct ami_corewindow *)&history_local_win->core;
	ULONG refresh_mode = WA_SmartRefresh;

	if(nsoption_bool(window_simple_refresh) == true) {
		refresh_mode = WA_SimpleRefresh;
	}

	ami_cw->objects[GID_CW_WIN] = WindowObj,
  	    WA_ScreenTitle, ami_gui_get_screen_title(),
       	WA_Title, ami_cw->wintitle,
       	WA_Activate, TRUE,
       	WA_DepthGadget, TRUE,
       	WA_DragBar, TRUE,
       	WA_CloseGadget, TRUE,
       	WA_SizeGadget, TRUE,
		WA_SizeBRight, TRUE,
		WA_Width, 100,
		WA_Height, 100,
		WA_PubScreen, ami_gui_get_screen(),
		WA_ReportMouse, TRUE,
		refresh_mode, TRUE,
		WA_IDCMP, IDCMP_MOUSEMOVE | IDCMP_MOUSEBUTTONS | IDCMP_NEWSIZE |
				IDCMP_RAWKEY | IDCMP_GADGETUP | IDCMP_IDCMPUPDATE |
				IDCMP_EXTENDEDMOUSE | IDCMP_SIZEVERIFY | IDCMP_REFRESHWINDOW,
		WINDOW_IDCMPHook, &ami_cw->idcmp_hook,
		WINDOW_IDCMPHookBits, IDCMP_IDCMPUPDATE | IDCMP_EXTENDEDMOUSE |
				IDCMP_SIZEVERIFY | IDCMP_REFRESHWINDOW,
		WINDOW_SharedPort, ami_gui_get_shared_msgport(),
		WINDOW_HorizProp, 1,
		WINDOW_VertProp, 1,
		WINDOW_UserData, history_local_win,
//		WINDOW_MenuStrip, NULL,
		WINDOW_MenuUserData, WGUD_HOOK,
		WINDOW_IconifyGadget, FALSE,
		WINDOW_Position, WPOS_CENTERSCREEN,
		WINDOW_ParentGroup, ami_cw->objects[GID_CW_MAIN] = LayoutVObj,
			LAYOUT_AddChild, ami_cw->objects[GID_CW_DRAW] = SpaceObj,
				GA_ID, GID_CW_DRAW,
				SPACE_Transparent, TRUE,
				SPACE_BevelStyle, BVS_DISPLAY,
				GA_RelVerify, TRUE,
   			SpaceEnd,
		EndGroup,
	EndWindow;

	if(ami_cw->objects[GID_CW_WIN] == NULL) {
		return NSERROR_NOMEM;
	}

	return NSERROR_OK;
}

/* exported interface documented in amiga/history_local.h */
nserror ami_history_local_present(struct gui_window *gw)
{
	struct ami_history_local_window *ncwin;
	nserror res;
	int width, height;

	if(history_local_window != NULL) {
		//windowtofront()

		if (ami_gui_get_history_window(gw) != NULL) {
			res = local_history_set(ami_gui_get_history_window(gw)->session, ami_gui_get_browser_window(gw));
			return res;
		}

		return NSERROR_OK;
	}

	ncwin = calloc(1, sizeof(struct ami_history_local_window));
	if (ncwin == NULL) {
		return NSERROR_NOMEM;
	}

	ncwin->core.wintitle = ami_utf8_easy((char *)messages_get("History"));

	res = ami_history_local_create_window(ncwin);
	if (res != NSERROR_OK) {
		NSLOG(netsurf, INFO, "SSL UI builder init failed");
		ami_utf8_free(ncwin->core.wintitle);
		free(ncwin);
		return res;
	}

	/* initialise Amiga core window */
	ncwin->core.draw = ami_history_local_draw;
	ncwin->core.key = ami_history_local_key;
	ncwin->core.mouse = ami_history_local_mouse;
	ncwin->core.close = ami_history_local_destroy_cw;
	ncwin->core.event = NULL;
	ncwin->core.drag_end = NULL;
	ncwin->core.icon_drop = NULL;

	res = ami_corewindow_init(&ncwin->core);
	if (res != NSERROR_OK) {
		ami_utf8_free(ncwin->core.wintitle);
		DisposeObject(ncwin->core.objects[GID_CW_WIN]);
		free(ncwin);
		return res;
	}

	res = local_history_init(ncwin->core.cb_table,
				 (struct core_window *)ncwin,
				 ami_gui_get_browser_window(gw),
				 &ncwin->session);
	if (res != NSERROR_OK) {
		ami_utf8_free(ncwin->core.wintitle);
		DisposeObject(ncwin->core.objects[GID_CW_WIN]);
		free(ncwin);
		return res;
	}

	res = local_history_get_size(ncwin->session,
					     &width,
					     &height);

	/*TODO: Adjust these to account for window borders */

	SetAttrs(ncwin->core.objects[GID_CW_WIN],
		WA_Width, width,
		WA_Height, height,
		TAG_DONE);

	ncwin->gw = gw;
	history_local_window = ncwin;
	ami_gui_set_history_window(gw, ncwin);

	local_history_scroll_to_cursor(ncwin->session);

	return NSERROR_OK;
}

