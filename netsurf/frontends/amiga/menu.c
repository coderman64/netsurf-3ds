/*
 * Copyright 2008-9, 2013, 2017 Chris Young <chris@unsatisfactorysoftware.co.uk>
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

#include "amiga/os3support.h"

#include <string.h>
#include <stdlib.h>

#include <proto/gadtools.h>
#include <proto/graphics.h>
#include <proto/intuition.h>

#include <libraries/gadtools.h>
#ifdef __amigaos4__
#include <intuition/menuclass.h>
#endif

#include <classes/window.h>
#include <proto/label.h>
#include <images/label.h>
#include <proto/bitmap.h>
#include <images/bitmap.h>

#include <reaction/reaction_macros.h>

#include "utils/log.h"
#include "utils/messages.h"

#include "amiga/gui.h"
#include "amiga/libs.h"
#include "amiga/menu.h"
#include "amiga/utf8.h"

enum {
	NSA_GLYPH_SUBMENU,
	NSA_GLYPH_AMIGAKEY,
	NSA_GLYPH_CHECKMARK,
	NSA_GLYPH_MX,
	NSA_GLYPH_MAX
};

#define NSA_MAX_HOTLIST_MENU_LEN 100

static Object *restrict menu_glyph[NSA_GLYPH_MAX];
static int menu_glyph_width[NSA_GLYPH_MAX];
static bool menu_glyphs_loaded = false;

bool ami_menu_get_selected(struct Menu *menu, struct IntuiMessage *msg)
{
	bool checked = false;

	if(LIB_IS_AT_LEAST((struct Library *)IntuitionBase, 54, 6)) {
#ifdef __amigaos4__
		ULONG state;
		struct ExtIntuiMessage *emsg = (struct ExtIntuiMessage *)msg;

		state = IDoMethod((Object *)menu, MM_GETSTATE, 0, emsg->eim_LongCode, MS_CHECKED);
		if(state & MS_CHECKED) checked = true;
#endif	
	} else {
		if(ItemAddress(menu, msg->Code)->Flags & CHECKED) checked = true;
	}

	return checked;
}

/* menu creation code */
void ami_menu_free_lab_item(struct ami_menu_data **md, int i)
{
	if(md[i] == NULL) return;
	if(md[i]->menulab &&
			(md[i]->menulab != NM_BARLABEL) &&
			(md[i]->menulab != ML_SEPARATOR)) {
		if(md[i]->menutype & MENU_IMAGE) {
			if(md[i]->menuobj) DisposeObject(md[i]->menuobj);
		}

		ami_utf8_free(md[i]->menulab);
	}

	if(md[i]->menukey != NULL) free(md[i]->menukey);

	md[i]->menulab = NULL;
	md[i]->menuobj = NULL;
	md[i]->menukey = NULL;
	md[i]->menutype = 0;
	free(md[i]);
	md[i] = NULL;
}

static void ami_menu_free_labs(struct ami_menu_data **md, int max)
{
	int i;

	for(i = 0; i <= max; i++) {
		ami_menu_free_lab_item(md, i);
	}
}

void ami_menu_alloc_item(struct ami_menu_data **md, int num, UBYTE type,
			const char *restrict label, const char *restrict key, const char *restrict icon,
			void *restrict func, void *restrict hookdata, UWORD flags)
{
	md[num] = calloc(1, sizeof(struct ami_menu_data));
	md[num]->menutype = type;
	md[num]->flags = flags;
	
	if(type == NM_END) return;

	if((label == NM_BARLABEL) || (strcmp(label, "--") == 0)) {
		md[num]->menulab = NM_BARLABEL;
		icon = NULL;
	} else { /* horrid non-generic stuff */
		if((num >= AMI_MENU_AREXX) && (num < AMI_MENU_AREXX_MAX)) {
			md[num]->menulab = strdup(label);		
		} else {
			md[num]->menulab = ami_utf8_easy(messages_get(label));
		}
	}

	md[num]->menuicon = NULL;
	if(key) md[num]->menukey = strdup(key);
	if(func) md[num]->menu_hook.h_Entry = (HOOKFUNC)func;
	if(hookdata) md[num]->menu_hook.h_Data = hookdata;

#ifdef __amigaos4__
	char menu_icon[1024];

	if(LIB_IS_AT_LEAST((struct Library *)GadToolsBase, 53, 7)) {
		if(icon) {
			if(ami_locate_resource(menu_icon, icon) == true) {
				md[num]->menuicon = (char *)strdup(menu_icon);
			} else {
				/* If the requested icon can't be found, put blank space in instead */
				md[num]->menuicon = (char *)strdup(NSA_SPACE);
			}
		}
	}
#endif
}

