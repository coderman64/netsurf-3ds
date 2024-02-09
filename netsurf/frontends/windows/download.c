/*
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
 * windows frontend download implementation
 *
 * \todo The windows download functionality is very buggy this needs redoing
 */

#include <limits.h>
#include "utils/inet.h" /* get correct winsock ordering */
#include <shlobj.h>
#include <windows.h>

#include "utils/sys_time.h"
#include "utils/log.h"
#include "utils/messages.h"
#include "utils/url.h"
#include "utils/nsurl.h"
#include "utils/utils.h"
#include "utils/string.h"
#include "content/fetch.h"
#include "netsurf/download.h"
#include "desktop/download.h"

#include "windows/download.h"
#include "windows/window.h"
#include "windows/gui.h"
#include "windows/resourceid.h"
#include "windows/schedule.h"

struct gui_download_window {
	HWND			hwnd;
	char			*title;
	char			*filename;
	char			*domain;
	char			*time_left;
	char			*total_size;
	char			*original_total_size;
	int			size;
	int			downloaded;
	unsigned int		progress;
	int			time_remaining;
	struct timeval		start_time;
	int			speed;
	int			error;
	struct gui_window	*window;
	FILE			*file;
	download_status		status;
};

static bool downloading = false;
static struct gui_download_window *download1;


static void nsws_download_update_label(void *p)
{
	struct gui_download_window *w = p;
	if (w->hwnd == NULL) {
		win32_schedule(-1, nsws_download_update_label, p);
		return;
	}
	HWND sub = GetDlgItem(w->hwnd, IDC_DOWNLOAD_LABEL);
	char *size = human_friendly_bytesize(w->downloaded);
	int i = 0, temp = w->time_remaining;
	if (temp == -1) {
		w->time_left = strdup(messages_get("UnknownSize"));
		i = strlen(w->time_left);
	} else {
		do {
			temp = temp / 10;
			i++;
		} while (temp > 2);
		w->time_left = malloc(i + SLEN(" s") + 1);
		if (w->time_left != NULL) {
			if (w->time_remaining > 3600)
				sprintf(w->time_left, "%d h",
					w->time_remaining / 3600);
			else if (w->time_remaining > 60)
				sprintf(w->time_left, "%d m",
					w->time_remaining / 60);
			else
				sprintf(w->time_left, "%d s",
					w->time_remaining);
		}
	}
	char label[strlen(w->title) + strlen(size) + strlen(w->total_size) +
		   + strlen(w->domain) + strlen(w->filename) +
		   SLEN("download  from  to \n[\t/\t]\n estimate of time"
			" remaining ") + i + 1];
	sprintf(label, "download %s  from %s to %s\n[%s\t/\t%s] [%d%%]\n"
		"estimate of time remaining %s", w->title, w->domain,
		w->filename, size, w->total_size, w->progress / 100,
		w->time_left);
	if (w->time_left != NULL) {
		free(w->time_left);
		w->time_left = NULL;
	}
	SendMessage(sub, WM_SETTEXT, (WPARAM)0, (LPARAM)label);
	if (w->progress < 10000) {
		win32_schedule(500, nsws_download_update_label, p);
	}
}


static void nsws_download_update_progress(void *p)
{
	struct gui_download_window *w = p;
	if (w->hwnd == NULL) {
		win32_schedule(-1, nsws_download_update_progress, p);
		return;
	}
	HWND sub = GetDlgItem(w->hwnd, IDC_DOWNLOAD_PROGRESS);
	SendMessage(sub, PBM_SETPOS, (WPARAM)(w->progress / 100), 0);
	if (w->progress < 10000) {
		win32_schedule(500, nsws_download_update_progress, p);
	}
}


static void nsws_download_clear_data(struct gui_download_window *w)
{
	if (w == NULL)
		return;
	if (w->title != NULL)
		free(w->title);
	if (w->filename != NULL)
		free(w->filename);
	if (w->domain != NULL)
		free(w->domain);
	if (w->time_left != NULL)
		free(w->time_left);
	if (w->total_size != NULL)
		free(w->total_size);
	if (w->file != NULL)
		fclose(w->file);
	win32_schedule(-1, nsws_download_update_progress, (void *)w);
	win32_schedule(-1, nsws_download_update_label, (void *)w);
}


static BOOL CALLBACK
nsws_download_event_callback(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	switch(msg) {
	case WM_INITDIALOG:
		nsws_download_update_label((void *)download1);
		nsws_download_update_progress((void *)download1);
		return TRUE;

	case WM_COMMAND:
		switch(LOWORD(wparam)) {
		case IDOK:
			if (download1->downloaded != download1->size)
				return TRUE;

		case IDCANCEL:
			nsws_download_clear_data(download1);
			download1 = NULL;
			downloading = false;
			EndDialog(hwnd, IDCANCEL);
			return FALSE;
		}
	}
	return FALSE;
}


