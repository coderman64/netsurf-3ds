/*
 * Copyright 2011 Vincent Sanders <vince@simtec.co.uk>
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
 * win32 implementation of drawable window showing browser context
 */

#include <stdbool.h>
#include <stdint.h>

#include "utils/config.h"

#include <windows.h>
#include <windowsx.h>

#include "utils/errors.h"
#include "utils/log.h"
#include "netsurf/browser_window.h"
#include "netsurf/plotters.h"
#include "netsurf/keypress.h"

#include "windows/windbg.h"
#include "windows/plot.h"
#include "windows/window.h"
#include "windows/local_history.h"
#include "windows/drawable.h"

static const wchar_t *windowclassname_drawable = L"nswsdrawablewindow";


/**
 * Handle wheel scroll messages.
 */
static LRESULT
nsws_drawable_wheel(struct gui_window *gw, HWND hwnd, WPARAM wparam)
{
	int i, z = GET_WHEEL_DELTA_WPARAM(wparam) / WHEEL_DELTA;
	int key = LOWORD(wparam);
	DWORD command;
	unsigned int newmessage = WM_VSCROLL;

	if (key == MK_SHIFT) {
		command = (z > 0) ? SB_LINERIGHT : SB_LINELEFT;
		newmessage = WM_HSCROLL;
	} else {
		/* add MK_CONTROL -> zoom */
		command = (z > 0) ? SB_LINEUP : SB_LINEDOWN;
	}

	z = (z < 0) ? -1 * z : z;

	for (i = 0; i < z; i++) {
		SendMessage(hwnd, newmessage, MAKELONG(command, 0), 0);
	}

	return 0;
}


/**
 * Handle vertical scroll messages.
 */
static LRESULT
nsws_drawable_vscroll(struct gui_window *gw, HWND hwnd, WPARAM wparam)
{
	int width, height;
	SCROLLINFO si;
	int mem;

	NSLOG(netsurf, INFO, "VSCROLL %d", gw->requestscrolly);

	if (gw->requestscrolly != 0)
		return 0;

	si.cbSize = sizeof(si);
	si.fMask = SIF_ALL;
	GetScrollInfo(hwnd, SB_VERT, &si);
	mem = si.nPos;

	switch (LOWORD(wparam))	{
	case SB_TOP:
		si.nPos = si.nMin;
		break;

	case SB_BOTTOM:
		si.nPos = si.nMax;
		break;

	case SB_LINEUP:
		si.nPos -= 30;
		break;

	case SB_LINEDOWN:
		si.nPos += 30;
		break;

	case SB_PAGEUP:
		si.nPos -= gw->height;
		break;

	case SB_PAGEDOWN:
		si.nPos += gw->height;
		break;

	case SB_THUMBTRACK:
		si.nPos = si.nTrackPos;
		break;

	default:
		break;
	}

	si.fMask = SIF_POS;
	if ((gw->bw != NULL) &&
	    (browser_window_get_extents(gw->bw, true,
					&width, &height) == NSERROR_OK)) {
		si.nPos = min(si.nPos, height - gw->height);
	}

	si.nPos = max(si.nPos, 0);
	SetScrollInfo(hwnd, SB_VERT, &si, TRUE);
	GetScrollInfo(hwnd, SB_VERT, &si);
	if (si.nPos != mem) {
		struct rect rect;
		rect.x0 = rect.x1 = gw->scrollx;
		rect.y0 = rect.y1 = gw->scrolly + gw->requestscrolly + si.nPos - mem;
		win32_window_set_scroll(gw, &rect);
	}

	return 0;
}


/**
 * Handle horizontal scroll messages.
 */