static void ami_menu_load_glyphs(struct DrawInfo *dri)
{
#ifdef __amigaos4__
	if(LIB_IS_AT_LEAST((struct Library *)GadToolsBase, 53, 7)) {
		for(int i = 0; i < NSA_GLYPH_MAX; i++)
			menu_glyph[i] = NULL;

		menu_glyph[NSA_GLYPH_SUBMENU] = NewObject(NULL, "sysiclass",
											SYSIA_Which, MENUSUB,
											SYSIA_DrawInfo, dri,
										TAG_DONE);
		menu_glyph[NSA_GLYPH_AMIGAKEY] = NewObject(NULL, "sysiclass",
											SYSIA_Which, AMIGAKEY,
											SYSIA_DrawInfo, dri,
										TAG_DONE);
		GetAttr(IA_Width, menu_glyph[NSA_GLYPH_SUBMENU],
			(ULONG *)&menu_glyph_width[NSA_GLYPH_SUBMENU]);
		GetAttr(IA_Width, menu_glyph[NSA_GLYPH_AMIGAKEY],
			(ULONG *)&menu_glyph_width[NSA_GLYPH_AMIGAKEY]);
	
		menu_glyphs_loaded = true;
	}
#endif
}

void ami_menu_free_glyphs(void)
{
#ifdef __amigaos4__
	if(LIB_IS_AT_LEAST((struct Library *)GadToolsBase, 53, 7)) {
		int i;
		if(menu_glyphs_loaded == false) return;

		for(i = 0; i < NSA_GLYPH_MAX; i++) {
			if(menu_glyph[i]) DisposeObject(menu_glyph[i]);
			menu_glyph[i] = NULL;
		};
	
		menu_glyphs_loaded = false;
	}
#endif
}

static int ami_menu_calc_item_width(struct ami_menu_data **md, int j, struct RastPort *rp)
{
	int space_width = TextLength(rp, " ", 1);
	int item_size;

	item_size = TextLength(rp, md[j]->menulab, strlen(md[j]->menulab));
	item_size += space_width;

	if(md[j]->menukey) {
		item_size += TextLength(rp, md[j]->menukey, 1);
		item_size += menu_glyph_width[NSA_GLYPH_AMIGAKEY];
		/**TODO: take account of the size of other imagery too
		 */
	} else {
		/* assume worst case - it doesn't really matter if we make menus wider */
		item_size += TextLength(rp, "M", 1);
		item_size += menu_glyph_width[NSA_GLYPH_AMIGAKEY];
	}

	if(md[j]->menuicon) {
		item_size += 16;
	}

	return item_size;
}

