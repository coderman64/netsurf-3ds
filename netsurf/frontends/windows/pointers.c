/*
 * Copyright 2015 Vincent Sanders <vince@netsurf-browser.org>
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

#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <windows.h>

#include "utils/errors.h"
#include "utils/nsurl.h"
#include "utils/log.h"
#include "utils/corestrings.h"
#include "utils/url.h"
#include "utils/file.h"
#include "netsurf/browser_window.h"
#include "netsurf/clipboard.h"

#include "windows/schedule.h"
#include "windows/window.h"
#include "windows/pointers.h"

struct nsws_pointers {
	HCURSOR hand;
	HCURSOR ibeam;
	HCURSOR cross;
	HCURSOR sizeall;
	HCURSOR sizewe;
	HCURSOR sizens;
	HCURSOR sizenesw;
	HCURSOR sizenwse;
	HCURSOR wait;
	HCURSOR appstarting;
	HCURSOR no;
	HCURSOR help;
	HCURSOR arrow;
};

/** pre loaded pointer cursors */
static struct nsws_pointers nsws_pointer;

/* exported interface documented in windows/pointers.h */
void nsws_window_init_pointers(HINSTANCE hinstance)
{
	nsws_pointer.hand = LoadCursor(NULL, IDC_HAND);
	nsws_pointer.ibeam = LoadCursor(NULL, IDC_IBEAM);
	nsws_pointer.cross = LoadCursor(NULL, IDC_CROSS);
	nsws_pointer.sizeall = LoadCursor(NULL, IDC_SIZEALL);
	nsws_pointer.sizewe = LoadCursor(NULL, IDC_SIZEWE);
	nsws_pointer.sizens = LoadCursor(NULL, IDC_SIZENS);
	nsws_pointer.sizenesw = LoadCursor(NULL, IDC_SIZENESW);
	nsws_pointer.sizenwse = LoadCursor(NULL, IDC_SIZENWSE);
	nsws_pointer.wait = LoadCursor(NULL, IDC_WAIT);
	nsws_pointer.appstarting = LoadCursor(NULL, IDC_APPSTARTING);
	nsws_pointer.no = LoadCursor(NULL, IDC_NO);
	nsws_pointer.help = LoadCursor(NULL, IDC_HELP);
	nsws_pointer.arrow = LoadCursor(NULL, IDC_ARROW);
}

/* exported interface documented in windows/pointers.h */
HCURSOR nsws_get_pointer(gui_pointer_shape shape)
{
	switch (shape) {
	case GUI_POINTER_POINT: /* link */
	case GUI_POINTER_MENU:
		return nsws_pointer.hand;

	case GUI_POINTER_CARET: /* input */
		return nsws_pointer.ibeam;

	case GUI_POINTER_CROSS:
		return nsws_pointer.cross;

	case GUI_POINTER_MOVE:
		return nsws_pointer.sizeall;

	case GUI_POINTER_RIGHT:
	case GUI_POINTER_LEFT:
		return nsws_pointer.sizewe;

	case GUI_POINTER_UP:
	case GUI_POINTER_DOWN:
		return nsws_pointer.sizens;

	case GUI_POINTER_RU:
	case GUI_POINTER_LD:
		return nsws_pointer.sizenesw;

	case GUI_POINTER_RD:
	case GUI_POINTER_LU:
		return nsws_pointer.sizenwse;

	case GUI_POINTER_WAIT:
		return nsws_pointer.wait;

	case GUI_POINTER_PROGRESS:
		return nsws_pointer.appstarting;

	case GUI_POINTER_NO_DROP:
	case GUI_POINTER_NOT_ALLOWED:
		return nsws_pointer.no;

	case GUI_POINTER_HELP:
		return nsws_pointer.help;

	default:
		break;
	}

	return nsws_pointer.arrow;
}
