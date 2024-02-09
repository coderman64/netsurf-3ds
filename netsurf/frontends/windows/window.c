/*
 * Copyright 2011-2016 Vincent Sanders <vince@netsurf-browser.org>
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
 * Main browser window handling for windows win32 frontend.
 */

#include "utils/config.h"

#include <stdbool.h>
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>

#include "utils/errors.h"
#include "utils/log.h"
#include "utils/utils.h"
#include "utils/nsoption.h"
#include "utils/nsurl.h"
#include "utils/messages.h"
#include "content/content.h"
#include "netsurf/browser_window.h"
#include "netsurf/window.h"
#include "netsurf/keypress.h"
#include "desktop/browser_history.h"

#include "windows/gui.h"
#include "windows/pointers.h"
#include "windows/about.h"
#include "windows/resourceid.h"
#include "windows/findfile.h"
#include "windows/windbg.h"
#include "windows/drawable.h"
#include "windows/font.h"
#include "windows/prefs.h"
#include "windows/local_history.h"
#include "windows/hotlist.h"
#include "windows/cookies.h"
#include "windows/global_history.h"
#include "windows/window.h"

/**
 * List of all gui windows
 */
static struct gui_window *window_list = NULL;

/**
 * The main window class name
 */
static const LPCWSTR windowclassname_main = L"nswsmainwindow";

/**
 * width of the throbber element
 */
#define NSWS_THROBBER_WIDTH 24

/**
 * height of the url entry box
 */
#define NSWS_URLBAR_HEIGHT 23

/**
 * height of the Page Information bitmap button
 */
#define NSW32_PGIBUTTON_HEIGHT 16

/**
 * Number of open windows
 */
static int open_windows = 0;


/**
 * create and attach accelerator table to main window
 *
 * \param gw gui window context.
 */
static void nsws_window_set_accels(struct gui_window *gw)
{
	int i, nitems = 13;
	ACCEL accels[nitems];

	for (i = 0; i < nitems; i++) {
		accels[i].fVirt = FCONTROL | FVIRTKEY;
	}

	accels[0].key = 0x51; /* Q */
	accels[0].cmd = IDM_FILE_QUIT;
	accels[1].key = 0x4E; /* N */
	accels[1].cmd = IDM_FILE_OPEN_WINDOW;
	accels[2].key = VK_LEFT;
	accels[2].cmd = IDM_NAV_BACK;
	accels[3].key = VK_RIGHT;
	accels[3].cmd = IDM_NAV_FORWARD;
	accels[4].key = VK_UP;
	accels[4].cmd = IDM_NAV_HOME;
	accels[5].key = VK_BACK;
	accels[5].cmd = IDM_NAV_STOP;
	accels[6].key = VK_SPACE;
	accels[6].cmd = IDM_NAV_RELOAD;
	accels[7].key = 0x4C; /* L */
	accels[7].cmd = IDM_FILE_OPEN_LOCATION;
	accels[8].key = 0x57; /* w */
	accels[8].cmd = IDM_FILE_CLOSE_WINDOW;
	accels[9].key = 0x41; /* A */
	accels[9].cmd = IDM_EDIT_SELECT_ALL;
	accels[10].key = VK_F8;
	accels[10].cmd = IDM_VIEW_SOURCE;
	accels[11].key = VK_RETURN;
	accels[11].fVirt = FVIRTKEY;
	accels[11].cmd = IDC_MAIN_LAUNCH_URL;
	accels[12].key = VK_F11;
	accels[12].fVirt = FVIRTKEY;
	accels[12].cmd = IDM_VIEW_FULLSCREEN;

	gw->acceltable = CreateAcceleratorTable(accels, nitems);
}


/**
 * creation of a new full browser window
 *
 * \param hInstance The application instance handle.
 * \param gw gui window context.
 * \return The newly created window instance.
 */
static HWND nsws_window_create(HINSTANCE hInstance, struct gui_window *gw)
{
	HWND hwnd;
	INITCOMMONCONTROLSEX icc;
	int xpos = CW_USEDEFAULT;
	int ypos = CW_USEDEFAULT;
	int width = CW_USEDEFAULT;
	int height = CW_USEDEFAULT;

	if ((nsoption_int(window_width) >= 100) &&
	    (nsoption_int(window_height) >= 100) &&
	    (nsoption_int(window_x) >= 0) &&
	    (nsoption_int(window_y) >= 0)) {
		xpos = nsoption_int(window_x);
		ypos = nsoption_int(window_y);
		width = nsoption_int(window_width);
		height = nsoption_int(window_height);

		NSLOG(netsurf, DEBUG, "Setting Window position %d,%d %d,%d",
		      xpos, ypos, width, height);
	}

	icc.dwSize = sizeof(icc);
	icc.dwICC = ICC_BAR_CLASSES | ICC_WIN95_CLASSES;
#if WINVER > 0x0501
	icc.dwICC |= ICC_STANDARD_CLASSES;
#endif
	InitCommonControlsEx(&icc);

	gw->mainmenu = LoadMenu(hInstance, MAKEINTRESOURCE(IDR_MENU_MAIN));
	gw->rclick = LoadMenu(hInstance, MAKEINTRESOURCE(IDR_MENU_CONTEXT));

	hwnd = CreateWindowExW(0,
			       windowclassname_main,
			       L"NetSurf Browser",
			       WS_OVERLAPPEDWINDOW |
			       WS_CLIPCHILDREN |
			       WS_CLIPSIBLINGS |
			       CS_DBLCLKS,
			       xpos,
			       ypos,
			       width,
			       height,
			       NULL,
			       gw->mainmenu,
			       hInstance,
			       (LPVOID)gw);

	if (hwnd == NULL) {
		NSLOG(netsurf, INFO, "Window create failed");
	} else {
		nsws_window_set_accels(gw);
	}

	return hwnd;
}


/**
 * toolbar command message handler
 *
 * \todo This entire command handler appears superfluous.
 *
 * \param gw The graphical window context
 * \param notification_code The notification code of the message
 * \param identifier The identifier the command was delivered for
 * \param ctrl_window The controlling window.
 */
static LRESULT
nsws_window_toolbar_command(struct gui_window *gw,
			    int notification_code,
			    int identifier,
			    HWND ctrl_window)
{
	NSLOG(netsurf, DEBUG,
	      "notification_code %d identifier %d ctrl_window %p",
	      notification_code,
	      identifier,
	      ctrl_window);

	switch(identifier) {

	case IDC_MAIN_URLBAR:
		switch (notification_code) {
		case EN_CHANGE:
			NSLOG(netsurf, DEBUG, "EN_CHANGE");
			break;

		case EN_ERRSPACE:
			NSLOG(netsurf, DEBUG, "EN_ERRSPACE");
			break;

		case EN_HSCROLL:
			NSLOG(netsurf, DEBUG, "EN_HSCROLL");
			break;

		case EN_KILLFOCUS:
			NSLOG(netsurf, DEBUG, "EN_KILLFOCUS");
			break;

		case EN_MAXTEXT:
			NSLOG(netsurf, DEBUG, "EN_MAXTEXT");
			break;

		case EN_SETFOCUS:
			NSLOG(netsurf, DEBUG, "EN_SETFOCUS");
			break;

		case EN_UPDATE:
			NSLOG(netsurf, DEBUG, "EN_UPDATE");
			break;

		case EN_VSCROLL:
			NSLOG(netsurf, DEBUG, "EN_VSCROLL");
			break;

		default:
			NSLOG(netsurf, DEBUG, "Unknown notification_code");
			break;
		}
		break;

	default:
		return 1; /* unhandled */

	}
	return 0; /* control message handled */
}


/**
 * calculate the dimensions of the url bar relative to the parent toolbar
 *
 * \param hWndParent The parent window of the url bar
 * \param toolbuttonsize size of the buttons
 * \param buttonc The number of buttons
 * \param[out] x The calculated x location
 * \param[out] y The calculated y location
 * \param[out] width The calculated width
 * \param[out] height The calculated height
 */
