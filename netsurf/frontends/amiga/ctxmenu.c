/*
 * Copyright 2015 Chris Young <chris@unsatisfactorysoftware.co.uk>
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

/** \file
 * Intuition-based context menu operations
 */

#ifdef __amigaos4__
#include <string.h>

#include <stdlib.h>
#include <proto/exec.h>
#include <proto/intuition.h>

#include <proto/bitmap.h>
#include <images/bitmap.h>
#include <proto/window.h>
#include <classes/window.h>

#include <intuition/menuclass.h>
#include <reaction/reaction_macros.h>

#include "utils/utils.h"
#include "utils/log.h"
#include "utils/messages.h"
#include "utils/nsoption.h"
#include "utils/nsurl.h"
#include "netsurf/content.h"
#include "netsurf/browser_window.h"
#include "netsurf/mouse.h"
#include "netsurf/keypress.h"
#include "desktop/browser_history.h"
#include "desktop/searchweb.h"

#include "amiga/bitmap.h"
#include "amiga/clipboard.h"
#include "amiga/ctxmenu.h"
#include "amiga/filetype.h"
#include "amiga/gui.h"
#include "amiga/libs.h"
#include "amiga/plugin_hack.h"
#include "amiga/theme.h"
#include "amiga/utf8.h"
#include "amiga/misc.h"

enum {
	AMI_CTX_ID_NONE = 0,

	/* Text selection */
	AMI_CTX_ID_SELCOPY,
	AMI_CTX_ID_WEBSEARCH,

	/* Links */
	AMI_CTX_ID_URLOPENTAB,
	AMI_CTX_ID_URLOPENWIN,
	AMI_CTX_ID_URLDOWNLOAD,
	AMI_CTX_ID_URLCOPY,

	/* Objects */
	AMI_CTX_ID_OBJSHOW,
	AMI_CTX_ID_OBJCOPY,
	AMI_CTX_ID_OBJCMD,

	/* Frames */
	AMI_CTX_ID_FRAMESHOW,

	/* History */
	AMI_CTX_ID_HISTORY,
	AMI_CTX_ID_HISTORY0,
	AMI_CTX_ID_HISTORY9F = AMI_CTX_ID_HISTORY0 + 19,

	/* Tabs */
	AMI_CTX_ID_TABNEW,
	AMI_CTX_ID_TABCLOSE_OTHER,

	AMI_CTX_ID_MAX
};

static Object *ctxmenu_obj = NULL;

static struct Hook ctxmenu_item_hook[AMI_CTX_ID_MAX];
static char *ctxmenu_item_label[AMI_CTX_ID_MAX];
static char *ctxmenu_item_shortcut[AMI_CTX_ID_MAX];
static Object *ctxmenu_item_image[AMI_CTX_ID_MAX];

/****************************
 * Menu item hook functions *
 ****************************/

/** Menu functions - called automatically by RA_HandleInput **/
HOOKF(void, ami_ctxmenu_item_selcopy, APTR, window, struct IntuiMessage *)
{
	struct gui_window_2 *gwin = (struct gui_window_2 *)hook->h_Data;

	browser_window_key_press(ami_gui2_get_browser_window(gwin), NS_KEY_COPY_SELECTION);
	browser_window_key_press(ami_gui2_get_browser_window(gwin), NS_KEY_CLEAR_SELECTION);
}

HOOKF(void, ami_ctxmenu_item_websearch, APTR, window, struct IntuiMessage *)
{
	nserror ret = NSERROR_OK;
	nsurl *url;

	struct gui_window_2 *gwin = (struct gui_window_2 *)hook->h_Data;
	char *sel = browser_window_get_selection(ami_gui2_get_browser_window(gwin));

	ret = search_web_omni(sel, SEARCH_WEB_OMNI_SEARCHONLY, &url);
	if (ret == NSERROR_OK) {
			browser_window_navigate(ami_gui2_get_browser_window(gwin),
					url,
					NULL,
					BW_NAVIGATE_HISTORY,
					NULL,
					NULL,
					NULL);
		nsurl_unref(url);
	}
	if (ret != NSERROR_OK) {
		amiga_warn_user(messages_get_errorcode(ret), 0);
	}

	free(sel);
}

