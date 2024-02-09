/*
 * Copyright 2016 Vincent Sanders <vince@netsurf-browser.org>
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
 * win32 generic core window implementation.
 *
 * Provides interface for core renderers to a win32 api client area.
 *
 * This module is an object that must be encapsulated. Client users
 * should embed a struct nsw32_corewindow at the beginning of their
 * context for this display surface, fill in relevant data and then
 * call nsw32_corewindow_init()
 *
 * The win32 core window structure requires the callback for draw, key
 * and mouse operations.
 */

#include <assert.h>
#include <string.h>
#include <math.h>
#include <windows.h>
#include <windowsx.h>

#include "utils/log.h"
#include "utils/messages.h"
#include "utils/utf8.h"
#include "netsurf/types.h"
#include "netsurf/keypress.h"
#include "netsurf/mouse.h"

#include "windows/windbg.h"
#include "windows/corewindow.h"
#include "windows/plot.h"

static const char windowclassname_corewindow[] = "nswscorewindowwindow";

/**
 * update the scrollbar visibility and size
 */
static void
update_scrollbars(struct nsw32_corewindow *nsw32_cw)
{
	RECT rc;
	SCROLLINFO si;

	GetClientRect(nsw32_cw->hWnd, &rc);

	if (nsw32_cw->content_width > rc.right) {
		/* content wider than window area */
		if (nsw32_cw->content_height > rc.bottom) {
			/* content higher than window area */
			ShowScrollBar(nsw32_cw->hWnd, SB_BOTH, TRUE);
			si.cbSize = sizeof(SCROLLINFO);
			si.fMask = SIF_RANGE | SIF_PAGE;
			si.nMin = 0;
			si.nMax = nsw32_cw->content_width;
			si.nPage = rc.right;
			SetScrollInfo(nsw32_cw->hWnd, SB_HORZ, &si, TRUE);
			si.cbSize = sizeof(SCROLLINFO);
			si.fMask = SIF_RANGE | SIF_PAGE;
			si.nMin = 0;
			si.nMax = nsw32_cw->content_height;
			si.nPage = rc.bottom;
			SetScrollInfo(nsw32_cw->hWnd, SB_VERT, &si, TRUE);
		} else {
			/* content shorter than window area */
			ShowScrollBar(nsw32_cw->hWnd, SB_VERT, FALSE);
			ShowScrollBar(nsw32_cw->hWnd, SB_HORZ, TRUE);
			si.cbSize = sizeof(SCROLLINFO);
			si.fMask = SIF_RANGE | SIF_PAGE;
			si.nMin = 0;
			si.nMax = nsw32_cw->content_width;
			si.nPage = rc.right;
			SetScrollInfo(nsw32_cw->hWnd, SB_HORZ, &si, TRUE);
		}
	} else {
		/* content narrower than window area */
		if (nsw32_cw->content_height > rc.bottom) {
			/* content higher than window area */
			ShowScrollBar(nsw32_cw->hWnd, SB_HORZ, FALSE);
			ShowScrollBar(nsw32_cw->hWnd, SB_VERT, TRUE);
			si.cbSize = sizeof(SCROLLINFO);
			si.fMask = SIF_RANGE | SIF_PAGE;
			si.nMin = 0;
			si.nMax = nsw32_cw->content_height;
			si.nPage = rc.bottom;
			SetScrollInfo(nsw32_cw->hWnd, SB_VERT, &si, TRUE);
		} else {
			/* content shorter than window area */
			ShowScrollBar(nsw32_cw->hWnd, SB_BOTH, FALSE);
		}
	}

}


/**
 * Handle paint messages.
 */
static LRESULT
nsw32_corewindow_paint(struct nsw32_corewindow *nsw32_cw, HWND hwnd)
{
	struct rect clip;
	PAINTSTRUCT ps;
	SCROLLINFO si; /* scroll information */
	int scrollx;
	int scrolly;

	/* get scroll positions */
	si.cbSize = sizeof(si);
	si.fMask = SIF_POS;
	GetScrollInfo(hwnd, SB_HORZ, &si);
	scrollx = si.nPos;
	GetScrollInfo(hwnd, SB_VERT, &si);
	scrolly = si.nPos;

	BeginPaint(hwnd, &ps);

	plot_hdc = ps.hdc;

	/* content clip rectangle setup */
	clip.x0 = ps.rcPaint.left + scrollx;
	clip.y0 = ps.rcPaint.top + scrolly;
	clip.x1 = ps.rcPaint.right + scrollx;
	clip.y1 = ps.rcPaint.bottom + scrolly;

	nsw32_cw->draw(nsw32_cw, scrollx, scrolly, &clip);

	EndPaint(hwnd, &ps);

	return 0;
}