static void
urlbar_dimensions(HWND hWndParent,
		  int toolbuttonsize,
		  int buttonc,
		  int *x,
		  int *y,
		  int *width,
		  int *height)
{
	RECT rc;
	const int cy_edit = NSWS_URLBAR_HEIGHT;

	GetClientRect(hWndParent, &rc);
	*x = (toolbuttonsize + 1) * (buttonc + 1) + (NSWS_THROBBER_WIDTH>>1);
	*y = ((((rc.bottom - 1) - cy_edit) >> 1) * 2) / 3;
	*width = (rc.right - 1) - *x - (NSWS_THROBBER_WIDTH>>1) - NSWS_THROBBER_WIDTH;
	*height = cy_edit;
}


/**
 * callback for toolbar events
 *
 * subclass message handler for toolbar window
 *
 * \param hwnd win32 window handle message arrived for
 * \param msg The message ID
 * \param wparam The w parameter of the message.
 * \param lparam The l parameter of the message.
 */
static LRESULT CALLBACK
nsws_window_toolbar_callback(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	struct gui_window *gw;
	int urlx, urly, urlwidth, urlheight;
	WNDPROC toolproc;

	LOG_WIN_MSG(hwnd, msg, wparam, lparam);

	toolproc = (WNDPROC)GetProp(hwnd, TEXT("OrigMsgProc"));
	assert(toolproc != NULL);

	gw = nsws_get_gui_window(hwnd);
	assert(gw != NULL);

	switch (msg) {
	case WM_SIZE:
		urlbar_dimensions(hwnd,
				  gw->toolbuttonsize,
				  gw->toolbuttonc,
				  &urlx, &urly, &urlwidth, &urlheight);

		/* resize url */
		if (gw->urlbar != NULL) {
			MoveWindow(gw->urlbar,
				   urlx, urly,
				   urlwidth, urlheight,
				   true);
		}

		/* move throbber */
		if (gw->throbber != NULL) {
			MoveWindow(gw->throbber,
				   LOWORD(lparam) - NSWS_THROBBER_WIDTH - 4,
				   urly,
				   NSWS_THROBBER_WIDTH,
				   NSWS_THROBBER_WIDTH,
				   true);
		}
		break;

	case WM_COMMAND:
		if (nsws_window_toolbar_command(gw,
						HIWORD(wparam),
						LOWORD(wparam),
						(HWND)lparam) == 0) {
			return 0;
		}
		break;

	case WM_NCDESTROY:
		/* remove properties if window is being destroyed */
		RemoveProp(hwnd, TEXT("GuiWnd"));
		RemoveProp(hwnd, TEXT("OrigMsgProc"));
		/* put the original message handler back */
		SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)toolproc);
		break;

	}

	/* chain to the next handler */
	return CallWindowProc(toolproc, hwnd, msg, wparam, lparam);
}


static void set_urlbar_edit_size(HWND hwnd)
{
	RECT rc;
	GetClientRect(hwnd, &rc);
	rc.left += NSW32_PGIBUTTON_HEIGHT;
	SendMessage(hwnd, EM_SETRECT, 0, (LPARAM)&rc);
	NSLOG(netsurf, DEBUG, "left:%ld right:%ld top:%ld bot:%ld",
	      rc.left,rc.right,rc.top,rc.bottom);
}


/**
 * callback for url bar events
 *
 * subclass message handler for urlbar window
 *
 * \param hwnd win32 window handle message arrived for
 * \param msg The message ID
 * \param wparam The w parameter of the message.
 * \param lparam The l parameter of the message.
 */
static LRESULT CALLBACK
nsws_window_urlbar_callback(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	struct gui_window *gw;
	WNDPROC urlproc;
	HFONT hFont;
	LRESULT result;

	LOG_WIN_MSG(hwnd, msg, wparam, lparam);

	urlproc = (WNDPROC)GetProp(hwnd, TEXT("OrigMsgProc"));
	assert(urlproc != NULL);

	gw = nsws_get_gui_window(hwnd);
	assert(gw != NULL);

	/* override messages */
	switch (msg) {
	case WM_CHAR:
		if (wparam == 1) {
			/* handle ^A */
			SendMessage(hwnd, EM_SETSEL, 0, -1);
			return 1;
		} else if (wparam == 13) {
			SendMessage(gw->main, WM_COMMAND, IDC_MAIN_LAUNCH_URL, 0);
			return 0;
		}
		break;

	case WM_DESTROY:
		hFont = (HFONT)SendMessage(hwnd, WM_GETFONT, 0, 0);
		if (hFont != NULL) {
			NSLOG(netsurf, INFO, "Destroyed font object");
			DeleteObject(hFont);
		}


	case WM_NCDESTROY:
		/* remove properties if window is being destroyed */
		RemoveProp(hwnd, TEXT("GuiWnd"));
		RemoveProp(hwnd, TEXT("OrigMsgProc"));
		/* put the original message handler back */
		SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)urlproc);
		break;

	case WM_SIZE:
		result = CallWindowProc(urlproc, hwnd, msg, wparam, lparam);
		set_urlbar_edit_size(hwnd);
		return result;

	}

	/* chain to the next handler */
	return CallWindowProc(urlproc, hwnd, msg, wparam, lparam);
}

/**
 * create a urlbar and message handler
 *
 * Create an Edit control for entering urls
 *
 * \param hInstance The application instance handle.
 * \param hWndParent The containing window.
 * \param gw win32 frontends window context.
 * \return win32 window handle of created window or NULL on error.
 */
static HWND
nsws_window_urlbar_create(HINSTANCE hInstance,
			  HWND hWndParent,
			  struct gui_window *gw)
{
	int urlx, urly, urlwidth, urlheight;
	HWND hwnd;
	HWND hbutton;
	WNDPROC	urlproc;
	HFONT hFont;

	urlbar_dimensions(hWndParent,
			  gw->toolbuttonsize,
			  gw->toolbuttonc,
			  &urlx, &urly, &urlwidth, &urlheight);

	/* Create the edit control */
	hwnd = CreateWindowEx(0L,
			      TEXT("Edit"),
			      NULL,
			      WS_CHILD | WS_BORDER | WS_VISIBLE |
			      ES_LEFT | ES_AUTOHSCROLL | ES_MULTILINE,
			      urlx,
			      urly,
			      urlwidth,
			      urlheight,
			      hWndParent,
			      (HMENU)IDC_MAIN_URLBAR,
			      hInstance,
			      NULL);

	if (hwnd == NULL) {
		return NULL;
	}

	/* set the gui window associated with this control */
	SetProp(hwnd, TEXT("GuiWnd"), (HANDLE)gw);

	/* subclass the message handler */
	urlproc = (WNDPROC)SetWindowLongPtr(hwnd,
					    GWLP_WNDPROC,
					    (LONG_PTR)nsws_window_urlbar_callback);

	/* save the real handler  */
	SetProp(hwnd, TEXT("OrigMsgProc"), (HANDLE)urlproc);

	hFont = CreateFont(urlheight - 4, 0, 0, 0, FW_BOLD, FALSE, FALSE,
			   FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS,
			   CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
			   DEFAULT_PITCH | FF_SWISS, "Arial");
	if (hFont != NULL) {
		NSLOG(netsurf, INFO, "Setting font object");
		SendMessage(hwnd, WM_SETFONT, (WPARAM)hFont, 0);
	}


	/* Create the page info button */
	hbutton = CreateWindowEx(0L,
				 TEXT("BUTTON"),
				 NULL,
				 WS_CHILD | WS_VISIBLE | BS_BITMAP | BS_FLAT,
				 (NSWS_URLBAR_HEIGHT - NSW32_PGIBUTTON_HEIGHT) /2,
				 (NSWS_URLBAR_HEIGHT - NSW32_PGIBUTTON_HEIGHT) /2,
				 NSW32_PGIBUTTON_HEIGHT,
				 NSW32_PGIBUTTON_HEIGHT,
				 hwnd,
				 (HMENU)IDC_PAGEINFO,
				 hInstance,
			     NULL);

	/* put a property on the parent toolbar so it can set the page info */
	SetProp(hWndParent, TEXT("hPGIbutton"), (HANDLE)hbutton);

