/*
 * Copyright 2019 Vincent Sanders <vince@netsurf-browser.org>
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
 * win32 clipboard implementation.
 */

#include <windows.h>

#include "utils/log.h"
#include "netsurf/clipboard.h"

#include "windows/clipboard.h"

/**
 * Core asks front end for clipboard contents.
 *
 * \param buffer UTF-8 text, allocated by front end, ownership yeilded to core
 * \param length Byte length of UTF-8 text in buffer
 */
static void gui_get_clipboard(char **buffer, size_t *length)
{
	HANDLE clipboard_handle;
	wchar_t *content;

	if (OpenClipboard(NULL)) {
		clipboard_handle = GetClipboardData(CF_UNICODETEXT);
		if (clipboard_handle != NULL) {
			content = GlobalLock(clipboard_handle);
			if (content != NULL) {
				int required_len;
				size_t content_len;

				content_len = wcslen(content);

				/* compute length */
				required_len = WideCharToMultiByte(
							CP_UTF8,
							WC_NO_BEST_FIT_CHARS,
							content,
							content_len,
							NULL,
							0,
							NULL,
							NULL);
				/* allocate buffer and do conversion */
				*buffer = malloc(required_len);
				*length = WideCharToMultiByte(
							CP_UTF8,
							WC_NO_BEST_FIT_CHARS,
							content,
							content_len,
							*buffer,
							required_len,
							NULL,
							NULL);

				GlobalUnlock(clipboard_handle);
			}
		}
		CloseClipboard();
	}
}


/**
 * Core tells front end to put given text in clipboard
 *
 * \param buffer UTF-8 text, owned by core
 * \param length Byte length of UTF-8 text in buffer
 * \param styles Array of styles given to text runs, owned by core, or NULL
 * \param n_styles Number of text run styles in array
 */
static void
gui_set_clipboard(const char *buffer,
		  size_t length,
		  nsclipboard_styles styles[],
		  int n_styles)
{
	HGLOBAL hglbCopy;
	wchar_t *content; /* clipboard content */
	int content_len; /* characters in content */

	if (OpenClipboard(NULL)) {
		EmptyClipboard();
		content_len = MultiByteToWideChar(CP_UTF8,
						  MB_PRECOMPOSED,
						  buffer, length,
						  NULL, 0);

		hglbCopy = GlobalAlloc(GMEM_MOVEABLE,
				       ((content_len + 1) * sizeof(wchar_t)));
		if (hglbCopy != NULL) {
			content = GlobalLock(hglbCopy);
			MultiByteToWideChar(CP_UTF8,
					    MB_PRECOMPOSED,
					    buffer, length,
					    content, content_len);
			content[content_len] = 0; /* null terminate */

			GlobalUnlock(hglbCopy);
			SetClipboardData(CF_UNICODETEXT, hglbCopy);
		}
		CloseClipboard();
	}
}



static struct gui_clipboard_table clipboard_table = {
	.get = gui_get_clipboard,
	.set = gui_set_clipboard,
};

struct gui_clipboard_table *win32_clipboard_table = &clipboard_table;