HOOKF(void, ami_ctxmenu_item_urlopentab, APTR, window, struct IntuiMessage *)
{
	struct browser_window *bw;
	nsurl *url = (nsurl *)hook->h_Data;
	struct gui_window_2 *gwin;

	GetAttr(WINDOW_UserData, (Object *)window, (ULONG *)&gwin);
	nserror error = browser_window_create(BW_CREATE_CLONE | BW_CREATE_HISTORY | BW_CREATE_TAB,
								      url,
								      browser_window_access_url(ami_gui2_get_browser_window(gwin)),
								      ami_gui2_get_browser_window(gwin),
								      &bw);

	if (error != NSERROR_OK)
		amiga_warn_user(messages_get_errorcode(error), 0);		
}

HOOKF(void, ami_ctxmenu_item_urlopenwin, APTR, window, struct IntuiMessage *)
{
	struct browser_window *bw;
	nsurl *url = (nsurl *)hook->h_Data;
	struct gui_window_2 *gwin;

	GetAttr(WINDOW_UserData, (Object *)window, (ULONG *)&gwin);
	nserror error = browser_window_create(BW_CREATE_CLONE | BW_CREATE_HISTORY,
								      url,
								      browser_window_access_url(ami_gui2_get_browser_window(gwin)),
								      ami_gui2_get_browser_window(gwin),
								      &bw);

	if (error != NSERROR_OK)
		amiga_warn_user(messages_get_errorcode(error), 0);		
}

HOOKF(void, ami_ctxmenu_item_urldownload, APTR, window, struct IntuiMessage *)
{
	nsurl *url = (nsurl *)hook->h_Data;
	struct gui_window_2 *gwin;

	GetAttr(WINDOW_UserData, (Object *)window, (ULONG *)&gwin);

	browser_window_navigate(ami_gui2_get_browser_window(gwin),
		url,
		browser_window_access_url(ami_gui2_get_browser_window(gwin)),
		BW_NAVIGATE_DOWNLOAD,
		NULL,
		NULL,
		NULL);		
}

HOOKF(void, ami_ctxmenu_item_urlcopy, APTR, window, struct IntuiMessage *)
{
	nsurl *url = (nsurl *)hook->h_Data;
	ami_easy_clipboard(nsurl_access(url));		
}

HOOKF(void, ami_ctxmenu_item_objshow, APTR, window, struct IntuiMessage *)
{
	struct gui_window_2 *gwin;
	GetAttr(WINDOW_UserData, (Object *)window, (ULONG *)&gwin);

	browser_window_navigate(ami_gui2_get_browser_window(gwin),
							hlcache_handle_get_url(hook->h_Data),
							browser_window_access_url(ami_gui2_get_browser_window(gwin)),
							BW_NAVIGATE_HISTORY,
							NULL,
							NULL,
							NULL);	
}

HOOKF(void, ami_ctxmenu_item_objcopy, APTR, window, struct IntuiMessage *)
{
	struct bitmap *bm;
	struct gui_window_2 *gwin;
	GetAttr(WINDOW_UserData, (Object *)window, (ULONG *)&gwin);

	struct hlcache_handle *object = (struct hlcache_handle *)hook->h_Data;
	if((bm = content_get_bitmap(object)))
	{
		ami_bitmap_set_url(bm, hlcache_handle_get_url(object));
		ami_bitmap_set_title(bm, content_get_title(object));
		ami_easy_clipboard_bitmap(bm);
	}
#ifdef WITH_NS_SVG
	else if(ami_mime_compare(object, "svg") == true)
	{
		ami_easy_clipboard_svg(object);
	}
#endif
}

HOOKF(void, ami_ctxmenu_item_objcmd, APTR, window, struct IntuiMessage *)
{
	amiga_plugin_hack_execute((struct hlcache_handle *)hook->h_Data);
}