	SendMessageW(hbutton, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)gw->hPageInfo[PAGE_STATE_UNKNOWN]);

	set_urlbar_edit_size(hwnd);

	NSLOG(netsurf, INFO,
	      "Created url bar hwnd:%p, x:%d, y:%d, w:%d, h:%d", hwnd, urlx,
	      urly, urlwidth, urlheight);

	return hwnd;
}


/**
 * creation of throbber
 *
 * \param hInstance The application instance handle.
 * \param hWndParent The containing window.
 * \param gw win32 frontends window context.
 * \return win32 window handle of created window or NULL on error.
 */
static HWND
nsws_window_throbber_create(HINSTANCE hInstance,
			    HWND hWndParent,
			    struct gui_window *gw)
{
	HWND hwnd;
	int urlx, urly, urlwidth, urlheight;

	urlbar_dimensions(hWndParent,
			  gw->toolbuttonsize,
			  gw->toolbuttonc,
			  &urlx, &urly, &urlwidth, &urlheight);

	hwnd = CreateWindow(ANIMATE_CLASS,
			    "",
			    WS_CHILD | WS_VISIBLE | ACS_TRANSPARENT,
			    gw->width - NSWS_THROBBER_WIDTH - 4,
			    urly,
			    NSWS_THROBBER_WIDTH,
			    NSWS_THROBBER_WIDTH,
			    hWndParent,
			    (HMENU) IDC_MAIN_THROBBER,
			    hInstance,
			    NULL);

	Animate_Open(hwnd, MAKEINTRESOURCE(IDR_THROBBER_AVI));

	if (gw->throbbing) {
		Animate_Play(hwnd, 0, -1, -1);
	} else {
		Animate_Seek(hwnd, 0);
	}
	ShowWindow(hwnd, SW_SHOWNORMAL);

	return hwnd;
}


/**
 * create a win32 image list for the toolbar.
 *
 * \param hInstance The application instance handle.
 * \param resid The resource ID of the image.
 * \param bsize The size of the image to load.
 * \param bcnt The number of bitmaps to load into the list.
 * \return The image list or NULL on error.
 */
static HIMAGELIST
get_imagelist(HINSTANCE hInstance, int resid, int bsize, int bcnt)
{
	HIMAGELIST hImageList;
	HBITMAP hScrBM;

	NSLOG(netsurf, INFO, "resource id %d, bzize %d, bcnt %d", resid,
	      bsize, bcnt);

	hImageList = ImageList_Create(bsize, bsize,
				      ILC_COLOR24 | ILC_MASK, 0,
				      bcnt);
	if (hImageList == NULL) {
		return NULL;
	}

	hScrBM = LoadImage(hInstance,
			   MAKEINTRESOURCE(resid),
			   IMAGE_BITMAP,
			   0,
			   0,
			   LR_DEFAULTCOLOR);
	if (hScrBM == NULL) {
		win_perror("LoadImage");
		return NULL;
	}

	if (ImageList_AddMasked(hImageList, hScrBM, 0xcccccc) == -1) {
		/* failed to add masked bitmap */
		ImageList_Destroy(hImageList);
		hImageList = NULL;
	}
	DeleteObject(hScrBM);

	return hImageList;
}


/**
 * create win32 main window toolbar and add controls and message handler
 *
 * Toolbar has buttons on the left, url entry space in the middle and
 * activity throbber on the right.
 *
 * \param hInstance The application instance handle.
 * \param hWndParent The containing window.
 * \param gw win32 frontends window context.
 * \return win32 window handle of created window or NULL on error.
 */
static HWND
nsws_window_create_toolbar(HINSTANCE hInstance,
			   HWND hWndParent,
			   struct gui_window *gw)
{
	HIMAGELIST hImageList;
	HWND hWndToolbar;
	/* Toolbar buttons */
	TBBUTTON tbButtons[] = {
		{0, IDM_NAV_BACK, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0},
		{1, IDM_NAV_FORWARD, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0},
		{2, IDM_NAV_HOME, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0},
		{3, IDM_NAV_RELOAD, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0},
		{4, IDM_NAV_STOP, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0},
	};
	WNDPROC	toolproc;

	/* Create the toolbar window and subclass its message handler. */
	hWndToolbar = CreateWindowEx(0,
				     TOOLBARCLASSNAME,
				     "Toolbar",
				     WS_CHILD | TBSTYLE_FLAT,
				     0, 0, 0, 0,
				     hWndParent,
				     NULL,
				     HINST_COMMCTRL,
				     NULL);
	if (!hWndToolbar) {
		return NULL;
	}

	/* set the gui window associated with this toolbar */
	SetProp(hWndToolbar, TEXT("GuiWnd"), (HANDLE)gw);

	/* subclass the message handler */
	toolproc = (WNDPROC)SetWindowLongPtr(hWndToolbar,
					     GWLP_WNDPROC,
					     (LONG_PTR)nsws_window_toolbar_callback);

	/* save the real handler  */
	SetProp(hWndToolbar, TEXT("OrigMsgProc"), (HANDLE)toolproc);

	/* remember how many buttons are being created */
	gw->toolbuttonc = sizeof(tbButtons) / sizeof(TBBUTTON);

	/* Create the standard image list and assign to toolbar. */
	hImageList = get_imagelist(hInstance,
				   IDR_TOOLBAR_BITMAP,
				   gw->toolbuttonsize,
				   gw->toolbuttonc);
	if (hImageList != NULL) {
		SendMessage(hWndToolbar,
			    TB_SETIMAGELIST,
			    0,
			    (LPARAM)hImageList);
	}

	/* Create the disabled image list and assign to toolbar. */
	hImageList = get_imagelist(hInstance,
				   IDR_TOOLBAR_BITMAP_GREY,
				   gw->toolbuttonsize,
				   gw->toolbuttonc);
	if (hImageList != NULL) {
		SendMessage(hWndToolbar,
			    TB_SETDISABLEDIMAGELIST,
			    0,
			    (LPARAM)hImageList);
	}

	/* Create the hot image list and assign to toolbar. */
	hImageList = get_imagelist(hInstance,
				   IDR_TOOLBAR_BITMAP_HOT,
				   gw->toolbuttonsize,
				   gw->toolbuttonc);
	if (hImageList != NULL) {
		SendMessage(hWndToolbar,
			    TB_SETHOTIMAGELIST,
			    0,
			    (LPARAM)hImageList);
	}

	/* Add buttons. */
	SendMessage(hWndToolbar,
		    TB_BUTTONSTRUCTSIZE,
		    (WPARAM)sizeof(TBBUTTON),
		    0);

	SendMessage(hWndToolbar,
		    TB_ADDBUTTONS,
		    (WPARAM)gw->toolbuttonc,
		    (LPARAM)&tbButtons);

	/* create url widget */
	gw->urlbar = nsws_window_urlbar_create(hInstance, hWndToolbar, gw);

	/* create throbber widget */
	gw->throbber = nsws_window_throbber_create(hInstance, hWndToolbar, gw);

	SendMessage(hWndToolbar, TB_AUTOSIZE, 0, 0);
	ShowWindow(hWndToolbar,  TRUE);

	return hWndToolbar;
}


/**
 * creation of status bar
 *
 * \param hInstance The application instance handle.
 * \param hWndParent The containing window.
 * \param gw win32 frontends window context.
 */
static HWND
nsws_window_create_statusbar(HINSTANCE hInstance,
			     HWND hWndParent,
			     struct gui_window *gw)
{
	HWND hwnd;
	hwnd = CreateWindowEx(0,
			      STATUSCLASSNAME,
			      NULL,
			      WS_CHILD | WS_VISIBLE,
			      0, 0, 0, 0,
			      hWndParent,
			      (HMENU)IDC_MAIN_STATUSBAR,
			      hInstance,
			      NULL);
	if (hwnd != NULL) {
		SendMessage(hwnd, SB_SETTEXT, 0, (LPARAM)"NetSurf");
	}
	return hwnd;
}


/**
 * Update popup context menu editing functionality
 *
 * \param w win32 frontends window context.
 */
