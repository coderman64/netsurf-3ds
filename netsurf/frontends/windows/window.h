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

#ifndef NETSURF_WINDOWS_WINDOW_H_
#define NETSURF_WINDOWS_WINDOW_H_

/** The window operation function table for win32 */
extern struct gui_window_table *win32_window_table;

#include "netsurf/mouse.h"

struct browser_mouse {
       struct gui_window *gui;
       struct box *box;

       double pressed_x;
       double pressed_y;
       bool waiting;
       browser_mouse_state state;
};

struct gui_window {
	/* The front's private data connected to a browser window */
	/* currently 1<->1 gui_window<->windows window [non-tabbed] */
	struct browser_window *bw; /**< the browser_window */

	HWND main; /**< handle to the actual window */
	HWND toolbar; /**< toolbar handle */
	HWND urlbar; /**< url bar handle */
	HWND throbber; /** throbber handle */
	HWND drawingarea; /**< drawing area handle */
	HWND statusbar; /**< status bar handle */
	HWND vscroll; /**< vertical scrollbar handle */
	HWND hscroll; /**< horizontal scrollbar handle */

	HMENU mainmenu; /**< the main menu */
	HMENU rclick; /**< the right-click menu */
	struct nsws_localhistory *localhistory;	/**< handle to local history window */
	int width; /**< width of window */
	int height; /**< height of drawing area */

	int toolbuttonc; /**< number of toolbar buttons */
	int toolbuttonsize; /**< width, height of buttons */
	bool throbbing; /**< whether currently throbbing */

	struct browser_mouse *mouse; /**< mouse state */

	HACCEL acceltable; /**< accelerators */

	HBITMAP hPageInfo[8]; /**< page info handles */

	int scrollx; /**< current scroll location */
	int scrolly; /**< current scroll location */

	RECT *fullscreen; /**< memorize non-fullscreen area */
	RECT redraw; /**< Area needing redraw. */
	int requestscrollx, requestscrolly; /**< scolling requested. */
	struct gui_window *next, *prev; /**< global linked list */
};

struct rect;

/**
 * Obtain gui window structure from window handle.
 *
 * \param hwnd The window handle.
 * \return The gui window associated with the window handle.
 */
struct gui_window *nsws_get_gui_window(HWND hwnd);

/**
 * Cause a browser window to navigate to a url
 *
 * \param hwnd The win32 handle to the browser window or one of its decendants.
 * \param urltxt The URL to navigate to.
 */
bool nsws_window_go(HWND hwnd, const char *urltxt);

/**
 * Set the scroll position of a win32 browser window.
 *
 * Scrolls the viewport to ensure the specified rectangle of the
 *   content is shown. The win32 implementation scrolls the contents so
 *   the specified point in the content is at the top of the viewport.
 *
 * \param gw The win32 gui window to scroll.
 * \param rect The rectangle to ensure is shown.
 * \return NSERROR_OK on success or apropriate error code.
 */
nserror win32_window_set_scroll(struct gui_window *gw, const struct rect *rect);

/**
 * Create the main browser window class.
 *
 * \param hinstance The application instance
 * \return NSERROR_OK on success or NSERROR_INIT_FAILED if the class
 *         creation failed.
 */
nserror nsws_create_main_class(HINSTANCE hinstance);

/**
 * Get the main win32 window handle from a gui window
 */
HWND gui_window_main_window(struct gui_window *gw);

/**
 * Get the localhistory win32 window handle from a gui window
 */
struct nsws_localhistory *gui_window_localhistory(struct gui_window *);


#endif /* _NETSURF_WINDOWS_WINDOW_H_ */