static LRESULT
nsws_drawable_hscroll(struct gui_window *gw, HWND hwnd, WPARAM wparam)
{
	int width, height;
	SCROLLINFO si;
	int mem;

	NSLOG(netsurf, INFO, "HSCROLL %d", gw->requestscrollx);

	if (gw->requestscrollx != 0)
		return 0;

	si.cbSize = sizeof(si);
	si.fMask = SIF_ALL;
	GetScrollInfo(hwnd, SB_HORZ, &si);
	mem = si.nPos;

	switch (LOWORD(wparam))	{
	case SB_LINELEFT:
		si.nPos -= 30;
		break;

	case SB_LINERIGHT:
		si.nPos += 30;
		break;

	case SB_PAGELEFT:
		si.nPos -= gw->width;
		break;

	case SB_PAGERIGHT:
		si.nPos += gw->width;
		break;

	case SB_THUMBTRACK:
		si.nPos = si.nTrackPos;
		break;

	default:
		break;
	}

	si.fMask = SIF_POS;

	if ((gw->bw != NULL) &&
	    (browser_window_get_extents(gw->bw, true,
					&width, &height) == NSERROR_OK)) {
		si.nPos = min(si.nPos, width - gw->width);
	}
	si.nPos = max(si.nPos, 0);
	SetScrollInfo(hwnd, SB_HORZ, &si, TRUE);
	GetScrollInfo(hwnd, SB_HORZ, &si);
	if (si.nPos != mem) {
		struct rect rect;
		rect.x0 = rect.x1 = gw->scrollx + gw->requestscrollx + si.nPos - mem;
		rect.y0 = rect.y1 = gw->scrolly;
		win32_window_set_scroll(gw, &rect);
	}

	return 0;
}


/**
 * Handle resize events.
 */
static LRESULT
nsws_drawable_resize(struct gui_window *gw)
{
	browser_window_schedule_reformat(gw->bw);
	return 0;
}

/**
 * Handle unicode character messages.
 */
static LRESULT
nsws_drawable_unichar(struct gui_window *gw, HWND hwnd, WPARAM wparam)
{
	uint32_t nskey;

	if (wparam == UNICODE_NOCHAR) {
		return 1;
	}

	nskey = wparam;
	browser_window_key_press(gw->bw, nskey);
	return 0;
}

/**
 * Handle character messages.
 *
 * WM_CHAR is generated when WM_KEYDOWN message are passed to
 * TranslateMessage; wParam is UTF-16.  If the codepoint is 4
 * bytes, there are 2 WM_CHAR message, one with the high
 * surrogate and one with the low surrogate.
 */
static LRESULT
nsws_drawable_char(struct gui_window *gw, HWND hwnd, WPARAM wparam)
{
	uint32_t nskey;

	nskey = wparam;

	const uint32_t utf16_hi_surrogate_start = 0xD800;
	const uint32_t utf16_lo_surrogate_start = 0xDC00;
	const uint32_t utf16_surrogate_end = 0xDFFF;

	static uint32_t highSurrogate = 0;

	if ((nskey >= utf16_hi_surrogate_start) &&
	    (nskey < utf16_lo_surrogate_start) ) {
		highSurrogate = nskey;
	} else {
		if ((nskey >= utf16_lo_surrogate_start) &&
		    (nskey <= utf16_surrogate_end)) {
			uint32_t lowSurrogate = nskey;
			nskey = (highSurrogate - utf16_hi_surrogate_start) << 10;
			nskey |= ( lowSurrogate - utf16_lo_surrogate_start );
			nskey += 0x10000;
		}
		highSurrogate = 0;

		browser_window_key_press(gw->bw, nskey);
	}

	return 0;
}

/**
 * Handle keydown messages.
 */
static LRESULT
nsws_drawable_keydown(struct gui_window *gw, HWND hwnd, WPARAM wparam)
{
	uint32_t i;
	bool shift = ((GetKeyState(VK_SHIFT) & 0x8000) == 0x8000);

	switch(wparam) {
	case VK_LEFT:
		i = NS_KEY_LEFT;
		if (shift)
			SendMessage(hwnd, WM_HSCROLL,
				    MAKELONG(SB_LINELEFT, 0), 0);
		break;

	case VK_RIGHT:
		i = NS_KEY_RIGHT;
		if (shift)
			SendMessage(hwnd, WM_HSCROLL,
				    MAKELONG(SB_LINERIGHT, 0), 0);
		break;

	case VK_UP:
		i = NS_KEY_UP;
		if (shift)
			SendMessage(hwnd, WM_VSCROLL,
				    MAKELONG(SB_LINEUP, 0), 0);
		break;

	case VK_DOWN:
		i = NS_KEY_DOWN;
		if (shift)
			SendMessage(hwnd, WM_VSCROLL,
				    MAKELONG(SB_LINEDOWN, 0), 0);
		break;

	case VK_HOME:
		i = NS_KEY_LINE_START;
		if (shift)
			SendMessage(hwnd, WM_HSCROLL,
				    MAKELONG(SB_PAGELEFT, 0), 0);
		break;

	case VK_END:
		i = NS_KEY_LINE_END;
		if (shift)
			SendMessage(hwnd, WM_HSCROLL,
				    MAKELONG(SB_PAGERIGHT, 0), 0);
		break;

	case VK_DELETE:
		i = NS_KEY_DELETE_RIGHT;
		break;

	case VK_NEXT:
		SendMessage(hwnd, WM_VSCROLL, MAKELONG(SB_PAGEDOWN, 0), 0);
		return 1;

	case VK_PRIOR:
		SendMessage(hwnd, WM_VSCROLL, MAKELONG(SB_PAGEUP, 0), 0);
		return 1;

	default:
		return 1;
	}

	browser_window_key_press(gw->bw, i);

	return 0;
}


