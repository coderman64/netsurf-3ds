/*
 * Copyright 2010 Chris Young <chris@unsatisfactorysoftware.co.uk>
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
#include <string.h>

#include <proto/wb.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/icon.h>

#include <graphics/blitattr.h>
#include <workbench/icon.h>

#include "utils/errors.h"
#include "utils/log.h"
#include "utils/messages.h"
#include "utils/utils.h"
#include "utils/nsoption.h"
#include "netsurf/mouse.h"
#include "netsurf/content.h"

#include "amiga/bitmap.h"
#include "amiga/clipboard.h"
#include "amiga/download.h"
#include "amiga/drag.h"
#include "amiga/file.h"
#include "amiga/filetype.h"
#include "amiga/gui.h"
#include "amiga/theme.h"

static void *drag_save_data;
static int drag_save;
static struct gui_window *drag_save_gui;
static struct Window *drag_icon = NULL;
static ULONG drag_icon_width;
static ULONG drag_icon_height;
static BOOL drag_in_progress = FALSE;
static bool ami_autoscroll = false;

void gui_drag_save_object(struct gui_window *g, struct hlcache_handle *c,
		gui_save_type type)
{
	const char *filetype = NULL;

	/* Check we are running on Workbench */
	if(nsoption_charp(pubscreen_name) == NULL) return;
	if(strcmp(nsoption_charp(pubscreen_name), "Workbench")) return;

	switch(type) {
		case GUI_SAVE_OBJECT_ORIG: // object
		case GUI_SAVE_SOURCE:
			filetype = ami_mime_content_to_filetype(c);
		break;
		case GUI_SAVE_COMPLETE:
			filetype = "drawer";
		break;
		case GUI_SAVE_OBJECT_NATIVE:
#ifdef WITH_NS_SVG
			if(ami_mime_compare(c, "svg") == true)
			{
				filetype = "dr2d";
			}
			else
#endif
			{
				filetype = "ilbm";
			}
		break;

		default:
		break;
	}

	ami_drag_icon_show(ami_gui_get_window(g), filetype);

	drag_save_data = c;
	drag_save_gui = g;
	drag_save = type;
}

void gui_drag_save_selection(struct gui_window *g, const char *selection)
{
	ami_drag_icon_show(ami_gui_get_window(g), "ascii");

	ami_autoscroll = true;
	drag_save_data = g;
	drag_save = GUI_SAVE_TEXT_SELECTION;
}

void ami_drag_save(struct Window *win)
{
	ULONG which = WBO_NONE, type;
	char path[1025], dpath[1025];
	struct Screen *scrn = ami_gui_get_screen();

	path[0] = 0; /* ensure path is terminated */

	ami_drag_icon_close(NULL);
	ami_autoscroll = true;

	if(nsoption_charp(pubscreen_name) && (strcmp(nsoption_charp(pubscreen_name),"Workbench") == 0))
	{
		which = WhichWorkbenchObject(NULL,scrn->MouseX,scrn->MouseY,
									WBOBJA_Type,&type,
									WBOBJA_FullPath,&path,
									WBOBJA_FullPathSize,1024,
									WBOBJA_DrawerPath,&dpath,
									WBOBJA_DrawerPathSize,1024,
									TAG_DONE);
	}

	if((which == WBO_DRAWER) || ((which == WBO_ICON) && (type > WBDRAWER)))
	{
		strcpy(path,dpath);
	}
	else if(which == WBO_NONE)
	{
		if(drag_save == GUI_SAVE_TEXT_SELECTION)
			ami_drag_selection((struct gui_window *)drag_save_data);
		else ami_gui_beep();

		drag_save = 0;
		drag_save_data = NULL;
		return;
	}

	if(path[0] == '\0')
	{
		ami_gui_beep();
		drag_save = 0;
		drag_save_data = NULL;
		return;
	}

	ami_update_pointer(win, GUI_POINTER_WAIT);

	switch(drag_save)
	{
		case GUI_SAVE_OBJECT_ORIG: // object
		case GUI_SAVE_SOURCE:
		{
			struct hlcache_handle *c = drag_save_data;

			AddPart(path, content_get_title(c), 1024);
			ami_file_save(AMINS_SAVE_SOURCE, path, win, c, NULL, NULL);
		}
		break;

		case GUI_SAVE_TEXT_SELECTION: // selection
			AddPart(path,"netsurf_text_selection",1024);
			struct gui_window *g = (struct gui_window *) drag_save_data;
			ami_file_save(AMINS_SAVE_SELECTION, path, win, NULL, NULL, ami_gui_get_browser_window(g));
		break;

		case GUI_SAVE_COMPLETE:
		{
			struct hlcache_handle *c = drag_save_data;

			AddPart(path, content_get_title(c), 1024);
			ami_file_save(AMINS_SAVE_COMPLETE, path, win, c, ami_gui_get_favicon(drag_save_gui), NULL);
		}
		break;

		case GUI_SAVE_OBJECT_NATIVE:
		{
			struct hlcache_handle *c = drag_save_data;
			AddPart(path, content_get_title(c), 1024);

			ami_file_save(AMINS_SAVE_IFF, path, win, c, NULL, NULL);
		}
		break;

		default:
			NSLOG(netsurf, INFO,
			      "Unsupported drag save operation %d", drag_save);
		break;
	}

	drag_save = 0;
	drag_save_data = NULL;
	
	ami_update_pointer(win, GUI_POINTER_DEFAULT);
}

