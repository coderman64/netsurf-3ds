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
 * Implementation of Amiga cookie viewer using core windows.
 */

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#include <proto/intuition.h>

#include <classes/window.h>
#include <gadgets/layout.h>
#include <gadgets/space.h>

#include <reaction/reaction_macros.h>

#include "desktop/cookie_manager.h"
#include "netsurf/keypress.h"
#include "netsurf/plotters.h"
#include "utils/log.h"
#include "utils/messages.h"
#include "utils/nsoption.h"

#include "amiga/cookies.h"
#include "amiga/corewindow.h"
#include "amiga/libs.h"
#include "amiga/menu.h"
#include "amiga/utf8.h"

enum {
	/* Project menu */
	AMI_COOKIE_M_PROJECT = 0,
	 AMI_COOKIE_M_EXPAND,
	  AMI_COOKIE_M_EXPAND_ALL,
	  AMI_COOKIE_M_EXPAND_DOMAINS,
	  AMI_COOKIE_M_EXPAND_COOKIES,
	 AMI_COOKIE_M_COLLAPSE,
	  AMI_COOKIE_M_COLLAPSE_ALL,
	  AMI_COOKIE_M_COLLAPSE_DOMAINS,
	  AMI_COOKIE_M_COLLAPSE_COOKIES,
	 AMI_COOKIE_M_BAR_P1,
	 AMI_COOKIE_M_SNAPSHOT,
	 AMI_COOKIE_M_BAR_P2,
	 AMI_COOKIE_M_CLOSE,
	/* Edit menu */
	AMI_COOKIE_M_EDIT,
	 AMI_COOKIE_M_SELECTALL,
	 AMI_COOKIE_M_CLEAR,
	 AMI_COOKIE_M_BAR_E1,
	 AMI_COOKIE_M_DELETE,
	AMI_COOKIE_M_LAST
};

/**
 * Amiga cookie viewer window context
 */
struct ami_cookie_window {
	/** Amiga core window context */
	struct ami_corewindow core;

	struct ami_menu_data *menu_data[AMI_COOKIE_M_LAST + 1];
	struct Menu *imenu; /* Intuition menu */
};

static struct ami_cookie_window *cookie_window = NULL;


static void
ami_cookies_menu_free(struct ami_cookie_window *cookie_win)
{
	SetAttrs(cookie_win->core.objects[GID_CW_WIN],
		WINDOW_MenuStrip, NULL,
	TAG_DONE);
	
	ami_menu_free_menu(cookie_win->menu_data, AMI_COOKIE_M_LAST, cookie_win->imenu);
}

/**
 * destroy a previously created cookie view
 */
static void
ami_cookies_destroy(struct ami_corewindow *ami_cw)
{
	nserror res;

	if(cookie_window == NULL)
		return;

	res = cookie_manager_fini();
	if (res == NSERROR_OK) {
		ami_cookies_menu_free(cookie_window);
		res = ami_corewindow_fini(&cookie_window->core); /* closes the window for us, frees cookie_win */
		cookie_window = NULL;
	}
}


/**
 * callback for mouse action for cookie viewer on core window
 *
 * \param ami_cw The Amiga core window structure.
 * \param mouse_state netsurf mouse state on event
 * \param x location of event
 * \param y location of event
 * \return NSERROR_OK on success otherwise apropriate error code
 */
static nserror
ami_cookies_mouse(struct ami_corewindow *ami_cw,
					browser_mouse_state mouse_state,
					int x, int y)
{
	cookie_manager_mouse_action(mouse_state, x, y);

	return NSERROR_OK;
}

/**
 * callback for keypress for cookies viewer on core window
 *
 * \param ami_cw The Amiga core window structure.
 * \param nskey The netsurf key code
 * \return NSERROR_OK on success otherwise apropriate error code
 */
static nserror
ami_cookies_key(struct ami_corewindow *ami_cw, uint32_t nskey)
{
	if (cookie_manager_keypress(nskey)) {
			return NSERROR_OK;
	}
	return NSERROR_NOT_IMPLEMENTED;
}

/**
 * callback on draw event for cookies viewer on core window
 *
 * \param ami_cw The Amiga core window structure.
 * \param x The x cordinate to plot at
 * \param y The y cordinate to plot at
 * \param r The rectangle of the window that needs updating.
 * \param ctx The drawing context
 * \return NSERROR_OK on success otherwise apropriate error code
 */
static nserror
ami_cookies_draw(struct ami_corewindow *ami_cw, int x, int y, struct rect *r, struct redraw_context *ctx)
{
	cookie_manager_redraw(x, y, r, ctx);

	return NSERROR_OK;
}