#ifdef __amigaos4__
static int ami_menu_layout_mc_recursive(Object *menu_parent, struct ami_menu_data **md, int level, int i, int max)
{
	int j;
	Object *menu_item = menu_parent;
	
	for(j = i; j < max; j++) {
		/* skip empty entries */
		if(md[j] == NULL) continue;
		if(md[j]->menutype == NM_IGNORE) continue;

		if(md[j]->menutype == level) {
			if(md[j]->menulab == NM_BARLABEL)
				md[j]->menulab = ML_SEPARATOR;

			if(level == NM_TITLE) {
				menu_item = NewObject(NULL, "menuclass",
					MA_Type, T_MENU,
					MA_ID, j,
					MA_Label, md[j]->menulab,
					TAG_DONE);
			} else {
				menu_item = NewObject(NULL, "menuclass",
					MA_Type, T_ITEM,
					MA_ID, j,
					MA_Label, md[j]->menulab,
					MA_Image,
						BitMapObj,
							IA_Scalable, TRUE,
							BITMAP_Screen, ami_gui_get_screen(),
							BITMAP_SourceFile, md[j]->menuicon,
							BITMAP_Masking, TRUE,
						BitMapEnd,
					MA_Key, md[j]->menukey,
					MA_UserData, &md[j]->menu_hook, /* NB: Intentionally UserData */
					MA_Disabled, (md[j]->flags & NM_ITEMDISABLED),
					MA_Selected, (md[j]->flags & CHECKED),
					MA_Toggle, (md[j]->flags & MENUTOGGLE),
					TAG_DONE);
			}

			NSLOG(netsurf, DEEPDEBUG,
			      "Adding item %p ID %d (%s) to parent %p",
			      menu_item, j, md[j]->menulab, menu_parent);
			IDoMethod(menu_parent, OM_ADDMEMBER, menu_item);
			continue;
		} else if (md[j]->menutype > level) {
			j = ami_menu_layout_mc_recursive(menu_item, md, md[j]->menutype, j, max);
		} else {
			break;
		}
	}
	return (j - 1);
}

static struct Menu *ami_menu_layout_mc(struct ami_menu_data **md, int max)
{
	Object *menu_root = NewObject(NULL, "menuclass",
		MA_Type, T_ROOT,
		MA_EmbeddedKey, FALSE,
		TAG_DONE);

	ami_menu_layout_mc_recursive(menu_root, md, NM_TITLE, 0, max);

	return (struct Menu *)menu_root;
}
#endif

static struct Menu *ami_menu_layout_gt(struct ami_menu_data **md, int max)
{
	int i, j;
	int txtlen = 0;
	int left_posn = 0;
	struct NewMenu *nm;
	struct Menu *imenu = NULL;
	struct VisualInfo *vi;
	struct Screen *scrn = ami_gui_get_screen();
	struct RastPort *rp = &scrn->RastPort;
	struct DrawInfo *dri = GetScreenDrawInfo(scrn);
	int space_width = TextLength(rp, " ", 1);

	if(menu_glyphs_loaded == false)
		ami_menu_load_glyphs(dri);

	nm = calloc(1, sizeof(struct NewMenu) * (max + 1));
	if(nm == NULL) return NULL;

	for(i = 0; i < max; i++) {
		if(md[i] == NULL) {
			nm[i].nm_Type = NM_IGNORE;
			continue;
		}

		if(md[i]->menutype == NM_TITLE) {
			j = i + 1;
			txtlen = 0;
			do {
				if(md[j]->menulab != NM_BARLABEL) {
					if(md[j]->menutype == NM_ITEM) {
						int item_size = ami_menu_calc_item_width(md, j, rp);
						if(item_size > txtlen) {
							txtlen = item_size;
						}
					}
				}
				j++;
			} while((j <= max) && (md[j] != NULL) && (md[j]->menutype != NM_TITLE) && (md[j]->menutype != 0));
		}
#ifdef __amigaos4__
		if(LIB_IS_AT_LEAST((struct Library *)GadToolsBase, 53, 7)) {
			/* GadTools 53.7+ only. For now we will only create the menu
				using label.image if there's a bitmap associated with the item. */
			if((md[i]->menuicon != NULL) && (md[i]->menulab != NM_BARLABEL)) {
				int icon_width = 0;
				Object *restrict submenuarrow = NULL;
				Object *restrict icon = BitMapObj,
						IA_Scalable, TRUE,
						BITMAP_Screen, scrn,
						BITMAP_SourceFile, md[i]->menuicon,
						BITMAP_Masking, TRUE,
					BitMapEnd;

				/* \todo make this scale the bitmap to these dimensions */
				SetAttrs(icon,
					BITMAP_Width, 16,
					BITMAP_Height, 16,
					TAG_DONE);

				GetAttr(IA_Width, icon, (ULONG *)&icon_width);

				if(md[i]->menutype != NM_SUB) {
					left_posn = txtlen;
				}

				left_posn = left_posn -
					TextLength(rp, md[i]->menulab, strlen(md[i]->menulab)) -
					icon_width - space_width;

				if((md[i]->menutype == NM_ITEM) && md[i+1] && (md[i+1]->menutype == NM_SUB)) {
					left_posn -= menu_glyph_width[NSA_GLYPH_SUBMENU];

					submenuarrow = NewObject(NULL, "sysiclass",
									SYSIA_Which, MENUSUB,
									SYSIA_DrawInfo, dri,
									IA_Left, left_posn,
									TAG_DONE);
				}

				md[i]->menuobj = LabelObj,
					LABEL_MenuMode, TRUE,
					LABEL_DrawInfo, dri,
					LABEL_DisposeImage, TRUE,
					LABEL_Image, icon,
					LABEL_Text, " ",
					LABEL_Text, md[i]->menulab,
					LABEL_DisposeImage, TRUE,
					LABEL_Image, submenuarrow,
				LabelEnd;

				if(md[i]->menuobj) md[i]->menutype |= MENU_IMAGE;
			}
		}
#endif
		nm[i].nm_Type = md[i]->menutype;
		
		if(md[i]->menuobj)
			nm[i].nm_Label = (void *)md[i]->menuobj;
		else
			nm[i].nm_Label = md[i]->menulab;

		if((md[i]->menukey) && (strlen(md[i]->menukey) == 1)) {
			nm[i].nm_CommKey = md[i]->menukey;
		}
		nm[i].nm_Flags = md[i]->flags;
		if(md[i]->menu_hook.h_Entry) nm[i].nm_UserData = &md[i]->menu_hook;

		if(md[i]->menuicon) {
			free(md[i]->menuicon);
			md[i]->menuicon = NULL;
		}
	}
	
