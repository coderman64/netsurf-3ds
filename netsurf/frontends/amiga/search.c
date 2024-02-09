/*
 * Copyright 2004 John M Bell <jmb202@ecs.soton.ac.uk>
 * Copyright 2005 Adrian Lees <adrianl@users.sourceforge.net>
 * Copyright 2008 Chris Young <chris@unsatisfactorysoftware.co.uk>
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
 * Free text search (implementation)
 */

#include "amiga/os3support.h"

#include "utils/config.h"
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#include <proto/intuition.h>
#include <proto/exec.h>
#include <proto/window.h>
#include <proto/layout.h>
#include <proto/string.h>
#include <proto/button.h>
#include <proto/label.h>
#include <proto/checkbox.h>
#include <classes/window.h>
#include <gadgets/layout.h>
#include <gadgets/string.h>
#include <gadgets/button.h>
#include <gadgets/checkbox.h>
#include <images/label.h>
#include <reaction/reaction_macros.h>

#include "utils/log.h"
#include "utils/messages.h"
#include "utils/utils.h"
#include "content/content.h"
#include "netsurf/browser_window.h"
#include "desktop/search.h"
#include "netsurf/mouse.h"
#include "netsurf/window.h"
#include "netsurf/search.h"

#include "amiga/libs.h"
#include "amiga/gui.h"
#include "amiga/memory.h"
#include "amiga/search.h"
#include "amiga/object.h"
#include "amiga/theme.h"
#include "amiga/utf8.h"

#ifndef NOF_ELEMENTS
#define NOF_ELEMENTS(array) (sizeof(array)/sizeof(*(array)))
#endif

static bool search_insert;

enum
{
    OID_S_MAIN = 0,
	GID_S_MAIN,
	GID_S_NEXT,
	GID_S_PREV,
	GID_S_SEARCHSTRING,
	GID_S_SHOWALL,
	GID_S_CASE,
	GID_S_LAST
};

enum {
	SSTR_TITLE = 0,
	SSTR_CASE,
	SSTR_SHOWALL,
	SSTR_PREV,
	SSTR_NEXT,
	SSTR_LAST
};

struct find_window {
	struct ami_generic_window w;
	struct Window *win;
	Object *objects[GID_S_LAST];
	struct gui_window *gwin;
	char *message[SSTR_LAST];
};

static struct find_window *fwin = NULL;

search_flags_t ami_search_flags(void);
char *ami_search_string(void);
static void ami_search_set_status(bool found, void *p);
static void ami_search_set_hourglass(bool active, void *p);
static void ami_search_add_recent(const char *string, void *p);
static void ami_search_set_forward_state(bool active, void *p);
static void ami_search_set_back_state(bool active, void *p);
static BOOL ami_search_event(void *w);

static struct gui_search_table search_table = {
	ami_search_set_status,
	ami_search_set_hourglass,
	ami_search_add_recent,
	ami_search_set_forward_state,
	ami_search_set_back_state,
};

static const struct ami_win_event_table ami_search_table = {
	ami_search_event,
	NULL, /* we don't explicitly close the search window on quit */
};

struct gui_search_table *amiga_search_table = &search_table;

struct gui_window *ami_search_get_gwin(struct find_window *fw)
{
	if (fw != NULL) {
		return fw->gwin;
	}
	return NULL;
}

/**
 * Change the displayed search status.
 *
 * \param gwin gui window to open search for.
 */