static void nsws_update_edit(struct gui_window *w)
{
	browser_editor_flags editor_flags = (w->bw == NULL) ?
		BW_EDITOR_NONE : browser_window_get_editor_flags(w->bw);
	bool paste, copy, del;
	bool sel = (editor_flags & BW_EDITOR_CAN_COPY);

	if (GetFocus() == w->urlbar) {
		DWORD i, ii;
		SendMessage(w->urlbar, EM_GETSEL, (WPARAM)&i, (LPARAM)&ii);
		paste = true;
		copy = (i != ii);
		del = (i != ii);

	} else if (sel) {
		paste = (editor_flags & BW_EDITOR_CAN_PASTE);
		copy = sel;
		del = (editor_flags & BW_EDITOR_CAN_CUT);
	} else {
		paste = false;
		copy = false;
		del = false;
	}
	EnableMenuItem(w->mainmenu,
		       IDM_EDIT_PASTE,
		       (paste ? MF_ENABLED : MF_GRAYED));

	EnableMenuItem(w->rclick,
		       IDM_EDIT_PASTE,
		       (paste ? MF_ENABLED : MF_GRAYED));

	EnableMenuItem(w->mainmenu,
		       IDM_EDIT_COPY,
		       (copy ? MF_ENABLED : MF_GRAYED));

	EnableMenuItem(w->rclick,
		       IDM_EDIT_COPY,
		       (copy ? MF_ENABLED : MF_GRAYED));

	if (del == true) {
		EnableMenuItem(w->mainmenu, IDM_EDIT_CUT, MF_ENABLED);
		EnableMenuItem(w->mainmenu, IDM_EDIT_DELETE, MF_ENABLED);
		EnableMenuItem(w->rclick, IDM_EDIT_CUT, MF_ENABLED);
		EnableMenuItem(w->rclick, IDM_EDIT_DELETE, MF_ENABLED);
	} else {
		EnableMenuItem(w->mainmenu, IDM_EDIT_CUT, MF_GRAYED);
		EnableMenuItem(w->mainmenu, IDM_EDIT_DELETE, MF_GRAYED);
		EnableMenuItem(w->rclick, IDM_EDIT_CUT, MF_GRAYED);
		EnableMenuItem(w->rclick, IDM_EDIT_DELETE, MF_GRAYED);
	}
}


/**
 * Handle win32 context menu message
 *
 * \param gw win32 frontends graphical window.
 * \param hwnd The win32 window handle
 * \param x The x coordinate of the event.
 * \param y the y coordinate of the event.
 * \return true if menu displayed else false
 */
static bool
nsws_ctx_menu(struct gui_window *gw, HWND hwnd, int x, int y)
{
	RECT rc; /* client area of window */
	POINT pt = { x, y }; /* location of mouse click */

	/* Get the bounding rectangle of the client area. */
	GetClientRect(hwnd, &rc);

	/* Convert the mouse position to client coordinates. */
	ScreenToClient(hwnd, &pt);

	/* If the position is in the client area, display a shortcut menu. */
	if (PtInRect(&rc, pt)) {
		ClientToScreen(hwnd, &pt);
		nsws_update_edit(gw);
		TrackPopupMenu(GetSubMenu(gw->rclick, 0),
			       TPM_CENTERALIGN | TPM_TOPALIGN,
			       x,
			       y,
			       0,
			       hwnd,
			       NULL);

		return true;
	}

	/* Return false if no menu is displayed. */
	return false;
}


/**
 * update state of forward/back buttons/menu items when page changes
 *
 * \param w win32 frontends graphical window.
 */
static void nsws_window_update_forward_back(struct gui_window *w)
{
	if (w->bw == NULL)
		return;

	bool forward = browser_window_history_forward_available(w->bw);
	bool back = browser_window_history_back_available(w->bw);

	if (w->mainmenu != NULL) {
		EnableMenuItem(w->mainmenu, IDM_NAV_FORWARD,
			       (forward ? MF_ENABLED : MF_GRAYED));
		EnableMenuItem(w->mainmenu, IDM_NAV_BACK,
			       (back ? MF_ENABLED : MF_GRAYED));
		EnableMenuItem(w->rclick, IDM_NAV_FORWARD,
			       (forward ? MF_ENABLED : MF_GRAYED));
		EnableMenuItem(w->rclick, IDM_NAV_BACK,
			       (back ? MF_ENABLED : MF_GRAYED));
	}

	if (w->toolbar != NULL) {
		SendMessage(w->toolbar, TB_SETSTATE,
			    (WPARAM) IDM_NAV_FORWARD,
			    MAKELONG((forward ? TBSTATE_ENABLED :
				      TBSTATE_INDETERMINATE), 0));
		SendMessage(w->toolbar, TB_SETSTATE,
			    (WPARAM) IDM_NAV_BACK,
			    MAKELONG((back ? TBSTATE_ENABLED :
				      TBSTATE_INDETERMINATE), 0));
	}
	nsw32_local_history_hide();
}


/**
 * Invalidate an area of a win32 browser window
 *
 * \param gw The netsurf window being invalidated.
 * \param rect area to redraw or NULL for entrire window area.
 * \return NSERROR_OK or appropriate error code.
 */
static nserror
win32_window_invalidate_area(struct gui_window *gw, const struct rect *rect)
{
	RECT *redrawrectp = NULL;
	RECT redrawrect;

	assert(gw != NULL);

	if (rect != NULL) {
		redrawrectp = &redrawrect;

		redrawrect.left = (long)rect->x0 - gw->scrollx;
		redrawrect.top = (long)rect->y0 - gw->scrolly;
		redrawrect.right =(long)rect->x1;
		redrawrect.bottom = (long)rect->y1;

	}
	RedrawWindow(gw->drawingarea,
		     redrawrectp,
		     NULL,
		     RDW_INVALIDATE | RDW_NOERASE);

	return NSERROR_OK;
}


/**
 * Create a new window due to menu selection
 *
 * \param gw frontends graphical window.
 * \return NSERROR_OK on success else appropriate error code.
 */
static nserror win32_open_new_window(struct gui_window *gw)
{
	const char *addr;
	nsurl *url;
	nserror ret;

	if (nsoption_charp(homepage_url) != NULL) {
		addr = nsoption_charp(homepage_url);
	} else {
		addr = NETSURF_HOMEPAGE;
	}

	ret = nsurl_create(addr, &url);
	if (ret == NSERROR_OK) {
		ret = browser_window_create(BW_CREATE_HISTORY,
					    url,
					    NULL,
					    gw->bw,
					    NULL);
		nsurl_unref(url);
	}

	return ret;
}


/**
 * handle command message on main browser window
 *
 * \param hwnd The win32 window handle
 * \param gw win32 gui window
 * \param notification_code notification code
 * \param identifier notification identifier
 * \param ctrl_window The win32 control window handle
 * \return appropriate response for command
 */
