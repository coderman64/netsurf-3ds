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
 * Implementation of Amiga global history viewer using core windows.
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

#include "desktop/global_history.h"
#include "netsurf/browser_window.h"
#include "netsurf/keypress.h"
#include "netsurf/plotters.h"
#include "utils/log.h"
#include "utils/messages.h"
#include "utils/nsoption.h"

#include "amiga/corewindow.h"
#include "amiga/drag.h"
#include "amiga/file.h"
#include "amiga/history.h"
#include "amiga/libs.h"
#include "amiga/menu.h"
#include "amiga/theme.h"
#include "amiga/utf8.h"

enum {
	/* Project menu */
	AMI_HISTORY_M_PROJECT = 0,
	 AMI_HISTORY_M_EXPORT,
	 AMI_HISTORY_M_BAR_P1,
	 AMI_HISTORY_M_EXPAND,
	  AMI_HISTORY_M_EXPAND_ALL,
	  AMI_HISTORY_M_EXPAND_FOLDERS,
	  AMI_HISTORY_M_EXPAND_LINKS,
	 AMI_HISTORY_M_COLLAPSE,
	  AMI_HISTORY_M_COLLAPSE_ALL,
	  AMI_HISTORY_M_COLLAPSE_FOLDERS,
	  AMI_HISTORY_M_COLLAPSE_LINKS,
	 AMI_HISTORY_M_BAR_P2,
	 AMI_HISTORY_M_SNAPSHOT,
	 AMI_HISTORY_M_BAR_P3,
	 AMI_HISTORY_M_CLOSE,
	/* Edit menu */
	AMI_HISTORY_M_EDIT,
	 AMI_HISTORY_M_SELECTALL,
	 AMI_HISTORY_M_CLEAR,
	 AMI_HISTORY_M_BAR_E1,
	 AMI_HISTORY_M_DELETE,
	AMI_HISTORY_M_LAST
};

/**
 * Amiga history viewer window context
 */
struct ami_history_global_window {
	/** Amiga core window context */
	struct ami_corewindow core;

	struct ami_menu_data *menu_data[AMI_HISTORY_M_LAST + 1];
	struct Menu *imenu; /* Intuition menu */
};

static struct ami_history_global_window *history_window = NULL;


static void
ami_history_global_menu_free(struct ami_history_global_window *history_win)
{
	SetAttrs(history_win->core.objects[GID_CW_WIN],
		WINDOW_MenuStrip, NULL,
	TAG_DONE);
	
	ami_menu_free_menu(history_win->menu_data, AMI_HISTORY_M_LAST, history_win->imenu);
}

/**
 * destroy a previously created history view
 */
static void
ami_history_global_destroy(struct ami_corewindow *ami_cw)
{
	nserror res;

	if(history_window == NULL)
		return;

	res = global_history_fini();
	if (res == NSERROR_OK) {
		ami_history_global_menu_free(history_window);
		res = ami_corewindow_fini(&history_window->core); /* closes the window for us, frees history_win */
		history_window = NULL;
	}
}


/**
 * callback for mouse action for history viewer on core window
 *
 * \param ami_cw The Amiga core window structure.
 * \param mouse_state netsurf mouse state on event
 * \param x location of event
 * \param y location of event
 * \return NSERROR_OK on success otherwise apropriate error code
 */
static nserror
ami_history_global_mouse(struct ami_corewindow *ami_cw,
					browser_mouse_state mouse_state,
					int x, int y)
{
	global_history_mouse_action(mouse_state, x, y);

	return NSERROR_OK;
}

/**
 * callback for keypress for history viewer on core window
 *
 * \param ami_cw The Amiga core window structure.
 * \param nskey The netsurf key code
 * \return NSERROR_OK on success otherwise apropriate error code
 */
static nserror
ami_history_global_key(struct ami_corewindow *ami_cw, uint32_t nskey)
{
	if (global_history_keypress(nskey)) {
			return NSERROR_OK;
	}
	return NSERROR_NOT_IMPLEMENTED;
}

