/*
 * Copyright 2008, 2009, 2017 Chris Young <chris@unsatisfactorysoftware.co.uk>
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
 * Implementation of Amiga hotlist viewer using core windows.
 */

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include <proto/asl.h>
#include <proto/dos.h>
#include <proto/intuition.h>

#include <classes/window.h>
#include <gadgets/layout.h>
#include <gadgets/space.h>

#include <reaction/reaction_macros.h>

#include "desktop/hotlist.h"
#include "netsurf/browser_window.h"
#include "netsurf/keypress.h"
#include "netsurf/plotters.h"
#include "utils/log.h"
#include "utils/messages.h"
#include "utils/nsoption.h"

#include "amiga/corewindow.h"
#include "amiga/drag.h"
#include "amiga/file.h"
#include "amiga/hotlist.h"
#include "amiga/libs.h"
#include "amiga/menu.h"
#include "amiga/theme.h"
#include "amiga/utf8.h"

enum {
	/* Project menu */
	AMI_HOTLIST_M_PROJECT = 0,
	 AMI_HOTLIST_M_EXPORT,
	 AMI_HOTLIST_M_BAR_P1,
	 AMI_HOTLIST_M_EXPAND,
	  AMI_HOTLIST_M_EXPAND_ALL,
	  AMI_HOTLIST_M_EXPAND_FOLDERS,
	  AMI_HOTLIST_M_EXPAND_LINKS,
	 AMI_HOTLIST_M_COLLAPSE,
	  AMI_HOTLIST_M_COLLAPSE_ALL,
	  AMI_HOTLIST_M_COLLAPSE_FOLDERS,
	  AMI_HOTLIST_M_COLLAPSE_LINKS,
	 AMI_HOTLIST_M_BAR_P2,
	 AMI_HOTLIST_M_SNAPSHOT,
	 AMI_HOTLIST_M_BAR_P3,
	 AMI_HOTLIST_M_CLOSE,
	/* Edit menu */
	AMI_HOTLIST_M_EDIT,
	 AMI_HOTLIST_M_NEWFOLDER,
	 AMI_HOTLIST_M_NEWLINK,
	 AMI_HOTLIST_M_EDIT_EDIT,
	 AMI_HOTLIST_M_BAR_E1,
	 AMI_HOTLIST_M_SELECTALL,
	 AMI_HOTLIST_M_CLEAR,
	 AMI_HOTLIST_M_BAR_E2,
	 AMI_HOTLIST_M_DELETE,
	AMI_HOTLIST_M_LAST
};

/**
 * Amiga hotlist viewer window context
 */
struct ami_hotlist_window {
	/** Amiga core window context */
	struct ami_corewindow core;

	struct ami_menu_data *menu_data[AMI_HOTLIST_M_LAST + 1];
	struct Menu *imenu; /* Intuition menu */
};

static struct ami_hotlist_window *hotlist_window = NULL;

struct ami_hotlist_ctx {
	void *userdata;
	int level;
	int item;
	const char *folder; /* folder we're interested in */
	bool in_menu; /* set if we are in that folder */
	bool found; /* set if the folder is found */
	bool (*cb)(void *userdata, int level, int item, const char *title, nsurl *url, bool folder);
};

/** hotlist scanner */
static nserror ami_hotlist_folder_enter_cb(void *ctx, const char *title)
{
	struct ami_hotlist_ctx *menu_ctx = (struct ami_hotlist_ctx *)ctx;

	if(menu_ctx->in_menu == true) {
		if(menu_ctx->cb(menu_ctx->userdata, menu_ctx->level, menu_ctx->item, title, NULL, true) == true)
			menu_ctx->item++;
	} else {
		if((menu_ctx->level == 0) && (strcmp(title, menu_ctx->folder) == 0)) {
			menu_ctx->in_menu = true;
			menu_ctx->found = true;
		}
	}
	menu_ctx->level++;
	return NSERROR_OK;
}

static nserror ami_hotlist_address_cb(void *ctx, nsurl *url, const char *title)
{
	struct ami_hotlist_ctx *menu_ctx = (struct ami_hotlist_ctx *)ctx;

	if(menu_ctx->in_menu == true) {
		if(menu_ctx->cb(menu_ctx->userdata, menu_ctx->level, menu_ctx->item, title, url, false) == true)
			menu_ctx->item++;
	}
	
	return NSERROR_OK;
}