void ami_drag_icon_show(struct Window *win, const char *type)
{
	struct DiskObject *dobj = NULL;
	ULONG width, height;
	int deftype = WBPROJECT;
	struct Screen *scrn = ami_gui_get_screen();
	drag_in_progress = TRUE;

	if(nsoption_bool(drag_save_icons) == false)
	{
		ami_update_pointer(win, AMI_GUI_POINTER_DRAG);
		return;
	}
	else
	{
		ami_update_pointer(win, GUI_POINTER_DEFAULT);
	}

	if(!strcmp(type, "drawer")) deftype = WBDRAWER;

	dobj = GetIconTags(NULL, ICONGETA_GetDefaultName, type,
					    ICONGETA_GetDefaultType, deftype,
					    TAG_DONE);

	IconControl(dobj,
                  ICONCTRLA_GetWidth,&width,
                  ICONCTRLA_GetHeight,&height,
                  TAG_DONE);

	drag_icon_width = width;
	drag_icon_height = height;

	drag_icon = OpenWindowTags(NULL,
				WA_Left, scrn->MouseX - (width/2),
				WA_Top, scrn->MouseY - (height/2),
				WA_Width, width,
				WA_Height, height,
				WA_PubScreen, scrn,
				WA_Borderless, TRUE,
				WA_ToolBox, TRUE,
				WA_StayTop, TRUE,
				WA_Opaqueness, 128,
				WA_OverrideOpaqueness, TRUE,
				TAG_DONE);

/* probably need layouticon and drawinfo stuff too */

	DrawIconState(drag_icon->RPort, dobj, NULL, 0, 0, IDS_NORMAL,
		ICONDRAWA_Frameless, TRUE,
		ICONDRAWA_Borderless, TRUE,
		TAG_DONE);
}

bool ami_drag_icon_move(void)
{
	if(drag_icon == NULL) return FALSE;
	struct Screen *scrn = ami_gui_get_screen();

	ChangeWindowBox(drag_icon, scrn->MouseX - (drag_icon_width / 2),
		scrn->MouseY - (drag_icon_height / 2),
		drag_icon_width, drag_icon_height);

	return ami_autoscroll;
}

/**
 * Close the drag icon (invisible) window if it is open
 *
 * \param win pointer to window to clear drag pointer
 */

void ami_drag_icon_close(struct Window *win)
{
	if(drag_icon) CloseWindow(drag_icon);
	if(win) ami_update_pointer(win, GUI_POINTER_DEFAULT);
	drag_icon = NULL;
	drag_in_progress = FALSE;
}

BOOL ami_drag_in_progress(void)
{
	return drag_in_progress;
}

bool ami_drag_has_data(void)
{
	if(drag_save_data != NULL) return true;
		else return false;
}

#else
#include <stddef.h>

#include "utils/errors.h"
#include "amiga/drag.h"

void gui_drag_save_object(struct gui_window *g, struct hlcache_handle *c,
		gui_save_type type)
{
}

void gui_drag_save_selection(struct gui_window *g, const char *selection)
{
}

void ami_drag_save(struct Window *win)
{
}

void ami_drag_icon_show(struct Window *win, const char *type)
{
}

void ami_drag_icon_close(struct Window *win)
{
}

bool ami_drag_icon_move(void)
{
	return false;
}

BOOL ami_drag_in_progress(void)
{
	return FALSE;
}

bool ami_drag_has_data(void)
{
	return false;
}
#endif