static LRESULT
nsw32_corewindow_vscroll(struct nsw32_corewindow *nsw32_cw,
			 HWND hwnd,
			 WPARAM wparam)
{
	SCROLLINFO si; /* current scroll information */
	SCROLLINFO usi; /* updated scroll infomation for scrollwindowex */

	NSLOG(netsurf, INFO, "VSCROLL");

	si.cbSize = sizeof(si);
	si.fMask = SIF_ALL;
	GetScrollInfo(hwnd, SB_VERT, &si);
	usi = si;

	switch (LOWORD(wparam))	{
	case SB_TOP:
		usi.nPos = si.nMin;
		break;

	case SB_BOTTOM:
		usi.nPos = si.nMax;
		break;

	case SB_LINEUP:
		usi.nPos -= 30;
		break;

	case SB_LINEDOWN:
		usi.nPos += 30;
		break;

	case SB_PAGEUP:
		usi.nPos -= si.nPage;
		break;

	case SB_PAGEDOWN:
		usi.nPos += si.nPage;
		break;

	case SB_THUMBTRACK:
		usi.nPos = si.nTrackPos;
		break;

	default:
		break;
	}

	if (usi.nPos < si.nMin) {
		usi.nPos = si.nMin;
	}
	if (usi.nPos > si.nMax) {
		usi.nPos = si.nMax;
	}

	SetScrollInfo(hwnd, SB_VERT, &usi, TRUE);

	ScrollWindowEx(hwnd,
		       0,
		       si.nPos - usi.nPos,
		       NULL,
		       NULL,
		       NULL,
		       NULL,
		       SW_INVALIDATE);

	/**
	 * /todo win32 corewindow vertical scrolling needs us to
	 * compute scroll values and call scrollwindowex()
	 */

	return 0;
}


static LRESULT
nsw32_corewindow_hscroll(struct nsw32_corewindow *nsw32_cw,
			 HWND hwnd,
			 WPARAM wparam)
{
	SCROLLINFO si; /* current scroll information */
	SCROLLINFO usi; /* updated scroll infomation for scrollwindowex */

	NSLOG(netsurf, INFO, "VSCROLL");

	si.cbSize = sizeof(si);
	si.fMask = SIF_ALL;
	GetScrollInfo(hwnd, SB_HORZ, &si);
	usi = si;

	switch (LOWORD(wparam))	{
	case SB_LINELEFT:
		usi.nPos -= 30;
		break;

	case SB_LINERIGHT:
		usi.nPos += 30;
		break;

	case SB_PAGELEFT:
		usi.nPos -= si.nPage;
		break;

	case SB_PAGERIGHT:
		usi.nPos += si.nPage;
		break;

	case SB_THUMBTRACK:
		usi.nPos = si.nTrackPos;
		break;

	default:
		break;
	}

	if (usi.nPos < si.nMin) {
		usi.nPos = si.nMin;
	}
	if (usi.nPos > si.nMax) {
		usi.nPos = si.nMax;
	}

	SetScrollInfo(hwnd, SB_HORZ, &usi, TRUE);

	ScrollWindowEx(hwnd,
		       si.nPos - usi.nPos,
		       0,
		       NULL,
		       NULL,
		       NULL,
		       NULL,
		       SW_INVALIDATE);

	return 0;
}


static LRESULT
nsw32_corewindow_mousedown(struct nsw32_corewindow *nsw32_cw,
			 HWND hwnd,
			   int x, int y,
			   browser_mouse_state button)
{
	SCROLLINFO si; /* scroll information */

	/* get scroll positions */
	si.cbSize = sizeof(si);
	si.fMask = SIF_POS;
	GetScrollInfo(hwnd, SB_HORZ, &si);
	x += si.nPos;
	GetScrollInfo(hwnd, SB_VERT, &si);
	y += si.nPos;

	nsw32_cw->mouse(nsw32_cw, button, x, y);
	return 0;
}

static LRESULT
nsw32_corewindow_mouseup(struct nsw32_corewindow *nsw32_cw,
			 HWND hwnd,
			 int x, int y,
			 browser_mouse_state button)
{
	SCROLLINFO si; /* scroll information */

	/* get scroll positions */
	si.cbSize = sizeof(si);
	si.fMask = SIF_POS;
	GetScrollInfo(hwnd, SB_HORZ, &si);
	x += si.nPos;
	GetScrollInfo(hwnd, SB_VERT, &si);
	y += si.nPos;

	nsw32_cw->mouse(nsw32_cw, button, x, y);
	return 0;
}