static nserror ami_hotlist_folder_leave_cb(void *ctx)
{
	struct ami_hotlist_ctx *menu_ctx = (struct ami_hotlist_ctx *)ctx;

	menu_ctx->level--;

	if((menu_ctx->in_menu == true) && (menu_ctx->level == 0))
		menu_ctx->in_menu = false;

	return NSERROR_OK;
}

nserror ami_hotlist_scan(void *userdata, int first_item, const char *folder,
	bool (*cb_add_item)(void *userdata, int level, int item, const char *title, nsurl *url, bool folder))
{
	nserror error;
	struct ami_hotlist_ctx ctx;

	ctx.level = 0;
	ctx.item = first_item;
	ctx.folder = folder;
	ctx.in_menu = false;
	ctx.userdata = userdata;
	ctx.cb = cb_add_item;
	ctx.found = false;

	error = hotlist_iterate(&ctx,
		ami_hotlist_folder_enter_cb,
		ami_hotlist_address_cb,
		ami_hotlist_folder_leave_cb);

	if((error == NSERROR_OK) && (ctx.found == false))
		hotlist_add_folder(folder, false, 0);

	return error;
}


/**
 * callback for mouse action for hotlist viewer on core window
 *
 * \param ami_cw The Amiga core window structure.
 * \param mouse_state netsurf mouse state on event
 * \param x location of event
 * \param y location of event
 * \return NSERROR_OK on success otherwise apropriate error code
 */
static nserror
ami_hotlist_mouse(struct ami_corewindow *ami_cw,
					browser_mouse_state mouse_state,
					int x, int y)
{
	hotlist_mouse_action(mouse_state, x, y);

	return NSERROR_OK;
}

/**
 * callback for keypress for hotlist viewer on core window
 *
 * \param ami_cw The Amiga core window structure.
 * \param nskey The netsurf key code
 * \return NSERROR_OK on success otherwise apropriate error code
 */
static nserror
ami_hotlist_key(struct ami_corewindow *ami_cw, uint32_t nskey)
{
	if (hotlist_keypress(nskey)) {
			return NSERROR_OK;
	}
	return NSERROR_NOT_IMPLEMENTED;
}

/**
 * callback on draw event for hotlist viewer on core window
 *
 * \param ami_cw The Amiga core window structure.
 * \param x The x coordinate of hotlist area to redraw
 * \param y The y coordinate of hotlist area to redraw
 * \param r The rectangle of the window that needs updating.
 * \param ctx The drawing context
 * \return NSERROR_OK on success otherwise apropriate error code
 */
static nserror
ami_hotlist_draw(struct ami_corewindow *ami_cw,
		 int x, int y, struct rect *r, struct redraw_context *ctx)
{
	hotlist_redraw(x, y, r, ctx);

	return NSERROR_OK;
}

/**
 * callback for drag end on Amiga core window
 * ie. a drag *from* this window has ended
 *
 * \param ami_cw The Amiga core window structure.
 * \param x mouse x co-ordinate
 * \param y mouse y co-ordinate
 * \return NSERROR_OK on success otherwise apropriate error code
 */
static nserror
ami_hotlist_drag_end(struct ami_corewindow *ami_cw, int x, int y)
{
	nsurl *url = NULL;
	const char *title = NULL;
	bool ok = false;
	struct gui_window_2 *gwin;
	struct ami_corewindow *cw;

	if(hotlist_has_selection()) {
		ok = hotlist_get_selection(&url, &title);
	}
	
	if((ok == false) || (url == NULL)) {
		ami_gui_beep();
	} else if(url) {
		if((gwin = ami_window_at_pointer(AMINS_WINDOW))) {
			browser_window_navigate(ami_gui2_get_browser_window(gwin),
					url,
					NULL,
					BW_NAVIGATE_HISTORY,
					NULL,
					NULL,
					NULL);
		} else if((cw = (struct ami_corewindow *)ami_window_at_pointer(AMINS_COREWINDOW)) &&
			(ami_cw->icon_drop != NULL)) {
			cw->icon_drop(cw, url, title, x, y);
		}
	}
	return NSERROR_OK;
}
		
/**
 * callback for icon drop on Amiga core window
 * ie. a drag has ended *above* this window
 * \todo this may not be very flexible but serves our current purposes
 *
 * \param ami_cw The Amiga core window structure.
 * \param url url of dropped icon
 * \param title title of dropped icon
 * \param x mouse x co-ordinate
 * \param y mouse y co-ordinate
 * \return NSERROR_OK on success otherwise apropriate error code
 */
static nserror
ami_hotlist_icon_drop(struct ami_corewindow *ami_cw, struct nsurl *url, const char *title, int x, int y)
{
	hotlist_add_entry(url, title, true, y);
	return NSERROR_OK;
}

