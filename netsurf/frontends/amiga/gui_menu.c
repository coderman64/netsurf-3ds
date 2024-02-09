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

#include "amiga/os3support.h"

#include <string.h>
#include <stdlib.h>

#include <proto/dos.h>
#include <proto/asl.h>
#include <proto/exec.h>
#include <proto/gadtools.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#ifdef __amigaos4__
#include <dos/anchorpath.h>
#include <dos/obsolete.h> /* Needed for ExAll() */
#endif

#include <libraries/gadtools.h>
#ifdef __amigaos4__
#include <intuition/menuclass.h>
#endif

#include <classes/window.h>
#include <proto/label.h>
#include <images/label.h>
#include <proto/bitmap.h>
#include <images/bitmap.h>

#include <reaction/reaction_macros.h>

#include "utils/nsoption.h"
#include "utils/messages.h"
#include "utils/log.h"
#include "utils/utils.h"
#include "utils/nsurl.h"
#include "netsurf/browser_window.h"
#include "netsurf/mouse.h"
#include "netsurf/window.h"
#include "netsurf/content.h"
#include "netsurf/keypress.h"
#include "desktop/hotlist.h"
#include "desktop/version.h"

#include "amiga/arexx.h"
#include "amiga/bitmap.h"
#include "amiga/clipboard.h"
#include "amiga/cookies.h"
#include "amiga/file.h"
#include "amiga/filetype.h"
#include "amiga/gui.h"
#include "amiga/gui_menu.h"
#include "amiga/gui_options.h"
#include "amiga/history.h"
#include "amiga/history_local.h"
#include "amiga/hotlist.h"
#include "amiga/libs.h"
#include "amiga/menu.h"
#include "amiga/misc.h"
#include "amiga/nsoption.h"
#include "amiga/print.h"
#include "amiga/search.h"
#include "amiga/theme.h"
#include "amiga/utf8.h"
#include "amiga/schedule.h"

#ifdef __amigaos4__
static struct Menu *restrict gui_menu = NULL;
static int gui_menu_count = 0;
struct ami_menu_data *gui_menu_data[AMI_MENU_AREXX_MAX + 1];
#endif

static bool ami_menu_check_toggled = false;
static bool menu_quit = false;

const char * const netsurf_version;
const char * const verdate;

static nserror ami_menu_scan(struct ami_menu_data **md);
void ami_menu_arexx_scan(struct ami_menu_data **md);

/*
 * The below functions are called automatically by window.class when menu items are selected.
 */

HOOKF(void, ami_menu_item_project_newwin, APTR, window, struct IntuiMessage *)
{
	nsurl *url;
	nserror error;

	error = nsurl_create(nsoption_charp(homepage_url), &url);
	if (error == NSERROR_OK) {
		error = browser_window_create(BW_CREATE_HISTORY,
					      url,
					      NULL,
					      NULL,
					      NULL);
		nsurl_unref(url);
	}
	if (error != NSERROR_OK) {
		amiga_warn_user(messages_get_errorcode(error), 0);
	}
}

HOOKF(void, ami_menu_item_project_newtab, APTR, window, struct IntuiMessage *)
{
	struct gui_window_2 *gwin;

	GetAttr(WINDOW_UserData, (Object *)window, (ULONG *)&gwin);
	ami_gui_new_blank_tab(gwin);
}

HOOKF(void, ami_menu_item_project_open, APTR, window, struct IntuiMessage *)
{
	struct gui_window_2 *gwin;
	GetAttr(WINDOW_UserData, (Object *)window, (ULONG *)&gwin);

	ami_file_open(gwin);
}

HOOKF(void, ami_menu_item_project_save, APTR, window, struct IntuiMessage *)
{
	struct gui_window_2 *gwin;
	ULONG type = (ULONG)hook->h_Data;

	GetAttr(WINDOW_UserData, (Object *)window, (ULONG *)&gwin);

	ami_file_save_req(type, gwin, browser_window_get_content(ami_gui2_get_browser_window(gwin)));
}

HOOKF(void, ami_menu_item_project_closetab, APTR, window, struct IntuiMessage *)
{
	struct gui_window_2 *gwin;
	GetAttr(WINDOW_UserData, (Object *)window, (ULONG *)&gwin);

	browser_window_destroy(ami_gui2_get_browser_window(gwin));
}

HOOKF(void, ami_menu_item_project_closewin, APTR, window, struct IntuiMessage *)
{
	struct gui_window_2 *gwin;
	GetAttr(WINDOW_UserData, (Object *)window, (ULONG *)&gwin);

	ami_gui2_set_closed(gwin, true);
}

HOOKF(void, ami_menu_item_project_print, APTR, window, struct IntuiMessage *)
{
	struct gui_window_2 *gwin;
	GetAttr(WINDOW_UserData, (Object *)window, (ULONG *)&gwin);

	ami_set_pointer(gwin, GUI_POINTER_WAIT, false);
	ami_print_ui(browser_window_get_content(ami_gui2_get_browser_window(gwin)));
	ami_reset_pointer(gwin);
}