HOOKF(void, ami_ctxmenu_item_frameshow, APTR, window, struct IntuiMessage *)
{
	struct gui_window_2 *gwin;
	GetAttr(WINDOW_UserData, (Object *)window, (ULONG *)&gwin);

	browser_window_navigate(ami_gui2_get_browser_window(gwin),
							hlcache_handle_get_url(hook->h_Data),
							browser_window_access_url(ami_gui2_get_browser_window(gwin)),
							BW_NAVIGATE_HISTORY,
							NULL,
							NULL,
							NULL);	
}

/** Hooks for clicktab context menu entries **/
HOOKF(void, ami_ctxmenu_item_tabnew, APTR, window, struct IntuiMessage *)
{
	struct gui_window_2 *gwin;

	GetAttr(WINDOW_UserData, (Object *)window, (ULONG *)&gwin);
	ami_gui_new_blank_tab(gwin);
}

HOOKF(void, ami_ctxmenu_item_tabclose_other, APTR, window, struct IntuiMessage *)
{
	struct gui_window_2 *gwin;

	GetAttr(WINDOW_UserData, (Object *)window, (ULONG *)&gwin);
	ami_gui_close_inactive_tabs(gwin);
}

/** Hook for history context menu entries **/
HOOKF(void, ami_ctxmenu_item_history, APTR, window, struct IntuiMessage *)
{
	struct gui_window_2 *gwin;

	GetAttr(WINDOW_UserData, (Object *)window, (ULONG *)&gwin);

	browser_window_history_go(ami_gui2_get_browser_window(gwin),
		(struct history_entry *)hook->h_Data, false);		
}


/*************************
 * Browser context menus *
 *************************/

/** Add an initialised item to a context menu **/
static void ami_ctxmenu_add_item(Object *root_menu, int id, APTR data)
{
	ctxmenu_item_hook[id].h_Data = data;

	IDoMethod(root_menu, OM_ADDMEMBER, MStrip,
							MA_Type, T_ITEM,
							MA_ID, id,
							MA_Label, ctxmenu_item_label[id],
							MA_Key, ctxmenu_item_shortcut[id],
							MA_Image, ctxmenu_item_image[id],
							MA_UserData, &ctxmenu_item_hook[id],
						MEnd);
}

/** Hook function called by Intuition, creates context menu structure **/
static uint32 ami_ctxmenu_hook_func(struct Hook *hook, struct Window *window, struct ContextMenuMsg *msg)
{
	Object *root_menu;
	bool ctxmenu_has_content = false;
	struct gui_window_2 *gwin = hook->h_Data;
	struct hlcache_handle *cc = browser_window_get_content(ami_gui2_get_browser_window(gwin));
	struct browser_window_features ccdata;
	int mx = window->MouseX;
	int my = window->MouseY;
	int x, y;
	char *sel;

	if(msg->State != CM_QUERY) return 0;
	if(nsoption_bool(kiosk_mode) == true) return 0;
// check window is active

	if(ctxmenu_obj != NULL) DisposeObject(ctxmenu_obj);

	ctxmenu_obj = MStrip,
					MA_Type, T_ROOT,
					MA_AddChild, root_menu = MStrip,
						MA_Type, T_MENU,
						MA_Label, NULL, //"NetSurf",
						MA_EmbeddedKey, FALSE,
						MA_FreeImage, FALSE,
					MEnd,
				MEnd;

	if(ami_mouse_to_ns_coords(gwin, &x, &y, mx, my) == false) {
		/* Outside browser render area */
		return 0;
	}

	browser_window_get_features(ami_gui2_get_browser_window(gwin), x, y, &ccdata);

	if((browser_window_can_select(ami_gui2_get_browser_window(gwin))) &&
		((browser_window_get_editor_flags(ami_gui2_get_browser_window(gwin)) & BW_EDITOR_CAN_COPY)) &&
		(sel = browser_window_get_selection(ami_gui2_get_browser_window(gwin)))) {

		ami_ctxmenu_add_item(root_menu, AMI_CTX_ID_SELCOPY, gwin);
		ami_ctxmenu_add_item(root_menu, AMI_CTX_ID_WEBSEARCH, gwin);

		ctxmenu_has_content = true;
		free(sel);
	}

	if(ccdata.link) {
		if(ctxmenu_has_content == true)
			ami_ctxmenu_add_item(root_menu, AMI_CTX_ID_NONE, NULL);

		ami_ctxmenu_add_item(root_menu, AMI_CTX_ID_URLOPENTAB, ccdata.link);
		ami_ctxmenu_add_item(root_menu, AMI_CTX_ID_URLOPENWIN, ccdata.link);
		ami_ctxmenu_add_item(root_menu, AMI_CTX_ID_URLDOWNLOAD, ccdata.link);
		ami_ctxmenu_add_item(root_menu, AMI_CTX_ID_URLCOPY, ccdata.link);
		ctxmenu_has_content = true;
	}

	if(ccdata.object) {
		if(ctxmenu_has_content == true)
			ami_ctxmenu_add_item(root_menu, AMI_CTX_ID_NONE, NULL);

		ami_ctxmenu_add_item(root_menu, AMI_CTX_ID_OBJSHOW, ccdata.object);

		if(content_get_type(ccdata.object) == CONTENT_IMAGE)
			ami_ctxmenu_add_item(root_menu, AMI_CTX_ID_OBJCOPY, ccdata.object);

		if(ami_mime_content_to_cmd(ccdata.object))
			ami_ctxmenu_add_item(root_menu, AMI_CTX_ID_OBJCMD, ccdata.object);

		ctxmenu_has_content = true;
	}

	if(ccdata.main && (ccdata.main != cc)) {
		if(ctxmenu_has_content == true)
			ami_ctxmenu_add_item(root_menu, AMI_CTX_ID_NONE, NULL);

		ami_ctxmenu_add_item(root_menu, AMI_CTX_ID_FRAMESHOW, ccdata.main);

		ctxmenu_has_content = true;
	}

	if(ctxmenu_has_content == true) {
		msg->Menu = ctxmenu_obj;
		ami_set_pointer(gwin, GUI_POINTER_DEFAULT, false);
	}

	return 0;
}