/**
 * menu stuff
 */

static void
ami_hotlist_menu_free(struct ami_hotlist_window *hotlist_win)
{
	SetAttrs(hotlist_win->core.objects[GID_CW_WIN],
		WINDOW_MenuStrip, NULL,
	TAG_DONE);
	
	ami_menu_free_menu(hotlist_win->menu_data, AMI_HOTLIST_M_LAST, hotlist_win->imenu);
}

 /* menu hook functions */
HOOKF(void, ami_hotlist_menu_item_project_export, APTR, window, struct IntuiMessage *)
{
	char fname[1024];
	struct ami_corewindow *ami_cw;
	GetAttr(WINDOW_UserData, (Object *)window, (ULONG *)&ami_cw);

	if(AslRequestTags(savereq,
		ASLFR_Window, ami_cw->win,
		ASLFR_SleepWindow, TRUE,
		ASLFR_TitleText, messages_get("NetSurf"),
		ASLFR_Screen, ami_gui_get_screen(),
		ASLFR_InitialFile, "hotlist.html",
		TAG_DONE)) {
			strlcpy(fname, savereq->fr_Drawer, 1024);
			AddPart(fname, savereq->fr_File, 1024);
			ami_update_pointer(ami_cw->win, GUI_POINTER_WAIT);
			hotlist_export(fname, NULL);
			ami_update_pointer(ami_cw->win, GUI_POINTER_DEFAULT);
	}
}
									
HOOKF(void, ami_hotlist_menu_item_project_expand_all, APTR, window, struct IntuiMessage *)
{
	hotlist_expand(false);
}
 
HOOKF(void, ami_hotlist_menu_item_project_expand_folders, APTR, window, struct IntuiMessage *)
{
	hotlist_expand(true);
}

HOOKF(void, ami_hotlist_menu_item_project_expand_links, APTR, window, struct IntuiMessage *)
{
	hotlist_expand(false);
}

HOOKF(void, ami_hotlist_menu_item_project_collapse_all, APTR, window, struct IntuiMessage *)
{
	hotlist_contract(true);
}
 
HOOKF(void, ami_hotlist_menu_item_project_collapse_folders, APTR, window, struct IntuiMessage *)
{
	hotlist_contract(true);
}

HOOKF(void, ami_hotlist_menu_item_project_collapse_links, APTR, window, struct IntuiMessage *)
{
	hotlist_contract(false);
}

HOOKF(void, ami_hotlist_menu_item_project_snapshot, APTR, window, struct IntuiMessage *)
{
	struct ami_corewindow *ami_cw;
	GetAttr(WINDOW_UserData, (Object *)window, (ULONG *)&ami_cw);

	nsoption_set_int(hotlist_window_ypos, ami_cw->win->TopEdge);
	nsoption_set_int(hotlist_window_xpos, ami_cw->win->LeftEdge);
	nsoption_set_int(hotlist_window_xsize, ami_cw->win->Width);
	nsoption_set_int(hotlist_window_ysize, ami_cw->win->Height);
}

HOOKF(void, ami_hotlist_menu_item_project_close, APTR, window, struct IntuiMessage *)
{
	struct ami_corewindow *ami_cw;
	GetAttr(WINDOW_UserData, (Object *)window, (ULONG *)&ami_cw);
	
	ami_cw->close_window = true;
}

HOOKF(void, ami_hotlist_menu_item_edit_newfolder, APTR, window, struct IntuiMessage *)
{
	hotlist_add_folder(NULL, false, 0);
}

HOOKF(void, ami_hotlist_menu_item_edit_newlink, APTR, window, struct IntuiMessage *)
{
	hotlist_add_entry(NULL, NULL, false, 0);
}

HOOKF(void, ami_hotlist_menu_item_edit_edit, APTR, window, struct IntuiMessage *)
{
	hotlist_edit_selection();
}

HOOKF(void, ami_hotlist_menu_item_edit_select_all, APTR, window, struct IntuiMessage *)
{
	hotlist_keypress(NS_KEY_SELECT_ALL);
}

HOOKF(void, ami_hotlist_menu_item_edit_clear, APTR, window, struct IntuiMessage *)
{
	hotlist_keypress(NS_KEY_CLEAR_SELECTION);
}

HOOKF(void, ami_hotlist_menu_item_edit_delete, APTR, window, struct IntuiMessage *)
{
	hotlist_keypress(NS_KEY_DELETE_LEFT);
}


/* menu setup */