	FreeScreenDrawInfo(scrn, dri);

	vi = GetVisualInfo(scrn, TAG_DONE);
	imenu = CreateMenus(nm, TAG_DONE);
	LayoutMenus(imenu, vi,
		GTMN_NewLookMenus, TRUE, TAG_DONE);
	free(nm);
	FreeVisualInfo(vi); /* Not using GadTools after layout so shouldn't need this */
	
	return imenu;
}

struct Menu *ami_menu_layout(struct ami_menu_data **md, int max)
{
	if(LIB_IS_AT_LEAST((struct Library *)IntuitionBase, 54, 6)) {
#ifdef __amigaos4__
		return ami_menu_layout_mc(md, max);
#endif	
	} else {
		return ami_menu_layout_gt(md, max);
	}
}

void ami_menu_free_menu(struct ami_menu_data **md, int max, struct Menu *imenu)
{
	ami_menu_free_labs(md, max);
	if(LIB_IS_AT_LEAST((struct Library *)IntuitionBase, 54, 6)) {
		DisposeObject((Object *)imenu); // if we detach our menu from the window we need to do this manually
	} else {
		FreeMenus(imenu);
	}
}

void ami_menu_refresh(struct Menu *menu, struct ami_menu_data **md, int menu_item, int max,
	nserror (*cb)(struct ami_menu_data **md))
{
#ifdef __amigaos4__
	Object *restrict obj;
	Object *restrict menu_item_obj;
	int i;

	if(menu == NULL) return;

	if(LIB_IS_AT_LEAST((struct Library *)IntuitionBase, 54, 6)) {
		/* find the address of the menu */
		menu_item_obj = (Object *)IDoMethod((Object *)menu, MM_FINDID, 0, menu_item);

		/* remove all children */
		while((obj = (Object *)IDoMethod(menu_item_obj, MM_NEXTCHILD, 0, NULL)) != NULL) {
			IDoMethod(menu_item_obj, OM_REMMEMBER, obj);
			DisposeObject(obj);
		}

		/* free associated data */
		for(i = (menu_item + 1); i <= max; i++) {
			if(md[i] == NULL) continue;
			ami_menu_free_lab_item(md, i);
		}

		/* get current data */
		cb(md);

		/* re-add items to menu */
		ami_menu_layout_mc_recursive(menu_item_obj, md, NM_ITEM, (menu_item + 1), max);
	}
#endif
}

