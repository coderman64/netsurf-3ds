/*
 * Copyright 2008 Vincent Sanders <vince@simtec.co.uk>
 * Copyright 2009 Mark Benjamin <netsurf-browser.org.MarkBenjamin@dfgh.net>
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
 * win32 gui implementation.
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
#include "utils/messages.h"
#include "netsurf/browser_window.h"

#include "windows/schedule.h"
#include "windows/window.h"
#include "windows/gui.h"

/* exported global defined in windows/gui.h */
HINSTANCE hinst;

/* exported global defined in windows/gui.h */
char **G_resource_pathv;

/* exported global defined in windows/gui.h */
char *G_config_path;

static bool win32_quit = false;

struct dialog_list_entry {
	struct dialog_list_entry *next;
	HWND hwnd;
};

static struct dialog_list_entry *dlglist = NULL;

/* exported interface documented in gui.h */
nserror nsw32_add_dialog(HWND hwndDlg)
{
	struct dialog_list_entry *nentry;
	nentry = malloc(sizeof(struct dialog_list_entry));
	if (nentry == NULL) {
		return NSERROR_NOMEM;
	}

	nentry->hwnd = hwndDlg;
	nentry->next = dlglist;
	dlglist = nentry;

	return NSERROR_OK;
}

/* exported interface documented in gui.h */
nserror nsw32_del_dialog(HWND hwndDlg)
{
	struct dialog_list_entry **prev;
	struct dialog_list_entry *cur;

	prev = &dlglist;
	cur = *prev;

	while (cur != NULL) {
		if (cur->hwnd == hwndDlg) {
			/* found match */
			*prev = cur->next;
			NSLOG(netsurf, DEBUG,
			      "removed hwnd %p entry %p", cur->hwnd, cur);
			free(cur);
			return NSERROR_OK;
		}
		prev = &cur->next;
		cur = *prev;
	}
	NSLOG(netsurf, INFO, "did not find hwnd %p", hwndDlg);

	return NSERROR_NOT_FOUND;
}

/**
 * walks dialog list and attempts to process any messages for them
 */
static nserror handle_dialog_message(LPMSG lpMsg)
{
	struct dialog_list_entry *cur;
	cur = dlglist;
	while (cur != NULL) {
		if (IsDialogMessage(cur->hwnd, lpMsg)) {
			NSLOG(netsurf, DEBUG,
			      "dispatched dialog hwnd %p", cur->hwnd);
			return NSERROR_OK;
		}
		cur = cur->next;
	}

	return NSERROR_NOT_FOUND;
}

/* exported interface documented in gui.h */
void win32_set_quit(bool q)
{
	win32_quit = q;
}

/* exported interface documented in gui.h */
void win32_run(void)
{
	MSG Msg; /* message from system */
	BOOL bRet; /* message fetch result */
	int timeout; /* timeout in miliseconds */
	UINT timer_id = 0;

	NSLOG(netsurf, INFO, "Starting messgae dispatcher");

	while (!win32_quit) {
		/* run the scheduler and discover how long to wait for
		 * the next event.
		 */
		timeout = schedule_run();

		if (timeout == 0) {
			bRet = PeekMessage(&Msg, NULL, 0, 0, PM_REMOVE);
		} else {
			if (timeout > 0) {
				/* set up a timer to ensure we get woken */
				timer_id = SetTimer(NULL, 0, timeout, NULL);
			}

			/* wait for a message */
			bRet = GetMessage(&Msg, NULL, 0, 0);

			/* if a timer was sucessfully created remove it */
			if (timer_id != 0) {
				KillTimer(NULL, timer_id);
				timer_id = 0;
			}
		}

		if ((bRet > 0) &&
		    (handle_dialog_message(&Msg) != NSERROR_OK)) {
			TranslateMessage(&Msg);
			DispatchMessage(&Msg);
		}
	}
}


/* exported function documented in windows/gui.h */
nserror win32_warning(const char *warning, const char *detail)
{
	size_t len = 1 + ((warning != NULL) ? strlen(messages_get(warning)) :
			  0) + ((detail != 0) ? strlen(detail) : 0);
	char message[len];
	snprintf(message, len, messages_get(warning), detail);
	MessageBox(NULL, message, "Warning", MB_ICONWARNING);

	return NSERROR_OK;
}


/* exported function documented in windows/gui.h */
nserror
win32_report_nserror(nserror error, const char *detail)
{
	size_t len = 1 +
		strlen(messages_get_errorcode(error)) +
		((detail != 0) ? strlen(detail) : 0);
	char message[len];
	snprintf(message, len, messages_get_errorcode(error), detail);
	MessageBox(NULL, message, "Warning", MB_ICONWARNING);

	return NSERROR_OK;
}

