/*
 * Copyright 2017 Vincent Sanders <vince@netsurf-browser.org>
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
 * Implementation of win32 local history interface.
 */

#include <stdint.h>
#include <stdlib.h>
#include <windows.h>

#include "utils/log.h"
#include "utils/nsoption.h"
#include "netsurf/keypress.h"
#include "netsurf/plotters.h"
#include "desktop/local_history.h"

#include "windows/plot.h"
#include "windows/corewindow.h"
#include "windows/local_history.h"


struct nsw32_local_history_window {
	struct nsw32_corewindow core;

	struct local_history_session *session;
};

static struct nsw32_local_history_window *local_history_window = NULL;

/**
 * callback for keypress on local_history window
 *
 * \param nsw32_cw The nsw32 core window structure.
 * \param nskey The netsurf key code
 * \return NSERROR_OK on success otherwise apropriate error code
 */
static nserror
nsw32_local_history_key(struct nsw32_corewindow *nsw32_cw, uint32_t nskey)
{
	struct nsw32_local_history_window *lhw;

	lhw = (struct nsw32_local_history_window *)nsw32_cw;

	if (local_history_keypress(lhw->session,nskey)) {
		return NSERROR_OK;
	}
	return NSERROR_NOT_IMPLEMENTED;
}

/**
 * callback for mouse action on local_history window
 *
 * \param nsw32_cw The nsw32 core window structure.
 * \param mouse_state netsurf mouse state on event
 * \param x location of event
 * \param y location of event
 * \return NSERROR_OK on success otherwise apropriate error code
 */
static nserror
nsw32_local_history_mouse(struct nsw32_corewindow *nsw32_cw,
		    browser_mouse_state mouse_state,
		    int x, int y)
{
	struct nsw32_local_history_window *lhw;

	lhw = (struct nsw32_local_history_window *)nsw32_cw;

	local_history_mouse_action(lhw->session, mouse_state, x, y);

	return NSERROR_OK;
}

/**
 * callback on draw event for local_history window
 *
 * \param nsw32_cw The nsw32 core window structure.
 * \param scrollx The horizontal scroll offset.
 * \param scrolly The vertical scroll offset.
 * \param r The rectangle of the window that needs updating.
 * \return NSERROR_OK on success otherwise apropriate error code
 */
static nserror
nsw32_local_history_draw(struct nsw32_corewindow *nsw32_cw,
			  int scrollx,
			  int scrolly,
			  struct rect *r)
{
	struct nsw32_local_history_window *lhw;
	struct redraw_context ctx = {
		.interactive = true,
		.background_images = true,
		.plot = &win_plotters
	};

	lhw = (struct nsw32_local_history_window *)nsw32_cw;

	local_history_redraw(lhw->session, -scrollx, -scrolly, r, &ctx);

	return NSERROR_OK;
}


static nserror
nsw32_local_history_close(struct nsw32_corewindow *nsw32_cw)
{
	ShowWindow(nsw32_cw->hWnd, SW_HIDE);

	return NSERROR_OK;
}

/**
 * Creates the window for the local_history tree.
 *
 * \return NSERROR_OK on success else appropriate error code on faliure.
 */
static nserror
nsw32_local_history_init(HINSTANCE hInstance,
			 struct browser_window *bw,
			 struct nsw32_local_history_window **win_out)
{
	struct nsw32_local_history_window *ncwin;
	nserror res;

	if ((*win_out) != NULL) {
		res = local_history_set((*win_out)->session, bw);
		return res;
	}

	ncwin = calloc(1, sizeof(*ncwin));
	if (ncwin == NULL) {
		return NSERROR_NOMEM;
	}

	ncwin->core.title = "NetSurf Local History";
	ncwin->core.draw = nsw32_local_history_draw;
	ncwin->core.key = nsw32_local_history_key;
	ncwin->core.mouse = nsw32_local_history_mouse;
	ncwin->core.close = nsw32_local_history_close;

	res = nsw32_corewindow_init(hInstance, NULL, &ncwin->core);
	if (res != NSERROR_OK) {
		free(ncwin);
		return res;
	}

	res = local_history_init(ncwin->core.cb_table,
				 (struct core_window *)ncwin,
				 bw,
				 &ncwin->session);
	if (res != NSERROR_OK) {
		free(ncwin);
		return res;
	}

	/* memoise window so it can be represented when necessary
	 * instead of recreating every time.
	 */
	*win_out = ncwin;

	return NSERROR_OK;
}


/* exported interface documented in windows/local_history.h */
nserror
nsw32_local_history_present(HWND hWndParent, struct browser_window *bw)
{
	nserror res;
	HINSTANCE hInstance;
	RECT parentr;
	int width, height;
	int margin = 50;

	hInstance = (HINSTANCE)GetWindowLongPtr(hWndParent, GWLP_HINSTANCE);

	res = nsw32_local_history_init(hInstance, bw, &local_history_window);
	if (res == NSERROR_OK) {
		GetWindowRect(hWndParent, &parentr);

		/* resize history widget ensureing the drawing area is
		 * no larger than parent window
		 */
		res = local_history_get_size(local_history_window->session,
					     &width,
					     &height);
		width += margin;
		height += margin;
		if ((parentr.right - parentr.left - margin) < width) {
			width = parentr.right - parentr.left - margin;
		}
		if ((parentr.bottom - parentr.top - margin) < height) {
			height = parentr.bottom - parentr.top - margin;
		}
		SetWindowPos(local_history_window->core.hWnd,
			     HWND_TOP,
			     parentr.left + (margin/2),
			     parentr.top + (margin/2),
			     width,
			     height,
			     SWP_SHOWWINDOW);
		local_history_scroll_to_cursor(local_history_window->session);
	}
	return res;
}


/* exported interface documented in windows/local_history.h */
nserror nsw32_local_history_hide(void)
{
	nserror res = NSERROR_OK;

	if (local_history_window != NULL) {
		ShowWindow(local_history_window->core.hWnd, SW_HIDE);

		res = local_history_set(local_history_window->session, NULL);
	}

	return res;
}

/* exported interface documented in windows/local_history.h */
nserror nsw32_local_history_finalise(void)
{
	nserror res;

	if (local_history_window == NULL) {
		return NSERROR_OK;
	}

	res = local_history_fini(local_history_window->session);
	if (res == NSERROR_OK) {
		res = nsw32_corewindow_fini(&local_history_window->core);
		DestroyWindow(local_history_window->core.hWnd);
		free(local_history_window);
		local_history_window = NULL;
	}

	return res;
}