HOOKF(void, ami_menu_item_project_about, APTR, window, struct IntuiMessage *)
{
	struct gui_window_2 *gwin;
	char *temp, *temp2;
	int sel;
	nsurl *url = NULL;
	nserror error = NSERROR_OK;

	GetAttr(WINDOW_UserData, (Object *)window, (ULONG *)&gwin);

	ami_set_pointer(gwin, GUI_POINTER_WAIT, false);

	temp = ASPrintf("%s|%s|%s", messages_get("OK"),
								messages_get("HelpCredits"),
								messages_get("HelpLicence"));

	temp2 = ami_utf8_easy(temp);
	FreeVec(temp);
#ifdef __amigaos4__
	sel = TimedDosRequesterTags(TDR_ImageType,TDRIMAGE_INFO,
				TDR_TitleString, messages_get("NetSurf"),
				TDR_Window, ami_gui2_get_window(gwin),
				TDR_GadgetString, temp2,
				TDR_FormatString,"NetSurf %s\nBuild date %s\n\nhttp://www.netsurf-browser.org",
				TDR_Arg1,netsurf_version,
				TDR_Arg2,verdate,
				TAG_DONE);
#else
	struct EasyStruct about_req = {
		sizeof(struct EasyStruct),
		0,
		"NetSurf",
		"NetSurf %s\nBuild date %s\n\nhttp://www.netsurf-browser.org",
		temp2,
	};

	sel = EasyRequest(ami_gui2_get_window(gwin), &about_req, NULL, netsurf_version, verdate);
#endif
	free(temp2);

	if(sel == 2) {
		error = nsurl_create("about:credits", &url);
	} else if(sel == 0) {
		error = nsurl_create("about:licence", &url);
	}

	if(url) {
		if (error == NSERROR_OK) {
			error = browser_window_create(BW_CREATE_HISTORY,
							  url,
							  NULL,
							  NULL,
							  NULL);
			nsurl_unref(url);
		}
		if (error != NSERROR_OK) {
			amiga_warn_user(messages_get_errorcode(error), 0);
		}
	}

	ami_reset_pointer(gwin);
}

HOOKF(void, ami_menu_item_project_quit, APTR, window, struct IntuiMessage *)
{
	menu_quit = true;
}

HOOKF(void, ami_menu_item_edit_cut, APTR, window, struct IntuiMessage *)
{
	struct gui_window_2 *gwin;
	GetAttr(WINDOW_UserData, (Object *)window, (ULONG *)&gwin);

	browser_window_key_press(ami_gui2_get_browser_window(gwin), NS_KEY_CUT_SELECTION);
}

HOOKF(void, ami_menu_item_edit_copy, APTR, window, struct IntuiMessage *)
{
	struct bitmap *bm;
	struct gui_window_2 *gwin;
	GetAttr(WINDOW_UserData, (Object *)window, (ULONG *)&gwin);

	if(browser_window_can_select(ami_gui2_get_browser_window(gwin))) {
		browser_window_key_press(ami_gui2_get_browser_window(gwin), NS_KEY_COPY_SELECTION);
		browser_window_key_press(ami_gui2_get_browser_window(gwin), NS_KEY_CLEAR_SELECTION);
	}
	else if((bm = content_get_bitmap(browser_window_get_content(ami_gui2_get_browser_window(gwin))))) {
		/** @todo It should be checked that the lifetime of
		 * the objects containing the values returned (and the
		 * constness cast away) is safe.
		 */
		ami_bitmap_set_url(bm, browser_window_access_url(ami_gui2_get_browser_window(gwin)));
		ami_bitmap_set_title(bm, browser_window_get_title(ami_gui2_get_browser_window(gwin)));
		ami_easy_clipboard_bitmap(bm);
	}
#ifdef WITH_NS_SVG
	else if(ami_mime_compare(browser_window_get_content(ami_gui2_get_browser_window(gwin)), "svg") == true) {
		ami_easy_clipboard_svg(browser_window_get_content(ami_gui2_get_browser_window(gwin)));
	}
#endif
}

HOOKF(void, ami_menu_item_edit_paste, APTR, window, struct IntuiMessage *)
{
	struct gui_window_2 *gwin;
	GetAttr(WINDOW_UserData, (Object *)window, (ULONG *)&gwin);

	browser_window_key_press(ami_gui2_get_browser_window(gwin), NS_KEY_PASTE);
}

HOOKF(void, ami_menu_item_edit_selectall, APTR, window, struct IntuiMessage *)
{
	struct gui_window_2 *gwin;
	GetAttr(WINDOW_UserData, (Object *)window, (ULONG *)&gwin);

	browser_window_key_press(ami_gui2_get_browser_window(gwin), NS_KEY_SELECT_ALL);
	gui_start_selection(ami_gui2_get_gui_window(gwin));
}

HOOKF(void, ami_menu_item_edit_clearsel, APTR, window, struct IntuiMessage *)
{
	struct gui_window_2 *gwin;
	GetAttr(WINDOW_UserData, (Object *)window, (ULONG *)&gwin);

	browser_window_key_press(ami_gui2_get_browser_window(gwin), NS_KEY_CLEAR_SELECTION);
}

HOOKF(void, ami_menu_item_edit_undo, APTR, window, struct IntuiMessage *)
{
	struct gui_window_2 *gwin;
	GetAttr(WINDOW_UserData, (Object *)window, (ULONG *)&gwin);

	browser_window_key_press(ami_gui2_get_browser_window(gwin), NS_KEY_UNDO);
}

HOOKF(void, ami_menu_item_edit_redo, APTR, window, struct IntuiMessage *)
{
	struct gui_window_2 *gwin;
	GetAttr(WINDOW_UserData, (Object *)window, (ULONG *)&gwin);

	browser_window_key_press(ami_gui2_get_browser_window(gwin), NS_KEY_REDO);
}

HOOKF(void, ami_menu_item_browser_find, APTR, window, struct IntuiMessage *)
{
	struct gui_window_2 *gwin;
	GetAttr(WINDOW_UserData, (Object *)window, (ULONG *)&gwin);

	ami_search_open(ami_gui2_get_gui_window(gwin));
}

HOOKF(void, ami_menu_item_browser_localhistory, APTR, window, struct IntuiMessage *)
{
	struct gui_window_2 *gwin;
	GetAttr(WINDOW_UserData, (Object *)window, (ULONG *)&gwin);

	ami_history_local_present(ami_gui2_get_gui_window(gwin));
}

HOOKF(void, ami_menu_item_browser_globalhistory, APTR, window, struct IntuiMessage *)
{
	ami_history_global_present();
}

HOOKF(void, ami_menu_item_browser_cookies, APTR, window, struct IntuiMessage *)
{
	ami_cookies_present(NULL);
}

HOOKF(void, ami_menu_item_browser_foreimg, APTR, window, struct IntuiMessage *)
{
	struct Menu *menustrip;
	bool checked = false;

	GetAttr(WINDOW_MenuStrip, (Object *)window, (ULONG *)&menustrip);
	checked = ami_menu_get_selected(menustrip, msg);

	nsoption_set_bool(foreground_images, checked);
	ami_gui_menu_set_check_toggled();
}

