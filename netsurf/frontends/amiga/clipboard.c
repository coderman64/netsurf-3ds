/*
 * Copyright 2008-2012 Chris Young <chris@unsatisfactorysoftware.co.uk>
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

#include <stdlib.h>
#include <string.h>
#include <proto/iffparse.h>
#include <proto/intuition.h>
#include <proto/exec.h>
#include <proto/datatypes.h>
#include <proto/diskfont.h>

#include <diskfont/diskfonttag.h>
#include <datatypes/textclass.h>
#include <datatypes/pictureclass.h>

#include "utils/nsoption.h"
#include "utils/utf8.h"
#include "utils/nsurl.h"
#include "netsurf/content.h"
#include "netsurf/browser_window.h"
#include "netsurf/plotters.h"
#include "netsurf/keypress.h"
#include "netsurf/window.h"
#include "netsurf/clipboard.h"

#include "amiga/bitmap.h"
#include "amiga/clipboard.h"
#include "amiga/drag.h"
#include "amiga/filetype.h"
#include "amiga/gui.h"
#include "amiga/iff_cset.h"
#include "amiga/iff_dr2d.h"
#include "amiga/gui_menu.h"
#include "amiga/utf8.h"

#define ID_UTF8  MAKE_ID('U','T','F','8')

static struct IFFHandle *iffh = NULL;

static struct IFFHandle *ami_clipboard_init_internal(int unit)
{
	struct IFFHandle *iffhandle = NULL;

	if((iffhandle = AllocIFF()))
	{
		if((iffhandle->iff_Stream = (ULONG)OpenClipboard(unit)))
		{
			InitIFFasClip(iffhandle);
		}
	}

	return iffhandle;
}

void ami_clipboard_init(void)
{
	iffh = ami_clipboard_init_internal(0);
}

static void ami_clipboard_free_internal(struct IFFHandle *iffhandle)
{
	if(iffhandle == NULL) return;
	if(iffhandle->iff_Stream) CloseClipboard((struct ClipboardHandle *)iffhandle->iff_Stream);
	FreeIFF(iffhandle);
}

void ami_clipboard_free(void)
{
	ami_clipboard_free_internal(iffh);
}

void gui_start_selection(struct gui_window *g)
{
	if(!g) return;
	if(!ami_gui_get_window(g)) return;
	if(nsoption_bool(kiosk_mode) == true) return;

	ami_gui_menu_set_disabled(ami_gui_get_window(g), ami_gui_get_menu(g), M_COPY, false);
	ami_gui_menu_set_disabled(ami_gui_get_window(g), ami_gui_get_menu(g), M_CLEAR, false);

	if (browser_window_get_editor_flags(ami_gui_get_browser_window(g)) & BW_EDITOR_CAN_CUT)
		ami_gui_menu_set_disabled(ami_gui_get_window(g), ami_gui_get_menu(g), M_CUT, false);
}

static char *ami_clipboard_cat_collection(struct CollectionItem *ci, LONG codeset, size_t *text_length)
{
	struct CollectionItem *ci_new = NULL, *ci_next = NULL, *ci_curr = ci;
	size_t len = 0;
	char *text = NULL, *p;

	/* Scan the collected chunks to find out the total size.
	 * If they are not in UTF-8, convert the chunks first and create a new CollectionItem list.
	 */
	do {
		switch(codeset) {
			case 106:
				len += ci_curr->ci_Size;
			break;
			
			case 0:
				if(ci_new) {
					ci_next->ci_Next = calloc(1, sizeof(struct CollectionItem));
					ci_next = ci_next->ci_Next;
				} else {
					ci_new = calloc(1, sizeof(struct CollectionItem));
					ci_next = ci_new;
				}
				
				utf8_from_local_encoding(ci_curr->ci_Data, ci_curr->ci_Size, (char **)&ci_next->ci_Data);
				ci_next->ci_Size = strlen(ci_next->ci_Data);
				len += ci_next->ci_Size;
			break;

			default:
				if(ci_new) {
					ci_next->ci_Next = calloc(1, sizeof(struct CollectionItem));
					ci_next = ci_next->ci_Next;
				} else {
					ci_new = calloc(1, sizeof(struct CollectionItem));
					ci_next = ci_new;
				}
				
				utf8_from_enc(ci_curr->ci_Data,
						(const char *)ObtainCharsetInfo(DFCS_NUMBER,
										codeset, DFCS_MIMENAME),
					      ci_curr->ci_Size, (char **)&ci_next->ci_Data, NULL);
				ci_next->ci_Size = strlen(ci_next->ci_Data);
				len += ci_next->ci_Size;
			break;
		}
	} while ((ci_curr = ci_curr->ci_Next));

	text = malloc(len);

	if(text == NULL) return NULL;

	/* p points to the end of the buffer. This is because the chunks are
	 * in the list in reverse order. */
	p = text + len;

	if(ci_new) {
		ci_curr = ci_new;
	} else {
		ci_curr = ci;
	}

	do {
		p -= ci_curr->ci_Size;
		memcpy(p, ci_curr->ci_Data, ci_curr->ci_Size);
		ci_next = ci_curr->ci_Next;
		
		if(ci_new) {
			free(ci_curr->ci_Data);
			free(ci_curr);
		}
	} while ((ci_curr = ci_next));

	*text_length = len;
	return text;
}