/**
 * Handle paint messages.
 */
static LRESULT
nsws_drawable_paint(struct gui_window *gw, HWND hwnd)
{
	struct rect clip;
	PAINTSTRUCT ps;
	struct redraw_context ctx = {
		.interactive = true,
		.background_images = true,
		.plot = &win_plotters
	};

	BeginPaint(hwnd, &ps);

	if (gw != NULL) {
		plot_hdc = ps.hdc;

		clip.x0 = ps.rcPaint.left;
		clip.y0 = ps.rcPaint.top;
		clip.x1 = ps.rcPaint.right;
		clip.y1 = ps.rcPaint.bottom;

                /**
		 * \todo work out why the heck scroll needs scaling
		 */
		
		browser_window_redraw(gw->bw,
				      -gw->scrollx,
				      -gw->scrolly,
				      &clip,
				      &ctx);
	}

	EndPaint(hwnd, &ps);

	return 0;
}


/**
 * Handle mouse button up messages.
 */
static LRESULT
nsws_drawable_mouseup(struct gui_window *gw,
		      int x,
		      int y,
		      browser_mouse_state press,
		      browser_mouse_state click)
{
	bool shift = ((GetKeyState(VK_SHIFT) & 0x8000) == 0x8000);
	bool ctrl = ((GetKeyState(VK_CONTROL) & 0x8000) == 0x8000);
	bool alt = ((GetKeyState(VK_MENU) & 0x8000) == 0x8000);

	if ((gw == NULL) ||
	    (gw->mouse == NULL) ||
	    (gw->bw == NULL))
		return 0;

	NSLOG(netsurf, INFO, "state 0x%x, press 0x%x", gw->mouse->state,
	      press);
	if ((gw->mouse->state & press) != 0) {
		gw->mouse->state &= ~press;
		gw->mouse->state |= click;
	}

	if (((gw->mouse->state & BROWSER_MOUSE_MOD_1) != 0) && !shift)
		gw->mouse->state &= ~BROWSER_MOUSE_MOD_1;
	if (((gw->mouse->state & BROWSER_MOUSE_MOD_2) != 0) && !ctrl)
		gw->mouse->state &= ~BROWSER_MOUSE_MOD_2;
	if (((gw->mouse->state & BROWSER_MOUSE_MOD_3) != 0) && !alt)
		gw->mouse->state &= ~BROWSER_MOUSE_MOD_3;

	if ((gw->mouse->state & click) != 0) {
		NSLOG(netsurf, INFO,
		      "mouse click bw %p, state 0x%x, x %d, y %d",
		      gw->bw,
		      gw->mouse->state,
		      x + gw->scrollx,
		      y + gw->scrolly);

		browser_window_mouse_click(gw->bw,
					   gw->mouse->state,
					   x + gw->scrollx,
					   y + gw->scrolly);
	} else {
		browser_window_mouse_track(gw->bw,
					   0,
					   x + gw->scrollx,
					   y + gw->scrolly);
	}

	gw->mouse->state = 0;
	return 0;
}


/**
 * Handle mouse button down messages.
 */
static LRESULT
nsws_drawable_mousedown(struct gui_window *gw,
			int x, int y,
			browser_mouse_state button)
{
	if ((gw == NULL) ||
	    (gw->mouse == NULL) ||
	    (gw->bw == NULL)) {
		nsw32_local_history_hide();
		return 0;
	}

	gw->mouse->state = button;
	if ((GetKeyState(VK_SHIFT) & 0x8000) == 0x8000)
		gw->mouse->state |= BROWSER_MOUSE_MOD_1;
	if ((GetKeyState(VK_CONTROL) & 0x8000) == 0x8000)
		gw->mouse->state |= BROWSER_MOUSE_MOD_2;
	if ((GetKeyState(VK_MENU) & 0x8000) == 0x8000)
		gw->mouse->state |= BROWSER_MOUSE_MOD_3;

	gw->mouse->pressed_x = x + gw->scrollx;
	gw->mouse->pressed_y = y + gw->scrolly;

	NSLOG(netsurf, INFO, "mouse click bw %p, state %x, x %d, y %d",
	      gw->bw,
	      gw->mouse->state,
	      x + gw->scrollx,
	      y + gw->scrolly);

	browser_window_mouse_click(gw->bw,
				   gw->mouse->state,
				   x + gw->scrollx,
				   y + gw->scrolly);

	return 0;
}