HOOKF(void, ami_menu_item_browser_backimg, APTR, window, struct IntuiMessage *)
{
	struct Menu *menustrip;
	bool checked = false;

	GetAttr(WINDOW_MenuStrip, (Object *)window, (ULONG *)&menustrip);
	checked = ami_menu_get_selected(menustrip, msg);
	
	nsoption_set_bool(background_images, checked);
	ami_gui_menu_set_check_toggled();
}

HOOKF(void, ami_menu_item_browser_enablejs, APTR, window, struct IntuiMessage *)
{
	struct Menu *menustrip;
	bool checked = false;

	GetAttr(WINDOW_MenuStrip, (Object *)window, (ULONG *)&menustrip);
	checked = ami_menu_get_selected(menustrip, msg);
	
	nsoption_set_bool(enable_javascript, checked);
	ami_gui_menu_set_check_toggled();
}

HOOKF(void, ami_menu_item_browser_scale_decrease, APTR, window, struct IntuiMessage *)
{
	struct gui_window_2 *gwin;
	GetAttr(WINDOW_UserData, (Object *)window, (ULONG *)&gwin);

	ami_gui_adjust_scale(ami_gui2_get_gui_window(gwin), -0.1);
}

HOOKF(void, ami_menu_item_browser_scale_normal, APTR, window, struct IntuiMessage *)
{
	struct gui_window_2 *gwin;
	GetAttr(WINDOW_UserData, (Object *)window, (ULONG *)&gwin);

	ami_gui_set_scale(ami_gui2_get_gui_window(gwin), 1.0);
}

HOOKF(void, ami_menu_item_browser_scale_increase, APTR, window, struct IntuiMessage *)
{
	struct gui_window_2 *gwin;
	GetAttr(WINDOW_UserData, (Object *)window, (ULONG *)&gwin);

	ami_gui_adjust_scale(ami_gui2_get_gui_window(gwin), +0.1);
}

HOOKF(void, ami_menu_item_browser_redraw, APTR, window, struct IntuiMessage *)
{
	struct gui_window_2 *gwin;
	GetAttr(WINDOW_UserData, (Object *)window, (ULONG *)&gwin);

	ami_schedule_redraw(gwin, true);
	ami_gui2_set_new_content(gwin, true);
}

HOOKF(void, ami_menu_item_hotlist_add, APTR, window, struct IntuiMessage *)
{
	struct browser_window *bw;
	struct gui_window_2 *gwin;
	GetAttr(WINDOW_UserData, (Object *)window, (ULONG *)&gwin);

	bw = ami_gui2_get_browser_window(gwin);

	if (bw == NULL || browser_window_has_content(bw) == false)
		return;

	hotlist_add_url(browser_window_access_url(bw));
	ami_gui_update_hotlist_button(gwin);
}

HOOKF(void, ami_menu_item_hotlist_show, APTR, window, struct IntuiMessage *)
{
	ami_hotlist_present();
}

HOOKF(void, ami_menu_item_hotlist_entries, APTR, window, struct IntuiMessage *)
{
	nsurl *url = hook->h_Data;
	struct gui_window_2 *gwin;
	GetAttr(WINDOW_UserData, (Object *)window, (ULONG *)&gwin);

	if(url == NULL) return;

	browser_window_navigate(ami_gui2_get_browser_window(gwin),
					url,
					NULL,
					BW_NAVIGATE_HISTORY,
					NULL,
					NULL,
					NULL);
}

HOOKF(void, ami_menu_item_settings_edit, APTR, window, struct IntuiMessage *)
{
	ami_gui_opts_open();
}

HOOKF(void, ami_menu_item_settings_snapshot, APTR, window, struct IntuiMessage *)
{
	struct gui_window_2 *gwin;
	GetAttr(WINDOW_UserData, (Object *)window, (ULONG *)&gwin);

	nsoption_set_int(window_x, ami_gui2_get_window(gwin)->LeftEdge);
	nsoption_set_int(window_y, ami_gui2_get_window(gwin)->TopEdge);
	nsoption_set_int(window_width, ami_gui2_get_window(gwin)->Width);
	nsoption_set_int(window_height, ami_gui2_get_window(gwin)->Height);
}

HOOKF(void, ami_menu_item_settings_save, APTR, window, struct IntuiMessage *)
{
	ami_nsoption_write();
}

HOOKF(void, ami_menu_item_arexx_execute, APTR, window, struct IntuiMessage *)
{
	char *temp;
	struct gui_window_2 *gwin;
	GetAttr(WINDOW_UserData, (Object *)window, (ULONG *)&gwin);

	if(AslRequestTags(filereq,
						ASLFR_Window, ami_gui2_get_window(gwin),
						ASLFR_SleepWindow, TRUE,
						ASLFR_TitleText, messages_get("NetSurf"),
						ASLFR_Screen, ami_gui_get_screen(),
						ASLFR_DoSaveMode, FALSE,
						ASLFR_InitialDrawer, nsoption_charp(arexx_dir),
						ASLFR_InitialPattern, "#?.nsrx",
						TAG_DONE)) {
		if((temp = malloc(1024))) {
			strlcpy(temp, filereq->fr_Drawer, 1024);
			AddPart(temp, filereq->fr_File, 1024);
			ami_arexx_execute(temp);
			free(temp);
		}
	}
}

HOOKF(void, ami_menu_item_arexx_entries, APTR, window, struct IntuiMessage *)
{
	char *script = hook->h_Data;
	char *temp;
	struct gui_window_2 *gwin;
	GetAttr(WINDOW_UserData, (Object *)window, (ULONG *)&gwin);

	if(script) {
		if((temp = malloc(1024))) {
			BPTR lock;
			if((lock = Lock(nsoption_charp(arexx_dir), SHARED_LOCK))) {
				DevNameFromLock(lock, temp, 1024, DN_FULLPATH);
				AddPart(temp, script, 1024);
				ami_arexx_execute(temp);
				UnLock(lock);
			}
			free(temp);
		}
	}
}