/**
 * callback on draw event for history viewer on core window
 *
 * \param ami_cw The Amiga core window structure.
 * \param x The x coordinate of global history area to redraw
 * \param y The y coordinate of global history area to redraw
 * \param r The rectangle of the window that needs updating.
 * \param ctx The drawing context
 * \return NSERROR_OK on success otherwise apropriate error code
 */
static nserror
ami_history_global_draw(struct ami_corewindow *ami_cw,
			int x, int y, struct rect *r,
			struct redraw_context *ctx)
{
	global_history_redraw(x, y, r, ctx);

	return NSERROR_OK;
}

/**
 * callback on drag end for history viewer
 *
 * \param ami_cw The Amiga core window structure.
 * \param x mouse x co-ordinate
 * \param y mouse y co-ordinate
 * \return NSERROR_OK on success otherwise apropriate error code
 */
static nserror
ami_history_global_drag_end(struct ami_corewindow *ami_cw, int x, int y)
{
	struct nsurl *url = NULL;
	const char *title = NULL;
	bool ok = false;
	struct gui_window_2 *gwin;
	struct ami_corewindow *cw;

	if(global_history_has_selection()) {
		ok = global_history_get_selection(&url, &title);
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
 * menu stuff
 */

 /* menu hook functions */
HOOKF(void, ami_history_global_menu_item_project_export, APTR, window, struct IntuiMessage *)
{
	char fname[1024];
	struct ami_corewindow *ami_cw;
	GetAttr(WINDOW_UserData, (Object *)window, (ULONG *)&ami_cw);

	if(AslRequestTags(savereq,
		ASLFR_Window, ami_cw->win,
		ASLFR_SleepWindow, TRUE,
		ASLFR_TitleText, messages_get("NetSurf"),
		ASLFR_Screen, ami_gui_get_screen(),
		ASLFR_InitialFile, "history.html",
		TAG_DONE)) {
			strlcpy(fname, savereq->fr_Drawer, 1024);
			AddPart(fname, savereq->fr_File, 1024);
			ami_update_pointer(ami_cw->win, GUI_POINTER_WAIT);
			global_history_export(fname, NULL);
			ami_update_pointer(ami_cw->win, GUI_POINTER_DEFAULT);
	}
}
									
HOOKF(void, ami_history_global_menu_item_project_expand_all, APTR, window, struct IntuiMessage *)
{
	global_history_expand(false);
}
 
HOOKF(void, ami_history_global_menu_item_project_expand_folders, APTR, window, struct IntuiMessage *)
{
	global_history_expand(true);
}

HOOKF(void, ami_history_global_menu_item_project_expand_links, APTR, window, struct IntuiMessage *)
{
	global_history_expand(false);
}

HOOKF(void, ami_history_global_menu_item_project_collapse_all, APTR, window, struct IntuiMessage *)
{
	global_history_contract(true);
}
 
HOOKF(void, ami_history_global_menu_item_project_collapse_folders, APTR, window, struct IntuiMessage *)
{
	global_history_contract(true);
}

HOOKF(void, ami_history_global_menu_item_project_collapse_links, APTR, window, struct IntuiMessage *)
{
	global_history_contract(false);
}

HOOKF(void, ami_history_global_menu_item_project_snapshot, APTR, window, struct IntuiMessage *)
{
	struct ami_corewindow *ami_cw;
	GetAttr(WINDOW_UserData, (Object *)window, (ULONG *)&ami_cw);

	nsoption_set_int(history_window_ypos, ami_cw->win->TopEdge);
	nsoption_set_int(history_window_xpos, ami_cw->win->LeftEdge);
	nsoption_set_int(history_window_xsize, ami_cw->win->Width);
	nsoption_set_int(history_window_ysize, ami_cw->win->Height);
}

HOOKF(void, ami_history_global_menu_item_project_close, APTR, window, struct IntuiMessage *)
{
	struct ami_corewindow *ami_cw;
	GetAttr(WINDOW_UserData, (Object *)window, (ULONG *)&ami_cw);
	
	ami_cw->close_window = true;
}

HOOKF(void, ami_history_global_menu_item_edit_select_all, APTR, window, struct IntuiMessage *)
{
	global_history_keypress(NS_KEY_SELECT_ALL);
}

HOOKF(void, ami_history_global_menu_item_edit_clear, APTR, window, struct IntuiMessage *)
{
	global_history_keypress(NS_KEY_CLEAR_SELECTION);
}

HOOKF(void, ami_history_global_menu_item_edit_delete, APTR, window, struct IntuiMessage *)
{
	global_history_keypress(NS_KEY_DELETE_LEFT);
}


/* menu setup */

static void ami_history_global_menulabs(struct ami_menu_data **md)
{
	ami_menu_alloc_item(md, AMI_HISTORY_M_PROJECT, NM_TITLE, "Tree", NULL, NULL, NULL, NULL, 0);
	ami_menu_alloc_item(md, AMI_HISTORY_M_EXPORT,   NM_ITEM, "TreeExport", "S", "TBImages:list_save",
		ami_history_global_menu_item_project_export, NULL, 0);
	ami_menu_alloc_item(md, AMI_HISTORY_M_BAR_P1, NM_ITEM, NM_BARLABEL, NULL, NULL, NULL, NULL, 0);
	ami_menu_alloc_item(md, AMI_HISTORY_M_EXPAND,   NM_ITEM, "Expand", NULL, "TBImages:list_folderunfold", NULL, NULL, 0);
	ami_menu_alloc_item(md, AMI_HISTORY_M_EXPAND_ALL,   NM_SUB, "All", "+", NULL,
		ami_history_global_menu_item_project_expand_all, NULL, 0);
	ami_menu_alloc_item(md, AMI_HISTORY_M_EXPAND_FOLDERS,   NM_SUB, "Folders", NULL, NULL,
		ami_history_global_menu_item_project_expand_folders, NULL, 0);
	ami_menu_alloc_item(md, AMI_HISTORY_M_EXPAND_LINKS,   NM_SUB, "Links", NULL, NULL,
		ami_history_global_menu_item_project_expand_links, NULL, 0);
	ami_menu_alloc_item(md, AMI_HISTORY_M_COLLAPSE,   NM_ITEM, "Collapse", NULL, "TBImages:list_folderfold", NULL, NULL, 0);
	ami_menu_alloc_item(md, AMI_HISTORY_M_COLLAPSE_ALL,   NM_SUB, "All", "-", NULL,
		ami_history_global_menu_item_project_collapse_all, NULL, 0);
	ami_menu_alloc_item(md, AMI_HISTORY_M_COLLAPSE_FOLDERS,   NM_SUB, "Folders", NULL, NULL,
		ami_history_global_menu_item_project_collapse_folders, NULL, 0);
	ami_menu_alloc_item(md, AMI_HISTORY_M_COLLAPSE_LINKS,   NM_SUB, "Links", NULL, NULL,
		ami_history_global_menu_item_project_collapse_links, NULL, 0);
	ami_menu_alloc_item(md, AMI_HISTORY_M_BAR_P2, NM_ITEM, NM_BARLABEL, NULL, NULL, NULL, NULL, 0);
	ami_menu_alloc_item(md, AMI_HISTORY_M_SNAPSHOT,   NM_ITEM, "SnapshotWindow", NULL, "TBImages:list_hold",
		ami_history_global_menu_item_project_snapshot, NULL, 0);
	ami_menu_alloc_item(md, AMI_HISTORY_M_BAR_P3, NM_ITEM, NM_BARLABEL, NULL, NULL, NULL, NULL, 0);
	ami_menu_alloc_item(md, AMI_HISTORY_M_CLOSE,   NM_ITEM, "CloseWindow", "K", "TBImages:list_cancel",
		ami_history_global_menu_item_project_close, NULL, 0);

	ami_menu_alloc_item(md, AMI_HISTORY_M_EDIT, NM_TITLE, "Edit", NULL, NULL, NULL, NULL, 0);
	ami_menu_alloc_item(md, AMI_HISTORY_M_SELECTALL,   NM_ITEM, "SelectAllNS", "A", NSA_SPACE,
		ami_history_global_menu_item_edit_select_all, NULL, 0);
	ami_menu_alloc_item(md, AMI_HISTORY_M_CLEAR,   NM_ITEM, "ClearNS", NULL, NSA_SPACE,
		ami_history_global_menu_item_edit_clear, NULL, 0);
	ami_menu_alloc_item(md, AMI_HISTORY_M_BAR_E1, NM_ITEM, NM_BARLABEL, NULL, NULL, NULL, NULL, 0);
	ami_menu_alloc_item(md, AMI_HISTORY_M_DELETE,   NM_ITEM, "TreeDelete", "Del", "TBImages:list_delete",
		ami_history_global_menu_item_edit_delete, NULL, 0);

	ami_menu_alloc_item(md, AMI_HISTORY_M_LAST,   NM_END, NULL, NULL, NULL, NULL, NULL, 0);
}

static struct Menu *
ami_history_global_menu_create(struct ami_history_global_window *history_win)
{
	ami_history_global_menulabs(history_win->menu_data);
	history_win->imenu = ami_menu_layout(history_win->menu_data, AMI_HISTORY_M_LAST);
	if(history_win->imenu == NULL) return NULL;

	return history_win->imenu;
}


static nserror
ami_history_global_create_window(struct ami_history_global_window *history_win)
{
	struct ami_corewindow *ami_cw = (struct ami_corewindow *)&history_win->core;
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
		WA_Top, nsoption_int(history_window_ypos),
		WA_Left, nsoption_int(history_window_xpos),
		WA_Width, nsoption_int(history_window_xsize),
		WA_Height, nsoption_int(history_window_ysize),
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
		WINDOW_UserData, history_win,
		WINDOW_MenuStrip, ami_history_global_menu_create(history_win),
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

/* exported interface documented in amiga/cookies.h */
nserror ami_history_global_present(void)
{
	struct ami_history_global_window *ncwin;
	nserror res;

	if(history_window != NULL) {
		//windowtofront()
		return NSERROR_OK;
	}

	ncwin = calloc(1, sizeof(struct ami_history_global_window));
	if (ncwin == NULL) {
		return NSERROR_NOMEM;
	}

	ncwin->core.wintitle = ami_utf8_easy((char *)messages_get("GlobalHistory"));

	res = ami_history_global_create_window(ncwin);
	if (res != NSERROR_OK) {
		NSLOG(netsurf, INFO, "SSL UI builder init failed");
		ami_utf8_free(ncwin->core.wintitle);
		free(ncwin);
		return res;
	}

	/* initialise Amiga core window */
	ncwin->core.draw = ami_history_global_draw;
	ncwin->core.key = ami_history_global_key;
	ncwin->core.mouse = ami_history_global_mouse;
	ncwin->core.close = ami_history_global_destroy;
	ncwin->core.event = NULL;
	ncwin->core.drag_end = ami_history_global_drag_end;
	ncwin->core.icon_drop = NULL;

	res = ami_corewindow_init(&ncwin->core);
	if (res != NSERROR_OK) {
		ami_utf8_free(ncwin->core.wintitle);
		DisposeObject(ncwin->core.objects[GID_CW_WIN]);
		free(ncwin);
		return res;
	}

	res = global_history_init(ncwin->core.cb_table, (struct core_window *)ncwin);
	if (res != NSERROR_OK) {
		ami_utf8_free(ncwin->core.wintitle);
		DisposeObject(ncwin->core.objects[GID_CW_WIN]);
		free(ncwin);
		return res;
	}

	history_window = ncwin;

	return NSERROR_OK;
}