/**
 * Handle mouse movement messages.
 */
static LRESULT
nsws_drawable_mousemove(struct gui_window *gw, int x, int y)
{
	bool shift = ((GetKeyState(VK_SHIFT) & 0x8000) == 0x8000);
	bool ctrl = ((GetKeyState(VK_CONTROL) & 0x8000) == 0x8000);
	bool alt = ((GetKeyState(VK_MENU) & 0x8000) == 0x8000);

	if ((gw == NULL) || (gw->mouse == NULL) || (gw->bw == NULL))
		return 0;

	/* add scroll offsets */
	x = x + gw->scrollx;
	y = y + gw->scrolly;

	/* if mouse button held down and pointer moved more than
	 * minimum distance drag is happening */
	if (((gw->mouse->state & (BROWSER_MOUSE_PRESS_1 | BROWSER_MOUSE_PRESS_2)) != 0) &&
	    (abs(x - gw->mouse->pressed_x) >= 5) &&
	    (abs(y - gw->mouse->pressed_y) >= 5)) {

		NSLOG(netsurf, INFO, "Drag start state 0x%x",
		      gw->mouse->state);

		if ((gw->mouse->state & BROWSER_MOUSE_PRESS_1) != 0) {
			browser_window_mouse_click(gw->bw, BROWSER_MOUSE_DRAG_1,
						   gw->mouse->pressed_x,
						   gw->mouse->pressed_y);
			gw->mouse->state &= ~BROWSER_MOUSE_PRESS_1;
			gw->mouse->state |= BROWSER_MOUSE_HOLDING_1 |
				BROWSER_MOUSE_DRAG_ON;
		}
		else if ((gw->mouse->state & BROWSER_MOUSE_PRESS_2) != 0) {
			browser_window_mouse_click(gw->bw, BROWSER_MOUSE_DRAG_2,
						   gw->mouse->pressed_x,
						   gw->mouse->pressed_y);
			gw->mouse->state &= ~BROWSER_MOUSE_PRESS_2;
			gw->mouse->state |= BROWSER_MOUSE_HOLDING_2 |
				BROWSER_MOUSE_DRAG_ON;
		}
	}

	if (((gw->mouse->state & BROWSER_MOUSE_MOD_1) != 0) && !shift)
		gw->mouse->state &= ~BROWSER_MOUSE_MOD_1;
	if (((gw->mouse->state & BROWSER_MOUSE_MOD_2) != 0) && !ctrl)
		gw->mouse->state &= ~BROWSER_MOUSE_MOD_2;
	if (((gw->mouse->state & BROWSER_MOUSE_MOD_3) != 0) && !alt)
		gw->mouse->state &= ~BROWSER_MOUSE_MOD_3;


	browser_window_mouse_track(gw->bw, gw->mouse->state, x, y);

	return 0;
}


/**
 * Called when activity occours within the drawable window.
 */