/* normal GUI-specific menu functions */

ULONG ami_gui_menu_number(int item)
{
	/* horrible, horrible, horrible */
	ULONG menu_num;

	switch(item) {
		case M_SAVETXT:
			menu_num = FULLMENUNUM(0,4,1);
		break;

		case M_SAVECOMP:
			menu_num = FULLMENUNUM(0,4,2);
		break;

		case M_SAVEIFF:
			menu_num = FULLMENUNUM(0,4,3);
		break;
#ifdef WITH_PDF_EXPORT
		case M_SAVEPDF:
			menu_num = FULLMENUNUM(0,4,4);
		break;
#endif
		case M_CLOSETAB:
			menu_num = FULLMENUNUM(0,8,0);
		break;

		case M_CUT:
			menu_num = FULLMENUNUM(1,0,0);
		break;

		case M_COPY:
			menu_num = FULLMENUNUM(1,1,0);
		break;

		case M_PASTE:
			menu_num = FULLMENUNUM(1,2,0);
		break;

		case M_SELALL:
			menu_num = FULLMENUNUM(1,4,0);
		break;

		case M_CLEAR:
			menu_num = FULLMENUNUM(1,5,0);
		break;

		case M_UNDO:
			menu_num = FULLMENUNUM(1,8,0);
		break;

		case M_REDO:
			menu_num = FULLMENUNUM(1,9,0);
		break;

		case M_FIND:
			menu_num = FULLMENUNUM(2,0,0);
		break;

		case M_IMGFORE:
			menu_num = FULLMENUNUM(2,8,0);
		break;

		case M_IMGBACK:
			menu_num = FULLMENUNUM(2,8,1);
		break;

		case M_JS:
			menu_num = FULLMENUNUM(2,9,0);
		break;

		default:
			NSLOG(netsurf, INFO,
			      "WARNING: Unrecognised menu item %d", item);
			menu_num = 0;
		break;
	}

	return menu_num;
}

#ifdef __amigaos4__
static void ami_gui_menu_set_checked_mc(struct Menu *menu, int item, bool check)
{
	ULONG check_state = MS_CHECKED;

	if(check == false) {
		check_state = 0;
	}

	if(menu == NULL) {
		menu = gui_menu;
	}

	IDoMethod((Object *)menu, MM_SETSTATE, 0, item, MS_CHECKED, check_state);
}
#endif

static void ami_gui_menu_set_checked_gt(struct Menu *menu, int item, bool check)
{
	if(menu == NULL) {
		return;
	}

	if(check == true) {
		if((ItemAddress(menu, ami_gui_menu_number(item))->Flags & CHECKED) == 0)
			ItemAddress(menu, ami_gui_menu_number(item))->Flags ^= CHECKED;
	} else {
		if(ItemAddress(menu, ami_gui_menu_number(item))->Flags & CHECKED)
			ItemAddress(menu, ami_gui_menu_number(item))->Flags ^= CHECKED;
	}
}

void ami_gui_menu_set_checked(struct Menu *menu, int item, bool check)
{
	if(LIB_IS_AT_LEAST((struct Library *)IntuitionBase, 54, 6)) {
#ifdef __amigaos4__
		return ami_gui_menu_set_checked_mc(menu, item, check);
#endif	
	} else {
		return ami_gui_menu_set_checked_gt(menu, item, check);
	}
}

#ifdef __amigaos4__
static void ami_gui_menu_set_disabled_mc(struct Window *win, struct Menu *menu, int item, bool disable)
{
	ULONG disable_state = MS_DISABLED;

	if(disable == false) {
		disable_state = 0;
	}

	IDoMethod((Object *)menu, MM_SETSTATE, 0, item, MS_DISABLED, disable_state);
}
#endif

static void ami_gui_menu_set_disabled_gt(struct Window *win, struct Menu *menu, int item, bool disable)
{
	ULONG menu_num = ami_gui_menu_number(item);

	if(disable == false) {
		OnMenu(win, menu_num);
	} else {
		OffMenu(win, menu_num);
	}
}

void ami_gui_menu_set_disabled(struct Window *win, struct Menu *menu, int item, bool disable)
{
	if(LIB_IS_AT_LEAST((struct Library *)IntuitionBase, 54, 6)) {
#ifdef __amigaos4__
		return ami_gui_menu_set_disabled_mc(win, menu, item, disable);
#endif	
	} else {
		return ami_gui_menu_set_disabled_gt(win, menu, item, disable);
	}
}


void ami_gui_menu_update_checked(struct gui_window_2 *gwin)
{
	if(LIB_IS_AT_LEAST((struct Library *)IntuitionBase, 54, 6)) {
		/* Irrelevant when using MenuClass */
		return;
	}

	struct Menu *menustrip;

	GetAttr(WINDOW_MenuStrip, ami_gui2_get_object(gwin, AMI_WIN_MAIN), (ULONG *)&menustrip);
	if(!menustrip) return;
	if(nsoption_bool(enable_javascript) == true) {
		if((ItemAddress(menustrip, ami_gui_menu_number(M_JS))->Flags & CHECKED) == 0)
			ItemAddress(menustrip, ami_gui_menu_number(M_JS))->Flags ^= CHECKED;
	} else {
		if(ItemAddress(menustrip, ami_gui_menu_number(M_JS))->Flags & CHECKED)
			ItemAddress(menustrip, ami_gui_menu_number(M_JS))->Flags ^= CHECKED;
	}
	if(nsoption_bool(foreground_images) == true) {
		if((ItemAddress(menustrip, ami_gui_menu_number(M_IMGFORE))->Flags & CHECKED) == 0)
			ItemAddress(menustrip, ami_gui_menu_number(M_IMGFORE))->Flags ^= CHECKED;
	} else {
		if(ItemAddress(menustrip, ami_gui_menu_number(M_IMGFORE))->Flags & CHECKED)
			ItemAddress(menustrip, ami_gui_menu_number(M_IMGFORE))->Flags ^= CHECKED;
	}

	if(nsoption_bool(background_images) == true) {
		if((ItemAddress(menustrip, ami_gui_menu_number(M_IMGBACK))->Flags & CHECKED) == 0)
			ItemAddress(menustrip, ami_gui_menu_number(M_IMGBACK))->Flags ^= CHECKED;
	} else {
		if(ItemAddress(menustrip, ami_gui_menu_number(M_IMGBACK))->Flags & CHECKED)
			ItemAddress(menustrip, ami_gui_menu_number(M_IMGBACK))->Flags ^= CHECKED;
	}

	ResetMenuStrip(ami_gui2_get_window(gwin), menustrip);
}

