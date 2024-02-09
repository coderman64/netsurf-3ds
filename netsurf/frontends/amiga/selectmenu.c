/*
 * Copyright 2008 - 2011 Chris Young <chris@unsatisfactorysoftware.co.uk>
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

#ifdef __amigaos4__

#include <stdbool.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/popupmenu.h>
#include <proto/utility.h>
#include <reaction/reaction_macros.h>

#include "utils/errors.h"
#include "utils/log.h"
#include "utils/messages.h"
#include "netsurf/form.h"
#include "netsurf/mouse.h"

#include "amiga/gui.h"
#include "amiga/selectmenu.h"
#include "amiga/theme.h"
#include "amiga/utf8.h"

/* Maximum number of items for a popupmenu.class select menu.
 * 50 is about the limit for my screen, and popupmenu doesn't scroll.
 * We may need to calculate a value for this based on screen/font size.
 *
 * Additional entries will be added to a "More" menu...
 */
#define AMI_SELECTMENU_PAGE_MAX 40

/* ...limited to the number of menus defined here... */
#define AMI_SELECTMENU_MENU_MAX 10

/* ...and resulting in this total number of entries. */
#define AMI_SELECTMENU_MAX (AMI_SELECTMENU_PAGE_MAX * AMI_SELECTMENU_MENU_MAX)


/** Exported interface documented in selectmenu.h **/
BOOL ami_selectmenu_is_safe(void)
{
	struct Library *PopupMenuBase;
	BOOL popupmenu_lib_ok = FALSE;

	if((PopupMenuBase = OpenLibrary("popupmenu.library", 53))) {
		NSLOG(netsurf, INFO, "popupmenu.library v%d.%d",
		      PopupMenuBase->lib_Version, PopupMenuBase->lib_Revision);
		if(LIB_IS_AT_LEAST((struct Library *)PopupMenuBase, 53, 11))
			popupmenu_lib_ok = TRUE;
		CloseLibrary(PopupMenuBase);
	}

	return popupmenu_lib_ok;
}

HOOKF(uint32, ami_popup_hook, Object *, item, APTR)
{
	uint32 itemid = 0;
	struct form_control *control = hook->h_Data;

	if(GetAttr(PMIA_ID, item, &itemid)) {
		form_select_process_selection(control, itemid);
	}

	return itemid;
}

void gui_create_form_select_menu(struct gui_window *g,
		struct form_control *control)
{
	struct Library *PopupMenuBase = NULL;
	struct PopupMenuIFace *IPopupMenu = NULL;
	struct Hook selectmenuhook;
	Object *selectmenuobj;
	Object *smenu = NULL;
	Object *currentmenu;
	Object *submenu = NULL;
	char *selectmenu_item[AMI_SELECTMENU_MAX];
	char *more_label;
	struct form_option *opt = form_select_get_option(control, 0);
	int i = 0;
	int n = 0;

	if(ami_selectmenu_is_safe() == FALSE) return;

	if((PopupMenuBase = OpenLibrary("popupmenu.class", 0))) {
		IPopupMenu = (struct PopupMenuIFace *)GetInterface(PopupMenuBase, "main", 1, NULL);
	}

	if(IPopupMenu == NULL) return;

	ClearMem(selectmenu_item, AMI_SELECTMENU_MAX * 4);
	more_label = ami_utf8_easy(messages_get("More"));

	selectmenuhook.h_Entry = ami_popup_hook;
	selectmenuhook.h_SubEntry = NULL;
	selectmenuhook.h_Data = control;

	selectmenuobj = PMMENU(form_control_get_name(control)),
                        PMA_MenuHandler, &selectmenuhook, End;

	currentmenu = selectmenuobj;

	while(opt) {
		selectmenu_item[i] = ami_utf8_easy(opt->text);

		IDoMethod(currentmenu, PM_INSERT,
			NewObject(POPUPMENU_GetItemClass(), NULL,
				PMIA_Title, (ULONG)selectmenu_item[i],
				PMIA_ID, i,
				PMIA_CheckIt, TRUE,
				PMIA_Checked, opt->selected,
				TAG_DONE),
			~0);

		opt = opt->next;
		i++;
		n++;

		if(n == AMI_SELECTMENU_PAGE_MAX) {
			if(submenu != NULL) {
				/* attach the previous submenu */
				IDoMethod(smenu, PM_INSERT,
					NewObject(NULL, "popupmenuitem.class",
						PMIA_Title, more_label,
						PMIA_CheckIt, TRUE,
						PMIA_SubMenu, submenu,
					TAG_DONE),
				~0);
			}

			submenu = NewObject(NULL, "popupmenu.class", TAG_DONE);
			smenu = currentmenu;
			currentmenu = submenu;
			n = 0;
		}

		if(i >= AMI_SELECTMENU_MAX) break;
	}

	if((submenu != NULL) && (n != 0)) {
		/* attach the previous submenu */
		IDoMethod(smenu, PM_INSERT,
			NewObject(NULL, "popupmenuitem.class",
				PMIA_Title, more_label,
				PMIA_CheckIt, TRUE,
				PMIA_SubMenu, submenu,
			TAG_DONE),
		~0);
	}

	ami_set_pointer(ami_gui_get_gui_window_2(g), GUI_POINTER_DEFAULT, false); // Clear the menu-style pointer

	IDoMethod(selectmenuobj, PM_OPEN, ami_gui_get_window(g));

	/* PM_OPEN is blocking, so dispose menu immediately... */
	if(selectmenuobj) DisposeObject(selectmenuobj);

	/* ...and get rid of popupmenu.class ASAP */
	if(IPopupMenu) DropInterface((struct Interface *)IPopupMenu);
	if(PopupMenuBase) CloseLibrary(PopupMenuBase);

	/* Free the menu labels */
	if(more_label) ami_utf8_free(more_label);
	for(i = 0; i < AMI_SELECTMENU_MAX; i++) {
		if(selectmenu_item[i] != NULL) {
			ami_utf8_free(selectmenu_item[i]);
			selectmenu_item[i] = NULL;
		}
	}
}

#else
#include "amiga/selectmenu.h"
void gui_create_form_select_menu(struct gui_window *g, struct form_control *control)
{
}

BOOL ami_selectmenu_is_safe()
{
	return FALSE;
}
#endif