static LRESULT
nsws_window_command(HWND hwnd,
		    struct gui_window *gw,
		    int notification_code,
		    int identifier,
		    HWND ctrl_window)
{
	nserror ret;

	NSLOG(netsurf, INFO,
	      "notification_code %x identifier %x ctrl_window %p",
	      notification_code,
	      identifier,
	      ctrl_window);

	switch(identifier) {

	case IDM_FILE_QUIT:
	{
		struct gui_window *w;
		w = window_list;
		while (w != NULL) {
			PostMessage(w->main, WM_CLOSE, 0, 0);
			w = w->next;
		}
		break;
	}

	case IDM_FILE_OPEN_LOCATION:
		SetFocus(gw->urlbar);
		break;

	case IDM_FILE_OPEN_WINDOW:
		ret = win32_open_new_window(gw);
		if (ret != NSERROR_OK) {
			win32_warning(messages_get_errorcode(ret), 0);
		}
		break;

	case IDM_FILE_CLOSE_WINDOW:
		PostMessage(gw->main, WM_CLOSE, 0, 0);
		break;

	case IDM_FILE_SAVE_PAGE:
		break;

	case IDM_FILE_SAVEAS_TEXT:
		break;

	case IDM_FILE_SAVEAS_PDF:
		break;

	case IDM_FILE_SAVEAS_POSTSCRIPT:
		break;

	case IDM_FILE_PRINT_PREVIEW:
		break;

	case IDM_FILE_PRINT:
		break;

	case IDM_EDIT_CUT:
		if (GetFocus() == gw->urlbar) {
			SendMessage(gw->urlbar, WM_CUT, 0, 0);
		} else {
			SendMessage(gw->drawingarea, WM_CUT, 0, 0);
		}
		break;

	case IDM_EDIT_COPY:
		if (GetFocus() == gw->urlbar) {
			SendMessage(gw->urlbar, WM_COPY, 0, 0);
		} else {
			SendMessage(gw->drawingarea, WM_COPY, 0, 0);
		}
		break;

	case IDM_EDIT_PASTE: {
		if (GetFocus() == gw->urlbar) {
			SendMessage(gw->urlbar, WM_PASTE, 0, 0);
		} else {
			SendMessage(gw->drawingarea, WM_PASTE, 0, 0);
		}
		break;
	}

	case IDM_EDIT_DELETE:
		if (GetFocus() == gw->urlbar)
			SendMessage(gw->urlbar, WM_CLEAR, 0, 0);
		else
			SendMessage(gw->drawingarea, WM_CLEAR, 0, 0);
		break;

	case IDM_EDIT_SELECT_ALL:
		if (GetFocus() == gw->urlbar)
			SendMessage(gw->urlbar, EM_SETSEL, 0, -1);
		else
			browser_window_key_press(gw->bw, NS_KEY_SELECT_ALL);
		break;

	case IDM_EDIT_SEARCH:
		break;

	case IDM_EDIT_PREFERENCES:
		nsws_prefs_dialog_init(hinst, gw->main);
		break;

	case IDM_NAV_BACK:
		if ((gw->bw != NULL) &&
		    (browser_window_history_back_available(gw->bw))) {
			browser_window_history_back(gw->bw, false);
		}
		nsws_window_update_forward_back(gw);
		break;

	case IDM_NAV_FORWARD:
		if ((gw->bw != NULL) &&
		    (browser_window_history_forward_available(gw->bw))) {
			browser_window_history_forward(gw->bw, false);
		}
		nsws_window_update_forward_back(gw);
		break;

	case IDM_NAV_HOME:
	{
		nsurl *url;
		ret = nsurl_create(nsoption_charp(homepage_url), &url);

		if (ret != NSERROR_OK) {
			win32_report_nserror(ret, 0);
		} else {
			browser_window_navigate(gw->bw,
						url,
						NULL,
						BW_NAVIGATE_HISTORY,
						NULL,
						NULL,
						NULL);
			nsurl_unref(url);
		}
		break;
	}

	case IDM_NAV_STOP:
		browser_window_stop(gw->bw);
		break;

	case IDM_NAV_RELOAD:
		browser_window_reload(gw->bw, true);
		break;

	case IDM_NAV_LOCALHISTORY:
		nsw32_local_history_present(gw->main, gw->bw);
		break;

	case IDM_NAV_GLOBALHISTORY:
		nsw32_global_history_present(hinst);
		break;

	case IDM_TOOLS_COOKIES:
		nsw32_cookies_present(NULL);
		break;

	case IDM_NAV_BOOKMARKS:
		nsw32_hotlist_present(hinst);
		break;

	case IDM_VIEW_ZOOMPLUS:
		browser_window_set_scale(gw->bw, 0.1, false);
		break;

	case IDM_VIEW_ZOOMMINUS:
		browser_window_set_scale(gw->bw, -0.1, false);
		break;

	case IDM_VIEW_ZOOMNORMAL:
		browser_window_set_scale(gw->bw, 1.0, true);
		break;

	case IDM_VIEW_SOURCE:
		break;

	case IDM_VIEW_SAVE_WIN_METRICS: {
		RECT r;
		GetWindowRect(gw->main, &r);
		nsoption_set_int(window_x, r.left);
		nsoption_set_int(window_y, r.top);
		nsoption_set_int(window_width, r.right - r.left);
		nsoption_set_int(window_height, r.bottom - r.top);

		nsws_prefs_save();
		break;
	}

	case IDM_VIEW_FULLSCREEN: {
		RECT rdesk;
		if (gw->fullscreen == NULL) {
			HWND desktop = GetDesktopWindow();
			gw->fullscreen = malloc(sizeof(RECT));
			if ((desktop == NULL) ||
			    (gw->fullscreen == NULL)) {
				win32_warning("NoMemory", 0);
				break;
			}
			GetWindowRect(desktop, &rdesk);
			GetWindowRect(gw->main, gw->fullscreen);
			DeleteObject(desktop);
			SetWindowLong(gw->main, GWL_STYLE, 0);
			SetWindowPos(gw->main, HWND_TOPMOST, 0, 0,
				     rdesk.right - rdesk.left,
				     rdesk.bottom - rdesk.top,
				     SWP_SHOWWINDOW);
		} else {
			SetWindowLong(gw->main, GWL_STYLE,
				      WS_OVERLAPPEDWINDOW |
				      WS_HSCROLL | WS_VSCROLL |
				      WS_CLIPCHILDREN |
				      WS_CLIPSIBLINGS | CS_DBLCLKS);
			SetWindowPos(gw->main, HWND_TOPMOST,
				     gw->fullscreen->left,
				     gw->fullscreen->top,
				     gw->fullscreen->right -
				     gw->fullscreen->left,
				     gw->fullscreen->bottom -
				     gw->fullscreen->top,
				     SWP_SHOWWINDOW | SWP_FRAMECHANGED);
			free(gw->fullscreen);
			gw->fullscreen = NULL;
		}
		break;
	}

	case IDM_TOOLS_DOWNLOADS:
		break;

	case IDM_VIEW_TOGGLE_DEBUG_RENDERING:
		if (gw->bw != NULL) {
			browser_window_debug(gw->bw, CONTENT_DEBUG_REDRAW);
			/* TODO: This should only redraw, not reformat.
			 * (Layout doesn't change, so reformat is a waste of time) */
			browser_window_schedule_reformat(gw->bw);
		}
		break;

	case IDM_VIEW_DEBUGGING_SAVE_BOXTREE:
		break;

	case IDM_VIEW_DEBUGGING_SAVE_DOMTREE:
		break;

	case IDM_HELP_CONTENTS:
		nsws_window_go(hwnd,
			       "https://www.netsurf-browser.org/documentation/");
		break;

	case IDM_HELP_GUIDE:
		nsws_window_go(hwnd,
			       "https://www.netsurf-browser.org/documentation/guide");
		break;

	case IDM_HELP_INFO:
		nsws_window_go(hwnd,
			       "https://www.netsurf-browser.org/documentation/info");
		break;

	case IDM_HELP_ABOUT:
		nsw32_about_dialog_init(hinst, gw->main);
		break;

	case IDC_MAIN_LAUNCH_URL:
	{
		nsurl *url;
		nserror err;

		if (GetFocus() != gw->urlbar)
			break;

		int len = SendMessage(gw->urlbar, WM_GETTEXTLENGTH, 0, 0);
		char addr[len + 1];
		SendMessage(gw->urlbar, WM_GETTEXT, (WPARAM)(len + 1), (LPARAM)addr);
		NSLOG(netsurf, INFO, "launching %s\n", addr);

		err = nsurl_create(addr, &url);

		if (err != NSERROR_OK) {
			win32_report_nserror(err, 0);
		} else {
			browser_window_navigate(gw->bw,
						url,
						NULL,
						BW_NAVIGATE_HISTORY,
						NULL,
						NULL,
						NULL);
			nsurl_unref(url);
		}

		break;
	}


	default:
		return 1; /* unhandled */

	}
	return 0; /* control message handled */
}


/**
 * Get the scroll position of a win32 browser window.
 *
 * \param gw gui_window
 * \param sx receives x ordinate of point at top-left of window
 * \param sy receives y ordinate of point at top-left of window
 * \return true iff successful
 */
static bool win32_window_get_scroll(struct gui_window *gw, int *sx, int *sy)
{
	NSLOG(netsurf, INFO, "get scroll");
	if (gw == NULL)
		return false;

	*sx = gw->scrollx;
	*sy = gw->scrolly;

	return true;
}