void ami_gui_menu_update_disabled(struct gui_window *g, struct hlcache_handle *c)
{
	struct Window *win = ami_gui_get_window(g);

	if(nsoption_bool(kiosk_mode) == true) return;

	if(content_get_type(c) <= CONTENT_CSS)
	{
		ami_gui_menu_set_disabled(win, ami_gui_get_menu(g), M_SAVETXT, false);
		ami_gui_menu_set_disabled(win, ami_gui_get_menu(g), M_SAVECOMP, false);
#ifdef WITH_PDF_EXPORT
		ami_gui_menu_set_disabled(win, ami_gui_get_menu(g), M_SAVEPDF, false);
#endif
#if 0
		if(browser_window_get_editor_flags(ami_gui_get_browser_window(g)) & BW_EDITOR_CAN_COPY) {
			OnMenu(win,AMI_MENU_COPY);
			OnMenu(win,AMI_MENU_CLEAR);
		} else {
			OffMenu(win,AMI_MENU_COPY);
			OffMenu(win,AMI_MENU_CLEAR);	
		}

		if(browser_window_get_editor_flags(ami_gui_get_browser_window(g)) & BW_EDITOR_CAN_CUT)
			OnMenu(win,AMI_MENU_CUT);
		else
			OffMenu(win,AMI_MENU_CUT);		
		
		if(browser_window_get_editor_flags(ami_gui_get_browser_window(g)) & BW_EDITOR_CAN_PASTE)
			OnMenu(win,AMI_MENU_PASTE);
		else
			OffMenu(win,AMI_MENU_PASTE);
#else
		ami_gui_menu_set_disabled(win, ami_gui_get_menu(g), M_CUT, false);
		ami_gui_menu_set_disabled(win, ami_gui_get_menu(g), M_COPY, false);
		ami_gui_menu_set_disabled(win, ami_gui_get_menu(g), M_PASTE, false);
		ami_gui_menu_set_disabled(win, ami_gui_get_menu(g), M_CLEAR, false);
#endif
		ami_gui_menu_set_disabled(win, ami_gui_get_menu(g), M_SELALL, false);
		ami_gui_menu_set_disabled(win, ami_gui_get_menu(g), M_FIND, false);
		ami_gui_menu_set_disabled(win, ami_gui_get_menu(g), M_SAVEIFF, true);
	}
	else
	{
		ami_gui_menu_set_disabled(win, ami_gui_get_menu(g), M_CUT, true);
		ami_gui_menu_set_disabled(win, ami_gui_get_menu(g), M_PASTE, true);
		ami_gui_menu_set_disabled(win, ami_gui_get_menu(g), M_CLEAR, true);

		ami_gui_menu_set_disabled(win, ami_gui_get_menu(g), M_SAVETXT, true);
		ami_gui_menu_set_disabled(win, ami_gui_get_menu(g), M_SAVECOMP, true);
#ifdef WITH_PDF_EXPORT
		ami_gui_menu_set_disabled(win, ami_gui_get_menu(g), M_SAVEPDF, true);
#endif

		ami_gui_menu_set_disabled(win, ami_gui_get_menu(g), M_SELALL, true);
		ami_gui_menu_set_disabled(win, ami_gui_get_menu(g), M_FIND, true);

#ifdef WITH_NS_SVG
		if(content_get_bitmap(c) || (ami_mime_compare(c, "svg") == true))
#else
		if(content_get_bitmap(c))
#endif
		{
			ami_gui_menu_set_disabled(win, ami_gui_get_menu(g), M_COPY, false);
			ami_gui_menu_set_disabled(win, ami_gui_get_menu(g), M_SAVEIFF, false);
		}
		else
		{
			ami_gui_menu_set_disabled(win, ami_gui_get_menu(g), M_COPY, true);
			ami_gui_menu_set_disabled(win, ami_gui_get_menu(g), M_SAVEIFF, true);
		}
	}
}

void ami_gui_menu_set_check_toggled(void)
{
	if(LIB_IS_AT_LEAST((struct Library *)IntuitionBase, 54, 6)) {
		/* Irrelevant when using MenuClass */
		return;
	}

	ami_menu_check_toggled = true;
}

bool ami_gui_menu_get_check_toggled(void)
{
	if(LIB_IS_AT_LEAST((struct Library *)IntuitionBase, 54, 6)) {
		/* Irrelevant when using MenuClass */
		return false;
	}

	bool check_toggled = ami_menu_check_toggled;
	ami_menu_check_toggled = false;
	return check_toggled;
}