static void gui_get_clipboard(char **buffer, size_t *length)
{
	struct CollectionItem *ci = NULL;
	struct StoredProperty *sp = NULL;
	struct CSet *cset;

	if(OpenIFF(iffh,IFFF_READ)) return;
	
	if(CollectionChunk(iffh,ID_FTXT,ID_CHRS)) return;
	if(PropChunk(iffh,ID_FTXT,ID_CSET)) return;
	if(CollectionChunk(iffh,ID_FTXT,ID_UTF8)) return;
	if(StopOnExit(iffh, ID_FTXT, ID_FORM)) return;
	
	ParseIFF(iffh,IFFPARSE_SCAN);

	if((ci = FindCollection(iffh, ID_FTXT, ID_UTF8))) {
		*buffer = ami_clipboard_cat_collection(ci, 106, length);
	} else if((ci = FindCollection(iffh, ID_FTXT, ID_CHRS))) {
		LONG codeset = 0;
		if((sp = FindProp(iffh, ID_FTXT, ID_CSET))) {
			cset = (struct CSet *)sp->sp_Data;
			codeset = cset->CodeSet;
		}
		*buffer = ami_clipboard_cat_collection(ci, codeset, length);
	}

	CloseIFF(iffh);
}

static void gui_set_clipboard(const char *buffer, size_t length,
	nsclipboard_styles styles[], int n_styles)
{
	char *text;
	struct CSet cset = {0, {0}};

	if(buffer == NULL) return;

	if(!(OpenIFF(iffh, IFFF_WRITE)))
	{
		if(!(PushChunk(iffh, ID_FTXT, ID_FORM, IFFSIZE_UNKNOWN)))
		{
			if(nsoption_bool(clipboard_write_utf8))
			{
				if(!(PushChunk(iffh, 0, ID_CSET, 32)))
				{
					cset.CodeSet = 106; // UTF-8
					WriteChunkBytes(iffh, &cset, 32);
					PopChunk(iffh);
				}
			}
		}
		else
		{
			PopChunk(iffh);
		}

		if(!(PushChunk(iffh, 0, ID_CHRS, IFFSIZE_UNKNOWN))) {
			if(nsoption_bool(clipboard_write_utf8)) {
				WriteChunkBytes(iffh, buffer, length);
			} else {
				if(utf8_to_local_encoding(buffer, length, &text) == NSERROR_OK) {
					char *p;

					p = text;

					while(*p != '\0') {
						if(*p == 0xa0) *p = 0x20;
						p++;
					}
					WriteChunkBytes(iffh, text, strlen(text));
					ami_utf8_free(text);
				}
			}

			PopChunk(iffh);
		} else {
			PopChunk(iffh);
		}

		if(!(PushChunk(iffh, 0, ID_UTF8, IFFSIZE_UNKNOWN))) {
			WriteChunkBytes(iffh, buffer, length);
			PopChunk(iffh);
		} else {
			PopChunk(iffh);
		}
		CloseIFF(iffh);
	}
}