static LRESULT CALLBACK
nsws_window_drawable_event_callback(HWND hwnd,
				    UINT msg,
				    WPARAM wparam,
				    LPARAM lparam)
{
	struct gui_window *gw;

	LOG_WIN_MSG(hwnd, msg, wparam, lparam);

	gw = nsws_get_gui_window(hwnd);
	if (gw == NULL) {
		NSLOG(netsurf, INFO,
		      "Unable to find gui window structure for hwnd %p", hwnd);
		return DefWindowProc(hwnd, msg, wparam, lparam);
	}

	switch(msg) {

	case WM_MOUSEMOVE:
		return nsws_drawable_mousemove(gw,
					       GET_X_LPARAM(lparam),
					       GET_Y_LPARAM(lparam));

	case WM_LBUTTONDOWN:
		nsws_drawable_mousedown(gw,
					GET_X_LPARAM(lparam),
					GET_Y_LPARAM(lparam),
					BROWSER_MOUSE_PRESS_1);
		SetFocus(hwnd);
		nsw32_local_history_hide();
		return 0;

	case WM_RBUTTONDOWN:
		nsws_drawable_mousedown(gw,
					GET_X_LPARAM(lparam),
					GET_Y_LPARAM(lparam),
					BROWSER_MOUSE_PRESS_2);
		SetFocus(hwnd);
		return 0;

	case WM_LBUTTONUP:
		return nsws_drawable_mouseup(gw,
					     GET_X_LPARAM(lparam),
					     GET_Y_LPARAM(lparam),
					     BROWSER_MOUSE_PRESS_1,
					     BROWSER_MOUSE_CLICK_1);

	case WM_RBUTTONUP:
		return nsws_drawable_mouseup(gw,
					     GET_X_LPARAM(lparam),
					     GET_Y_LPARAM(lparam),
					     BROWSER_MOUSE_PRESS_2,
					     BROWSER_MOUSE_CLICK_2);

	case WM_ERASEBKGND: /* ignore as drawable window is redrawn on paint */
		return 0;

	case WM_PAINT: /* redraw the exposed part of the window */
		return nsws_drawable_paint(gw, hwnd);

	case WM_KEYDOWN:
		if (nsws_drawable_keydown(gw, hwnd, wparam) == 0) {
			return 0;
		}
		break;

	case WM_CHAR:
		return nsws_drawable_char(gw, hwnd, wparam);

	case WM_UNICHAR:
		return nsws_drawable_unichar(gw, hwnd, wparam);

	case WM_SIZE:
		return nsws_drawable_resize(gw);

	case WM_HSCROLL:
		return nsws_drawable_hscroll(gw, hwnd, wparam);

	case WM_VSCROLL:
		return nsws_drawable_vscroll(gw, hwnd, wparam);

	case WM_MOUSEWHEEL:
		return nsws_drawable_wheel(gw, hwnd, wparam);

	case WM_PASTE:
		browser_window_key_press(gw->bw, NS_KEY_PASTE);
		return 0;

	case WM_COPY:
		browser_window_key_press(gw->bw, NS_KEY_COPY_SELECTION);
		return 0;

	case WM_CUT:
		browser_window_key_press(gw->bw, NS_KEY_CUT_SELECTION);
		return 0;

	case WM_CLEAR:
		/**
		 * \todo win32 clear operation deletes the contents of
		 *       the selection but ns clear selection only
		 *       removes the highlight.
		 */
		browser_window_key_press(gw->bw, NS_KEY_CLEAR_SELECTION);
		return 0;



	}
	return DefWindowProc(hwnd, msg, wparam, lparam);
}


/**
 * Create a drawable window.
 */
HWND
nsws_window_create_drawable(HINSTANCE hinstance,
			    HWND hparent,
			    struct gui_window *gw)
{
	HWND hwnd;
	hwnd = CreateWindowExW(0,
			       windowclassname_drawable,
			       NULL,
			       WS_VISIBLE | WS_CHILD,
			       0, 0,
			       0, 0,
			       hparent,
			       NULL,
			       hinstance,
			       NULL);

	if (hwnd == NULL) {
		win_perror("WindowCreateDrawable");
		NSLOG(netsurf, INFO, "Window creation failed");
		return NULL;
	}

	/* set the gui window associated with this toolbar */
	SetProp(hwnd, TEXT("GuiWnd"), (HANDLE)gw);

	return hwnd;
}


/**
 * Create the drawable window class.
 */
nserror
nsws_create_drawable_class(HINSTANCE hinstance) {
	nserror ret = NSERROR_OK;
	WNDCLASSEXW w;

	/* drawable area */
	w.cbSize = sizeof(WNDCLASSEX);
	w.style	= 0;
	w.lpfnWndProc = nsws_window_drawable_event_callback;
	w.cbClsExtra = 0;
	w.cbWndExtra = 0;
	w.hInstance = hinstance;
	w.hIcon = NULL;
	w.hCursor = NULL;
	w.hbrBackground	= (HBRUSH)(COLOR_MENU + 1);
	w.lpszMenuName = NULL;
	w.lpszClassName = windowclassname_drawable;
	w.hIconSm = NULL;

	if (RegisterClassExW(&w) == 0) {
		win_perror("DrawableClass");
		ret = NSERROR_INIT_FAILED;
	}

	return ret;
}