static void ami_hotlist_menulabs(struct ami_menu_data **md)
{
	ami_menu_alloc_item(md, AMI_HOTLIST_M_PROJECT, NM_TITLE, "Tree", NULL, NULL, NULL, NULL, 0);
	ami_menu_alloc_item(md, AMI_HOTLIST_M_EXPORT,   NM_ITEM, "TreeExport", "S", "TBImages:list_save",
		ami_hotlist_menu_item_project_export, NULL, 0);
	ami_menu_alloc_item(md, AMI_HOTLIST_M_BAR_P1, NM_ITEM, NM_BARLABEL, NULL, NULL, NULL, NULL, 0);
	ami_menu_alloc_item(md, AMI_HOTLIST_M_EXPAND,   NM_ITEM, "Expand", NULL, "TBImages:list_folderunfold", NULL, NULL, 0);
	ami_menu_alloc_item(md, AMI_HOTLIST_M_EXPAND_ALL,   NM_SUB, "All", "+", NULL,
		ami_hotlist_menu_item_project_expand_all, NULL, 0);
	ami_menu_alloc_item(md, AMI_HOTLIST_M_EXPAND_FOLDERS,   NM_SUB, "Folders", NULL, NULL,
		ami_hotlist_menu_item_project_expand_folders, NULL, 0);
	ami_menu_alloc_item(md, AMI_HOTLIST_M_EXPAND_LINKS,   NM_SUB, "Links", NULL, NULL,
		ami_hotlist_menu_item_project_expand_links, NULL, 0);
	ami_menu_alloc_item(md, AMI_HOTLIST_M_COLLAPSE,   NM_ITEM, "Collapse", NULL, "TBImages:list_folderfold", NULL, NULL, 0);
	ami_menu_alloc_item(md, AMI_HOTLIST_M_COLLAPSE_ALL,   NM_SUB, "All", "-", NULL,
		ami_hotlist_menu_item_project_collapse_all, NULL, 0);
	ami_menu_alloc_item(md, AMI_HOTLIST_M_COLLAPSE_FOLDERS,   NM_SUB, "Folders", NULL, NULL,
		ami_hotlist_menu_item_project_collapse_folders, NULL, 0);
	ami_menu_alloc_item(md, AMI_HOTLIST_M_COLLAPSE_LINKS,   NM_SUB, "Links", NULL, NULL,
		ami_hotlist_menu_item_project_collapse_links, NULL, 0);
	ami_menu_alloc_item(md, AMI_HOTLIST_M_BAR_P2, NM_ITEM, NM_BARLABEL, NULL, NULL, NULL, NULL, 0);
	ami_menu_alloc_item(md, AMI_HOTLIST_M_SNAPSHOT,   NM_ITEM, "SnapshotWindow", NULL, "TBImages:list_hold",
		ami_hotlist_menu_item_project_snapshot, NULL, 0);
	ami_menu_alloc_item(md, AMI_HOTLIST_M_BAR_P3, NM_ITEM, NM_BARLABEL, NULL, NULL, NULL, NULL, 0);
	ami_menu_alloc_item(md, AMI_HOTLIST_M_CLOSE,   NM_ITEM, "CloseWindow", "K", "TBImages:list_cancel",
		ami_hotlist_menu_item_project_close, NULL, 0);

	ami_menu_alloc_item(md, AMI_HOTLIST_M_EDIT, NM_TITLE, "Edit", NULL, NULL, NULL, NULL, 0);

	ami_menu_alloc_item(md, AMI_HOTLIST_M_NEWFOLDER,   NM_ITEM, "TreeNewFolder", "N", "TBImages:list_drawer",
		ami_hotlist_menu_item_edit_newfolder, NULL, 0);
	ami_menu_alloc_item(md, AMI_HOTLIST_M_NEWLINK,   NM_ITEM, "TreeNewLink", NULL, "TBImages:list_favouriteadd",
		ami_hotlist_menu_item_edit_newlink, NULL, 0);
	ami_menu_alloc_item(md, AMI_HOTLIST_M_EDIT_EDIT,   NM_ITEM, "TreeEdit", "E", "TBImages:list_edit",
		ami_hotlist_menu_item_edit_edit, NULL, 0);
	ami_menu_alloc_item(md, AMI_HOTLIST_M_BAR_E1, NM_ITEM, NM_BARLABEL, NULL, NULL, NULL, NULL, 0);
	ami_menu_alloc_item(md, AMI_HOTLIST_M_SELECTALL,   NM_ITEM, "SelectAllNS", "A", NSA_SPACE,
		ami_hotlist_menu_item_edit_select_all, NULL, 0);
	ami_menu_alloc_item(md, AMI_HOTLIST_M_CLEAR,   NM_ITEM, "ClearNS", NULL, NSA_SPACE,
		ami_hotlist_menu_item_edit_clear, NULL, 0);
	ami_menu_alloc_item(md, AMI_HOTLIST_M_BAR_E2, NM_ITEM, NM_BARLABEL, NULL, NULL, NULL, NULL, 0);
	ami_menu_alloc_item(md, AMI_HOTLIST_M_DELETE,   NM_ITEM, "TreeDelete", "Del", "TBImages:list_delete",
		ami_hotlist_menu_item_edit_delete, NULL, 0);

	ami_menu_alloc_item(md, AMI_HOTLIST_M_LAST,   NM_END, NULL, NULL, NULL, NULL, NULL, 0);
}