/** Initial menu item creation **/
static void ami_ctxmenu_alloc_item(int id, const char *label, const char *key, const char *image, void *func)
{
	if(label == ML_SEPARATOR) {
		ctxmenu_item_label[id] = ML_SEPARATOR;
	} else {
		ctxmenu_item_label[id] = ami_utf8_easy(messages_get(label));
	}

	if(key != NULL) {
		ctxmenu_item_shortcut[id] = strdup(key);
	} else {
		ctxmenu_item_shortcut[id] = NULL;
	}

	if(image != NULL) {
		ctxmenu_item_image[id] = BitMapObj,
									BITMAP_Screen, ami_gui_get_screen(),
									BITMAP_SourceFile, image,
									BITMAP_Masking, TRUE,
									BitMapEnd;

		SetAttrs(ctxmenu_item_image[id],
						BITMAP_Width, 16,
						BITMAP_Height, 16,
						TAG_DONE);
	}

	ctxmenu_item_hook[id].h_Entry = func;
	ctxmenu_item_hook[id].h_Data = 0;
}	

/** Exported interface documented in ctxmenu.h **/
struct Hook *ami_ctxmenu_get_hook(APTR data)
{
	return AllocSysObjectTags(ASOT_HOOK,
		ASOHOOK_Entry, (HOOKFUNC)ami_ctxmenu_hook_func,
		ASOHOOK_Data, data,
		TAG_DONE);
}

/** Exported interface documented in ctxmenu.h **/
void ami_ctxmenu_release_hook(struct Hook *hook)
{
	FreeSysObject(ASOT_HOOK, hook);
}

/** Exported interface documented in ctxmenu.h **/
void ami_ctxmenu_free(void)
{
	for(int i = 1; i < AMI_CTX_ID_MAX; i++) {
		if((ctxmenu_item_label[i] != NULL) && (ctxmenu_item_label[i] != ML_SEPARATOR)) {
			ami_utf8_free(ctxmenu_item_label[i]);
		}
		ctxmenu_item_label[i] = NULL;

		if(ctxmenu_item_shortcut[i] != NULL) {
			free(ctxmenu_item_shortcut[i]);
			ctxmenu_item_shortcut[i] = NULL;
		}

		if(ctxmenu_item_image[i] != NULL) {
			DisposeObject(ctxmenu_item_image[i]);
			ctxmenu_item_image[i] = NULL;
		}
	}

	if(ctxmenu_obj != NULL) DisposeObject(ctxmenu_obj);
	ctxmenu_obj = NULL;
}