/**
 * handle WM_SIZE message on main browser window
 *
 * \param gw win32 gui window
 * \param hwnd The win32 window handle
 * \param wparam The w win32 parameter
 * \param lparam The l win32 parameter
 * \return appropriate response for resize
 */
static LRESULT
nsws_window_resize(struct gui_window *gw,
		   HWND hwnd,
		   WPARAM wparam,
		   LPARAM lparam)
{
	RECT rstatus, rtool;

	if ((gw->toolbar == NULL) ||
	    (gw->urlbar == NULL) ||
	    (gw->statusbar == NULL))
		return 0;

	SendMessage(gw->statusbar, WM_SIZE, wparam, lparam);
	SendMessage(gw->toolbar, WM_SIZE, wparam, lparam);

	GetClientRect(gw->toolbar, &rtool);
	GetWindowRect(gw->statusbar, &rstatus);
	gw->width = LOWORD(lparam);
	gw->height = HIWORD(lparam) - (rtool.bottom - rtool.top) - (rstatus.bottom - rstatus.top);

	if (gw->drawingarea != NULL) {
		MoveWindow(gw->drawingarea,
			   0,
			   rtool.bottom,
			   gw->width,
			   gw->height,
			   true);
	}
	nsws_window_update_forward_back(gw);

	if (gw->toolbar != NULL) {
		SendMessage(gw->toolbar, TB_SETSTATE,
			    (WPARAM) IDM_NAV_STOP,
			    MAKELONG(TBSTATE_INDETERMINATE, 0));
	}

	return 0;
}


/**
 * callback for browser window win32 events
 *
 * \param hwnd The win32 window handle
 * \param msg The win32 message identifier
 * \param wparam The w win32 parameter
 * \param lparam The l win32 parameter
 */
static LRESULT CALLBACK
nsws_window_event_callback(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	struct gui_window *gw;
	RECT rmain;
	LPCREATESTRUCTW createstruct;

	LOG_WIN_MSG(hwnd, msg, wparam, lparam);

	gw = nsws_get_gui_window(hwnd);

	switch (msg) {
	case WM_NCCREATE: /* non client area create */
		/* gw is passed as the lpParam from createwindowex() */
		createstruct = (LPCREATESTRUCTW)lparam;
		gw = (struct gui_window *)createstruct->lpCreateParams;

		/* set the gui window associated with this window handle */
		SetProp(hwnd, TEXT("GuiWnd"), (HANDLE)gw);

		NSLOG(netsurf, INFO,
		      "created hWnd:%p hInstance %p GUI window %p",
		      hwnd, createstruct->hInstance, gw);

		break;

	case WM_CREATE:
		/*
		 * To cause all the component child windows to be
		 * re-sized correctly a WM_SIZE message of the actual
		 * created size must be sent.
		 *
		 * The message must be posted here because the actual
		 * size values of the component windows are not known
		 * until after the WM_CREATE message is dispatched.
		 */
		GetClientRect(hwnd, &rmain);
		PostMessage(hwnd, WM_SIZE, 0,
			    MAKELPARAM(rmain.right, rmain.bottom));
		break;

	case WM_CONTEXTMENU:
		if (nsws_ctx_menu(gw, hwnd, GET_X_LPARAM(lparam),
				  GET_Y_LPARAM(lparam))) {
			return 0;
		}
		break;

	case WM_COMMAND:
		if (nsws_window_command(hwnd, gw, HIWORD(wparam),
					LOWORD(wparam), (HWND)lparam) == 0) {
			return 0;
		}
		break;

	case WM_SIZE:
		return nsws_window_resize(gw, hwnd, wparam, lparam);

	case WM_NCDESTROY:
		RemoveProp(hwnd, TEXT("GuiWnd"));
		nsw32_local_history_hide();
		browser_window_destroy(gw->bw);
		if (--open_windows <= 0) {
			win32_set_quit(true);
		}
		break;

	}

	return DefWindowProcW(hwnd, msg, wparam, lparam);
}

static void destroy_page_info_bitmaps(struct gui_window *gw)
{
	DeleteObject(gw->hPageInfo[PAGE_STATE_UNKNOWN]);
	DeleteObject(gw->hPageInfo[PAGE_STATE_INTERNAL]);
	DeleteObject(gw->hPageInfo[PAGE_STATE_LOCAL]);
	DeleteObject(gw->hPageInfo[PAGE_STATE_INSECURE]);
	DeleteObject(gw->hPageInfo[PAGE_STATE_SECURE_OVERRIDE]);
	DeleteObject(gw->hPageInfo[PAGE_STATE_SECURE_ISSUES]);
	DeleteObject(gw->hPageInfo[PAGE_STATE_SECURE]);
}

static void load_page_info_bitmaps(HINSTANCE hInstance, struct gui_window *gw)
{
	gw->hPageInfo[PAGE_STATE_UNKNOWN] = LoadImage(hInstance,
			     MAKEINTRESOURCE(IDB_PAGEINFO_INTERNAL),
			     IMAGE_BITMAP,
			     0,
			     0,
			     LR_DEFAULTCOLOR);
	gw->hPageInfo[PAGE_STATE_INTERNAL] = LoadImage(hInstance,
			     MAKEINTRESOURCE(IDB_PAGEINFO_INTERNAL),
			     IMAGE_BITMAP,
			     0,
			     0,
			     LR_DEFAULTCOLOR);
	gw->hPageInfo[PAGE_STATE_LOCAL] = LoadImage(hInstance,
			     MAKEINTRESOURCE(IDB_PAGEINFO_LOCAL),
			     IMAGE_BITMAP,
			     0,
			     0,
			     LR_DEFAULTCOLOR);
	gw->hPageInfo[PAGE_STATE_INSECURE] = LoadImage(hInstance,
			     MAKEINTRESOURCE(IDB_PAGEINFO_INSECURE),
			     IMAGE_BITMAP,
			     0,
			     0,
			     LR_DEFAULTCOLOR);
	gw->hPageInfo[PAGE_STATE_SECURE_OVERRIDE] = LoadImage(hInstance,
			     MAKEINTRESOURCE(IDB_PAGEINFO_WARNING),
			     IMAGE_BITMAP,
			     0,
			     0,
			     LR_DEFAULTCOLOR);
	gw->hPageInfo[PAGE_STATE_SECURE_ISSUES] = LoadImage(hInstance,
			     MAKEINTRESOURCE(IDB_PAGEINFO_WARNING),
			     IMAGE_BITMAP,
			     0,
			     0,
			     LR_DEFAULTCOLOR);
	gw->hPageInfo[PAGE_STATE_SECURE] = LoadImage(hInstance,
			     MAKEINTRESOURCE(IDB_PAGEINFO_SECURE),
			     IMAGE_BITMAP,
			     0,
			     0,
			     LR_DEFAULTCOLOR);
}


/**
 * create a new gui_window to contain a browser_window.
 *
 * \param bw the browser_window to connect to the new gui_window
 * \param existing An existing window.
 * \param flags The flags controlling the construction.
 * \return The new win32 gui window or NULL on error.
 */
static struct gui_window *
win32_window_create(struct browser_window *bw,
		    struct gui_window *existing,
		    gui_window_create_flags flags)
{
	struct gui_window *gw;

	NSLOG(netsurf, INFO, "Creating gui window for browser window %p", bw);

	gw = calloc(1, sizeof(struct gui_window));
	if (gw == NULL) {
		return NULL;
	}

	/* connect gui window to browser window */
	gw->bw = bw;

	gw->width = 800;
	gw->height = 600;
	gw->toolbuttonsize = 24;
	gw->requestscrollx = 0;
	gw->requestscrolly = 0;
	gw->localhistory = NULL;

	load_page_info_bitmaps(hinst, gw);

	gw->mouse = malloc(sizeof(struct browser_mouse));
	if (gw->mouse == NULL) {
		free(gw);
		NSLOG(netsurf, INFO, "Unable to allocate mouse state");
		return NULL;
	}
	gw->mouse->gui = gw;
	gw->mouse->state = 0;
	gw->mouse->pressed_x = 0;
	gw->mouse->pressed_y = 0;

	/* add window to list */
	if (window_list != NULL) {
		window_list->prev = gw;
	}
	gw->next = window_list;
	window_list = gw;

	gw->main = nsws_window_create(hinst, gw);
	gw->toolbar = nsws_window_create_toolbar(hinst, gw->main, gw);
	gw->statusbar = nsws_window_create_statusbar(hinst, gw->main, gw);
	gw->drawingarea = nsws_window_create_drawable(hinst, gw->main, gw);

	NSLOG(netsurf, INFO,
	      "new window: main:%p toolbar:%p statusbar %p drawingarea %p",
	      gw->main,
	      gw->toolbar,
	      gw->statusbar,
	      gw->drawingarea);

	font_hwnd = gw->drawingarea;
	open_windows++;
	ShowWindow(gw->main, SW_SHOWNORMAL);

	return gw;
}