static struct Menu *
ami_hotlist_menu_create(struct ami_hotlist_window *hotlist_win)
{
	ami_hotlist_menulabs(hotlist_win->menu_data);
	hotlist_win->imenu = ami_menu_layout(hotlist_win->menu_data, AMI_HOTLIST_M_LAST);
	if(hotlist_win->imenu == NULL) return NULL;

	return hotlist_win->imenu;
}


static nserror
ami_hotlist_create_window(struct ami_hotlist_window *hotlist_win)
{
	struct ami_corewindow *ami_cw = (struct ami_corewindow *)&hotlist_win->core;
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
		WA_Top, nsoption_int(hotlist_window_ypos),
		WA_Left, nsoption_int(hotlist_window_xpos),
		WA_Width, nsoption_int(hotlist_window_xsize),
		WA_Height, nsoption_int(hotlist_window_ysize),
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
		WINDOW_UserData, hotlist_win,
		WINDOW_MenuStrip, ami_hotlist_menu_create(hotlist_win),
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

/**
 * destroy a previously created hotlist view
 */
static void
ami_hotlist_destroy(struct ami_corewindow *ami_cw)
{
	nserror res;

	if(hotlist_window == NULL)
		return;

	res = hotlist_manager_fini();
	if (res == NSERROR_OK) {
		ami_hotlist_menu_free(hotlist_window);
		res = ami_corewindow_fini(&hotlist_window->core); /* closes the window for us, frees hotlist_win */
		hotlist_window = NULL;
	}

	ami_gui_hotlist_update_all();
}


/* exported interface documented in amiga/hotlist.h */
nserror ami_hotlist_present(void)
{
	struct ami_hotlist_window *ncwin;
	nserror res;

	if(hotlist_window != NULL) {
		//windowtofront()
		return NSERROR_OK;
	}

	ncwin = calloc(1, sizeof(struct ami_hotlist_window));
	if (ncwin == NULL) {
		return NSERROR_NOMEM;
	}

	ncwin->core.wintitle = ami_utf8_easy((char *)messages_get("Hotlist"));

	res = ami_hotlist_create_window(ncwin);
	if (res != NSERROR_OK) {
		NSLOG(netsurf, INFO, "SSL UI builder init failed");
		ami_utf8_free(ncwin->core.wintitle);
		free(ncwin);
		return res;
	}

	/* initialise Amiga core window */
	ncwin->core.draw = ami_hotlist_draw;
	ncwin->core.key = ami_hotlist_key;
	ncwin->core.mouse = ami_hotlist_mouse;
	ncwin->core.close = ami_hotlist_destroy;
	ncwin->core.event = NULL;
	ncwin->core.drag_end = ami_hotlist_drag_end;
	ncwin->core.icon_drop = ami_hotlist_icon_drop;

	res = ami_corewindow_init(&ncwin->core);
	if (res != NSERROR_OK) {
		ami_utf8_free(ncwin->core.wintitle);
		DisposeObject(ncwin->core.objects[GID_CW_WIN]);
		free(ncwin);
		return res;
	}

	res = hotlist_manager_init(ncwin->core.cb_table, (struct core_window *)ncwin);
	if (res != NSERROR_OK) {
		ami_utf8_free(ncwin->core.wintitle);
		DisposeObject(ncwin->core.objects[GID_CW_WIN]);
		free(ncwin);
		return res;
	}

	hotlist_window = ncwin;

	return NSERROR_OK;
}

/* exported interface documented in amiga/hotlist.h */
void ami_hotlist_close(void)
{
	ami_hotlist_destroy((struct ami_corewindow *)hotlist_window);
}