/** Exported interface documented in ctxmenu.h **/
void ami_ctxmenu_init(void)
{
	ami_ctxmenu_alloc_item(AMI_CTX_ID_NONE, 		ML_SEPARATOR, NULL, NULL, NULL);

	ami_ctxmenu_alloc_item(AMI_CTX_ID_SELCOPY, 		"CopyNS",		"C",	"TBImages:list_copy",
		ami_ctxmenu_item_selcopy);
	ami_ctxmenu_alloc_item(AMI_CTX_ID_WEBSEARCH, 	"SearchWeb",	NULL,	"TBImages:list_search",
		ami_ctxmenu_item_websearch);

	ami_ctxmenu_alloc_item(AMI_CTX_ID_URLOPENTAB, 	"LinkNewTab",	NULL,	"TBImages:list_tab",
		ami_ctxmenu_item_urlopentab);
	ami_ctxmenu_alloc_item(AMI_CTX_ID_URLOPENWIN, 	"LinkNewWin",	NULL,	"TBImages:list_app",
		ami_ctxmenu_item_urlopenwin);
	ami_ctxmenu_alloc_item(AMI_CTX_ID_URLDOWNLOAD, 	"LinkDload",	NULL,	"TBImages:list_save",
		ami_ctxmenu_item_urldownload);
	ami_ctxmenu_alloc_item(AMI_CTX_ID_URLCOPY, 		"CopyURL",		NULL,	"TBImages:list_copy",
		ami_ctxmenu_item_urlcopy);

	ami_ctxmenu_alloc_item(AMI_CTX_ID_OBJSHOW, 		"ObjShow",		NULL,	"TBImages:list_preview",
		ami_ctxmenu_item_objshow);
	ami_ctxmenu_alloc_item(AMI_CTX_ID_OBJCOPY, 		"CopyClip",		NULL,	"TBImages:list_copy",
		ami_ctxmenu_item_objcopy);
	ami_ctxmenu_alloc_item(AMI_CTX_ID_OBJCMD, 		"ExternalApp",	NULL,	"TBImages:list_tool",
		ami_ctxmenu_item_objcmd);

	ami_ctxmenu_alloc_item(AMI_CTX_ID_FRAMESHOW, 	"FrameOnly",	NULL,	"TBImages:list_preview",
		ami_ctxmenu_item_frameshow);

	ami_ctxmenu_alloc_item(AMI_CTX_ID_TABNEW, "NewTab", "T", "TBImages:list_tab",
		ami_ctxmenu_item_tabnew);
	ami_ctxmenu_alloc_item(AMI_CTX_ID_TABCLOSE_OTHER, "CloseInactive", "K", "TBImages:list_cancel",
		ami_ctxmenu_item_tabclose_other);
}

/********************************
 * History button context menus *
 ********************************/

/** Create menu entries from browser history **/
static bool ami_ctxmenu_history(int direction, struct gui_window_2 *gwin, const struct history_entry *entry)
{
	Object *history_root;
	int id = AMI_CTX_ID_HISTORY0 + ami_gui2_get_ctxmenu_history_tmp(gwin);
	if(direction == AMI_CTXMENU_HISTORY_FORWARD) id += 10;

	if(ami_gui2_get_ctxmenu_history_tmp(gwin) >= 10) return false;

	ctxmenu_item_hook[id].h_Entry = (HOOKFUNC)ami_ctxmenu_item_history;
	ctxmenu_item_hook[id].h_Data = (APTR)entry;

	history_root = (Object *)IDoMethod(ami_gui2_get_ctxmenu_history(gwin, direction), MM_FINDID, 0, AMI_CTX_ID_HISTORY);

	IDoMethod(history_root, OM_ADDMEMBER, MStrip,
							MA_Type, T_ITEM,
							/* TODO: MA_Label should be in local charset */
							MA_Label, browser_window_history_entry_get_title(entry),
							MA_ID, id,
							MA_Image, NULL,
							MA_UserData, &ctxmenu_item_hook[id],
						MEnd);

	ami_gui2_set_ctxmenu_history_tmp(gwin, ami_gui2_get_ctxmenu_history_tmp(gwin) + 1);

	return true;
}