/**
 * menu stuff
 */

 /* menu hook functions */
 
HOOKF(void, ami_cookies_menu_item_project_expand_all, APTR, window, struct IntuiMessage *)
{
	cookie_manager_expand(false);
}
 
HOOKF(void, ami_cookies_menu_item_project_expand_domains, APTR, window, struct IntuiMessage *)
{
	cookie_manager_expand(true);
}

HOOKF(void, ami_cookies_menu_item_project_expand_cookies, APTR, window, struct IntuiMessage *)
{
	cookie_manager_expand(false);
}

HOOKF(void, ami_cookies_menu_item_project_collapse_all, APTR, window, struct IntuiMessage *)
{
	cookie_manager_contract(true);
}
 
HOOKF(void, ami_cookies_menu_item_project_collapse_domains, APTR, window, struct IntuiMessage *)
{
	cookie_manager_contract(true);
}

HOOKF(void, ami_cookies_menu_item_project_collapse_cookies, APTR, window, struct IntuiMessage *)
{
	cookie_manager_contract(false);
}

HOOKF(void, ami_cookies_menu_item_project_snapshot, APTR, window, struct IntuiMessage *)
{
	struct ami_corewindow *ami_cw;
	GetAttr(WINDOW_UserData, (Object *)window, (ULONG *)&ami_cw);

	nsoption_set_int(cookies_window_ypos, ami_cw->win->TopEdge);
	nsoption_set_int(cookies_window_xpos, ami_cw->win->LeftEdge);
	nsoption_set_int(cookies_window_xsize, ami_cw->win->Width);
	nsoption_set_int(cookies_window_ysize, ami_cw->win->Height);
}

HOOKF(void, ami_cookies_menu_item_project_close, APTR, window, struct IntuiMessage *)
{
	struct ami_corewindow *ami_cw;
	GetAttr(WINDOW_UserData, (Object *)window, (ULONG *)&ami_cw);
	
	ami_cw->close_window = true;
}

HOOKF(void, ami_cookies_menu_item_edit_select_all, APTR, window, struct IntuiMessage *)
{
	cookie_manager_keypress(NS_KEY_SELECT_ALL);
}

HOOKF(void, ami_cookies_menu_item_edit_clear, APTR, window, struct IntuiMessage *)
{
	cookie_manager_keypress(NS_KEY_CLEAR_SELECTION);
}

HOOKF(void, ami_cookies_menu_item_edit_delete, APTR, window, struct IntuiMessage *)
{
	cookie_manager_keypress(NS_KEY_DELETE_LEFT);
}


/* menu setup */

static void ami_cookies_menulabs(struct ami_menu_data **md)
{
	ami_menu_alloc_item(md, AMI_COOKIE_M_PROJECT, NM_TITLE, "Tree", NULL, NULL, NULL, NULL, 0);
	ami_menu_alloc_item(md, AMI_COOKIE_M_EXPAND,   NM_ITEM, "Expand", NULL, "TBImages:list_folderunfold", NULL, NULL, 0);
	ami_menu_alloc_item(md, AMI_COOKIE_M_EXPAND_ALL,   NM_SUB, "All", "+", NULL,
		ami_cookies_menu_item_project_expand_all, NULL, 0);
	ami_menu_alloc_item(md, AMI_COOKIE_M_EXPAND_DOMAINS,   NM_SUB, "Domains", NULL, NULL,
		ami_cookies_menu_item_project_expand_domains, NULL, 0);
	ami_menu_alloc_item(md, AMI_COOKIE_M_EXPAND_COOKIES,   NM_SUB, "Cookies", NULL, NULL,
		ami_cookies_menu_item_project_expand_cookies, NULL, 0);
	ami_menu_alloc_item(md, AMI_COOKIE_M_COLLAPSE,   NM_ITEM, "Collapse", NULL, "TBImages:list_folderfold", NULL, NULL, 0);
	ami_menu_alloc_item(md, AMI_COOKIE_M_COLLAPSE_ALL,   NM_SUB, "All", "-", NULL,
		ami_cookies_menu_item_project_collapse_all, NULL, 0);
	ami_menu_alloc_item(md, AMI_COOKIE_M_COLLAPSE_DOMAINS,   NM_SUB, "Domains", NULL, NULL,
		ami_cookies_menu_item_project_collapse_domains, NULL, 0);
	ami_menu_alloc_item(md, AMI_COOKIE_M_COLLAPSE_COOKIES,   NM_SUB, "Cookies", NULL, NULL,
		ami_cookies_menu_item_project_collapse_cookies, NULL, 0);
	ami_menu_alloc_item(md, AMI_COOKIE_M_BAR_P1, NM_ITEM, NM_BARLABEL, NULL, NULL, NULL, NULL, 0);
	ami_menu_alloc_item(md, AMI_COOKIE_M_SNAPSHOT,   NM_ITEM, "SnapshotWindow", NULL, "TBImages:list_hold",
		ami_cookies_menu_item_project_snapshot, NULL, 0);
	ami_menu_alloc_item(md, AMI_COOKIE_M_BAR_P2, NM_ITEM, NM_BARLABEL, NULL, NULL, NULL, NULL, 0);
	ami_menu_alloc_item(md, AMI_COOKIE_M_CLOSE,   NM_ITEM, "CloseWindow", "K", "TBImages:list_cancel",
		ami_cookies_menu_item_project_close, NULL, 0);

	ami_menu_alloc_item(md, AMI_COOKIE_M_EDIT, NM_TITLE, "Edit", NULL, NULL, NULL, NULL, 0);
	ami_menu_alloc_item(md, AMI_COOKIE_M_SELECTALL,   NM_ITEM, "SelectAllNS", "A", NSA_SPACE,
		ami_cookies_menu_item_edit_select_all, NULL, 0);
	ami_menu_alloc_item(md, AMI_COOKIE_M_CLEAR,   NM_ITEM, "ClearNS", NULL, NSA_SPACE,
		ami_cookies_menu_item_edit_clear, NULL, 0);
	ami_menu_alloc_item(md, AMI_COOKIE_M_BAR_E1, NM_ITEM, NM_BARLABEL, NULL, NULL, NULL, NULL, 0);
	ami_menu_alloc_item(md, AMI_COOKIE_M_DELETE,   NM_ITEM, "TreeDelete", "Del", "TBImages:list_delete",
		ami_cookies_menu_item_edit_delete, NULL, 0);

	ami_menu_alloc_item(md, AMI_COOKIE_M_LAST,   NM_END, NULL, NULL, NULL, NULL, NULL, 0);
}