void ami_menu_arexx_scan(struct ami_menu_data **md)
{
	/**\todo Rewrite this to not use ExAll() **/
	int item = AMI_MENU_AREXX;
	BPTR lock = 0;
	UBYTE *buffer;
	struct ExAllControl *ctrl;
	char matchpatt[16];
	LONG cont;
	struct ExAllData *ead;
	char *menu_lab;

	if((lock = Lock(nsoption_charp(arexx_dir), SHARED_LOCK))) {
		if((buffer = malloc(1024))) {
			if((ctrl = AllocDosObject(DOS_EXALLCONTROL,NULL))) {
				ctrl->eac_LastKey = 0;

				if(ParsePatternNoCase("#?.nsrx",(char *)&matchpatt,16) != -1) {
					ctrl->eac_MatchString = (char *)&matchpatt;
				}

				do {
					cont = ExAll(lock,(struct ExAllData *)buffer,1024,ED_COMMENT,ctrl);
					if((!cont) && (IoErr() != ERROR_NO_MORE_ENTRIES)) break;
					if(!ctrl->eac_Entries) continue;

					for(ead = (struct ExAllData *)buffer; ead; ead = ead->ed_Next) {
						if(item >= AMI_MENU_AREXX_MAX) continue;
						if(EAD_IS_FILE(ead)) {
							if(ead->ed_Comment[0] != '\0')
								menu_lab = ead->ed_Comment;
							else
								menu_lab = ead->ed_Name;

							ami_menu_alloc_item(md, item, NM_ITEM, menu_lab, NULL, NSA_SPACE,
								ami_menu_item_arexx_entries, (void *)strdup(ead->ed_Name), 0);

							item++;
						}
					}
				} while(cont);
				FreeDosObject(DOS_EXALLCONTROL,ctrl);
			}
			free(buffer);
		}
		UnLock(lock);
	}

	ami_menu_alloc_item(md, item, NM_END, NULL, NULL, NULL, NULL, NULL, 0);
}

static bool ami_menu_hotlist_add(void *userdata, int level, int item, const char *title, nsurl *url, bool is_folder)
{
	UBYTE type;
	STRPTR icon;
	UWORD flags = 0;
	struct ami_menu_data **md = (struct ami_menu_data **)userdata;

	if(item >= AMI_MENU_HOTLIST_MAX) return false;

	switch(level) {
		case 1:
			type = NM_ITEM;
		break;
		case 2:
			type = NM_SUB;
		break;
		default:
			if(LIB_IS_AT_LEAST((struct Library *)IntuitionBase, 54, 6)) {
				type = NM_SUB + (level - 2);
			} else {
				/* entries not at level 1 or 2 are not able to be added */
				return false;
			}
		break;
	}

	if(is_folder == true) {
		icon = ASPrintf("icons/directory.png");
	} else {
		icon = ami_gui_get_cache_favicon_name(url, true);
		if (icon == NULL) icon = ASPrintf("icons/content.png");
	}

	if(!LIB_IS_AT_LEAST((struct Library *)IntuitionBase, 54, 6)) {
		if((is_folder == true) && (type == NM_SUB)) {
			flags = NM_ITEMDISABLED;
		}
	}

	ami_menu_alloc_item(md, item, type, title,
		NULL, icon, ami_menu_item_hotlist_entries, (void *)url, flags);

	if(icon) FreeVec(icon);

	return true;
}

static nserror ami_menu_scan(struct ami_menu_data **md)
{
	ami_menu_alloc_item(md, M_HLADD,    NM_ITEM, "HotlistAdd",   "B", "TBImages:list_favouriteadd",
			ami_menu_item_hotlist_add, NULL, 0);
	ami_menu_alloc_item(md, M_HLSHOW,   NM_ITEM,"HotlistShowNS", "H", "TBImages:list_favourite",
			ami_menu_item_hotlist_show, NULL, 0);
	ami_menu_alloc_item(md, M_BAR_H1,   NM_ITEM, NM_BARLABEL,    NULL, NULL, NULL, NULL, 0);

	return ami_hotlist_scan((void *)md, AMI_MENU_HOTLIST, messages_get("HotlistMenu"), ami_menu_hotlist_add);
}