/**
 * Destroy previously created win32 window
 *
 * \param w The gui window to destroy.
 */
static void win32_window_destroy(struct gui_window *w)
{
	if (w == NULL)
		return;

	if (w->prev != NULL)
		w->prev->next = w->next;
	else
		window_list = w->next;

	if (w->next != NULL)
		w->next->prev = w->prev;

	DestroyAcceleratorTable(w->acceltable);

	destroy_page_info_bitmaps(w);

	free(w);
	w = NULL;
}


/**
 * Find the current dimensions of a win32 browser window's content area.
 *
 * \param gw gui_window to measure
 * \param width	 receives width of window
 * \param height receives height of window
 * \return NSERROR_OK and width and height updated
 */
static nserror
win32_window_get_dimensions(struct gui_window *gw, int *width, int *height)
{
	*width = gw->width;
	*height = gw->height;

	NSLOG(netsurf, INFO, "gw:%p w=%d h=%d", gw, *width, *height);

	return NSERROR_OK;
}


/**
 * Update the extent of the inside of a browser window to that of the
 * current content.
 *
 * \param w gui_window to update the extent of
 */
static void win32_window_update_extent(struct gui_window *gw)
{
	struct rect rect;
	rect.x0 = rect.x1 = gw->scrollx;
	rect.y0 = rect.y1 = gw->scrolly;
	win32_window_set_scroll(gw, &rect);
}


/**
 * set win32 browser window title
 *
 * \param w the win32 gui window.
 * \param title to set on window
 */
static void win32_window_set_title(struct gui_window *w, const char *title)
{
	char *fulltitle;
	int wlen;
	LPWSTR enctitle;

	if (w == NULL) {
		return;
	}

	NSLOG(netsurf, INFO, "%p, title %s", w, title);
	fulltitle = malloc(strlen(title) + SLEN("  -  NetSurf") + 1);
	if (fulltitle == NULL) {
		NSLOG(netsurf, ERROR, "%s",
		      messages_get_errorcode(NSERROR_NOMEM));
		return;
	}

	strcpy(fulltitle, title);
	strcat(fulltitle, "  -  NetSurf");

	wlen = MultiByteToWideChar(CP_UTF8, 0, fulltitle, -1, NULL, 0);
	if (wlen == 0) {
		NSLOG(netsurf, ERROR, "failed encoding \"%s\"", fulltitle);
		free(fulltitle);
		return;
	}

	enctitle = malloc(2 * (wlen + 1));
	if (enctitle == NULL) {
		NSLOG(netsurf, ERROR, "%s encoding \"%s\" len %d",
		      messages_get_errorcode(NSERROR_NOMEM), fulltitle, wlen);
		free(fulltitle);
		return;
	}

	MultiByteToWideChar(CP_UTF8, 0, fulltitle, -1, enctitle, wlen);
	SetWindowTextW(w->main, enctitle);
	free(enctitle);
	free(fulltitle);
}


/**
 * Set the navigation url in a win32 browser window.
 *
 * \param gw window to update.
 * \param url The url to use as icon.
 */
static nserror win32_window_set_url(struct gui_window *gw, nsurl *url)
{
	SendMessage(gw->urlbar, WM_SETTEXT, 0, (LPARAM) nsurl_access(url));

	return NSERROR_OK;
}


/**
 * Set the status bar of a win32 browser window.
 *
 * \param w gui_window to update
 * \param text new status text
 */
static void win32_window_set_status(struct gui_window *w, const char *text)
{
	if (w == NULL) {
		return;
	}
	SendMessage(w->statusbar, WM_SETTEXT, 0, (LPARAM)text);
}


/**
 * Change the win32 mouse pointer shape
 *
 * \param w The gui window to change pointer shape in.
 * \param shape The new shape to change to.
 */
static void
win32_window_set_pointer(struct gui_window *w, gui_pointer_shape shape)
{
	SetCursor(nsws_get_pointer(shape));
}


/**
 * Give the win32 input focus to a window
 *
 * \param w window with caret
 * \param x coordinates of caret
 * \param y coordinates of caret
 * \param height height of caret
 * \param clip rectangle to clip caret or NULL if none
 */
static void
win32_window_place_caret(struct gui_window *w, int x, int y,
			 int height, const struct rect *clip)
{
	if (w == NULL) {
		return;
	}

	CreateCaret(w->drawingarea, (HBITMAP)NULL, 1, height );
	SetCaretPos(x - w->scrollx, y  - w->scrolly);
	ShowCaret(w->drawingarea);
}


/**
 * Remove the win32 input focus from window
 *
 * \param gw window with caret
 */
static void win32_window_remove_caret(struct gui_window *gw)
{
	if (gw == NULL)
		return;
	HideCaret(gw->drawingarea);
}


/**
 * start a win32 navigation throbber.
 *
 * \param w window in which to start throbber.
 */
static void win32_window_start_throbber(struct gui_window *w)
{
	if (w == NULL)
		return;
	nsws_window_update_forward_back(w);

	if (w->mainmenu != NULL) {
		EnableMenuItem(w->mainmenu, IDM_NAV_STOP, MF_ENABLED);
		EnableMenuItem(w->mainmenu, IDM_NAV_RELOAD, MF_GRAYED);
	}
	if (w->rclick != NULL) {
		EnableMenuItem(w->rclick, IDM_NAV_STOP, MF_ENABLED);
		EnableMenuItem(w->rclick, IDM_NAV_RELOAD, MF_GRAYED);
	}
	if (w->toolbar != NULL) {
		SendMessage(w->toolbar, TB_SETSTATE, (WPARAM) IDM_NAV_STOP,
			    MAKELONG(TBSTATE_ENABLED, 0));
		SendMessage(w->toolbar, TB_SETSTATE,
			    (WPARAM) IDM_NAV_RELOAD,
			    MAKELONG(TBSTATE_INDETERMINATE, 0));
	}
	w->throbbing = true;
	Animate_Play(w->throbber, 0, -1, -1);
}


/**
 * stop a win32 navigation throbber.
 *
 * \param w window with throbber to stop
 */
static void win32_window_stop_throbber(struct gui_window *w)
{
	if (w == NULL)
		return;

	nsws_window_update_forward_back(w);
	if (w->mainmenu != NULL) {
		EnableMenuItem(w->mainmenu, IDM_NAV_STOP, MF_GRAYED);
		EnableMenuItem(w->mainmenu, IDM_NAV_RELOAD, MF_ENABLED);
	}

	if (w->rclick != NULL) {
		EnableMenuItem(w->rclick, IDM_NAV_STOP, MF_GRAYED);
		EnableMenuItem(w->rclick, IDM_NAV_RELOAD, MF_ENABLED);
	}

	if (w->toolbar != NULL) {
		SendMessage(w->toolbar, TB_SETSTATE, (WPARAM) IDM_NAV_STOP,
			    MAKELONG(TBSTATE_INDETERMINATE, 0));
		SendMessage(w->toolbar, TB_SETSTATE,
			    (WPARAM) IDM_NAV_RELOAD,
			    MAKELONG(TBSTATE_ENABLED, 0));
	}

	w->throbbing = false;
	Animate_Stop(w->throbber);
	Animate_Seek(w->throbber, 0);
}