static struct Menu *
ami_cookies_menu_create(struct ami_cookie_window *cookie_win)
{
	ami_cookies_menulabs(cookie_win->menu_data);
	cookie_win->imenu = ami_menu_layout(cookie_win->menu_data, AMI_COOKIE_M_LAST);
	if(cookie_win->imenu == NULL) return NULL;

	return cookie_win->imenu;
}


static nserror
ami_cookies_create_window(struct ami_cookie_window *cookie_win)
{
	struct ami_corewindow *ami_cw = (struct ami_corewindow *)&cookie_win->core;
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
		WA_Top, nsoption_int(cookies_window_ypos),
		WA_Left, nsoption_int(cookies_window_xpos),
		WA_Width, nsoption_int(cookies_window_xsize),
		WA_Height, nsoption_int(cookies_window_ysize),
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
		WINDOW_UserData, cookie_win,
		WINDOW_MenuStrip, ami_cookies_menu_create(cookie_win),
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
nserror ami_cookies_present(const char *search_term)
{
	struct ami_cookie_window *ncwin;
	nserror res;

	if(cookie_window != NULL) {
		//windowtofront()
		return NSERROR_OK;
	}

	ncwin = calloc(1, sizeof(struct ami_cookie_window));
	if (ncwin == NULL) {
		return NSERROR_NOMEM;
	}

	ncwin->core.wintitle = ami_utf8_easy((char *)messages_get("Cookies"));

	res = ami_cookies_create_window(ncwin);
	if (res != NSERROR_OK) {
		NSLOG(netsurf, INFO, "SSL UI builder init failed");
		ami_utf8_free(ncwin->core.wintitle);
		free(ncwin);
		return res;
	}

	/* initialise Amiga core window */
	ncwin->core.draw = ami_cookies_draw;
	ncwin->core.key = ami_cookies_key;
	ncwin->core.mouse = ami_cookies_mouse;
	ncwin->core.close = ami_cookies_destroy;
	ncwin->core.event = NULL;

	res = ami_corewindow_init(&ncwin->core);
	if (res != NSERROR_OK) {
		ami_utf8_free(ncwin->core.wintitle);
		DisposeObject(ncwin->core.objects[GID_CW_WIN]);
		free(ncwin);
		return res;
	}

	res = cookie_manager_init(ncwin->core.cb_table, (struct core_window *)ncwin);
	if (res != NSERROR_OK) {
		ami_utf8_free(ncwin->core.wintitle);
		DisposeObject(ncwin->core.objects[GID_CW_WIN]);
		free(ncwin);
		return res;
	}

	cookie_window = ncwin;

	if (search_term != NULL) {
		res = cookie_manager_set_search_string(search_term);
	}

	return res;
}