static void ami_init_menulabs(struct ami_menu_data **md)
{
	UWORD js_flags = CHECKIT | MENUTOGGLE;
	if(nsoption_bool(enable_javascript) == true)
		js_flags |= CHECKED;

	UWORD imgfore_flags = CHECKIT | MENUTOGGLE;
	if(nsoption_bool(foreground_images) == true)
		imgfore_flags |= CHECKED;

	UWORD imgback_flags = CHECKIT | MENUTOGGLE;
	if(nsoption_bool(background_images) == true)
		imgback_flags |= CHECKED;

	ami_menu_alloc_item(md, M_PROJECT, NM_TITLE, "Project",      NULL, NULL, NULL, NULL, 0);
	ami_menu_alloc_item(md, M_NEWWIN,   NM_ITEM, "NewWindowNS",  "N", "TBImages:list_app",
			ami_menu_item_project_newwin, NULL, 0);
	ami_menu_alloc_item(md, M_NEWTAB,   NM_ITEM, "NewTab",       "T", "TBImages:list_tab",
			ami_menu_item_project_newtab, NULL, 0);
	ami_menu_alloc_item(md, M_BAR_P1,   NM_ITEM, NM_BARLABEL,    NULL, NULL, NULL, NULL, 0);
	ami_menu_alloc_item(md, M_OPEN,     NM_ITEM, "OpenFile",     "O", "TBImages:list_folder_misc",
			ami_menu_item_project_open, NULL, 0);
	ami_menu_alloc_item(md, M_SAVEAS,   NM_ITEM, "SaveAsNS",     NULL, "TBImages:list_saveas", NULL, NULL, 0);
	ami_menu_alloc_item(md, M_SAVESRC,   NM_SUB, "Source",       "S", NULL,
			ami_menu_item_project_save, (void *)AMINS_SAVE_SOURCE, 0);
	ami_menu_alloc_item(md, M_SAVETXT,   NM_SUB, "TextNS",       NULL, NULL,
			ami_menu_item_project_save, (void *)AMINS_SAVE_TEXT, 0);
	ami_menu_alloc_item(md, M_SAVECOMP,  NM_SUB, "SaveCompNS",   NULL, NULL,
			ami_menu_item_project_save, (void *)AMINS_SAVE_COMPLETE, 0);
#ifdef WITH_PDF_EXPORT
	ami_menu_alloc_item(md, M_SAVEPDF,   NM_SUB, "PDFNS",        NULL, NULL,
			ami_menu_item_project_save, (void *)AMINS_SAVE_PDF, 0);
#endif
	ami_menu_alloc_item(md, M_SAVEIFF,   NM_SUB, "IFF",          NULL, NULL,
			ami_menu_item_project_save, (void *)AMINS_SAVE_IFF, 0);
	ami_menu_alloc_item(md, M_BAR_P2,   NM_ITEM, NM_BARLABEL,    NULL, NULL, NULL, NULL, 0);
	ami_menu_alloc_item(md, M_PRINT,    NM_ITEM, "PrintNS",      "P", "TBImages:list_print",
			ami_menu_item_project_print, NULL, NM_ITEMDISABLED);
	ami_menu_alloc_item(md, M_BAR_P3,   NM_ITEM, NM_BARLABEL,    NULL, NULL, NULL, NULL, 0);
	ami_menu_alloc_item(md, M_CLOSETAB, NM_ITEM, "CloseTab",     "K", "TBImages:list_remove",
			ami_menu_item_project_closetab, NULL, 0);
	ami_menu_alloc_item(md, M_CLOSEWIN, NM_ITEM, "CloseWindow",  NULL, "TBImages:list_cancel",
			ami_menu_item_project_closewin, NULL, 0);
	ami_menu_alloc_item(md, M_BAR_P4,   NM_ITEM, NM_BARLABEL,    NULL, NULL, NULL, NULL, 0);				
	ami_menu_alloc_item(md, M_ABOUT,    NM_ITEM, "About",        "?", "TBImages:list_info",
			ami_menu_item_project_about, NULL, 0);
	ami_menu_alloc_item(md, M_BAR_P5,   NM_ITEM, NM_BARLABEL,    NULL, NULL, NULL, NULL, 0);				
	ami_menu_alloc_item(md, M_QUIT,     NM_ITEM, "Quit",         "Q", "TBImages:list_warning",
			ami_menu_item_project_quit, NULL, 0);

	ami_menu_alloc_item(md, M_EDIT,    NM_TITLE, "Edit",         NULL, NULL, NULL, NULL, 0);
	ami_menu_alloc_item(md, M_CUT,      NM_ITEM, "CutNS",        "X", "TBImages:list_cut",
			ami_menu_item_edit_cut, NULL, 0);
	ami_menu_alloc_item(md, M_COPY,     NM_ITEM, "CopyNS",       "C", "TBImages:list_copy",
			ami_menu_item_edit_copy, NULL, 0);
	ami_menu_alloc_item(md, M_PASTE,    NM_ITEM, "PasteNS",      "V", "TBImages:list_paste",
			ami_menu_item_edit_paste, NULL, 0);
	ami_menu_alloc_item(md, M_BAR_E1,   NM_ITEM, NM_BARLABEL,    NULL, NULL, NULL, NULL, 0);
	ami_menu_alloc_item(md, M_SELALL,   NM_ITEM, "SelectAllNS",  "A", NSA_SPACE,
			ami_menu_item_edit_selectall, NULL, 0);
	ami_menu_alloc_item(md, M_CLEAR,    NM_ITEM, "ClearNS",      NULL, NSA_SPACE,
			ami_menu_item_edit_clearsel, NULL, 0);
	ami_menu_alloc_item(md, M_BAR_E2,   NM_ITEM, NM_BARLABEL,    NULL, NULL, NULL, NULL, 0);
	ami_menu_alloc_item(md, M_UNDO,     NM_ITEM, "Undo",         "Z", "TBImages:list_undo",
			ami_menu_item_edit_undo, NULL, 0);
	ami_menu_alloc_item(md, M_REDO,     NM_ITEM, "Redo",         "Y", "TBImages:list_redo",
			ami_menu_item_edit_redo, NULL, 0);

	ami_menu_alloc_item(md, M_BROWSER, NM_TITLE, "Browser",      NULL, NULL, NULL, NULL, 0);
	ami_menu_alloc_item(md, M_FIND,     NM_ITEM, "FindTextNS",   "F", "TBImages:list_search",
			ami_menu_item_browser_find, NULL, 0);
	ami_menu_alloc_item(md, M_BAR_B1,   NM_ITEM, NM_BARLABEL,    NULL, NULL, NULL, NULL, 0);
	ami_menu_alloc_item(md, M_HISTLOCL, NM_ITEM, "HistLocalNS",  NULL, "TBImages:list_history",
			ami_menu_item_browser_localhistory, NULL, 0);
	ami_menu_alloc_item(md, M_HISTGLBL, NM_ITEM, "HistGlobalNS", NULL, "TBImages:list_history",
			ami_menu_item_browser_globalhistory, NULL, 0);
	ami_menu_alloc_item(md, M_BAR_B2,   NM_ITEM, NM_BARLABEL,    NULL, NULL, NULL, NULL, 0);
	ami_menu_alloc_item(md, M_COOKIES,  NM_ITEM, "ShowCookiesNS",NULL, "TBImages:list_internet",
			ami_menu_item_browser_cookies, NULL, 0);
	ami_menu_alloc_item(md, M_BAR_B3,   NM_ITEM, NM_BARLABEL,    NULL, NULL, NULL, NULL, 0);
	ami_menu_alloc_item(md, M_SCALE,    NM_ITEM, "ScaleNS",      NULL, "TBImages:list_preview", NULL, NULL, 0);
	ami_menu_alloc_item(md, M_SCALEDEC,  NM_SUB, "ScaleDec",     "-", "TBImages:list_zoom_out",
			ami_menu_item_browser_scale_decrease, NULL, 0);
	ami_menu_alloc_item(md, M_SCALENRM,  NM_SUB, "ScaleNorm",    "=", "TBImages:list_zoom_100",
			ami_menu_item_browser_scale_normal, NULL, 0);
	ami_menu_alloc_item(md, M_SCALEINC,  NM_SUB, "ScaleInc",     "+", "TBImages:list_zoom_in",
			ami_menu_item_browser_scale_increase, NULL, 0);
	ami_menu_alloc_item(md, M_IMAGES,   NM_ITEM, "Images",       NULL, "TBImages:list_image", NULL, NULL, 0);
	ami_menu_alloc_item(md, M_IMGFORE,   NM_SUB, "ForeImg",      NULL, NULL,
			ami_menu_item_browser_foreimg, NULL, imgfore_flags);
	ami_menu_alloc_item(md, M_IMGBACK,   NM_SUB, "BackImg",      NULL, NULL,
			ami_menu_item_browser_backimg, NULL, imgback_flags);
	ami_menu_alloc_item(md, M_JS,       NM_ITEM, "EnableJS",     NULL, NULL,
			ami_menu_item_browser_enablejs, NULL, js_flags);
	ami_menu_alloc_item(md, M_BAR_B4,   NM_ITEM, NM_BARLABEL,    NULL, NULL, NULL, NULL, 0);
	ami_menu_alloc_item(md, M_REDRAW,   NM_ITEM, "Redraw",       NULL, "TBImages:list_wand",
			ami_menu_item_browser_redraw, NULL, 0);

	ami_menu_alloc_item(md, M_HOTLIST, NM_TITLE, "Hotlist",      NULL, NULL, NULL, NULL, 0);
	/* see ami_menu_scan for the rest of this menu */

	ami_menu_alloc_item(md, M_PREFS,   NM_TITLE, "Settings",     NULL, NULL, NULL, NULL, 0);
	ami_menu_alloc_item(md, M_PREDIT,   NM_ITEM, "SettingsEdit", NULL, "TBImages:list_prefs",
			ami_menu_item_settings_edit, NULL, 0);
	ami_menu_alloc_item(md, M_BAR_S1,   NM_ITEM, NM_BARLABEL,    NULL, NULL, NULL, NULL, 0);
	ami_menu_alloc_item(md, M_SNAPSHOT, NM_ITEM, "SnapshotWindow",NULL, "TBImages:list_hold",
			ami_menu_item_settings_snapshot, NULL, 0);
	ami_menu_alloc_item(md, M_PRSAVE,   NM_ITEM, "SettingsSave", NULL, "TBImages:list_use",
			ami_menu_item_settings_save, NULL, 0);

	ami_menu_alloc_item(md, M_AREXX,   NM_TITLE, "ARexx",        NULL, NULL, NULL, NULL, 0);
	ami_menu_alloc_item(md, M_AREXXEX,  NM_ITEM, "ARexxExecute", "E", "TBImages:list_arexx",
			ami_menu_item_arexx_execute, NULL, 0);
	ami_menu_alloc_item(md, M_BAR_A1,   NM_ITEM, NM_BARLABEL,    NULL, NULL, NULL, NULL, 0);
	ami_menu_alloc_item(md, AMI_MENU_AREXX_MAX,   NM_END, NULL,  NULL, NULL, NULL, NULL, 0);
}