void ami_search_open(struct gui_window *gwin)
{
	search_insert = true;

	if(fwin)
	{
		browser_window_search_clear(ami_gui_get_browser_window(fwin->gwin));
		ami_gui_set_find_window(fwin->gwin, NULL);
		fwin->gwin = gwin;
		ami_gui_set_find_window(fwin->gwin, fwin);
		WindowToFront(fwin->win);
		ActivateWindow(fwin->win);
		return;
	}

	fwin = calloc(1, sizeof(struct find_window));

	/* Get local charset messages. If any of these are NULL it doesn't matter */
	fwin->message[SSTR_TITLE] = ami_utf8_easy(messages_get("FindTextNS"));
	fwin->message[SSTR_CASE] = ami_utf8_easy(messages_get("CaseSens"));
	fwin->message[SSTR_SHOWALL] = ami_utf8_easy(messages_get("ShowAll"));
	fwin->message[SSTR_PREV] = ami_utf8_easy(messages_get("Prev"));
	fwin->message[SSTR_NEXT] = ami_utf8_easy(messages_get("Next"));

	fwin->objects[OID_S_MAIN] = WindowObj,
      	WA_ScreenTitle, ami_gui_get_screen_title(),
       	WA_Title, fwin->message[SSTR_TITLE],
       	WA_Activate, TRUE,
       	WA_DepthGadget, TRUE,
       	WA_DragBar, TRUE,
       	WA_CloseGadget, TRUE,
       	WA_SizeGadget, TRUE,
		WA_PubScreen, ami_gui_get_screen(),
		WINDOW_SharedPort, ami_gui_get_shared_msgport(),
		WINDOW_UserData, fwin,
		WINDOW_IconifyGadget, FALSE,
		WINDOW_LockHeight, TRUE,
         	WINDOW_Position, WPOS_CENTERSCREEN,
           	WINDOW_ParentGroup, fwin->objects[GID_S_MAIN] = LayoutVObj,
				LAYOUT_AddChild, fwin->objects[GID_S_SEARCHSTRING] = StringObj,
					GA_ID, GID_S_SEARCHSTRING,
					GA_TabCycle, TRUE,
					GA_RelVerify, TRUE,
				StringEnd,
				CHILD_WeightedHeight, 0,
				LAYOUT_AddChild, fwin->objects[GID_S_CASE] = CheckBoxObj,
					GA_ID, GID_S_CASE,
					GA_Text, fwin->message[SSTR_CASE],
					GA_Selected, FALSE,
					GA_TabCycle, TRUE,
					GA_RelVerify, TRUE,
				CheckBoxEnd,
				LAYOUT_AddChild, fwin->objects[GID_S_SHOWALL] = CheckBoxObj,
					GA_ID,GID_S_SHOWALL,
					GA_Text, fwin->message[SSTR_SHOWALL],
					GA_Selected, FALSE,
					GA_TabCycle, TRUE,
					GA_RelVerify, TRUE,
				CheckBoxEnd,
				LAYOUT_AddChild, LayoutHObj,
					LAYOUT_AddChild, fwin->objects[GID_S_PREV] = ButtonObj,
						GA_ID, GID_S_PREV,
						GA_RelVerify, TRUE,
						GA_Text, fwin->message[SSTR_PREV],
						GA_TabCycle, TRUE,
						GA_Disabled, TRUE,
					ButtonEnd,
					CHILD_WeightedHeight, 0,
					LAYOUT_AddChild, fwin->objects[GID_S_NEXT] = ButtonObj,
						GA_ID, GID_S_NEXT,
						GA_RelVerify, TRUE,
						GA_Text, fwin->message[SSTR_NEXT],
						GA_TabCycle, TRUE,
						GA_Disabled, TRUE,
					ButtonEnd,
				LayoutEnd,
				CHILD_WeightedHeight,0,
			EndGroup,
		EndWindow;

	fwin->win = (struct Window *)RA_OpenWindow(fwin->objects[OID_S_MAIN]);
	fwin->gwin = gwin;
	ami_gui_win_list_add(fwin, AMINS_FINDWINDOW, &ami_search_table);
	ami_gui_set_find_window(fwin->gwin, fwin);
	
	ActivateLayoutGadget((struct Gadget *)fwin->objects[GID_S_MAIN], fwin->win,
			NULL, (ULONG)fwin->objects[GID_S_SEARCHSTRING]);
}

void ami_search_close(void)
{
	browser_window_search_clear(ami_gui_get_browser_window(fwin->gwin));
	ami_gui_set_find_window(fwin->gwin, NULL);
	DisposeObject(fwin->objects[OID_S_MAIN]);

	/* Free local charset version of messages */
	for(int i = 0; i < SSTR_LAST; i++) {
		ami_utf8_free(fwin->message[i]);
	}

	ami_gui_win_list_remove(fwin);
	fwin = NULL;
}