void ami_drag_selection(struct gui_window *g)
{
	int x;
	int y;
	char *utf8text;
	char *sel;
	struct IFFHandle *old_iffh = iffh;
	struct gui_window_2 *gwin = ami_window_at_pointer(AMINS_WINDOW);
	
	/* NB: 'gwin' is at the drop point, 'g' is where the selection was dragged from.
	 * These may be different if the selection has been dragged between windows. */
	
	if(!gwin)
	{
		ami_gui_beep();
		return;
	}

	x = ami_gui2_get_window(gwin)->MouseX;
	y = ami_gui2_get_window(gwin)->MouseY;

	if(ami_text_box_at_point(gwin, (ULONG *)&x, (ULONG *)&y))
	{
		iffh = ami_clipboard_init_internal(1);

		browser_window_key_press(ami_gui_get_browser_window(g), NS_KEY_COPY_SELECTION);
		browser_window_mouse_click(ami_gui2_get_browser_window(gwin), BROWSER_MOUSE_PRESS_1, x, y);
		browser_window_key_press(ami_gui2_get_browser_window(gwin), NS_KEY_PASTE);

		ami_clipboard_free_internal(iffh);
		iffh = old_iffh;
	}
	else
	{
		x = ami_gui2_get_window(gwin)->MouseX;
		y = ami_gui2_get_window(gwin)->MouseY;

		if(ami_gadget_hit(ami_gui2_get_object(gwin, AMI_GAD_URL), x, y))
		{
			if((sel = browser_window_get_selection(ami_gui_get_browser_window(g))))
			{
				utf8text = ami_utf8_easy(sel);
				RefreshSetGadgetAttrs((struct Gadget *)ami_gui2_get_object(gwin, AMI_GAD_URL),
					ami_gui2_get_window(gwin), NULL, STRINGA_TextVal, utf8text, TAG_DONE);
				free(sel);
				ami_utf8_free(utf8text);
			}
		}
		else if(ami_gadget_hit(ami_gui2_get_object(gwin, AMI_GAD_SEARCH), x, y))
		{
			if((sel = browser_window_get_selection(ami_gui_get_browser_window(g))))
			{
				utf8text = ami_utf8_easy(sel);
				RefreshSetGadgetAttrs((struct Gadget *)ami_gui2_get_object(gwin, AMI_GAD_SEARCH),
					ami_gui2_get_window(gwin), NULL, STRINGA_TextVal, utf8text, TAG_DONE);
				free(sel);
				ami_utf8_free(utf8text);
			}
		}
		else
		{
			ami_gui_beep();
		}
	}
}

bool ami_easy_clipboard(const char *text)
{
	gui_set_clipboard(text, strlen(text), NULL, 0);
	return true;
}

bool ami_easy_clipboard_bitmap(struct bitmap *bitmap)
{
	Object *dto = NULL;

	if((dto = ami_datatype_object_from_bitmap(bitmap)))
	{
		DoDTMethod(dto,NULL,NULL,DTM_COPY,NULL);
		DisposeDTObject(dto);
	}
	return true;
}

#ifdef WITH_NS_SVG
bool ami_easy_clipboard_svg(struct hlcache_handle *c)
{
	const uint8_t *source_data;
	size_t source_size;

	if (ami_mime_compare(c, "svg") == false) {
		return false;
	}
	source_data = content_get_source_data(c, &source_size);
	if (source_data == NULL) {
		return false;
	}

	if (!(OpenIFF(iffh,IFFF_WRITE))) {
		ami_svg_to_dr2d(iffh,
				(const char *)source_data,
				source_size,
				nsurl_access(hlcache_handle_get_url(c)));
		CloseIFF(iffh);
	}

	return true;
}
#endif

static struct gui_clipboard_table clipboard_table = {
	.get = gui_get_clipboard,
	.set = gui_set_clipboard,
};

struct gui_clipboard_table *amiga_clipboard_table = &clipboard_table;