struct Menu *ami_gui_menu_create(struct gui_window_2 *gwin)
{
	if(LIB_IS_AT_LEAST((struct Library *)IntuitionBase, 54, 6)) {
#ifdef __amigaos4__
		if(gui_menu != NULL) {
			ami_gui2_set_menu(gwin, gui_menu);
			gui_menu_count++;
			return gui_menu;
		}
		ami_init_menulabs(gui_menu_data);
		ami_menu_scan(gui_menu_data);
		ami_menu_arexx_scan(gui_menu_data);
		gui_menu = ami_menu_layout(gui_menu_data, AMI_MENU_AREXX_MAX);

		ami_gui2_set_menu(gwin, gui_menu);
		gui_menu_count++;
		return gui_menu;
#endif
	} else {
		struct Menu *temp_menu;
		struct ami_menu_data **md = ami_gui2_get_menu_data(gwin);

		ami_init_menulabs(md);
		ami_menu_scan(md);
		ami_menu_arexx_scan(md);
		temp_menu = ami_menu_layout(md, AMI_MENU_AREXX_MAX);
		ami_gui2_set_menu(gwin, temp_menu);
		return temp_menu;
	}

	return NULL; // shouldn't get this far
}

static void ami_free_menulabs(struct ami_menu_data **md)
{
	int i;

	for(i=0;i<=AMI_MENU_AREXX_MAX;i++) {
		if(md[i] == NULL) continue;
		if(md[i]->menulab &&
				(md[i]->menulab != NM_BARLABEL) &&
				(md[i]->menulab != ML_SEPARATOR)) {
			if(md[i]->menutype & MENU_IMAGE) {
				if(md[i]->menuobj) DisposeObject(md[i]->menuobj);
			}

			ami_utf8_free(md[i]->menulab);

			if(i >= AMI_MENU_AREXX) {
				if(md[i]->menu_hook.h_Data) free(md[i]->menu_hook.h_Data);
				md[i]->menu_hook.h_Data = NULL;
			}
		}

		if(md[i]->menukey != NULL) free(md[i]->menukey);

		md[i]->menulab = NULL;
		md[i]->menuobj = NULL;
		md[i]->menukey = NULL;
		md[i]->menutype = 0;
		free(md[i]);
		md[i] = NULL;
	}
}

void ami_gui_menu_freemenus(struct Menu *menu, struct ami_menu_data **md)
{
	if(menu != NULL) {
		FreeMenus(menu);
	}

	if(md != NULL) {
		ami_free_menulabs(md);
	}
}

void ami_gui_menu_free(struct gui_window_2 *gwin)
{
	if(LIB_IS_AT_LEAST((struct Library *)IntuitionBase, 54, 6)) {
#ifdef __amigaos4__
		gui_menu_count--;

		SetAttrs(ami_gui2_get_object(gwin, AMI_WIN_MAIN), WINDOW_MenuStrip, NULL, TAG_DONE);

		if(gui_menu_count == 0) {
			ami_free_menulabs(gui_menu_data);
			// if we detach our menu from the window we need to do this manually
			DisposeObject((Object *)gui_menu);
			gui_menu = NULL;
		}
#endif
	} else {
		ami_gui2_set_menu(gwin, NULL);
	}
}

bool ami_gui_menu_quit_selected(void)
{
	return menu_quit;
}

void ami_gui_menu_refresh_hotlist(void)
{
#ifdef __amigaos4__
	ami_menu_refresh(gui_menu, gui_menu_data, M_HOTLIST, AMI_MENU_HOTLIST_MAX, ami_menu_scan);
#endif
}