/** Callback for browser_window_history_enumerate **/
static bool ami_ctxmenu_history_back(const struct browser_window *bw,
		int x0, int y0, int x1, int y1,
		const struct history_entry *entry, void *user_data)
{
	return ami_ctxmenu_history(AMI_CTXMENU_HISTORY_BACK, (struct gui_window_2 *)user_data, entry);
}

/** Callback for browser_window_history_enumerate **/
static bool ami_ctxmenu_history_forward(const struct browser_window *bw,
		int x0, int y0, int x1, int y1,
		const struct history_entry *entry, void *user_data)
{
	return ami_ctxmenu_history(AMI_CTXMENU_HISTORY_FORWARD, (struct gui_window_2 *)user_data, entry);
}

/** Exported interface documented in ctxmenu.h **/
struct Menu *ami_ctxmenu_history_create(int direction, struct gui_window_2 *gwin)
{
	Object *obj;

	if(ami_gui2_get_ctxmenu_history(gwin, direction) == NULL) {
		if(ctxmenu_item_label[AMI_CTX_ID_HISTORY] == NULL)
			ctxmenu_item_label[AMI_CTX_ID_HISTORY] = ami_utf8_easy(messages_get("History"));

		obj = MStrip,
						MA_Type, T_ROOT,
						MA_AddChild, MStrip,
							MA_Type, T_MENU,
							MA_ID, AMI_CTX_ID_HISTORY,
							MA_Label, ctxmenu_item_label[AMI_CTX_ID_HISTORY],
							MA_EmbeddedKey, FALSE,
							//MA_FreeImage, FALSE,
						MEnd,
					MEnd;

		ami_gui2_set_ctxmenu_history(gwin, direction, obj);

	} else {
		for (int i = 0; i < 20; i++) {
			obj = (Object *)IDoMethod(ami_gui2_get_ctxmenu_history(gwin, direction),
					MM_FINDID, 0, AMI_CTX_ID_HISTORY0 + i);
			if(obj != NULL) IDoMethod(ami_gui2_get_ctxmenu_history(gwin, direction), OM_REMMEMBER, obj);
		}

		ami_gui2_set_ctxmenu_history_tmp(gwin, 0);

		if(direction == AMI_CTXMENU_HISTORY_BACK) {
			browser_window_history_enumerate_back(ami_gui2_get_browser_window(gwin), ami_ctxmenu_history_back, gwin);
		} else {
			browser_window_history_enumerate_forward(ami_gui2_get_browser_window(gwin), ami_ctxmenu_history_forward, gwin);
		}
	}

	return (struct Menu *)ami_gui2_get_ctxmenu_history(gwin, direction);
}


/**************************
 * ClickTab context menus *
 **************************/

/** Exported interface documented in ctxmenu.h **/
struct Menu *ami_ctxmenu_clicktab_create(struct gui_window_2 *gwin, Object **clicktab_obj)
{
	Object *root_menu;
	Object *clicktab;

	if(*clicktab_obj != NULL) {
		return (struct Menu *)*clicktab_obj;
	}

	clicktab = MStrip,
					MA_Type, T_ROOT,
					MA_AddChild, root_menu = MStrip,
						MA_Type, T_MENU,
						MA_Label, NULL,
						MA_EmbeddedKey, FALSE,
					MEnd,
				MEnd;

	*clicktab_obj = clicktab;

	ami_ctxmenu_add_item(root_menu, AMI_CTX_ID_TABNEW, gwin);
	ami_ctxmenu_add_item(root_menu, AMI_CTX_ID_TABCLOSE_OTHER, gwin);

	return (struct Menu *)clicktab;
}


#endif