static bool nsws_download_window_up(struct gui_download_window *w)
{
	w->hwnd = CreateDialog(hinst,
			       MAKEINTRESOURCE(IDD_DOWNLOAD),
			       gui_window_main_window(w->window),
			       nsws_download_event_callback);
	if (w->hwnd == NULL) {
		return false;
	}
	ShowWindow(w->hwnd, SW_SHOW);
	return true;
}


static struct gui_download_window *
gui_download_window_create(download_context *ctx, struct gui_window *gui)
{
	if (downloading) {
		/* initial implementation */
		win32_warning("1 download at a time please", 0);
		return NULL;
	}
	downloading = true;
	struct gui_download_window *w =
		malloc(sizeof(struct gui_download_window));
	if (w == NULL) {
		win32_warning(messages_get("NoMemory"), 0);
		return NULL;
	}
	int total_size = download_context_get_total_length(ctx);
	char *domain, *filename, *destination;
	nsurl *url = download_context_get_url(ctx);
	bool unknown_size = (total_size == 0);
	const char *size = (unknown_size) ?
		messages_get("UnknownSize") :
		human_friendly_bytesize(total_size);

	if (nsurl_nice(url, &filename, false) != NSERROR_OK) {
		filename = strdup(messages_get("UnknownFile"));
	}
	if (filename == NULL) {
		win32_warning(messages_get("NoMemory"), 0);
		free(w);
		return NULL;
	}

	if (nsurl_has_component(url, NSURL_HOST)) {
		domain = strdup(lwc_string_data(nsurl_get_component(url, NSURL_HOST)));
	} else {
		domain = strdup(messages_get("UnknownHost"));
	}
	if (domain == NULL) {
		win32_warning(messages_get("NoMemory"), 0);
		free(filename);
		free(w);
		return NULL;
	}
	destination = malloc(PATH_MAX);
	if (destination == NULL) {
		win32_warning(messages_get("NoMemory"), 0);
		free(domain);
		free(filename);
		free(w);
		return NULL;
	}
	SHGetFolderPath(NULL, CSIDL_DESKTOP, NULL, SHGFP_TYPE_CURRENT,
			destination);
	if (strlen(destination) < PATH_MAX - 2)
		strcat(destination, "/");
	if (strlen(destination) + strlen(filename) < PATH_MAX - 1)
		strcat(destination, filename);
	NSLOG(netsurf, INFO, "download %s [%s] from %s to %s", filename,
	      size, domain, destination);
	w->title = filename;
	w->domain = domain;
	w->size = total_size;
	w->total_size = strdup(size);
	if (w->total_size == NULL) {
		win32_warning(messages_get("NoMemory"), 0);
		free(destination);
		free(domain);
		free(filename);
		free(w);
		return NULL;
	}
	w->downloaded = 0;
	w->speed = 0;
	gettimeofday(&(w->start_time), NULL);
	w->time_remaining = -1;
	w->time_left = NULL;
	w->status = DOWNLOAD_NONE;
	w->filename = destination;
	w->progress = 0;
	w->error = 0;
	w->window = gui;
	w->file = fopen(destination, "wb");
	if (w->file == NULL) {
		win32_warning(messages_get("FileOpenWriteError"), destination);
		free(destination);
		free(domain);
		free(filename);
		free(w->total_size);
		free(w->time_left);
		free(w);
		return NULL;
	}
	download1 = w;

	if (nsws_download_window_up(w) == false) {
		win32_warning(messages_get("NoMemory"), 0);
		free(destination);
		free(domain);
		free(filename);
		free(w->total_size);
		free(w->time_left);
		free(w);
		return NULL;
	}
	return w;
}


static nserror
gui_download_window_data(struct gui_download_window *w, const char *data,
			 unsigned int size)
{
	if ((w == NULL) || (w->file == NULL))
		return NSERROR_SAVE_FAILED;
	size_t res;
	struct timeval val;
	res = fwrite((void *)data, 1, size, w->file);
	if (res != size)
		NSLOG(netsurf, INFO, "file write error %d of %d", size - res,
		      size);
	w->downloaded += res;
	w->progress = (unsigned int)(((long long)(w->downloaded) * 10000)
				     / w->size);
	gettimeofday(&val, NULL);
	w->time_remaining = (w->progress == 0) ? -1 :
		(int)((val.tv_sec - w->start_time.tv_sec) *
		      (10000 - w->progress) / w->progress);
	return NSERROR_OK;
}

static void gui_download_window_error(struct gui_download_window *w,
				      const char *error_msg)
{
	NSLOG(netsurf, INFO, "error %s", error_msg);
}

static void gui_download_window_done(struct gui_download_window *w)
{
	if (w == NULL)
		return;
	downloading = false;
	if (w->hwnd != NULL)
		EndDialog(w->hwnd, IDOK);
	nsws_download_clear_data(w);
}

static struct gui_download_table download_table = {
	.create = gui_download_window_create,
	.data = gui_download_window_data,
	.error = gui_download_window_error,
	.done = gui_download_window_done,
};

struct gui_download_table *win32_download_table = &download_table;