static LRESULT
nsw32_corewindow_close(struct nsw32_corewindow *nsw32_cw)
{
	nsw32_cw->close(nsw32_cw);
	return 0;
}

/**
 * callback for core window win32 events
 *
 * \param hwnd The win32 window handle
 * \param msg The win32 message identifier
 * \param wparam The w win32 parameter
 * \param lparam The l win32 parameter
 */
static LRESULT CALLBACK
nsw32_window_corewindow_event_callback(HWND hwnd,
				    UINT msg,
				    WPARAM wparam,
				    LPARAM lparam)
{
	struct nsw32_corewindow *nsw32_cw;

	nsw32_cw = GetProp(hwnd, TEXT("CoreWnd"));
	if (nsw32_cw != NULL) {
		switch (msg) {
		case WM_PAINT: /* redraw the exposed part of the window */
			return nsw32_corewindow_paint(nsw32_cw, hwnd);

		case WM_SIZE:
			update_scrollbars(nsw32_cw);
			break;

		case WM_VSCROLL:
			return nsw32_corewindow_vscroll(nsw32_cw, hwnd, wparam);

		case WM_HSCROLL:
			return nsw32_corewindow_hscroll(nsw32_cw, hwnd, wparam);

		case WM_LBUTTONDOWN:
			return nsw32_corewindow_mousedown(nsw32_cw, hwnd,
							  GET_X_LPARAM(lparam),
							  GET_Y_LPARAM(lparam),
							  BROWSER_MOUSE_PRESS_1);

		case WM_RBUTTONDOWN:
			return nsw32_corewindow_mousedown(nsw32_cw, hwnd,
							   GET_X_LPARAM(lparam),
							   GET_Y_LPARAM(lparam),
							   BROWSER_MOUSE_PRESS_2);

		case WM_LBUTTONUP:
			return nsw32_corewindow_mouseup(nsw32_cw, hwnd,
							GET_X_LPARAM(lparam),
							GET_Y_LPARAM(lparam),
							BROWSER_MOUSE_CLICK_1);

		case WM_RBUTTONUP:
			return nsw32_corewindow_mouseup(nsw32_cw, hwnd,
							GET_X_LPARAM(lparam),
							GET_Y_LPARAM(lparam),
							BROWSER_MOUSE_CLICK_2);

		case WM_CLOSE:
			return nsw32_corewindow_close(nsw32_cw);
		}
	}

	return DefWindowProc(hwnd, msg, wparam, lparam);
}


/**
 * callback from core to request an invalidation of a window area.
 *
 * The specified area of the window should now be considered
 *  out of date. If the area is NULL the entire window must be
 *  invalidated.
 *
 * \param[in] cw The core window to invalidate.
 * \param[in] rect area to redraw or NULL for the entire window area.
 * \return NSERROR_OK on success or appropriate error code.
 */
static nserror
nsw32_cw_invalidate_area(struct core_window *cw, const struct rect *rect)
{
	struct nsw32_corewindow *nsw32_cw = (struct nsw32_corewindow *)cw;
	RECT *redrawrectp = NULL;
	RECT redrawrect;

	if (rect != NULL) {
		SCROLLINFO si; /* scroll information */

		/* get scroll positions */
		si.cbSize = sizeof(si);
		si.fMask = SIF_POS;
		GetScrollInfo(nsw32_cw->hWnd, SB_HORZ, &si);
		redrawrect.left = (long)rect->x0 - si.nPos;
		redrawrect.right = (long)rect->x1 - si.nPos;

		GetScrollInfo(nsw32_cw->hWnd, SB_VERT, &si);
		redrawrect.top = (long)rect->y0 - si.nPos;
		redrawrect.bottom = (long)rect->y1 - si.nPos;

		redrawrectp = &redrawrect;
	}

	RedrawWindow(nsw32_cw->hWnd,
		     redrawrectp,
		     NULL,
		     RDW_INVALIDATE | RDW_NOERASE);

	return NSERROR_OK;
}


/**
 * Callback from the core to update the content area size
 */
static nserror
nsw32_cw_update_size(struct core_window *cw, int width, int height)
{
	struct nsw32_corewindow *nsw32_cw = (struct nsw32_corewindow *)cw;

	nsw32_cw->content_width = width;
	nsw32_cw->content_height = height;
	NSLOG(netsurf, INFO, "new content size w:%d h:%d", width, height);

	update_scrollbars(nsw32_cw);
	return NSERROR_OK;
}