static BOOL ami_search_event(void *w)
{
	/* return TRUE if window destroyed */
	ULONG result;
	uint16 code;
	search_flags_t flags;

	while((result = RA_HandleInput(fwin->objects[OID_S_MAIN],&code)) != WMHI_LASTMSG)
	{
       	switch(result & WMHI_CLASSMASK) // class
	{
	case WMHI_GADGETUP:
		switch(result & WMHI_GADGETMASK)
		{
			case GID_S_SEARCHSTRING:
				browser_window_search_clear(ami_gui_get_browser_window(fwin->gwin));
						
				RefreshSetGadgetAttrs((struct Gadget *)fwin->objects[GID_S_PREV],
					fwin->win, NULL,
					GA_Disabled, FALSE,
					TAG_DONE);

				RefreshSetGadgetAttrs((struct Gadget *)fwin->objects[GID_S_NEXT],
					fwin->win, NULL,
					GA_Disabled, FALSE,
					TAG_DONE);
	
				/* fall through */

			case GID_S_NEXT:
				search_insert = true;
				flags = SEARCH_FLAG_FORWARDS |
					ami_search_flags();
				browser_window_search(
						ami_gui_get_browser_window(fwin->gwin),
						NULL,
						flags, ami_search_string());
				ActivateWindow(ami_gui_get_window(fwin->gwin));
			break;

			case GID_S_PREV:
				search_insert = true;
				flags = ~SEARCH_FLAG_FORWARDS &
					ami_search_flags();
				browser_window_search(
						ami_gui_get_browser_window(fwin->gwin),
						NULL,
						flags, ami_search_string());
				ActivateWindow(ami_gui_get_window(fwin->gwin));
			break;
		}
		break;

	case WMHI_CLOSEWINDOW:
		ami_search_close();
		return TRUE;
		break;
	}
	}
	return FALSE;
}

/**
* Change the displayed search status.
* \param found  search pattern matched in text
* \param p the pointer sent to search_verify_new() / search_create_context()
*/

void ami_search_set_status(bool found, void *p)
{
}

/**
* display hourglass while searching
* \param active start/stop indicator
* \param p the pointer sent to search_verify_new() / search_create_context()
*/

void ami_search_set_hourglass(bool active, void *p)
{
	if(active)
		ami_update_pointer(fwin->win, GUI_POINTER_WAIT);
	else
		ami_update_pointer(fwin->win, GUI_POINTER_DEFAULT);
}

/**
* retrieve string being searched for from gui
*/

char *ami_search_string(void)
{
	char *text;
	GetAttr(STRINGA_TextVal,fwin->objects[GID_S_SEARCHSTRING],(ULONG *)&text);
	return text;

}

/**
* add search string to recent searches list
* front is at liberty how to implement the bare notification
* should normally store a strdup() of the string;
* core gives no guarantee of the integrity of the const char *
* \param string search pattern
* \param p the pointer sent to search_verify_new() / search_create_context()
*/

void ami_search_add_recent(const char *string, void *p)
{
}

/**
* activate search forwards button in gui
* \param active activate/inactivate
* \param p the pointer sent to search_verify_new() / search_create_context()
*/

void ami_search_set_forward_state(bool active, void *p)
{
	RefreshSetGadgetAttrs((struct Gadget *)fwin->objects[GID_S_NEXT],
			fwin->win, NULL,
			GA_Disabled, active ? FALSE : TRUE, TAG_DONE);

}

/**
* activate search forwards button in gui
* \param active activate/inactivate
* \param p the pointer sent to search_verify_new() / search_create_context()
*/

void ami_search_set_back_state(bool active, void *p)
{
	RefreshSetGadgetAttrs((struct Gadget *)fwin->objects[GID_S_PREV],
			fwin->win, NULL,
			GA_Disabled, active ? FALSE : TRUE, TAG_DONE);
}

/**
* retrieve state of 'case sensitive', 'show all' checks in gui
*/

search_flags_t ami_search_flags(void)
{
	ULONG case_sensitive, showall;
	search_flags_t flags;
	GetAttr(GA_Selected,fwin->objects[GID_S_CASE],(ULONG *)&case_sensitive);
	GetAttr(GA_Selected,fwin->objects[GID_S_SHOWALL],(ULONG *)&showall);
	flags = 0 | (case_sensitive ? SEARCH_FLAG_CASE_SENSITIVE : 0) |
			(showall ? SEARCH_FLAG_SHOWALL : 0);
	return flags;
}