/**
 * win32 page info change.
 *
 * \param gw window to chnage info on
 */
static void win32_window_page_info_change(struct gui_window *gw)
{
	HWND hbutton;
	browser_window_page_info_state pistate;

	hbutton = GetProp(gw->toolbar, TEXT("hPGIbutton"));

	pistate = browser_window_get_page_info_state(gw->bw);

	SendMessageW(hbutton, BM_SETIMAGE, IMAGE_BITMAP,
		     (LPARAM)gw->hPageInfo[pistate]);
}


/**
 * process miscellaneous window events
 *
 * \param gw The window receiving the event.
 * \param event The event code.
 * \return NSERROR_OK when processed ok
 */
static nserror
win32_window_event(struct gui_window *gw, enum gui_window_event event)
{
	switch (event) {
	case GW_EVENT_UPDATE_EXTENT:
		win32_window_update_extent(gw);
		break;

	case GW_EVENT_REMOVE_CARET:
		win32_window_remove_caret(gw);
		break;

	case GW_EVENT_START_THROBBER:
		win32_window_start_throbber(gw);
		break;

	case GW_EVENT_STOP_THROBBER:
		win32_window_stop_throbber(gw);
		break;

	case GW_EVENT_PAGE_INFO_CHANGE:
		win32_window_page_info_change(gw);
		break;

	default:
		break;
	}
	return NSERROR_OK;
}

/**
 * win32 frontend browser window handling operation table
 */
static struct gui_window_table window_table = {
	.create = win32_window_create,
	.destroy = win32_window_destroy,
	.invalidate = win32_window_invalidate_area,
	.get_scroll = win32_window_get_scroll,
	.set_scroll = win32_window_set_scroll,
	.get_dimensions = win32_window_get_dimensions,
	.event = win32_window_event,

	.set_title = win32_window_set_title,
	.set_url = win32_window_set_url,
	.set_status = win32_window_set_status,
	.set_pointer = win32_window_set_pointer,
	.place_caret = win32_window_place_caret,
};

struct gui_window_table *win32_window_table = &window_table;


/* exported interface documented in windows/window.h */
struct gui_window *nsws_get_gui_window(HWND hwnd)
{
	struct gui_window *gw = NULL;
	HWND phwnd = hwnd;

	/* scan the window hierarchy for gui window */
	while (phwnd != NULL) {
		gw = GetProp(phwnd, TEXT("GuiWnd"));
		if (gw != NULL)
			break;
		phwnd = GetParent(phwnd);
	}

	if (gw == NULL) {
		/* try again looking for owner windows instead */
		phwnd = hwnd;
		while (phwnd != NULL) {
			gw = GetProp(phwnd, TEXT("GuiWnd"));
			if (gw != NULL)
				break;
			phwnd = GetWindow(phwnd, GW_OWNER);
		}
	}

	return gw;
}


/* exported interface documented in windows/window.h */
bool nsws_window_go(HWND hwnd, const char *urltxt)
{
	struct gui_window *gw;
	nsurl *url;
	nserror ret;

	gw = nsws_get_gui_window(hwnd);
	if (gw == NULL)
		return false;
	ret = nsurl_create(urltxt, &url);

	if (ret != NSERROR_OK) {
		win32_report_nserror(ret, 0);
	} else {
		browser_window_navigate(gw->bw,
					url,
					NULL,
					BW_NAVIGATE_HISTORY,
					NULL,
					NULL,
					NULL);
		nsurl_unref(url);
	}

	return true;
}


/* exported interface documented in windows/window.h */
nserror win32_window_set_scroll(struct gui_window *gw, const struct rect *rect)
{
	SCROLLINFO si;
	nserror res;
	int height;
	int width;
	POINT p;

	if ((gw == NULL) || (gw->bw == NULL)) {
		return NSERROR_BAD_PARAMETER;
	}

	res = browser_window_get_extents(gw->bw, true, &width, &height);
	if (res != NSERROR_OK) {
		return res;
	}

	/* The resulting gui window scroll must remain within the
	 * windows bounding box.
	 */
	if (rect->x0 < 0) {
		gw->requestscrollx = -gw->scrollx;
	} else if (rect->x0 > (width - gw->width)) {
		gw->requestscrollx = (width - gw->width) - gw->scrollx;
	} else {
		gw->requestscrollx = rect->x0 - gw->scrollx;
	}
	if (rect->y0 < 0) {
		gw->requestscrolly = -gw->scrolly;
	} else if (rect->y0 > (height - gw->height)) {
		gw->requestscrolly = (height - gw->height) - gw->scrolly;
	} else {
		gw->requestscrolly = rect->y0 - gw->scrolly;
	}

	NSLOG(netsurf, DEEPDEBUG,
	      "requestscroll x,y:%d,%d",
	      gw->requestscrollx, gw->requestscrolly);

	/* set the vertical scroll offset */
	si.cbSize = sizeof(si);
	si.fMask = SIF_ALL;
	si.nMin = 0;
	si.nMax = height - 1;
	si.nPage = gw->height;
	si.nPos = max(gw->scrolly + gw->requestscrolly, 0);
	si.nPos = min(si.nPos, height - gw->height);
	SetScrollInfo(gw->drawingarea, SB_VERT, &si, TRUE);
	NSLOG(netsurf, DEEPDEBUG,
	      "SetScrollInfo VERT min:%d max:%d page:%d pos:%d",
	      si.nMin, si.nMax, si.nPage, si.nPos);

	/* set the horizontal scroll offset */
	si.cbSize = sizeof(si);
	si.fMask = SIF_ALL;
	si.nMin = 0;
	si.nMax = width -1;
	si.nPage = gw->width;
	si.nPos = max(gw->scrollx + gw->requestscrollx, 0);
	si.nPos = min(si.nPos, width - gw->width);
	SetScrollInfo(gw->drawingarea, SB_HORZ, &si, TRUE);
	NSLOG(netsurf, DEEPDEBUG,
	      "SetScrollInfo HORZ min:%d max:%d page:%d pos:%d",
	      si.nMin, si.nMax, si.nPage, si.nPos);

	/* Set caret position */
	GetCaretPos(&p);
	HideCaret(gw->drawingarea);
	SetCaretPos(p.x - gw->requestscrollx, p.y - gw->requestscrolly);
	ShowCaret(gw->drawingarea);

	RECT r, redraw;
	r.top = 0;
	r.bottom = gw->height + 1;
	r.left = 0;
	r.right = gw->width + 1;
	ScrollWindowEx(gw->drawingarea,
		       - gw->requestscrollx,
		       - gw->requestscrolly,
		       &r,
		       NULL,
		       NULL,
		       &redraw,
		       SW_INVALIDATE);
	NSLOG(netsurf, DEEPDEBUG,
	      "ScrollWindowEx %d, %d",
	      - gw->requestscrollx,
	      - gw->requestscrolly);

	gw->scrolly += gw->requestscrolly;
	gw->scrollx += gw->requestscrollx;
	gw->requestscrollx = 0;
	gw->requestscrolly = 0;

	return NSERROR_OK;
}


/* exported interface documented in windows/window.h */
nserror
nsws_create_main_class(HINSTANCE hinstance)
{
	nserror ret = NSERROR_OK;
	WNDCLASSEXW wc;

	/* main window */
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = 0;
	wc.lpfnWndProc = nsws_window_event_callback;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hinstance;
	wc.hIcon = LoadIcon(hinstance, MAKEINTRESOURCE(IDR_NETSURF_ICON));
	wc.hCursor = NULL;
	wc.hbrBackground = (HBRUSH)(COLOR_MENU + 1);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = windowclassname_main;
	wc.hIconSm = LoadIcon(hinstance, MAKEINTRESOURCE(IDR_NETSURF_ICON));

	if (RegisterClassExW(&wc) == 0) {
		win_perror("MainWindowClass");
		ret = NSERROR_INIT_FAILED;
	}

	return ret;
}


/* exported interface documented in windows/window.h */
HWND gui_window_main_window(struct gui_window *w)
{
	if (w == NULL)
		return NULL;
	return w->main;
}