static nserror
nsw32_cw_set_scroll(struct core_window *cw, int x, int y)
{
	/** /todo call setscroll apropriately */
	return NSERROR_OK;
}


static nserror
nsw32_cw_get_scroll(const struct core_window *cw, int *x, int *y)
{
	/** /todo call getscroll apropriately */
	return NSERROR_NOT_IMPLEMENTED;
}


/**
 * Callback from the core to obtain the window viewport dimensions
 *
 * \param[in] cw the core window object
 * \param[out] width to be set to viewport width in px
 * \param[out] height to be set to viewport height in px
 */
static nserror
nsw32_cw_get_window_dimensions(const struct core_window *cw,
		int *width, int *height)
{
	struct nsw32_corewindow *nsw32_cw = (struct nsw32_corewindow *)cw;

	RECT rc;
	GetClientRect(nsw32_cw->hWnd, &rc);
	*width = rc.right;
	*height = rc.bottom;
	return NSERROR_OK;
}


static nserror
nsw32_cw_drag_status(struct core_window *cw, core_window_drag_status ds)
{
	struct nsw32_corewindow *nsw32_cw = (struct nsw32_corewindow *)cw;
	nsw32_cw->drag_status = ds;
	return NSERROR_OK;
}


struct core_window_callback_table nsw32_cw_cb_table = {
	.invalidate = nsw32_cw_invalidate_area,
	.update_size = nsw32_cw_update_size,
	.set_scroll = nsw32_cw_set_scroll,
	.get_scroll = nsw32_cw_get_scroll,
	.get_window_dimensions = nsw32_cw_get_window_dimensions,
	.drag_status = nsw32_cw_drag_status
};

/* exported function documented nsw32/corewindow.h */
nserror
nsw32_corewindow_init(HINSTANCE hInstance,
		      HWND hWndParent,
		      struct nsw32_corewindow *nsw32_cw)
{
	DWORD dwStyle;

	/* setup the core window callback table */
	nsw32_cw->cb_table = &nsw32_cw_cb_table;
	nsw32_cw->drag_status = CORE_WINDOW_DRAG_NONE;

	/* start with the content area being as small as possible */
	nsw32_cw->content_width = -1;
	nsw32_cw->content_height = -1;

	if (hWndParent != NULL) {
		dwStyle = WS_CHILDWINDOW |
			WS_VISIBLE |
			CS_DBLCLKS;
	} else {
		dwStyle = WS_OVERLAPPEDWINDOW |
			WS_HSCROLL |
			WS_VSCROLL |
			WS_CLIPSIBLINGS |
			WS_CLIPCHILDREN |
			CS_DBLCLKS;
	}

	NSLOG(netsurf, INFO, "creating hInstance %p core window", hInstance);
	nsw32_cw->hWnd = CreateWindowEx(0,
					windowclassname_corewindow,
					nsw32_cw->title,
					dwStyle,
					CW_USEDEFAULT,
					CW_USEDEFAULT,
					500,
					400,
					hWndParent,
					NULL,
					hInstance,
					NULL);
	if (nsw32_cw->hWnd == NULL) {
		NSLOG(netsurf, INFO, "Window create failed");
		return NSERROR_NOMEM;
	}

	SetProp(nsw32_cw->hWnd, TEXT("CoreWnd"), (HANDLE)nsw32_cw);

	/* zero scroll offsets */
	SCROLLINFO si;
	si.cbSize = sizeof(si);
	si.fMask = SIF_POS;
	si.nPos = 0;
	SetScrollInfo(nsw32_cw->hWnd, SB_VERT, &si, FALSE);
	SetScrollInfo(nsw32_cw->hWnd, SB_HORZ, &si, FALSE);

	return NSERROR_OK;
}

/* exported interface documented in nsw32/corewindow.h */
nserror nsw32_corewindow_fini(struct nsw32_corewindow *nsw32_cw)
{
	return NSERROR_OK;
}


/* exported interface documented in windows/corewindow.h */
nserror nsw32_create_corewindow_class(HINSTANCE hInstance)
{
	nserror ret = NSERROR_OK;
	WNDCLASSEX wc;

	/* drawable area */
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = 0;
	wc.lpfnWndProc = nsw32_window_corewindow_event_callback;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = NULL;
	wc.hCursor = NULL;
	wc.hbrBackground = (HBRUSH)(COLOR_MENU + 1);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = windowclassname_corewindow;
	wc.hIconSm = NULL;

	if (RegisterClassEx(&wc) == 0) {
		win_perror("CorewindowClass");
		ret = NSERROR_INIT_FAILED;
	}

	return ret;
}
