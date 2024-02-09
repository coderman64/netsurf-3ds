/*
 * Copyright 2008-2020 Chris Young <chris@unsatisfactorysoftware.co.uk>
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
/* Custom StringView class */
#include "amiga/stringview/stringview.h"
#include "amiga/stringview/urlhistory.h"
#endif

/* AmigaOS libraries */
#ifdef __amigaos4__
#include <proto/application.h>
#endif
#include <proto/asl.h>
#include <proto/datatypes.h>
#include <proto/diskfont.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/icon.h>
#include <proto/intuition.h>
#include <proto/keymap.h>
#include <proto/layers.h>
#include <proto/locale.h>
#include <proto/utility.h>
#include <proto/wb.h>

#ifdef WITH_AMISSL
/* AmiSSL needs everything to use bsdsocket.library directly to avoid problems */
#include <proto/bsdsocket.h>
#define waitselect WaitSelect
#endif

/* Other OS includes */
#include <datatypes/textclass.h>
#include <devices/inputevent.h>
#include <graphics/gfxbase.h>
#include <graphics/rpattr.h>
#ifdef __amigaos4__
#include <diskfont/diskfonttag.h>
#include <graphics/blitattr.h>
#include <intuition/gui.h>
#include <libraries/application.h>
#include <libraries/keymap.h>
#endif
#include <intuition/icclass.h>
#include <intuition/screens.h>
#include <libraries/gadtools.h>
#include <workbench/workbench.h>

/* ReAction libraries */
#include <proto/bevel.h>
#include <proto/bitmap.h>
#include <proto/button.h>
#include <proto/chooser.h>
#include <proto/clicktab.h>
#include <proto/label.h>
#include <proto/layout.h>
#include <proto/listbrowser.h>
#include <proto/scroller.h>
#include <proto/space.h>
#include <proto/speedbar.h>
#include <proto/string.h>
#include <proto/window.h>

#include <classes/window.h>
#include <gadgets/button.h>
#include <gadgets/chooser.h>
#include <gadgets/clicktab.h>
#include <gadgets/layout.h>
#include <gadgets/listbrowser.h>
#include <gadgets/scroller.h>
#include <gadgets/space.h>
#include <gadgets/speedbar.h>
#include <gadgets/string.h>
#include <images/bevel.h>
#include <images/bitmap.h>
#include <images/label.h>

#include <reaction/reaction_macros.h>

/* newlib includes */
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>

/* NetSurf core includes */
#include "utils/log.h"
#include "utils/messages.h"
#include "utils/nsoption.h"
#include "utils/utf8.h"
#include "utils/utils.h"
#include "utils/nsurl.h"
#include "utils/file.h"
#include "netsurf/window.h"
#include "netsurf/fetch.h"
#include "netsurf/misc.h"
#include "netsurf/mouse.h"
#include "netsurf/netsurf.h"
#include "netsurf/content.h"
#include "netsurf/browser_window.h"
#include "netsurf/cookie_db.h"
#include "netsurf/url_db.h"
#include "netsurf/keypress.h"
#include "content/backing_store.h"
#include "content/fetch.h"
#include "desktop/browser_history.h"
#include "desktop/hotlist.h"
#include "desktop/version.h"
#include "desktop/save_complete.h"
#include "desktop/searchweb.h"

/* NetSurf Amiga platform includes */
#include "amiga/gui.h"
#include "amiga/arexx.h"
#include "amiga/bitmap.h"
#include "amiga/clipboard.h"
#include "amiga/cookies.h"
#include "amiga/ctxmenu.h"
#include "amiga/datatypes.h"
#include "amiga/download.h"
#include "amiga/drag.h"
#include "amiga/file.h"
#include "amiga/filetype.h"
#include "amiga/font.h"
#include "amiga/gui_options.h"
#include "amiga/help.h"
#include "amiga/history.h"
#include "amiga/history_local.h"
#include "amiga/hotlist.h"
#include "amiga/icon.h"
#include "amiga/launch.h"
#include "amiga/libs.h"
#include "amiga/memory.h"
#include "amiga/menu.h"
#include "amiga/misc.h"
#include "amiga/nsoption.h"
#include "amiga/pageinfo.h"
#include "amiga/plotters.h"
#include "amiga/plugin_hack.h"
#include "amiga/print.h"
#include "amiga/schedule.h"
#include "amiga/search.h"
#include "amiga/selectmenu.h"
#include "amiga/theme.h"
#include "amiga/utf8.h"

#define AMINS_SCROLLERPEN NUMDRIPENS
#define NSA_KBD_SCROLL_PX 10
#define NSA_MAX_HOTLIST_BUTTON_LEN 20

#define SCROLL_TOP INT_MIN
#define SCROLL_PAGE_UP (INT_MIN + 1)
#define SCROLL_PAGE_DOWN (INT_MAX - 1)
#define SCROLL_BOTTOM (INT_MAX)

/* Extra mouse button defines to match those in intuition/intuition.h */
#define SIDEDOWN  (IECODE_4TH_BUTTON)
#define SIDEUP    (IECODE_4TH_BUTTON | IECODE_UP_PREFIX)
#define EXTRADOWN (IECODE_5TH_BUTTON)
#define EXTRAUP   (IECODE_5TH_BUTTON | IECODE_UP_PREFIX)

/* Left OR Right Shift/Alt keys */
#define NSA_QUAL_SHIFT (IEQUALIFIER_RSHIFT | IEQUALIFIER_LSHIFT)
#define NSA_QUAL_ALT (IEQUALIFIER_RALT | IEQUALIFIER_LALT)

#ifdef __amigaos4__
#define NSA_STATUS_TEXT GA_Text
#else
#define NSA_STATUS_TEXT STRINGA_TextVal
#endif

#ifdef __amigaos4__
#define BOOL_MISMATCH(a,b) ((a == FALSE) && (b != FALSE)) || ((a != FALSE) && (b == FALSE))
#else
#define BOOL_MISMATCH(a,b) (1)
#endif

extern struct gui_utf8_table *amiga_utf8_table;

enum
{
    OID_MAIN = 0,
	OID_VSCROLL,
	OID_HSCROLL,
	GID_MAIN,
	GID_TABLAYOUT,
	GID_BROWSER,
	GID_STATUS,
	GID_URL,
	GID_ICON,
	GID_STOP,
	GID_RELOAD,
	GID_HOME,
	GID_BACK,
	GID_FORWARD,
	GID_THROBBER,
	GID_SEARCH_ICON,
	GID_PAGEINFO,
	GID_PAGEINFO_INSECURE_BM,
	GID_PAGEINFO_INTERNAL_BM,
	GID_PAGEINFO_LOCAL_BM,
	GID_PAGEINFO_SECURE_BM,
	GID_PAGEINFO_WARNING_BM,
	GID_FAVE,
	GID_FAVE_ADD,
	GID_FAVE_RMV,
	GID_CLOSETAB,
	GID_CLOSETAB_BM,
	GID_ADDTAB,
	GID_ADDTAB_BM,
	GID_TABS,
	GID_TABS_FLAG,
	GID_SEARCHSTRING,
	GID_TOOLBARLAYOUT,
	GID_HOTLIST,
	GID_HOTLISTLAYOUT,
	GID_HOTLISTSEPBAR,
	GID_HSCROLL,
	GID_HSCROLLLAYOUT,
	GID_VSCROLL,
	GID_VSCROLLLAYOUT,
	GID_LOGLAYOUT,
	GID_LOG,
	GID_LAST
};

struct gui_window_2 {
	struct ami_generic_window w;
	struct Window *win;
	Object *restrict objects[GID_LAST];
	struct gui_window *gw; /* currently-displayed gui_window */
	bool redraw_required;
	int throbber_frame;
	struct List tab_list;
	ULONG tabs;
	ULONG next_tab;
	struct Node *last_new_tab;
	struct Hook scrollerhook;
	browser_mouse_state mouse_state;
	browser_mouse_state key_state;
	ULONG throbber_update_count;
	struct find_window *searchwin;
	ULONG oldh;
	ULONG oldv;
	int temp;
	bool redraw_scroll;
	bool new_content;
	struct ami_menu_data *menu_data[AMI_MENU_AREXX_MAX + 1]; /* only for GadTools menus */
	ULONG hotlist_items;
	Object *restrict hotlist_toolbar_lab[AMI_GUI_TOOLBAR_MAX];
	struct List hotlist_toolbar_list;
	struct List *web_search_list;
	Object *search_bm;
	char *restrict svbuffer;
	char *restrict status;
	char *restrict wintitle;
	char *restrict helphints[GID_LAST];
	browser_mouse_state prev_mouse_state;
	struct timeval lastclick;
	struct AppIcon *appicon; /* iconify appicon */
	struct DiskObject *dobj; /* iconify appicon */
	struct Hook favicon_hook;
	struct Hook throbber_hook;
	struct Hook browser_hook;
	struct Hook *ctxmenu_hook;
	Object *restrict history_ctxmenu[2];
	Object *clicktab_ctxmenu;
	gui_drag_type drag_op;
	struct IBox *ptr_lock;
	struct AppWindow *appwin;
	struct MinList *shared_pens;
	gui_pointer_shape mouse_pointer;
	struct Menu *imenu; /* Intuition menu */
	bool closed; /* Window has been closed (via menu) */
};

struct gui_window
{
	struct gui_window_2 *shared;
	int tab;
	struct Node *tab_node;
	int c_x; /* Caret X posn */
	int c_y; /* Caret Y posn */
	int c_w; /* Caret width */
	int c_h; /* Caret height */
	int c_h_temp;
	int scrollx;
	int scrolly;
	struct ami_history_local_window *hw;
	struct List dllist;
	struct hlcache_handle *favicon;
	bool throbbing;
	char *tabtitle;
	APTR deferred_rects_pool;
	struct MinList *deferred_rects;
	struct browser_window *bw;
	struct ColumnInfo *logcolumns;
	struct List loglist;
};

struct ami_gui_tb_userdata {
	struct List *sblist;
	struct gui_window_2 *gw;
	int items;
};

static struct MinList *window_list = NULL;
static struct Screen *scrn = NULL;
static struct MsgPort *sport = NULL;
static struct gui_window *cur_gw = NULL;

static bool ami_quit = false;

static struct MsgPort *schedulermsgport = NULL;
static struct MsgPort *appport;
#ifdef __amigaos4__
static Class *urlStringClass;
#endif

static BOOL locked_screen = FALSE;
static int screen_signal = -1;
static bool win_destroyed;
static STRPTR nsscreentitle;
static struct gui_globals *browserglob = NULL;

static struct MsgPort *applibport = NULL;
static uint32 ami_appid = 0;
static ULONG applibsig = 0;
static ULONG rxsig = 0;
static struct Hook newprefs_hook;

static STRPTR temp_homepage_url = NULL;
static bool cli_force = false;

#define USERS_DIR "PROGDIR:Users"
static char *users_dir = NULL;
static char *current_user_dir;
static char *current_user_faviconcache;

static const __attribute__((used)) char *stack_cookie = "\0$STACK:196608\0";

const char * const versvn;
const char * const verdate;

static void ami_switch_tab(struct gui_window_2 *gwin, bool redraw);
static void ami_change_tab(struct gui_window_2 *gwin, int direction);
static void ami_get_hscroll_pos(struct gui_window_2 *gwin, ULONG *xs);
static void ami_get_vscroll_pos(struct gui_window_2 *gwin, ULONG *ys);
static void ami_quit_netsurf_delayed(void);
static Object *ami_gui_splash_open(void);
static void ami_gui_splash_close(Object *win_obj);
static bool ami_gui_map_filename(char **remapped, const char *restrict path,
	const char *restrict file, const char *restrict map);
static void ami_gui_window_update_box_deferred(struct gui_window *g, bool draw);
static void ami_do_redraw(struct gui_window_2 *g);
static void ami_schedule_redraw_remove(struct gui_window_2 *gwin);

static bool gui_window_get_scroll(struct gui_window *g, int *restrict sx, int *restrict sy);
static nserror gui_window_set_scroll(struct gui_window *g, const struct rect *rect);
static void gui_window_remove_caret(struct gui_window *g);
static void gui_window_place_caret(struct gui_window *g, int x, int y, int height, const struct rect *clip);

HOOKF(uint32, ami_set_favicon_render_hook, APTR, space, struct gpRender *);
HOOKF(uint32, ami_set_throbber_render_hook, APTR, space, struct gpRender *);
HOOKF(uint32, ami_gui_browser_render_hook, APTR, space, struct gpRender *);

/* accessors for default options - user option is updated if it is set as per default */
#define nsoption_default_set_int(OPTION, VALUE)				\
	if (nsoptions_default[NSOPTION_##OPTION].value.i == nsoptions[NSOPTION_##OPTION].value.i)	\
		nsoptions[NSOPTION_##OPTION].value.i = VALUE;	\
	nsoptions_default[NSOPTION_##OPTION].value.i = VALUE

/* Functions documented in gui.h */
struct MsgPort *ami_gui_get_shared_msgport(void)
{
	assert(sport != NULL);
	return sport;
}

struct gui_window *ami_gui_get_active_gw(void)
{
	return cur_gw;
}

struct Screen *ami_gui_get_screen(void)
{
	return scrn;
}

struct MinList *ami_gui_get_window_list(void)
{
	assert(window_list != NULL);
	return window_list;
}

void ami_gui_beep(void)
{
	DisplayBeep(scrn);
}

struct browser_window *ami_gui_get_browser_window(struct gui_window *gw)
{
	assert(gw != NULL);
	return gw->bw;
}

struct browser_window *ami_gui2_get_browser_window(struct gui_window_2 *gwin)
{
	assert(gwin != NULL);
	return ami_gui_get_browser_window(gwin->gw);
}

struct List *ami_gui_get_download_list(struct gui_window *gw)
{
	assert(gw != NULL);
	return &gw->dllist;
}

struct gui_window_2 *ami_gui_get_gui_window_2(struct gui_window *gw)
{
	assert(gw != NULL);
	return gw->shared;
}

struct gui_window *ami_gui2_get_gui_window(struct gui_window_2 *gwin)
{
	assert(gwin != NULL);
	return gwin->gw;
}

const char *ami_gui_get_win_title(struct gui_window *gw)
{
	assert(gw != NULL);
	assert(gw->shared != NULL);
	return (const char *)gw->shared->wintitle;
}

const char *ami_gui_get_tab_title(struct gui_window *gw)
{
	assert(gw != NULL);
	return (const char *)gw->tabtitle;
}

struct Node *ami_gui_get_tab_node(struct gui_window *gw)
{
	assert(gw != NULL);
	return gw->tab_node;
}

ULONG ami_gui2_get_tabs(struct gui_window_2 *gwin)
{
	assert(gwin != NULL);
	return gwin->tabs;
}

struct List *ami_gui2_get_tab_list(struct gui_window_2 *gwin)
{
	assert(gwin != NULL);
	return &gwin->tab_list;
}

struct hlcache_handle *ami_gui_get_favicon(struct gui_window *gw)
{
	assert(gw != NULL);
	return gw->favicon;
}

struct ami_history_local_window *ami_gui_get_history_window(struct gui_window *gw)
{
	assert(gw != NULL);
	return gw->hw;
}

void ami_gui_set_history_window(struct gui_window *gw, struct ami_history_local_window *hw)
{
	assert(gw != NULL);
	gw->hw = hw;
}

void ami_gui_set_find_window(struct gui_window *gw, struct find_window *fw)
{
	/* This needs to be in gui_window_2 as it is shared amongst tabs (I think),
	 * it just happens that the find code only knows of the gui_window
	 */
	assert(gw != NULL);
	assert(gw->shared != NULL);
	gw->shared->searchwin = fw;
}

bool ami_gui_get_throbbing(struct gui_window *gw)
{
	assert(gw != NULL);
	return gw->throbbing;
}

void ami_gui_set_throbbing(struct gui_window *gw, bool throbbing)
{
	assert(gw != NULL);
	gw->throbbing = throbbing;
}

int ami_gui_get_throbber_frame(struct gui_window *gw)
{
	assert(gw != NULL);
	assert(gw->shared != NULL);
	return gw->shared->throbber_frame;
}

void ami_gui_set_throbber_frame(struct gui_window *gw, int frame)
{
	assert(gw != NULL);
	assert(gw->shared != NULL);
	gw->shared->throbber_frame = frame;
}

Object *ami_gui2_get_object(struct gui_window_2 *gwin, int object_type)
{
	ULONG obj = 0;

	assert(gwin != NULL);

	switch(object_type) {
		case AMI_WIN_MAIN:
			obj = OID_MAIN;
		break;

		case AMI_GAD_THROBBER:
			obj = GID_THROBBER;
		break;

		case AMI_GAD_TABS:
			obj = GID_TABS;
		break;

		case AMI_GAD_URL:
			obj = GID_URL;
		break;

		case AMI_GAD_SEARCH:
			obj = GID_SEARCHSTRING;
		break;

		default:
			return NULL;
		break;
	}

	return gwin->objects[obj];
}


struct Window *ami_gui2_get_window(struct gui_window_2 *gwin)
{
	assert(gwin != NULL);
	return gwin->win;
}

struct Window *ami_gui_get_window(struct gui_window *gw)
{
	assert(gw != NULL);
	return ami_gui2_get_window(gw->shared);
}

struct Menu *ami_gui_get_menu(struct gui_window *gw)
{
	assert(gw != NULL);
	assert(gw->shared != NULL);
	return gw->shared->imenu;
}

void ami_gui2_set_menu(struct gui_window_2 *gwin, struct Menu *menu)
{
	if(menu != NULL) {
		gwin->imenu = menu;
	} else {
		ami_gui_menu_freemenus(gwin->imenu, gwin->menu_data);
	}
}

struct ami_menu_data **ami_gui2_get_menu_data(struct gui_window_2 *gwin)
{
	assert(gwin != NULL);
	return gwin->menu_data;
}

void ami_gui2_set_ctxmenu_history_tmp(struct gui_window_2 *gwin, int temp)
{
	assert(gwin != NULL);
	gwin->temp = temp;
}

int ami_gui2_get_ctxmenu_history_tmp(struct gui_window_2 *gwin)
{
	assert(gwin != NULL);
	return gwin->temp;
}

Object *ami_gui2_get_ctxmenu_history(struct gui_window_2 *gwin, ULONG direction)
{
	assert(gwin != NULL);
	return gwin->history_ctxmenu[direction];
}

void ami_gui2_set_ctxmenu_history(struct gui_window_2 *gwin, ULONG direction, Object *ctx_hist)
{
	assert(gwin != NULL);
	gwin->history_ctxmenu[direction] = ctx_hist;
}

void ami_gui2_set_closed(struct gui_window_2 *gwin, bool closed)
{
	assert(gwin != NULL);
	gwin->closed = closed;
}

void ami_gui2_set_new_content(struct gui_window_2 *gwin, bool new_content)
{
	assert(gwin != NULL);
	gwin->new_content = new_content;
}

/** undocumented, or internal, or documented elsewhere **/

#ifdef __amigaos4__
static void *ami_find_gwin_by_id(struct Window *win, uint32 type)
{
	struct nsObject *node, *nnode;
	struct gui_window_2 *gwin;

	if(!IsMinListEmpty(window_list))
	{
		node = (struct nsObject *)GetHead((struct List *)window_list);

		do
		{
			nnode=(struct nsObject *)GetSucc((struct Node *)node);

			if(node->Type == type)
			{
				gwin = node->objstruct;
				if(win == ami_gui2_get_window(gwin)) return gwin;
			}
		} while((node = nnode));
	}
	return NULL;
}

void *ami_window_at_pointer(int type)
{
	struct Layer *layer;
	struct Screen *scrn = ami_gui_get_screen();

	LockLayerInfo(&scrn->LayerInfo);

	layer = WhichLayer(&scrn->LayerInfo, scrn->MouseX, scrn->MouseY);

	UnlockLayerInfo(&scrn->LayerInfo);

	if(layer) return ami_find_gwin_by_id(layer->Window, type);
		else return NULL;
}
#else
/**\todo check if OS4 version of this function will build on OS3, even if it isn't called */
void *ami_window_at_pointer(int type)
{
	return NULL;
}
#endif

void ami_set_pointer(struct gui_window_2 *gwin, gui_pointer_shape shape, bool update)
{
	if(gwin->mouse_pointer == shape) return;
	ami_update_pointer(ami_gui2_get_window(gwin), shape);
	if(update == true) gwin->mouse_pointer = shape;
}

/* reset the mouse pointer back to what NetSurf last set it as */
void ami_reset_pointer(struct gui_window_2 *gwin)
{
	ami_update_pointer(ami_gui2_get_window(gwin), gwin->mouse_pointer);
}


STRPTR ami_locale_langs(int *codeset)
{
	struct Locale *locale;
	STRPTR acceptlangs = NULL;
	char *remapped = NULL;

	if((locale = OpenLocale(NULL)))
	{
		if(codeset != NULL) *codeset = locale->loc_CodeSet;

		for(int i = 0; i < 10; i++)
		{
			if(locale->loc_PrefLanguages[i])
			{
				if(ami_gui_map_filename(&remapped, "PROGDIR:Resources",
					locale->loc_PrefLanguages[i], "LangNames"))
				{
					if(acceptlangs)
					{
						STRPTR acceptlangs2 = acceptlangs;
						acceptlangs = ASPrintf("%s, %s",acceptlangs2, remapped);
						FreeVec(acceptlangs2);
						acceptlangs2 = NULL;
					}
					else
					{
						acceptlangs = ASPrintf("%s", remapped);
					}
				}
				if(remapped != NULL) free(remapped);
			}
			else
			{
				continue;
			}
		}
		CloseLocale(locale);
	}
	return acceptlangs;
}

static bool ami_gui_map_filename(char **remapped, const char *restrict path,
		const char *restrict file, const char *restrict map)
{
	BPTR fh = 0;
	char *mapfile = NULL;
	size_t mapfile_size = 0;
	char buffer[1024];
	char *restrict realfname;
	bool found = false;

	netsurf_mkpath(&mapfile, &mapfile_size, 2, path, map);

	if(mapfile == NULL) return false;

	fh = FOpen(mapfile, MODE_OLDFILE, 0);
	if(fh)
	{
		while(FGets(fh, buffer, 1024) != 0)
		{
			if((buffer[0] == '#') ||
				(buffer[0] == '\n') ||
				(buffer[0] == '\0')) continue;

			realfname = strchr(buffer, ':');
			if(realfname)
			{
				if(strncmp(buffer, file, strlen(file)) == 0)
				{
					if(realfname[strlen(realfname)-1] == '\n')
						realfname[strlen(realfname)-1] = '\0';
					*remapped = strdup(realfname + 1);
					found = true;
					break;
				}
			}
		}
		FClose(fh);
	}

	if(found == false) *remapped = strdup(file);
		else NSLOG(netsurf, INFO,
			   "Remapped %s to %s in path %s using %s", file,
			   *remapped, path, map);

	free(mapfile);

	return found;
}

static bool ami_gui_check_resource(char *fullpath, const char *file)
{
	bool found = false;
	char *remapped;
	BPTR lock = 0;
	size_t fullpath_len = 1024;

	ami_gui_map_filename(&remapped, fullpath, file, "Resource.map");
	netsurf_mkpath(&fullpath, &fullpath_len, 2, fullpath, remapped);

	lock = Lock(fullpath, ACCESS_READ);
	if(lock) {
		UnLock(lock);
		found = true;
	}

	if(found) NSLOG(netsurf, INFO, "Found %s", fullpath);
	free(remapped);

	return found;
}

bool ami_locate_resource(char *fullpath, const char *file)
{
	struct Locale *locale;
	int i;
	bool found = false;
	char *remapped = NULL;
	size_t fullpath_len = 1024;

	/* Check NetSurf user data area first */

	if(current_user_dir != NULL) {
		strcpy(fullpath, current_user_dir);
		found = ami_gui_check_resource(fullpath, file);
		if(found) return true;
	}

	/* Check current theme directory */
	if(nsoption_charp(theme)) {
		strcpy(fullpath, nsoption_charp(theme));
		found = ami_gui_check_resource(fullpath, file);
		if(found) return true;
	}

	/* If not found, start on the user's preferred languages */

	locale = OpenLocale(NULL);

	for(i=0;i<10;i++) {
		strcpy(fullpath, "PROGDIR:Resources/");

		if(locale->loc_PrefLanguages[i]) {
			if(ami_gui_map_filename(&remapped, "PROGDIR:Resources",
					locale->loc_PrefLanguages[i], "LangNames") == true) {
				netsurf_mkpath(&fullpath, &fullpath_len, 2, fullpath, remapped);
				found = ami_gui_check_resource(fullpath, file);
				free(remapped);
			}
		} else {
			continue;
		}

		if(found) break;
	}

	if(!found) {
		/* If not found yet, check in PROGDIR:Resources/en,
		 * might not be in user's preferred languages */

		strcpy(fullpath, "PROGDIR:Resources/en/");
		found = ami_gui_check_resource(fullpath, file);
	}

	CloseLocale(locale);

	if(!found) {
		/* Lastly check directly in PROGDIR:Resources */

		strcpy(fullpath, "PROGDIR:Resources/");
		found = ami_gui_check_resource(fullpath, file);
	}

	return found;
}

static void ami_gui_resources_free(void)
{
	ami_schedule_free();
	ami_object_fini();

	FreeSysObject(ASOT_PORT, appport);
	FreeSysObject(ASOT_PORT, sport);
	FreeSysObject(ASOT_PORT, schedulermsgport);
}

static bool ami_gui_resources_open(void)
{
#ifdef __amigaos4__
	urlStringClass = MakeStringClass();
#endif

    if(!(appport = AllocSysObjectTags(ASOT_PORT,
							ASO_NoTrack, FALSE,
							TAG_DONE))) return false;

    if(!(sport = AllocSysObjectTags(ASOT_PORT,
							ASO_NoTrack, FALSE,
							TAG_DONE))) return false;

    if(!(schedulermsgport = AllocSysObjectTags(ASOT_PORT,
							ASO_NoTrack, FALSE,
							TAG_DONE))) return false;

	if(ami_schedule_create(schedulermsgport) != NSERROR_OK) {
		ami_misc_fatal_error("Failed to initialise scheduler");
		return false;
	}

	ami_object_init();

	return true;
}

static UWORD ami_system_colour_scrollbar_fgpen(struct DrawInfo *drinfo)
{
	LONG scrollerfillpen = FALSE;
#ifdef __amigaos4__
	GetGUIAttrs(NULL, drinfo, GUIA_PropKnobColor, &scrollerfillpen, TAG_DONE);

	if(scrollerfillpen) return FILLPEN;
		else return FOREGROUNDPEN;
#else
	return FILLPEN;
#endif

}

/**
 * set option from pen
 */
static nserror
colour_option_from_pen(UWORD pen,
			   enum nsoption_e option,
			   struct Screen *screen,
			   colour def_colour)
{
	ULONG colr[3];
	struct DrawInfo *drinfo;

	if((option < NSOPTION_SYS_COLOUR_START) ||
	   (option > NSOPTION_SYS_COLOUR_END) ||
	   (nsoptions[option].type != OPTION_COLOUR)) {
		return NSERROR_BAD_PARAMETER;
	}

	if(screen != NULL) {
		drinfo = GetScreenDrawInfo(screen);
		if(drinfo != NULL) {

			if(pen == AMINS_SCROLLERPEN) pen = ami_system_colour_scrollbar_fgpen(drinfo);

			/* Get the colour of the pen being used for "pen" */
			GetRGB32(screen->ViewPort.ColorMap, drinfo->dri_Pens[pen], 1, (ULONG *)&colr);

			/* convert it to a color */
			def_colour = ((colr[0] & 0xff000000) >> 24) |
				((colr[1] & 0xff000000) >> 16) |
				((colr[2] & 0xff000000) >> 8);

			FreeScreenDrawInfo(screen, drinfo);
		}
	}

	if (nsoptions_default[option].value.c == nsoptions[option].value.c)
		nsoptions[option].value.c = def_colour;
	nsoptions_default[option].value.c = def_colour;

	return NSERROR_OK;
}

/* exported interface documented in amiga/gui.h */
STRPTR ami_gui_get_screen_title(void)
{
	if(nsscreentitle == NULL) {
		nsscreentitle = ASPrintf("NetSurf %s", netsurf_version);
		/* If this fails it will be NULL, which means we'll get the screen's
		 * default titlebar text instead - so no need to check for error. */
	}

	return nsscreentitle;
}

static void ami_set_screen_defaults(struct Screen *screen)
{
	/* various window size/position defaults */
	int width = screen->Width / 2;
	int height = screen->Height / 2;
	int top = (screen->Height / 2) - (height / 2);
	int left = (screen->Width / 2) - (width / 2);

	nsoption_default_set_int(cookies_window_ypos, top);
	nsoption_default_set_int(cookies_window_xpos, left);
	nsoption_default_set_int(cookies_window_xsize, width);
	nsoption_default_set_int(cookies_window_ysize, height);

	nsoption_default_set_int(history_window_ypos, top);
	nsoption_default_set_int(history_window_xpos, left);
	nsoption_default_set_int(history_window_xsize, width);
	nsoption_default_set_int(history_window_ysize, height);

	nsoption_default_set_int(hotlist_window_ypos, top);
	nsoption_default_set_int(hotlist_window_xpos, left);
	nsoption_default_set_int(hotlist_window_xsize, width);
	nsoption_default_set_int(hotlist_window_ysize, height);


	nsoption_default_set_int(window_x, 0);
	nsoption_default_set_int(window_y, screen->BarHeight + 1);
	nsoption_default_set_int(window_width, screen->Width);
	nsoption_default_set_int(window_height, screen->Height - screen->BarHeight - 1);

#ifdef __amigaos4__
	nsoption_default_set_int(redraw_tile_size_x, screen->Width);
	nsoption_default_set_int(redraw_tile_size_y, screen->Height);

	/* set system colours for amiga ui */
	colour_option_from_pen(FILLPEN, NSOPTION_sys_colour_ActiveBorder, screen, 0x00000000);
	colour_option_from_pen(FILLPEN, NSOPTION_sys_colour_ActiveCaption, screen, 0x00dddddd);
	colour_option_from_pen(BACKGROUNDPEN, NSOPTION_sys_colour_AppWorkspace, screen, 0x00eeeeee);
	colour_option_from_pen(BACKGROUNDPEN, NSOPTION_sys_colour_Background, screen, 0x00aa0000);
	colour_option_from_pen(FOREGROUNDPEN, NSOPTION_sys_colour_ButtonFace, screen, 0x00aaaaaa);
	colour_option_from_pen(FORESHINEPEN, NSOPTION_sys_colour_ButtonHighlight, screen, 0x00cccccc);
	colour_option_from_pen(FORESHADOWPEN, NSOPTION_sys_colour_ButtonShadow, screen, 0x00bbbbbb);
	colour_option_from_pen(TEXTPEN, NSOPTION_sys_colour_ButtonText, screen, 0x00000000);
	colour_option_from_pen(FILLTEXTPEN, NSOPTION_sys_colour_CaptionText, screen, 0x00000000);
	colour_option_from_pen(DISABLEDTEXTPEN, NSOPTION_sys_colour_GrayText, screen, 0x00777777);
	colour_option_from_pen(SELECTPEN, NSOPTION_sys_colour_Highlight, screen, 0x00ee0000);
	colour_option_from_pen(SELECTTEXTPEN, NSOPTION_sys_colour_HighlightText, screen, 0x00000000);
	colour_option_from_pen(INACTIVEFILLPEN, NSOPTION_sys_colour_InactiveBorder, screen, 0x00000000);
	colour_option_from_pen(INACTIVEFILLPEN, NSOPTION_sys_colour_InactiveCaption, screen, 0x00ffffff);
	colour_option_from_pen(INACTIVEFILLTEXTPEN, NSOPTION_sys_colour_InactiveCaptionText, screen, 0x00cccccc);
	colour_option_from_pen(BACKGROUNDPEN, NSOPTION_sys_colour_InfoBackground, screen, 0x00aaaaaa);/* This is wrong, HelpHint backgrounds are pale yellow but doesn't seem to be a DrawInfo pen defined for it. */
	colour_option_from_pen(TEXTPEN, NSOPTION_sys_colour_InfoText, screen, 0x00000000);
	colour_option_from_pen(MENUBACKGROUNDPEN, NSOPTION_sys_colour_Menu, screen, 0x00aaaaaa);
	colour_option_from_pen(MENUTEXTPEN, NSOPTION_sys_colour_MenuText, screen, 0x00000000);
	colour_option_from_pen(AMINS_SCROLLERPEN, NSOPTION_sys_colour_Scrollbar, screen, 0x00aaaaaa);
	colour_option_from_pen(FORESHADOWPEN, NSOPTION_sys_colour_ThreeDDarkShadow, screen, 0x00555555);
	colour_option_from_pen(FOREGROUNDPEN, NSOPTION_sys_colour_ThreeDFace, screen, 0x00dddddd);
	colour_option_from_pen(FORESHINEPEN, NSOPTION_sys_colour_ThreeDHighlight, screen, 0x00aaaaaa);
	colour_option_from_pen(HALFSHINEPEN, NSOPTION_sys_colour_ThreeDLightShadow, screen, 0x00999999);
	colour_option_from_pen(HALFSHADOWPEN, NSOPTION_sys_colour_ThreeDShadow, screen, 0x00777777);
	colour_option_from_pen(BACKGROUNDPEN, NSOPTION_sys_colour_Window, screen, 0x00aaaaaa);
	colour_option_from_pen(INACTIVEFILLPEN, NSOPTION_sys_colour_WindowFrame, screen, 0x00000000);
	colour_option_from_pen(TEXTPEN, NSOPTION_sys_colour_WindowText, screen, 0x00000000);
#else
	nsoption_default_set_int(redraw_tile_size_x, 100);
	nsoption_default_set_int(redraw_tile_size_y, 100);
#endif
}


/**
 * Set option defaults for amiga frontend
 *
 * @param defaults The option table to update.
 * @return error status.
 */
static nserror ami_set_options(struct nsoption_s *defaults)
{
	STRPTR tempacceptlangs;
	char temp[1024];
	int codeset = 0;

	/* The following line disables the popupmenu.class select menu.
	** It's not recommended to use it!
	*/
	nsoption_set_bool(core_select_menu, true);

	/* ClickTab < 53 doesn't work with the auto show/hide tab-bar (for reasons forgotten) */
	if(ClickTabBase->lib_Version < 53)
		nsoption_set_bool(tab_always_show, true);

	if((!nsoption_charp(accept_language)) || 
	   (nsoption_charp(accept_language)[0] == '\0') ||
	   (nsoption_bool(accept_lang_locale) == true))
	{
		if((tempacceptlangs = ami_locale_langs(&codeset)))
		{
			nsoption_set_charp(accept_language,
					   (char *)strdup(tempacceptlangs));
			FreeVec(tempacceptlangs);
		}
	}

	/* Some OS-specific overrides */
#ifdef __amigaos4__
	if(!LIB_IS_AT_LEAST((struct Library *)SysBase, 53, 89)) {
		/* Disable ExtMem usage pre-OS4.1FEU1 */
		nsoption_set_bool(use_extmem, false);
	}

	if(codeset == 0) codeset = 4; /* ISO-8859-1 */
	const char *encname = (const char *)ObtainCharsetInfo(DFCS_NUMBER, codeset,
							DFCS_MIMENAME);
	nsoption_set_charp(local_charset, strdup(encname));
	nsoption_set_int(local_codeset, codeset);
#else
	nsoption_set_bool(download_notify, false);
	nsoption_set_bool(font_antialiasing, false);
	nsoption_set_bool(truecolour_mouse_pointers, false);
	nsoption_set_bool(use_openurl_lib, true);
	nsoption_set_bool(bitmap_fonts, true);
#endif

	sprintf(temp, "%s/Cookies", current_user_dir);
	nsoption_setnull_charp(cookie_file, 
			       (char *)strdup(temp));

	sprintf(temp, "%s/Hotlist", current_user_dir);
	nsoption_setnull_charp(hotlist_file, 
			       (char *)strdup(temp));

	sprintf(temp, "%s/URLdb", current_user_dir);
	nsoption_setnull_charp(url_file,
			       (char *)strdup(temp));

	sprintf(temp, "%s/FontGlyphCache", current_user_dir);
	nsoption_setnull_charp(font_unicode_file,
			       (char *)strdup(temp));

	nsoption_setnull_charp(ca_bundle,
			       (char *)strdup("PROGDIR:Resources/ca-bundle"));

	/* font defaults */
#ifdef __amigaos4__
	nsoption_setnull_charp(font_sans, (char *)strdup("DejaVu Sans"));
	nsoption_setnull_charp(font_serif, (char *)strdup("DejaVu Serif"));
	nsoption_setnull_charp(font_mono, (char *)strdup("DejaVu Sans Mono"));
	nsoption_setnull_charp(font_cursive, (char *)strdup("DejaVu Sans"));
	nsoption_setnull_charp(font_fantasy, (char *)strdup("DejaVu Serif"));
#else
	nsoption_setnull_charp(font_sans, (char *)strdup("helvetica"));
	nsoption_setnull_charp(font_serif, (char *)strdup("times"));
	nsoption_setnull_charp(font_mono, (char *)strdup("topaz"));
	nsoption_setnull_charp(font_cursive, (char *)strdup("garnet"));
	nsoption_setnull_charp(font_fantasy, (char *)strdup("emerald"));
/* Default CG fonts for OS3 - these work with use_diskfont both on and off,
	however they are slow in both cases. The bitmap fonts don't work when
	use_diskfont is off. The bitmap fonts performance on 68k is far superior,
	so default to those for now whilst testing.
	\todo maybe add some buttons to the prefs GUI to toggle?
	nsoption_setnull_charp(font_sans, (char *)strdup("CGTriumvirate"));
	nsoption_setnull_charp(font_serif, (char *)strdup("CGTimes"));
	nsoption_setnull_charp(font_mono, (char *)strdup("LetterGothic"));
	nsoption_setnull_charp(font_cursive, (char *)strdup("CGTriumvirate"));
	nsoption_setnull_charp(font_fantasy, (char *)strdup("CGTimes"));
*/
#endif

	if (nsoption_charp(font_unicode) == NULL)
	{
		BPTR lock = 0;
		/* Search for some likely candidates */

		if((lock = Lock("FONTS:Code2000.otag", ACCESS_READ)))
		{
			UnLock(lock);
			nsoption_set_charp(font_unicode, 
					   (char *)strdup("Code2000"));
		}
		else if((lock = Lock("FONTS:Bitstream Cyberbit.otag", ACCESS_READ)))
		{
			UnLock(lock);
			nsoption_set_charp(font_unicode,
					   (char *)strdup("Bitstream Cyberbit"));
		}
	}

	if (nsoption_charp(font_surrogate) == NULL) {
		BPTR lock = 0;
		/* Search for some likely candidates -
		 * Ideally we should pick a font during the scan process which announces it
		 * contains UCR_SURROGATES, but nothing appears to have the tag.
		 */
		if((lock = Lock("FONTS:Symbola.otag", ACCESS_READ))) {
			UnLock(lock);
			nsoption_set_charp(font_surrogate, 
					   (char *)strdup("Symbola"));
		}
	}

	return NSERROR_OK;
}

static void ami_amiupdate(void)
{
	/* Create AppPath location for AmiUpdate use */

	BPTR lock = 0;

	if(((lock = Lock("ENVARC:AppPaths",SHARED_LOCK)) == 0))
	{
		lock = CreateDir("ENVARC:AppPaths");
	}
	
	UnLock(lock);

	if((lock = Lock("PROGDIR:", ACCESS_READ)))
	{
		char filename[1024];
		BPTR amiupdatefh;

		DevNameFromLock(lock, (STRPTR)&filename, 1024L, DN_FULLPATH);

		if((amiupdatefh = FOpen("ENVARC:AppPaths/NetSurf", MODE_NEWFILE, 0))) {
			FPuts(amiupdatefh, (CONST_STRPTR)&filename);
			FClose(amiupdatefh);
		}

		UnLock(lock);
	}
}

static nsurl *gui_get_resource_url(const char *path)
{
	char buf[1024];
	nsurl *url = NULL;

	if(ami_locate_resource(buf, path) == false)
		return NULL;

	netsurf_path_to_nsurl(buf, &url);

	return url;
}

HOOKF(void, ami_gui_newprefs_hook, APTR, window, APTR)
{
	ami_set_screen_defaults(scrn);
}

static void ami_openscreen(void)
{
	ULONG id = 0;
	ULONG compositing;

	if (nsoption_int(screen_compositing) == -1)
		compositing = ~0UL;
	else compositing = nsoption_int(screen_compositing);

	if (nsoption_charp(pubscreen_name) == NULL)
	{
		if((nsoption_charp(screen_modeid)) && 
		   (strncmp(nsoption_charp(screen_modeid), "0x", 2) == 0))
		{
			id = strtoul(nsoption_charp(screen_modeid), NULL, 0);
		}
		else
		{
			struct ScreenModeRequester *screenmodereq = NULL;

			if((screenmodereq = AllocAslRequest(ASL_ScreenModeRequest,NULL))) {
				if(AslRequestTags(screenmodereq,
						ASLSM_MinDepth, 0,
						ASLSM_MaxDepth, 32,
						TAG_DONE))
				{
					char *modeid = malloc(20);
					id = screenmodereq->sm_DisplayID;
					sprintf(modeid, "0x%lx", id);
					nsoption_set_charp(screen_modeid, modeid);
					ami_nsoption_write();
				}
				FreeAslRequest(screenmodereq);
			}
		}

		if(screen_signal == -1) screen_signal = AllocSignal(-1);
		NSLOG(netsurf, INFO, "Screen signal %d", screen_signal);
		scrn = OpenScreenTags(NULL,
					/**\todo specify screen depth */
					SA_DisplayID, id,
					SA_Title, ami_gui_get_screen_title(),
					SA_Type, PUBLICSCREEN,
					SA_PubName, "NetSurf",
					SA_PubSig, screen_signal,
					SA_PubTask, FindTask(0),
					SA_LikeWorkbench, TRUE,
					SA_Compositing, compositing,
					TAG_DONE);

		if(scrn)
		{
			PubScreenStatus(scrn,0);
		}
		else
		{
			FreeSignal(screen_signal);
			screen_signal = -1;

			if((scrn = LockPubScreen("NetSurf")))
			{
				locked_screen = TRUE;
			}
			else
			{
				nsoption_set_charp(pubscreen_name,
						   strdup("Workbench"));
			}
		}
	}

	if (nsoption_charp(pubscreen_name) != NULL)
	{
		scrn = LockPubScreen(nsoption_charp(pubscreen_name));

		if(scrn == NULL)
		{
			scrn = LockPubScreen("Workbench");
		}
		locked_screen = TRUE;
	}

	ami_font_setdevicedpi(id);
	ami_set_screen_defaults(scrn);
	ami_help_new_screen(scrn);
}

static void ami_openscreenfirst(void)
{
	ami_openscreen();
	if(browserglob == NULL) browserglob = ami_plot_ra_alloc(0, 0, false, false);
	ami_theme_throbber_setup();
}

static struct RDArgs *ami_gui_commandline(int *restrict argc, char ** argv,
		int *restrict nargc, char ** nargv)
{
	struct RDArgs *args;
	CONST_STRPTR template = "-v/S,NSOPTS/M,URL/K,USERSDIR/K,FORCE/S";
	long rarray[] = {0,0,0,0,0};
	enum
	{
		A_VERBOSE, /* ignored */
		A_NSOPTS, /* ignored */
		A_URL,
		A_USERSDIR,
		A_FORCE
	};

	if(*argc == 0) return NULL; // argc==0 is started from wb

	if((args = ReadArgs(template, rarray, NULL))) {
		if(rarray[A_URL]) {
			NSLOG(netsurf, INFO,
			      "URL %s specified on command line",
			      (char *)rarray[A_URL]);
			temp_homepage_url = strdup((char *)rarray[A_URL]); /**\todo allow IDNs */
		}

		if(rarray[A_USERSDIR]) {
			NSLOG(netsurf, INFO,
			      "USERSDIR %s specified on command line",
			      (char *)rarray[A_USERSDIR]);
			users_dir = ASPrintf("%s", rarray[A_USERSDIR]);
		}

		if(rarray[A_FORCE]) {
			NSLOG(netsurf, INFO,
			      "FORCE specified on command line");
			cli_force = true;
		}

		if(rarray[A_NSOPTS]) {
		/* The NSOPTS/M parameter specified in the ReadArgs template is
		 * special. The /M means it collects all arguments that can't
		 * be assigned to any other parameter, and stores them in an
		 * array.  We collect these and pass them as a fake argc/argv
		 * to nsoption_commandline().
		 * This trickery is necessary because if ReadArgs() is called
		 * first, nsoption_commandline() can no longer parse (fetch?)
		 * the arguments.  If nsoption_commandline() is called first,
		 * then ReadArgs cannot fetch the arguments.
		 *\todo this was totally broken so to stop startup crashing
		 * has been temporarily removed (core cli not called when func
		 * returns NULL).
		 */
		}
	} else {
		NSLOG(netsurf, INFO, "ReadArgs failed to parse command line");
	}

	FreeArgs(args);
	return NULL;
}

static char *ami_gui_read_tooltypes(struct WBArg *wbarg)
{
	struct DiskObject *dobj;
	STRPTR *toolarray;
	char *s;
	char *current_user = NULL;

	if((*wbarg->wa_Name) && (dobj = GetDiskObject(wbarg->wa_Name))) {
		toolarray = (STRPTR *)dobj->do_ToolTypes;

		if((s = (char *)FindToolType(toolarray,"USERSDIR"))) users_dir = ASPrintf("%s", s);
		if((s = (char *)FindToolType(toolarray,"USER"))) current_user = ASPrintf("%s", s);

		FreeDiskObject(dobj);
	}
	return current_user;
}

static STRPTR ami_gui_read_all_tooltypes(int argc, char **argv)
{
	struct WBStartup *WBenchMsg;
	struct WBArg *wbarg;
	char i = 0;
	char *current_user = NULL;
	char *cur_user = NULL;

	if(argc == 0) { /* Started from WB */
		WBenchMsg = (struct WBStartup *)argv;
		for(i = 0, wbarg = WBenchMsg->sm_ArgList; i < WBenchMsg->sm_NumArgs; i++,wbarg++) {
			LONG olddir =-1;
			if((wbarg->wa_Lock) && (*wbarg->wa_Name))
				olddir = SetCurrentDir(wbarg->wa_Lock);

			cur_user = ami_gui_read_tooltypes(wbarg);
			if(cur_user != NULL) {
				if(current_user != NULL) FreeVec(current_user);
				current_user = cur_user;
			}

			if(olddir !=-1) SetCurrentDir(olddir);
		}
	}

	return current_user;
}

static void gui_init2(int argc, char** argv)
{
	struct Screen *screen;
	BOOL notalreadyrunning;
	nsurl *url;
	nserror error;
	struct browser_window *bw = NULL;

	notalreadyrunning = ami_arexx_init(&rxsig);

	/* ...and this ensures the treeview at least gets the WB colour palette to work with */
	if(scrn == NULL) {
		if((screen = LockPubScreen("Workbench"))) {
			ami_set_screen_defaults(screen);
			UnlockPubScreen(NULL, screen);
		}
	} else {
		ami_set_screen_defaults(scrn);
	}
	/**/

	hotlist_init(nsoption_charp(hotlist_file),
			nsoption_charp(hotlist_file));
	search_web_select_provider(nsoption_int(search_provider));

	if (notalreadyrunning && 
	    (nsoption_bool(startup_no_window) == false))
		ami_openscreenfirst();

	if(cli_force == true) {
		notalreadyrunning = TRUE;
	}

	if(temp_homepage_url && notalreadyrunning) {
		error = nsurl_create(temp_homepage_url, &url);
		if (error == NSERROR_OK) {
			error = browser_window_create(BW_CREATE_HISTORY,
					url,
					NULL,
					NULL,
					&bw);
			nsurl_unref(url);
		}
		if (error != NSERROR_OK) {
			amiga_warn_user(messages_get_errorcode(error), 0);
		}
		free(temp_homepage_url);
		temp_homepage_url = NULL;
	}

	if(argc == 0) { // WB
		struct WBStartup *WBenchMsg = (struct WBStartup *)argv;
		struct WBArg *wbarg;
		int first=0,i=0;
		char fullpath[1024];

		for(i=0,wbarg=WBenchMsg->sm_ArgList;i<WBenchMsg->sm_NumArgs;i++,wbarg++)
		{
			if(i==0) continue;
			if((wbarg->wa_Lock)&&(*wbarg->wa_Name))
			{
				DevNameFromLock(wbarg->wa_Lock,fullpath,1024,DN_FULLPATH);
				AddPart(fullpath,wbarg->wa_Name,1024);

				if(!temp_homepage_url) {
					nsurl *temp_url;
					if (netsurf_path_to_nsurl(fullpath, &temp_url) == NSERROR_OK) {
						temp_homepage_url = strdup(nsurl_access(temp_url));
						nsurl_unref(temp_url);
					}
				}

				if(notalreadyrunning)
				{
					error = nsurl_create(temp_homepage_url, &url);

					if (error == NSERROR_OK) {
						if(!first)
						{
							error = browser_window_create(BW_CREATE_HISTORY,
										      url,
										      NULL,
										      NULL,
										      &bw);

							first=1;
						}
						else
						{
							error = browser_window_create(BW_CREATE_CLONE | BW_CREATE_HISTORY,
										      url,
										      NULL,
										      bw,
										      &bw);

						}
						nsurl_unref(url);

					}
					if (error != NSERROR_OK) {
						amiga_warn_user(messages_get_errorcode(error), 0);
					}
					free(temp_homepage_url);
					temp_homepage_url = NULL;
				}
			}
			/* this should be where we read tooltypes, but it's too late for that now */
		}
	}

	nsoption_setnull_charp(homepage_url, (char *)strdup(NETSURF_HOMEPAGE));

	if(!notalreadyrunning)
	{
		STRPTR sendcmd = NULL;
		char newtab[11] = "\0";

		if(nsoption_bool(tab_new_session) == true) {
			strcpy(newtab, "TAB ACTIVE");
		}

		if(temp_homepage_url) {
			sendcmd = ASPrintf("OPEN \"%s\" NEW%s", temp_homepage_url, newtab);
			free(temp_homepage_url);
			temp_homepage_url = NULL;
		} else {
			sendcmd = ASPrintf("OPEN \"%s\" NEW%s", nsoption_charp(homepage_url), newtab);
		}
		ami_arexx_self(sendcmd);
		FreeVec(sendcmd);

		/* Bring the screen to the front. Intuition may have already done this, but it doesn't hurt. */
		ami_arexx_self("TOFRONT");

		ami_quit=true;
		return;
	}
#ifdef __amigaos4__
	if(IApplication)
	{
		if(argc == 0)
		{
			ULONG noicon = TAG_IGNORE;

			if (nsoption_bool(hide_docky_icon)) 
				noicon = REGAPP_NoIcon;

			ami_appid = RegisterApplication(messages_get("NetSurf"),
				REGAPP_URLIdentifier, "netsurf-browser.org",
				REGAPP_WBStartup, (struct WBStartup *)argv,
				noicon, TRUE,
				REGAPP_HasPrefsWindow, TRUE,
				REGAPP_CanCreateNewDocs, TRUE,
				REGAPP_UniqueApplication, TRUE,
				REGAPP_Description, messages_get("NetSurfDesc"),
				TAG_DONE);
		}
		else
		{
/* TODO: Specify icon when run from Shell */
			ami_appid = RegisterApplication(messages_get("NetSurf"),
				REGAPP_URLIdentifier, "netsurf-browser.org",
				REGAPP_FileName, argv[0],
				REGAPP_NoIcon, TRUE,
				REGAPP_HasPrefsWindow, TRUE,
				REGAPP_CanCreateNewDocs, TRUE,
				REGAPP_UniqueApplication, TRUE,
				REGAPP_Description, messages_get("NetSurfDesc"),
				TAG_DONE);
		}

		GetApplicationAttrs(ami_appid, APPATTR_Port, (ULONG)&applibport, TAG_DONE);
		if(applibport) applibsig = (1L << applibport->mp_SigBit);
	}
#endif
	if(!bw && (nsoption_bool(startup_no_window) == false)) {
		error = nsurl_create(nsoption_charp(homepage_url), &url);
		if (error == NSERROR_OK) {
			error = browser_window_create(BW_CREATE_HISTORY,
						      url,
						      NULL,
						      NULL,
						      NULL);
			nsurl_unref(url);
		}
		if (error != NSERROR_OK) {
			amiga_warn_user(messages_get_errorcode(error), 0);
		}
	}
}

static void ami_update_buttons(struct gui_window_2 *gwin)
{
	long back=FALSE, forward=FALSE, tabclose=FALSE, stop=FALSE, reload=FALSE;
	long s_back, s_forward, s_tabclose, s_stop, s_reload;

	if(!browser_window_back_available(gwin->gw->bw))
		back=TRUE;

	if(!browser_window_forward_available(gwin->gw->bw))
		forward=TRUE;

	if(!browser_window_stop_available(gwin->gw->bw))
		stop=TRUE;

	if(!browser_window_reload_available(gwin->gw->bw))
		reload=TRUE;

	if(nsoption_bool(kiosk_mode) == false) {
		if(gwin->tabs <= 1) {
			tabclose=TRUE;
			ami_gui_menu_set_disabled(gwin->win, gwin->imenu, M_CLOSETAB, true);
		} else {
			ami_gui_menu_set_disabled(gwin->win, gwin->imenu, M_CLOSETAB, false);
		}
	}

	GetAttr(GA_Disabled, gwin->objects[GID_BACK], (uint32 *)&s_back);
	GetAttr(GA_Disabled, gwin->objects[GID_FORWARD], (uint32 *)&s_forward);
	GetAttr(GA_Disabled, gwin->objects[GID_RELOAD], (uint32 *)&s_reload);
	GetAttr(GA_Disabled, gwin->objects[GID_STOP], (uint32 *)&s_stop);

	if(BOOL_MISMATCH(s_back, back))
		SetGadgetAttrs((struct Gadget *)gwin->objects[GID_BACK],
			gwin->win, NULL, GA_Disabled, back, TAG_DONE);

	if(BOOL_MISMATCH(s_forward, forward))
		SetGadgetAttrs((struct Gadget *)gwin->objects[GID_FORWARD],
			gwin->win, NULL, GA_Disabled, forward, TAG_DONE);

	if(BOOL_MISMATCH(s_reload, reload))
		SetGadgetAttrs((struct Gadget *)gwin->objects[GID_RELOAD],
			gwin->win, NULL, GA_Disabled, reload, TAG_DONE);

	if(BOOL_MISMATCH(s_stop, stop))
		SetGadgetAttrs((struct Gadget *)gwin->objects[GID_STOP],
			gwin->win, NULL, GA_Disabled, stop, TAG_DONE);

	if(ClickTabBase->lib_Version < 53) {
		if(gwin->tabs <= 1) tabclose = TRUE;

		GetAttr(GA_Disabled, gwin->objects[GID_CLOSETAB], (uint32 *)&s_tabclose);

		if(BOOL_MISMATCH(s_tabclose, tabclose))
			SetGadgetAttrs((struct Gadget *)gwin->objects[GID_CLOSETAB],
				gwin->win, NULL, GA_Disabled, tabclose, TAG_DONE);
	}

	/* Update the back/forward buttons history context menu */
	ami_ctxmenu_history_create(AMI_CTXMENU_HISTORY_BACK, gwin);
	ami_ctxmenu_history_create(AMI_CTXMENU_HISTORY_FORWARD, gwin);
}

void ami_gui_history(struct gui_window_2 *gwin, bool back)
{
	if(back == true)
	{
		if(browser_window_back_available(gwin->gw->bw))
			browser_window_history_back(gwin->gw->bw, false);
	}
	else
	{
		if(browser_window_forward_available(gwin->gw->bw))
			browser_window_history_forward(gwin->gw->bw, false);
	}

	ami_update_buttons(gwin);
}

int ami_key_to_nskey(ULONG keycode, struct InputEvent *ie)
{
	int nskey = 0, chars;
	char buffer[20];
	char *utf8 = NULL;

	if(keycode >= IECODE_UP_PREFIX) return 0;

	switch(keycode)
	{
		case RAWKEY_CRSRUP:
			if(ie->ie_Qualifier & NSA_QUAL_SHIFT) {
				nskey = NS_KEY_PAGE_UP;
			} else if(ie->ie_Qualifier & NSA_QUAL_ALT) {
				nskey = NS_KEY_TEXT_START;
			}
			else nskey = NS_KEY_UP;
		break;
		case RAWKEY_CRSRDOWN:
			if(ie->ie_Qualifier & NSA_QUAL_SHIFT) {
				nskey = NS_KEY_PAGE_DOWN;
			} else if(ie->ie_Qualifier & NSA_QUAL_ALT) {
				nskey = NS_KEY_TEXT_END;
			}
			else nskey = NS_KEY_DOWN;
		break;
		case RAWKEY_CRSRLEFT:
			if(ie->ie_Qualifier & NSA_QUAL_SHIFT) {
				nskey = NS_KEY_LINE_START;
			}else if(ie->ie_Qualifier & NSA_QUAL_ALT) {
				nskey = NS_KEY_WORD_LEFT;
			}
			else nskey = NS_KEY_LEFT;
		break;
		case RAWKEY_CRSRRIGHT:
			if(ie->ie_Qualifier & NSA_QUAL_SHIFT) {
				nskey = NS_KEY_LINE_END;
			}else if(ie->ie_Qualifier & NSA_QUAL_ALT) {
				nskey = NS_KEY_WORD_RIGHT;
			}
			else nskey = NS_KEY_RIGHT;
		break;
		case RAWKEY_ESC:
			nskey = NS_KEY_ESCAPE;
		break;
		case RAWKEY_PAGEUP:
			nskey = NS_KEY_PAGE_UP;
		break;
		case RAWKEY_PAGEDOWN:
			nskey = NS_KEY_PAGE_DOWN;
		break;
		case RAWKEY_HOME:
			nskey = NS_KEY_TEXT_START;
		break;
		case RAWKEY_END:
			nskey = NS_KEY_TEXT_END;
		break;
		case RAWKEY_BACKSPACE:
			if(ie->ie_Qualifier & NSA_QUAL_SHIFT) {
				nskey = NS_KEY_DELETE_LINE_START;
			} else {
				nskey = NS_KEY_DELETE_LEFT;
			}
		break;
		case RAWKEY_DEL:
			if(ie->ie_Qualifier & NSA_QUAL_SHIFT) {
				nskey = NS_KEY_DELETE_LINE_END;
			} else {
				nskey = NS_KEY_DELETE_RIGHT;
			}
		break;
		case RAWKEY_TAB:
			if(ie->ie_Qualifier & NSA_QUAL_SHIFT) {
				nskey = NS_KEY_SHIFT_TAB;
			} else {
				nskey = NS_KEY_TAB;
			}
		break;
		case RAWKEY_F5:
		case RAWKEY_F8:
		case RAWKEY_F9:
		case RAWKEY_F10:
		case RAWKEY_F12:
		case RAWKEY_HELP:
			// don't translate
			nskey = keycode;
		break;
		default:
			if((chars = MapRawKey(ie,buffer,20,NULL)) > 0) {
				if(utf8_from_local_encoding(buffer, chars, &utf8) != NSERROR_OK) return 0;
				nskey = utf8_to_ucs4(utf8, utf8_char_byte_length(utf8));
				free(utf8);

				if(ie->ie_Qualifier & IEQUALIFIER_RCOMMAND) {
					switch(nskey) {
						case 'a':
							nskey = NS_KEY_SELECT_ALL;
						break;
						case 'c':
							nskey = NS_KEY_COPY_SELECTION;
						break;
						case 'v':
							nskey = NS_KEY_PASTE;
						break;
						case 'x':
							nskey = NS_KEY_CUT_SELECTION;
						break;
						case 'y':
							nskey = NS_KEY_REDO;
						break;
						case 'z':
							nskey = NS_KEY_UNDO;
						break;
					}
				}
			}
		break;
	}

	return nskey;
}

int ami_gui_get_quals(Object *win_obj)
{
	uint32 quals = 0;
	int key_state = 0;
#ifdef __amigaos4__
	GetAttr(WINDOW_Qualifier, win_obj, (uint32 *)&quals);
#else
#warning qualifier needs fixing for OS3
#endif

	if(quals & NSA_QUAL_SHIFT) {
		key_state |= BROWSER_MOUSE_MOD_1;
	}

	if(quals & IEQUALIFIER_CONTROL) {
		key_state |= BROWSER_MOUSE_MOD_2;
	}

	if(quals & NSA_QUAL_ALT) {
		key_state |= BROWSER_MOUSE_MOD_3;
	}

	return key_state;
}

static void ami_update_quals(struct gui_window_2 *gwin)
{
	gwin->key_state = ami_gui_get_quals(gwin->objects[OID_MAIN]);
}

/* exported interface documented in amiga/gui.h */
nserror ami_gui_get_space_box(Object *obj, struct IBox **bbox)
{
#ifdef __amigaos4__
	if(LIB_IS_AT_LEAST((struct Library *)SpaceBase, 53, 6)) {
		*bbox = malloc(sizeof(struct IBox));
		if(*bbox == NULL) return NSERROR_NOMEM;
		GetAttr(SPACE_RenderBox, obj, (ULONG *)*bbox);
	} else
#endif
	{
		GetAttr(SPACE_AreaBox, obj, (ULONG *)bbox);
	}

	return NSERROR_OK;
}

/* exported interface documented in amiga/gui.h */
void ami_gui_free_space_box(struct IBox *bbox)
{
#ifdef __amigaos4__
	if(LIB_IS_AT_LEAST((struct Library *)SpaceBase, 53, 6)) {
		free(bbox);
	}
#endif
}

static bool ami_spacebox_to_ns_coords(struct gui_window_2 *gwin,
		int *restrict x, int *restrict y, int space_x, int space_y)
{
	int ns_x = space_x;
	int ns_y = space_y;

	ns_x += gwin->gw->scrollx;
	ns_y += gwin->gw->scrolly;

	*x = ns_x;
	*y = ns_y;

	return true;	
}

bool ami_mouse_to_ns_coords(struct gui_window_2 *gwin, int *restrict x, int *restrict y,
	int mouse_x, int mouse_y)
{
	int ns_x, ns_y;
	struct IBox *bbox;

	if(mouse_x == -1) mouse_x = gwin->win->MouseX;
	if(mouse_y == -1) mouse_y = gwin->win->MouseY;

	if(ami_gui_get_space_box((Object *)gwin->objects[GID_BROWSER], &bbox) == NSERROR_OK) {
		ns_x = (ULONG)(mouse_x - bbox->Left);
		ns_y = (ULONG)(mouse_y - bbox->Top);

		if((ns_x < 0) || (ns_x > bbox->Width) || (ns_y < 0) || (ns_y > bbox->Height))
			return false;

		ami_gui_free_space_box(bbox);
	} else {
		amiga_warn_user("NoMemory", "");
		return false;
	}

	return ami_spacebox_to_ns_coords(gwin, x, y, ns_x, ns_y);
}

static void ami_gui_scroll_internal(struct gui_window_2 *gwin, int xs, int ys)
{
	struct IBox *bbox;
	int x, y;
	struct rect rect;

	if(ami_mouse_to_ns_coords(gwin, &x, &y, -1, -1) == true)
	{
		if(browser_window_scroll_at_point(gwin->gw->bw, x, y, xs, ys) == false)
		{
			int width, height;

			gui_window_get_scroll(gwin->gw,
				&gwin->gw->scrollx,
				&gwin->gw->scrolly);

			if(ami_gui_get_space_box((Object *)gwin->objects[GID_BROWSER], &bbox) != NSERROR_OK) {
				amiga_warn_user("NoMemory", "");
				return;
			}

			browser_window_get_extents(gwin->gw->bw, false, &width, &height);

			switch(xs)
			{
				case SCROLL_PAGE_UP:
					xs = gwin->gw->scrollx - bbox->Width;
				break;

				case SCROLL_PAGE_DOWN:
					xs = gwin->gw->scrollx + bbox->Width;
				break;

				case SCROLL_TOP:
					xs = 0;
				break;

				case SCROLL_BOTTOM:
					xs = width;
				break;

				default:
					xs += gwin->gw->scrollx;
				break;
			}

			switch(ys)
			{
				case SCROLL_PAGE_UP:
					ys = gwin->gw->scrolly - bbox->Height;
				break;

				case SCROLL_PAGE_DOWN:
					ys = gwin->gw->scrolly + bbox->Height;
				break;

				case SCROLL_TOP:
					ys = 0;
				break;

				case SCROLL_BOTTOM:
					ys = height;
				break;

				default:
					ys += gwin->gw->scrolly;
				break;
			}

			ami_gui_free_space_box(bbox);
			rect.x0 = rect.x1 = xs;
			rect.y0 = rect.y1 = ys;
			gui_window_set_scroll(gwin->gw, &rect);
		}
	}
}

static struct IBox *ami_ns_rect_to_ibox(struct gui_window_2 *gwin, const struct rect *rect)
{
	struct IBox *bbox, *ibox;

	ibox = malloc(sizeof(struct IBox));
	if(ibox == NULL) return NULL;

	if(ami_gui_get_space_box((Object *)gwin->objects[GID_BROWSER], &bbox) != NSERROR_OK) {
		free(ibox);
		amiga_warn_user("NoMemory", "");
		return NULL;
	}

	ibox->Left = gwin->win->MouseX + (rect->x0);
	ibox->Top = gwin->win->MouseY + (rect->y0);

	ibox->Width = (rect->x1 - rect->x0);
	ibox->Height = (rect->y1 - rect->y0);

	if(ibox->Left < bbox->Left) ibox->Left = bbox->Left;
	if(ibox->Top < bbox->Top) ibox->Top = bbox->Top;

	if((ibox->Left > (bbox->Left + bbox->Width)) ||
		(ibox->Top > (bbox->Top + bbox->Height)) ||
		(ibox->Width < 0) || (ibox->Height < 0))
	{
		free(ibox);
		ami_gui_free_space_box(bbox);
		return NULL;
	}

	ami_gui_free_space_box(bbox);
	return ibox;
}

static void ami_gui_trap_mouse(struct gui_window_2 *gwin)
{
#ifdef __amigaos4__
	switch(gwin->drag_op)
	{
		case GDRAGGING_NONE:
		case GDRAGGING_SCROLLBAR:
		case GDRAGGING_OTHER:
		break;

		default:
			if(gwin->ptr_lock)
			{
				SetWindowAttrs(gwin->win, WA_GrabFocus, 10,
					WA_MouseLimits, gwin->ptr_lock, TAG_DONE);
			}
		break;
	}
#endif
}

static void ami_gui_menu_update_all(void)
{
	struct nsObject *node;
	struct nsObject *nnode;
	struct gui_window_2 *gwin;

	if(IsMinListEmpty(window_list))	return;

	node = (struct nsObject *)GetHead((struct List *)window_list);

	do {
		nnode=(struct nsObject *)GetSucc((struct Node *)node);
		gwin = node->objstruct;

		if(node->Type == AMINS_WINDOW)
		{
			ami_gui_menu_update_checked(gwin);
		}
	} while((node = nnode));
}

/**
 * Find the current dimensions of a amiga browser window content area.
 *
 * \param gw The gui window to measure content area of.
 * \param width receives width of window
 * \param height receives height of window
 * \return NSERROR_OK on sucess and width and height updated
 *          else error code.
 */
static nserror
gui_window_get_dimensions(struct gui_window *gw,
			  int *restrict width,
			  int *restrict height)
{
	struct IBox *bbox;
	nserror res;

	res = ami_gui_get_space_box((Object *)gw->shared->objects[GID_BROWSER],
				    &bbox);
	if (res != NSERROR_OK) {
		amiga_warn_user("NoMemory", "");
		return res;
	}

	*width = bbox->Width;
	*height = bbox->Height;

	ami_gui_free_space_box(bbox);

	return NSERROR_OK;
}

/* Add a horizontal scroller, if not already present
 * Returns true if changed, false otherwise */
static bool ami_gui_hscroll_add(struct gui_window_2 *gwin)
{
	struct TagItem attrs[2];

	if(gwin->objects[GID_HSCROLL] != NULL) return false;

	attrs[0].ti_Tag = CHILD_MinWidth;
	attrs[0].ti_Data = 0;
	attrs[1].ti_Tag = TAG_DONE;
	attrs[1].ti_Data = 0;

	gwin->objects[GID_HSCROLL] = ScrollerObj,
					GA_ID, GID_HSCROLL,
					GA_RelVerify, TRUE,
					SCROLLER_Orientation, SORIENT_HORIZ,
					ICA_TARGET, ICTARGET_IDCMP,
				ScrollerEnd;
#ifdef __amigaos4__
	IDoMethod(gwin->objects[GID_HSCROLLLAYOUT], LM_ADDCHILD,
			gwin->win, gwin->objects[GID_HSCROLL], attrs);
#else
	SetAttrs(gwin->objects[GID_HSCROLLLAYOUT],
			LAYOUT_AddChild, gwin->objects[GID_HSCROLL], TAG_MORE, &attrs);
#endif
	return true;
}

/* Remove the horizontal scroller, if present */
static bool ami_gui_hscroll_remove(struct gui_window_2 *gwin)
{
	if(gwin->objects[GID_HSCROLL] == NULL) return false;

#ifdef __amigaos4__
	IDoMethod(gwin->objects[GID_HSCROLLLAYOUT], LM_REMOVECHILD,
			gwin->win, gwin->objects[GID_HSCROLL]);
#else
	SetAttrs(gwin->objects[GID_HSCROLLLAYOUT], LAYOUT_RemoveChild, gwin->objects[GID_HSCROLL], TAG_DONE);
#endif

	gwin->objects[GID_HSCROLL] = NULL;

	return true;
}

/* Add a vertical scroller, if not already present
 * Returns true if changed, false otherwise */
static bool ami_gui_vscroll_add(struct gui_window_2 *gwin)
{
	struct TagItem attrs[2];

	if(gwin->objects[GID_VSCROLL] != NULL) return false;

	attrs[0].ti_Tag = CHILD_MinWidth;
	attrs[0].ti_Data = 0;
	attrs[1].ti_Tag = TAG_DONE;
	attrs[1].ti_Data = 0;

	gwin->objects[GID_VSCROLL] = ScrollerObj,
					GA_ID, GID_VSCROLL,
					GA_RelVerify, TRUE,
					ICA_TARGET, ICTARGET_IDCMP,
				ScrollerEnd;
#ifdef __amigaos4__
	IDoMethod(gwin->objects[GID_VSCROLLLAYOUT], LM_ADDCHILD,
			gwin->win, gwin->objects[GID_VSCROLL], attrs);
#else
	SetAttrs(gwin->objects[GID_VSCROLLLAYOUT],
			LAYOUT_AddChild, gwin->objects[GID_VSCROLL], TAG_MORE, &attrs);
#endif
	return true;
}

/* Remove the vertical scroller, if present */
static bool ami_gui_vscroll_remove(struct gui_window_2 *gwin)
{
	if(gwin->objects[GID_VSCROLL] == NULL) return false;

#ifdef __amigaos4__
	IDoMethod(gwin->objects[GID_VSCROLLLAYOUT], LM_REMOVECHILD,
			gwin->win, gwin->objects[GID_VSCROLL]);
#else
	SetAttrs(gwin->objects[GID_VSCROLLLAYOUT], LAYOUT_RemoveChild, gwin->objects[GID_VSCROLL], TAG_DONE);
#endif

	gwin->objects[GID_VSCROLL] = NULL;

	return true;
}

/**
 * Check the scroll bar requirements for a browser window, and add/remove
 * the vertical scroller as appropriate.  This should be the main entry
 * point used to perform this task.
 *
 * \param  gwin      "Shared" GUI window to check the state of
 */
static void ami_gui_scroller_update(struct gui_window_2 *gwin)
{
	int h = 1, w = 1, wh = 0, ww = 0;
	bool rethinkv = false;
	bool rethinkh = false;
	browser_scrolling hscroll = BW_SCROLLING_YES;
	browser_scrolling vscroll = BW_SCROLLING_YES;

	browser_window_get_scrollbar_type(gwin->gw->bw, &hscroll, &vscroll);

	if(browser_window_is_frameset(gwin->gw->bw) == true) {
		rethinkv = ami_gui_vscroll_remove(gwin);
		rethinkh = ami_gui_hscroll_remove(gwin);
	} else {
		if((browser_window_get_extents(gwin->gw->bw, false, &w, &h) == NSERROR_OK)) {
			gui_window_get_dimensions(gwin->gw, &ww, &wh);
		}

		if(vscroll == BW_SCROLLING_NO) {
			rethinkv = ami_gui_vscroll_remove(gwin);
		} else {
			if (h > wh) rethinkv = ami_gui_vscroll_add(gwin);
				else rethinkv = ami_gui_vscroll_remove(gwin);
		}

		if(hscroll == BW_SCROLLING_NO) {
			rethinkh = ami_gui_hscroll_remove(gwin);
		} else {
			if (w > ww) rethinkh = ami_gui_hscroll_add(gwin);
				else rethinkh = ami_gui_hscroll_remove(gwin);
		}
	}

	if(rethinkv || rethinkh) {
		FlushLayoutDomainCache((struct Gadget *)gwin->objects[GID_MAIN]);
		RethinkLayout((struct Gadget *)gwin->objects[GID_MAIN],
				gwin->win, NULL, TRUE);
		browser_window_schedule_reformat(gwin->gw->bw);
	}
}

/* For future use
static void ami_gui_console_log_clear(struct gui_window *g)
{
	if(g->shared->objects[GID_LOG] != NULL) {
		SetGadgetAttrs((struct Gadget *)g->shared->objects[GID_LOG], g->shared->win, NULL,
						LISTBROWSER_Labels, NULL,
						TAG_DONE);
	}
	
	FreeListBrowserList(&g->loglist);

	NewList(&g->loglist);

	if(g->shared->objects[GID_LOG] != NULL) {
		SetGadgetAttrs((struct Gadget *)g->shared->objects[GID_LOG], g->shared->win, NULL,
						LISTBROWSER_Labels, &g->loglist,
						TAG_DONE);
	}
}
*/

static void ami_gui_console_log_add(struct gui_window *g)
{
	struct TagItem attrs[2];

	if(g->shared->objects[GID_LOG] != NULL) return;

	attrs[0].ti_Tag = CHILD_MinHeight;
	attrs[0].ti_Data = 50;
	attrs[1].ti_Tag = TAG_DONE;
	attrs[1].ti_Data = 0;

	g->shared->objects[GID_LOG] = ListBrowserObj,
					GA_ID, GID_LOG,
					LISTBROWSER_ColumnInfo, g->logcolumns,
					LISTBROWSER_ColumnTitles, TRUE,
					LISTBROWSER_Labels, &g->loglist,
					LISTBROWSER_Striping, LBS_ROWS,
				ListBrowserEnd;

#ifdef __amigaos4__
	IDoMethod(g->shared->objects[GID_LOGLAYOUT], LM_ADDCHILD,
			g->shared->win, g->shared->objects[GID_LOG], NULL);
#else
	SetAttrs(g->shared->objects[GID_LOGLAYOUT],
		LAYOUT_AddChild, g->shared->objects[GID_LOG], TAG_MORE, &attrs);
#endif

	FlushLayoutDomainCache((struct Gadget *)g->shared->objects[GID_MAIN]);

	RethinkLayout((struct Gadget *)g->shared->objects[GID_MAIN],
			g->shared->win, NULL, TRUE);
		
	ami_schedule_redraw(g->shared, true);
}

static void ami_gui_console_log_remove(struct gui_window *g)
{
	if(g->shared->objects[GID_LOG] == NULL) return;

#ifdef __amigaos4__
	IDoMethod(g->shared->objects[GID_LOGLAYOUT], LM_REMOVECHILD,
			g->shared->win, g->shared->objects[GID_LOG]);
#else
	SetAttrs(g->shared->objects[GID_LOGLAYOUT],
		LAYOUT_RemoveChild, g->shared->objects[GID_LOG], TAG_DONE);
#endif

	g->shared->objects[GID_LOG] = NULL;

	FlushLayoutDomainCache((struct Gadget *)g->shared->objects[GID_MAIN]);

	RethinkLayout((struct Gadget *)g->shared->objects[GID_MAIN],
			g->shared->win, NULL, TRUE);

	ami_schedule_redraw(g->shared, true);
}

static bool ami_gui_console_log_toggle(struct gui_window *g)
{
	if(g->shared->objects[GID_LOG] == NULL) {
		ami_gui_console_log_add(g);
		return true;
	} else {
		ami_gui_console_log_remove(g);
		return false;
	}
}

static void ami_gui_console_log_switch(struct gui_window *g)
{
	if(g->shared->objects[GID_LOG] == NULL) return;

	RefreshSetGadgetAttrs((struct Gadget *)g->shared->objects[GID_LOG], g->shared->win, NULL,
					LISTBROWSER_ColumnInfo, g->logcolumns,
					LISTBROWSER_Labels, &g->loglist,
					TAG_DONE);
}

static void
gui_window_console_log(struct gui_window *g,
		       browser_window_console_source src,
		       const char *msg,
		       size_t msglen,
		       browser_window_console_flags flags)
{
	bool foldable = !!(flags & BW_CS_FLAG_FOLDABLE);
	const char *src_text;
	const char *level_text;
	struct Node *node;
	ULONG style = 0;
	ULONG fgpen = TEXTPEN;
	ULONG lbflags = LBFLG_READONLY;
	char timestamp[256];
	time_t now = time(NULL);
	struct tm *timedata = localtime(&now);

	strftime(timestamp, 256, "%c", timedata);

	if(foldable) lbflags |= LBFLG_HASCHILDREN;

	switch (src) {
	case BW_CS_INPUT:
		src_text = "client-input";
		break;
	case BW_CS_SCRIPT_ERROR:
		src_text = "scripting-error";
		break;
	case BW_CS_SCRIPT_CONSOLE:
		src_text = "scripting-console";
		break;
	default:
		assert(0 && "Unknown scripting source");
		src_text = "unknown";
		break;
	}

	switch (flags & BW_CS_FLAG_LEVEL_MASK) {
	case BW_CS_FLAG_LEVEL_DEBUG:
		level_text = "DEBUG";
		fgpen = DISABLEDTEXTPEN;
		lbflags |= LBFLG_CUSTOMPENS;
		break;
	case BW_CS_FLAG_LEVEL_LOG:
		level_text = "LOG";
		fgpen = DISABLEDTEXTPEN;
		lbflags |= LBFLG_CUSTOMPENS;
		break;
	case BW_CS_FLAG_LEVEL_INFO:
		level_text = "INFO";
		break;
	case BW_CS_FLAG_LEVEL_WARN:
		level_text = "WARN";
		break;
	case BW_CS_FLAG_LEVEL_ERROR:
		level_text = "ERROR";
		style = FSF_BOLD;
		break;
	default:
		assert(0 && "Unknown console logging level");
		level_text = "unknown";
		break;
	}

	if(g->shared->objects[GID_LOG] != NULL) {
		SetGadgetAttrs((struct Gadget *)g->shared->objects[GID_LOG], g->shared->win, NULL,
						LISTBROWSER_Labels, NULL,
						TAG_DONE);
	}

	/* Add log entry to list irrespective of whether the log is open. */
	if((node = AllocListBrowserNode(4,
				LBNA_Flags, lbflags,
				LBNA_Column, 0,
					LBNCA_SoftStyle, style,
					LBNCA_FGPen, fgpen,
					LBNCA_CopyText, TRUE,
					LBNCA_Text, timestamp,
				LBNA_Column, 1,
					LBNCA_SoftStyle, style,
					LBNCA_FGPen, fgpen,
					LBNCA_CopyText, TRUE,
					LBNCA_Text, src_text,
				LBNA_Column, 2,
					LBNCA_SoftStyle, style,
					LBNCA_FGPen, fgpen,
					LBNCA_CopyText, TRUE,
					LBNCA_Text, level_text,
				LBNA_Column, 3,
					LBNCA_SoftStyle, style,
					LBNCA_FGPen, fgpen,
					LBNCA_CopyText, TRUE,
					LBNCA_Text, msg,
				TAG_DONE))) {
		AddTail(&g->loglist, node);
	}

	if(g->shared->objects[GID_LOG] != NULL) {
		RefreshSetGadgetAttrs((struct Gadget *)g->shared->objects[GID_LOG], g->shared->win, NULL,
						LISTBROWSER_Labels, &g->loglist,
						TAG_DONE);
	}

#ifdef __amigaos4__
	DebugPrintF("NETSURF: CONSOLE_LOG SOURCE %s %sFOLDABLE %s %.*s\n",
	      src_text, foldable ? "" : "NOT-", level_text,
	      (int)msglen, msg);
#endif
}


/**
 * function to add retrieved favicon to gui
 */
static void gui_window_set_icon(struct gui_window *g, struct hlcache_handle *icon)
{
	struct BitMap *bm = NULL;
	struct IBox *bbox;
	struct bitmap *icon_bitmap = NULL;

	if(nsoption_bool(kiosk_mode) == true) return;
	if(!g) return;

	if ((icon != NULL) && ((icon_bitmap = content_get_bitmap(icon)) != NULL))
	{
		bm = ami_bitmap_get_native(icon_bitmap, 16, 16, ami_plot_screen_is_palettemapped(),
					g->shared->win->RPort->BitMap);
	}

	if(g == g->shared->gw) {
		RefreshGList((struct Gadget *)g->shared->objects[GID_ICON],
					g->shared->win, NULL, 1);

		if(bm)
		{
			ULONG tag, tag_data, minterm;

			if(ami_plot_screen_is_palettemapped() == false) {
				tag = BLITA_UseSrcAlpha;
				tag_data = !amiga_bitmap_get_opaque(icon_bitmap);
				minterm = 0xc0;
			} else {
				tag = BLITA_MaskPlane;
				tag_data = (ULONG)ami_bitmap_get_mask(icon_bitmap, 16, 16, bm);
				minterm = MINTERM_SRCMASK;
			}

			if(ami_gui_get_space_box((Object *)g->shared->objects[GID_ICON], &bbox) != NSERROR_OK) {
				amiga_warn_user("NoMemory", "");
				return;
			}

			EraseRect(g->shared->win->RPort, bbox->Left, bbox->Top,
						bbox->Left + 16, bbox->Top + 16);

#ifdef __amigaos4__
			BltBitMapTags(BLITA_SrcX, 0,
						BLITA_SrcY, 0,
						BLITA_DestX, bbox->Left,
						BLITA_DestY, bbox->Top,
						BLITA_Width, 16,
						BLITA_Height, 16,
						BLITA_Source, bm,
						BLITA_Dest, g->shared->win->RPort,
						BLITA_SrcType, BLITT_BITMAP,
						BLITA_DestType, BLITT_RASTPORT,
						BLITA_Minterm, minterm,
						tag, tag_data,
						TAG_DONE);
#else
			if(tag_data) {
				BltMaskBitMapRastPort(bm, 0, 0, g->shared->win->RPort,
							bbox->Left, bbox->Top, 16, 16, minterm, tag_data);
			} else {
				BltBitMapRastPort(bm, 0, 0, g->shared->win->RPort,
							bbox->Left, bbox->Top, 16, 16, 0xc0);
			}
#endif
			ami_gui_free_space_box(bbox);
		}
	}

	g->favicon = icon;
}

static void ami_gui_refresh_favicon(void *p)
{
	struct gui_window_2 *gwin = (struct gui_window_2 *)p;
	gui_window_set_icon(gwin->gw, gwin->gw->favicon);
}

/* Gets the size that border gadget 1 (status) needs to be.
 * Returns the width of the size gadget as a convenience.
 */
#ifdef __amigaos4__
static ULONG ami_get_border_gadget_size(struct gui_window_2 *gwin,
		ULONG *restrict width, ULONG *restrict height)
{
	static ULONG sz_gad_width = 0;
	static ULONG sz_gad_height = 0;
	ULONG available_width;

	if((sz_gad_width == 0) || (sz_gad_height == 0)) {
		struct DrawInfo *dri = GetScreenDrawInfo(scrn);
		GetGUIAttrs(NULL, dri,
			GUIA_SizeGadgetWidth, &sz_gad_width,
			GUIA_SizeGadgetHeight, &sz_gad_height,
		TAG_DONE);
		FreeScreenDrawInfo(scrn, dri);
	}
	available_width = gwin->win->Width - scrn->WBorLeft - sz_gad_width;

	*width = available_width;
	*height = sz_gad_height;

	return sz_gad_width;
}
#endif

static void ami_set_border_gadget_size(struct gui_window_2 *gwin)
{
#ifdef __amigaos4__
	/* Reset gadget widths according to new calculation */
	ULONG size1, size2;

	ami_get_border_gadget_size(gwin, &size1, &size2);

	RefreshSetGadgetAttrs((struct Gadget *)(APTR)gwin->objects[GID_STATUS],
			gwin->win, NULL,
			GA_Width, size1,
			TAG_DONE);

	RefreshWindowFrame(gwin->win);
#endif
}

static BOOL ami_handle_msg(void)
{
	struct ami_generic_window *w = NULL;
	struct nsObject *node;
	struct nsObject *nnode;
	BOOL win_closed = FALSE;

	if(IsMinListEmpty(window_list)) {
		/* no windows in list, so NetSurf should not be running */
		ami_try_quit();
		return FALSE;
	}

	node = (struct nsObject *)GetHead((struct List *)window_list);

	do {
		nnode=(struct nsObject *)GetSucc((struct Node *)node);

		w = node->objstruct;
		if(w == NULL) continue;

		if(w->tbl->event != NULL) {
			if((win_closed = w->tbl->event(w))) {
				if((node->Type != AMINS_GUIOPTSWINDOW) ||
					((node->Type == AMINS_GUIOPTSWINDOW) && (scrn != NULL))) {
					ami_try_quit();
					break;
				}
			} else {
				node = nnode;
				continue;
			}
		}
	} while((node = nnode));

	if(ami_gui_menu_quit_selected() == true) {
		ami_quit_netsurf();
	}
	
	if(ami_gui_menu_get_check_toggled() == true) {
		ami_gui_menu_update_all();
	}

	return win_closed;
}

static BOOL ami_gui_event(void *w)
{
	struct gui_window_2 *gwin = (struct gui_window_2 *)w;
	ULONG result, storage = 0, x, y, xs, ys, width = 800, height = 600;
	uint16 code;
	struct IBox *bbox;
	struct InputEvent *ie;
	struct Node *tabnode;
	int nskey;
	struct timeval curtime;
	static int drag_x_move = 0, drag_y_move = 0;
	char *utf8 = NULL;
	nsurl *url;
	BOOL win_closed = FALSE;

	while((result = RA_HandleInput(gwin->objects[OID_MAIN], &code)) != WMHI_LASTMSG) {
        switch(result & WMHI_CLASSMASK) // class
	   	{
			case WMHI_MOUSEMOVE:
				ami_gui_trap_mouse(gwin); /* re-assert mouse area */

				drag_x_move = 0;
				drag_y_move = 0;

				if(ami_gui_get_space_box((Object *)gwin->objects[GID_BROWSER], &bbox) != NSERROR_OK) {
					amiga_warn_user("NoMemory", "");
					break;
				}

				x = (ULONG)((gwin->win->MouseX - bbox->Left));
				y = (ULONG)((gwin->win->MouseY - bbox->Top));

				ami_get_hscroll_pos(gwin, (ULONG *)&xs);
				ami_get_vscroll_pos(gwin, (ULONG *)&ys);

				x += xs;
				y += ys;

				width=bbox->Width;
				height=bbox->Height;

				if(gwin->mouse_state & BROWSER_MOUSE_DRAG_ON)
				{
					if(ami_drag_icon_move() == TRUE) {
						if((gwin->win->MouseX < bbox->Left) &&
							((gwin->win->MouseX - bbox->Left) > -AMI_DRAG_THRESHOLD))
							drag_x_move = gwin->win->MouseX - bbox->Left;
						if((gwin->win->MouseX > (bbox->Left + bbox->Width)) &&
							((gwin->win->MouseX - (bbox->Left + bbox->Width)) < AMI_DRAG_THRESHOLD))
							drag_x_move = gwin->win->MouseX - (bbox->Left + bbox->Width);
						if((gwin->win->MouseY < bbox->Top) &&
							((gwin->win->MouseY - bbox->Top) > -AMI_DRAG_THRESHOLD))
							drag_y_move = gwin->win->MouseY - bbox->Top;
						if((gwin->win->MouseY > (bbox->Top + bbox->Height)) &&
							((gwin->win->MouseY - (bbox->Top + bbox->Height)) < AMI_DRAG_THRESHOLD))
							drag_y_move = gwin->win->MouseY - (bbox->Top + bbox->Height);
					}
				}

				ami_gui_free_space_box(bbox);

				if((x>=xs) && (y>=ys) && (x<width+xs) && (y<height+ys))
				{
					ami_update_quals(gwin);

					if(gwin->mouse_state & BROWSER_MOUSE_PRESS_1)
					{
						browser_window_mouse_track(gwin->gw->bw,BROWSER_MOUSE_DRAG_1 | gwin->key_state,x,y);
						gwin->mouse_state = BROWSER_MOUSE_HOLDING_1 | BROWSER_MOUSE_DRAG_ON;
					}
					else if(gwin->mouse_state & BROWSER_MOUSE_PRESS_2)
					{
						browser_window_mouse_track(gwin->gw->bw,BROWSER_MOUSE_DRAG_2 | gwin->key_state,x,y);
						gwin->mouse_state = BROWSER_MOUSE_HOLDING_2 | BROWSER_MOUSE_DRAG_ON;
					}
					else
					{
						browser_window_mouse_track(gwin->gw->bw,gwin->mouse_state | gwin->key_state,x,y);
					}
				} else {
					if(!gwin->mouse_state) ami_set_pointer(gwin, GUI_POINTER_DEFAULT, true);
				}
			break;

			case WMHI_MOUSEBUTTONS:
				if(ami_gui_get_space_box((Object *)gwin->objects[GID_BROWSER], &bbox) != NSERROR_OK) {
					amiga_warn_user("NoMemory", "");
					return FALSE;
				}

				x = (ULONG)(gwin->win->MouseX - bbox->Left);
				y = (ULONG)(gwin->win->MouseY - bbox->Top);

				ami_get_hscroll_pos(gwin, (ULONG *)&xs);
				ami_get_vscroll_pos(gwin, (ULONG *)&ys);

				x += xs;
				y += ys;

				width=bbox->Width;
				height=bbox->Height;

				ami_gui_free_space_box(bbox);

				ami_update_quals(gwin);

				if((x>=xs) && (y>=ys) && (x<width+xs) && (y<height+ys))
				{
					//code = code>>16;
					switch(code)
					{
						case SELECTDOWN:
							browser_window_mouse_click(gwin->gw->bw,BROWSER_MOUSE_PRESS_1 | gwin->key_state,x,y);
							gwin->mouse_state=BROWSER_MOUSE_PRESS_1;
						break;
						case MIDDLEDOWN:
							browser_window_mouse_click(gwin->gw->bw,BROWSER_MOUSE_PRESS_2 | gwin->key_state,x,y);
							gwin->mouse_state=BROWSER_MOUSE_PRESS_2;
						break;
					}
				}

				if(x<xs) x=xs;
				if(y<ys) y=ys;
				if(x>=width+xs) x=width+xs-1;
				if(y>=height+ys) y=height+ys-1;

				switch(code)
				{
					case SELECTUP:
						if(gwin->mouse_state & BROWSER_MOUSE_PRESS_1)
						{
							CurrentTime((ULONG *)&curtime.tv_sec, (ULONG *)&curtime.tv_usec);

							gwin->mouse_state = BROWSER_MOUSE_CLICK_1;

							if(gwin->lastclick.tv_sec)
							{
								if(DoubleClick(gwin->lastclick.tv_sec,
											gwin->lastclick.tv_usec,
											curtime.tv_sec, curtime.tv_usec)) {
									if(gwin->prev_mouse_state & BROWSER_MOUSE_DOUBLE_CLICK) {
										gwin->mouse_state |= BROWSER_MOUSE_TRIPLE_CLICK;
									} else {
										gwin->mouse_state |= BROWSER_MOUSE_DOUBLE_CLICK;
									}
								}
							}

							browser_window_mouse_click(gwin->gw->bw,
								gwin->mouse_state | gwin->key_state,x,y);

							if(gwin->mouse_state & BROWSER_MOUSE_TRIPLE_CLICK)
							{
								gwin->lastclick.tv_sec = 0;
								gwin->lastclick.tv_usec = 0;
							}
							else
							{
								gwin->lastclick.tv_sec = curtime.tv_sec;
								gwin->lastclick.tv_usec = curtime.tv_usec;
							}
						}
						else
						{
							browser_window_mouse_track(gwin->gw->bw, 0, x, y);
						}
						gwin->prev_mouse_state = gwin->mouse_state;
						gwin->mouse_state=0;
					break;

					case MIDDLEUP:
						if(gwin->mouse_state & BROWSER_MOUSE_PRESS_2)
						{
							CurrentTime((ULONG *)&curtime.tv_sec, (ULONG *)&curtime.tv_usec);

							gwin->mouse_state = BROWSER_MOUSE_CLICK_2;

							if(gwin->lastclick.tv_sec)
							{
								if(DoubleClick(gwin->lastclick.tv_sec,
											gwin->lastclick.tv_usec,
											curtime.tv_sec, curtime.tv_usec)) {
									if(gwin->prev_mouse_state & BROWSER_MOUSE_DOUBLE_CLICK) {
										gwin->mouse_state |= BROWSER_MOUSE_TRIPLE_CLICK;
									} else {
										gwin->mouse_state |= BROWSER_MOUSE_DOUBLE_CLICK;
									}
								}
							}

							browser_window_mouse_click(gwin->gw->bw,
								gwin->mouse_state | gwin->key_state,x,y);

							if(gwin->mouse_state & BROWSER_MOUSE_TRIPLE_CLICK)
							{
								gwin->lastclick.tv_sec = 0;
								gwin->lastclick.tv_usec = 0;
							}
							else
							{
								gwin->lastclick.tv_sec = curtime.tv_sec;
								gwin->lastclick.tv_usec = curtime.tv_usec;
							}
						}
						else
						{
							browser_window_mouse_track(gwin->gw->bw, 0, x, y);
						}
						gwin->prev_mouse_state = gwin->mouse_state;
						gwin->mouse_state=0;
					break;
#ifdef __amigaos4__
					case SIDEUP:
						ami_gui_history(gwin, true);
					break;

					case EXTRAUP:
						ami_gui_history(gwin, false);
					break;
#endif
				}

				if(ami_drag_has_data() && !gwin->mouse_state)
					ami_drag_save(gwin->win);
			break;

			case WMHI_GADGETUP:
				switch(result & WMHI_GADGETMASK)
				{
					case GID_TABS:
						if(gwin->objects[GID_TABS] == NULL) break;
						if(ClickTabBase->lib_Version >= 53) {
							GetAttrs(gwin->objects[GID_TABS],
								CLICKTAB_NodeClosed, &tabnode, TAG_DONE);
						} else {
							tabnode = NULL;
						}

						if(tabnode) {
							struct gui_window *closedgw;

							GetClickTabNodeAttrs(tabnode,
								TNA_UserData, &closedgw,
								TAG_DONE);

							browser_window_destroy(closedgw->bw);
						} else {
							ami_switch_tab(gwin, true);
						}
					break;

					case GID_CLOSETAB:
						browser_window_destroy(gwin->gw->bw);
					break;

					case GID_ADDTAB:
						ami_gui_new_blank_tab(gwin);
					break;

					case GID_URL:
					{
						nserror ret;
						nsurl *url;
						GetAttr(STRINGA_TextVal,
							(Object *)gwin->objects[GID_URL],
							(ULONG *)&storage);
						utf8 = ami_to_utf8_easy((const char *)storage);

						ret = search_web_omni(utf8, SEARCH_WEB_OMNI_NONE, &url);
						ami_utf8_free(utf8);
						if (ret == NSERROR_OK) {
								browser_window_navigate(gwin->gw->bw,
										url,
										NULL,
										BW_NAVIGATE_HISTORY,
										NULL,
										NULL,
										NULL);
								nsurl_unref(url);
						}
						if (ret != NSERROR_OK) {
							amiga_warn_user(messages_get_errorcode(ret), 0);
						}
					}
					break;

					case GID_TOOLBARLAYOUT:
						/* Need fixing: never gets here */
						search_web_select_provider(-1);
					break;

					case GID_SEARCH_ICON:
						GetAttr(CHOOSER_Selected, gwin->objects[GID_SEARCH_ICON], (ULONG *)&storage);
						search_web_select_provider(storage);
					break;

					case GID_SEARCHSTRING:
					{
						nserror ret;
						nsurl *url;

						GetAttr(STRINGA_TextVal,
							(Object *)gwin->objects[GID_SEARCHSTRING],
							(ULONG *)&storage);

						utf8 = ami_to_utf8_easy((const char *)storage);

						ret = search_web_omni(utf8, SEARCH_WEB_OMNI_SEARCHONLY, &url);
						ami_utf8_free(utf8);
						if (ret == NSERROR_OK) {
								browser_window_navigate(gwin->gw->bw,
										url,
										NULL,
										BW_NAVIGATE_HISTORY,
										NULL,
										NULL,
										NULL);
							nsurl_unref(url);
						}
						if (ret != NSERROR_OK) {
							amiga_warn_user(messages_get_errorcode(ret), 0);
						}

					}
					break;

					case GID_HOME:
						{
							if (nsurl_create(nsoption_charp(homepage_url), &url) != NSERROR_OK) {
								amiga_warn_user("NoMemory", 0);
							} else {
								browser_window_navigate(gwin->gw->bw,
										url,
										NULL,
										BW_NAVIGATE_HISTORY,
										NULL,
										NULL,
										NULL);
								nsurl_unref(url);
							}
						}
					break;

					case GID_STOP:
						if(browser_window_stop_available(gwin->gw->bw))
							browser_window_stop(gwin->gw->bw);
					break;

					case GID_RELOAD:
						ami_update_quals(gwin);

						if(browser_window_reload_available(gwin->gw->bw))
						{
							if(gwin->key_state & BROWSER_MOUSE_MOD_1)
							{
								browser_window_reload(gwin->gw->bw, true);
							}
							else
							{
								browser_window_reload(gwin->gw->bw, false);
							}
						}
					break;

					case GID_BACK:
						ami_gui_history(gwin, true);
					break;

					case GID_FORWARD:
						ami_gui_history(gwin, false);
					break;

					case GID_PAGEINFO:
						{
							ULONG w_top, w_left;
							ULONG g_top, g_left, g_height;

							GetAttr(WA_Top, gwin->objects[OID_MAIN], &w_top);
							GetAttr(WA_Left, gwin->objects[OID_MAIN], &w_left);
							GetAttr(GA_Top, gwin->objects[GID_PAGEINFO], &g_top);
							GetAttr(GA_Left, gwin->objects[GID_PAGEINFO], &g_left);
							GetAttr(GA_Height, gwin->objects[GID_PAGEINFO], &g_height);
														
							if(ami_pageinfo_open(gwin->gw->bw,
											w_left + g_left,
											w_top + g_top + g_height) != NSERROR_OK) {
								NSLOG(netsurf, INFO, "Unable to open page info window");
							}
						}
					break;

					case GID_FAVE:
						GetAttr(STRINGA_TextVal,
							(Object *)gwin->objects[GID_URL],
							(ULONG *)&storage);
						if(nsurl_create((const char *)storage, &url) == NSERROR_OK) {
							if(hotlist_has_url(url)) {
								hotlist_remove_url(url);
							} else {
								hotlist_add_url(url);
							}
							nsurl_unref(url);
						}
						ami_gui_update_hotlist_button(gwin);
					break;

					case GID_HOTLIST:
					default:
//							printf("GADGET: %ld\n",(result & WMHI_GADGETMASK));
					break;
				}
			break;

			case WMHI_RAWKEY:
				ami_update_quals(gwin);
			
				storage = result & WMHI_GADGETMASK;
				if(storage >= IECODE_UP_PREFIX) break;

				GetAttr(WINDOW_InputEvent,gwin->objects[OID_MAIN],(ULONG *)&ie);

				nskey = ami_key_to_nskey(storage, ie);

				if((ie->ie_Qualifier & IEQUALIFIER_RCOMMAND) &&
					((31 < nskey) && (nskey < 127))) {
				/* NB: Some keypresses are converted to generic keypresses above
				 * rather than being "menu-emulated" here. */
					switch(nskey)
					{
						/* The following aren't available from the menu at the moment */

						case 'r': // reload
							if(browser_window_reload_available(gwin->gw->bw))
								browser_window_reload(gwin->gw->bw, false);
						break;

						case 'u': // open url
							if((nsoption_bool(kiosk_mode) == false))
								ActivateLayoutGadget((struct Gadget *)gwin->objects[GID_MAIN],
									gwin->win, NULL, (uint32)gwin->objects[GID_URL]);
						break;
					}
				}
				else
				{
					if(!browser_window_key_press(gwin->gw->bw, nskey))
					{
						switch(nskey)
						{
							case NS_KEY_UP:
								ami_gui_scroll_internal(gwin, 0, -NSA_KBD_SCROLL_PX);
							break;

							case NS_KEY_DOWN:
								ami_gui_scroll_internal(gwin, 0, +NSA_KBD_SCROLL_PX);
							break;

							case NS_KEY_LEFT:
								ami_gui_scroll_internal(gwin, -NSA_KBD_SCROLL_PX, 0);
							break;

							case NS_KEY_RIGHT:
								ami_gui_scroll_internal(gwin, +NSA_KBD_SCROLL_PX, 0);
							break;

							case NS_KEY_PAGE_UP:
								ami_gui_scroll_internal(gwin, 0, SCROLL_PAGE_UP);
							break;

							case NS_KEY_PAGE_DOWN:
							case ' ':
								ami_gui_scroll_internal(gwin, 0, SCROLL_PAGE_DOWN);
							break;

							case NS_KEY_LINE_START: // page left
								ami_gui_scroll_internal(gwin, SCROLL_PAGE_UP, 0);
							break;

							case NS_KEY_LINE_END: // page right
								ami_gui_scroll_internal(gwin, SCROLL_PAGE_DOWN, 0);
							break;

							case NS_KEY_TEXT_START: // home
								ami_gui_scroll_internal(gwin, SCROLL_TOP, SCROLL_TOP);
							break;

							case NS_KEY_TEXT_END: // end
								ami_gui_scroll_internal(gwin, SCROLL_BOTTOM, SCROLL_BOTTOM);
							break;

							case NS_KEY_WORD_RIGHT: // alt+right
								ami_change_tab(gwin, 1);
							break;

							case NS_KEY_WORD_LEFT: // alt+left
								ami_change_tab(gwin, -1);
							break;

							case NS_KEY_DELETE_LEFT: // backspace
								ami_gui_history(gwin, true);
							break;

							/* RawKeys. NB: These are passthrus in ami_key_to_nskey() */
							case RAWKEY_F5: // reload
								if(browser_window_reload_available(gwin->gw->bw))
									browser_window_reload(gwin->gw->bw,false);
							break;

							case RAWKEY_F8: // scale 100%
								ami_gui_set_scale(gwin->gw, 1.0);
							break;

							case RAWKEY_F9: // decrease scale
								ami_gui_adjust_scale(gwin->gw, -0.1);
							break;

							case RAWKEY_F10: // increase scale
								ami_gui_adjust_scale(gwin->gw, +0.1);
							break;
							
							case RAWKEY_F12: // console log
								ami_gui_console_log_toggle(gwin->gw);
							break;

							case RAWKEY_HELP: // help
								ami_help_open(AMI_HELP_GUI, scrn);
							break;
						}
					} else if(nskey == NS_KEY_COPY_SELECTION) {
						/* if we've copied a selection we need to clear it - style guide rules */
						browser_window_key_press(gwin->gw->bw, NS_KEY_CLEAR_SELECTION);
					}
				}
			break;

			case WMHI_NEWSIZE:
				ami_set_border_gadget_size(gwin);
				ami_throbber_redraw_schedule(0, gwin->gw);
				ami_schedule(0, ami_gui_refresh_favicon, gwin);
				browser_window_schedule_reformat(gwin->gw->bw);
			break;

			case WMHI_CLOSEWINDOW:
				ami_gui_close_window(gwin);
				win_closed = TRUE;
	        break;
#ifdef __amigaos4__
			case WMHI_ICONIFY:
			{
				struct bitmap *bm = NULL;
				browser_window_history_get_thumbnail(gwin->gw->bw,
								     &bm);
				gwin->dobj = amiga_icon_from_bitmap(bm);
				amiga_icon_superimpose_favicon_internal(gwin->gw->favicon,
					gwin->dobj);
				HideWindow(gwin->win);
				gwin->appicon = AddAppIcon((ULONG)gwin->objects[OID_MAIN],
									(ULONG)gwin, gwin->win->Title, appport,
									0, gwin->dobj, NULL);

				cur_gw = NULL;
			}
			break;
#endif
			case WMHI_INACTIVE:
				gwin->gw->c_h_temp = gwin->gw->c_h;
				gui_window_remove_caret(gwin->gw);
			break;

			case WMHI_ACTIVE:
				if(gwin->gw->bw) cur_gw = gwin->gw;
				if(gwin->gw->c_h_temp)
					gwin->gw->c_h = gwin->gw->c_h_temp;
			break;

			case WMHI_INTUITICK:
			break;

   	     	default:
				//printf("class: %ld\n",(result & WMHI_CLASSMASK));
       		break;
		}

		if(win_destroyed)
		{
				/* we can't be sure what state our window_list is in, so let's
				jump out of the function and start again */

			win_destroyed = false;
			return TRUE;
		}

		if(drag_x_move || drag_y_move)
		{
			struct rect rect;

			gui_window_get_scroll(gwin->gw,
				&gwin->gw->scrollx, &gwin->gw->scrolly);

			rect.x0 = rect.x1 = gwin->gw->scrollx + drag_x_move;
			rect.y0 = rect.y1 = gwin->gw->scrolly + drag_y_move;

			gui_window_set_scroll(gwin->gw, &rect);
		}

//	ReplyMsg((struct Message *)message);
	}

	if(gwin->closed == true) {
		win_closed = TRUE;
		ami_gui_close_window(gwin);
	}

	return win_closed;
}

static void ami_gui_appicon_remove(struct gui_window_2 *gwin)
{
	if(gwin->appicon)
	{
		RemoveAppIcon(gwin->appicon);
		amiga_icon_free(gwin->dobj);
		gwin->appicon = NULL;
	}
}

static nserror gui_page_info_change(struct gui_window *gw)
{
	int bm_idx;
	browser_window_page_info_state pistate;
	struct gui_window_2 *gwin = ami_gui_get_gui_window_2(gw);
	struct browser_window *bw = ami_gui_get_browser_window(gw);

	/* if this isn't the visible tab, don't do anything */
	if((gwin == NULL) || (gwin->gw != gw)) return NSERROR_OK;

	pistate = browser_window_get_page_info_state(bw);

	switch(pistate) {
		case PAGE_STATE_INTERNAL:
			bm_idx = GID_PAGEINFO_INTERNAL_BM;
		break;

		case PAGE_STATE_LOCAL:
			bm_idx = GID_PAGEINFO_LOCAL_BM;
		break;

		case PAGE_STATE_INSECURE:
			bm_idx = GID_PAGEINFO_INSECURE_BM;
		break;

		case PAGE_STATE_SECURE_OVERRIDE:
			bm_idx = GID_PAGEINFO_WARNING_BM;
		break;

		case PAGE_STATE_SECURE_ISSUES:
			bm_idx = GID_PAGEINFO_WARNING_BM;
		break;

		case PAGE_STATE_SECURE:
			bm_idx = GID_PAGEINFO_SECURE_BM;
		break;

		default:
			bm_idx = GID_PAGEINFO_INTERNAL_BM;
		break;
	}

	RefreshSetGadgetAttrs((struct Gadget *)gwin->objects[GID_PAGEINFO], gwin->win, NULL,
				BUTTON_RenderImage, gwin->objects[bm_idx],
				GA_HintInfo, gwin->helphints[bm_idx],
				TAG_DONE);

	return NSERROR_OK;
}

static void ami_handle_appmsg(void)
{
	struct AppMessage *appmsg;
	struct gui_window_2 *gwin;
	int x, y;
	struct WBArg *appwinargs;
	STRPTR filename;
	int i = 0;

	while((appmsg = (struct AppMessage *)GetMsg(appport)))
	{
		gwin = (struct gui_window_2 *)appmsg->am_UserData;

		if(appmsg->am_Type == AMTYPE_APPICON)
		{
			ami_gui_appicon_remove(gwin);
			ShowWindow(gwin->win, WINDOW_FRONTMOST);
			ActivateWindow(gwin->win);
		}
		else if(appmsg->am_Type == AMTYPE_APPWINDOW)
		{
			for(i = 0; i < appmsg->am_NumArgs; ++i)
			{
				if((appwinargs = &appmsg->am_ArgList[i]))
				{
					if((filename = malloc(1024)))
					{
						if(appwinargs->wa_Lock)
						{
							NameFromLock(appwinargs->wa_Lock, filename, 1024);
						}

						AddPart(filename, appwinargs->wa_Name, 1024);

						if(ami_mouse_to_ns_coords(gwin, &x, &y,
							appmsg->am_MouseX, appmsg->am_MouseY) == false)
						{
							nsurl *url;

							if (netsurf_path_to_nsurl(filename, &url) != NSERROR_OK) {
								amiga_warn_user("NoMemory", 0);
							}
							else
							{
								if(i == 0)
								{
									browser_window_navigate(gwin->gw->bw,
										url,
										NULL,
										BW_NAVIGATE_HISTORY,
										NULL,
										NULL,
										NULL);

									ActivateWindow(gwin->win);
								}
								else
								{
									browser_window_create(BW_CREATE_CLONE | BW_CREATE_HISTORY |
											      BW_CREATE_TAB,
											      url,
											      NULL,
											      gwin->gw->bw,
											      NULL);
								}
								nsurl_unref(url);
							}
						}
						else
						{
							if(browser_window_drop_file_at_point(gwin->gw->bw, x, y, filename) == false)
							{
								nsurl *url;

								if (netsurf_path_to_nsurl(filename, &url) != NSERROR_OK) {
									amiga_warn_user("NoMemory", 0);
								}
								else
								{

									if(i == 0)
									{
										browser_window_navigate(gwin->gw->bw,
											url,
											NULL,
											BW_NAVIGATE_HISTORY,
											NULL,
											NULL,
											NULL);

										ActivateWindow(gwin->win);
									}
									else
									{
										browser_window_create(BW_CREATE_CLONE | BW_CREATE_HISTORY |
												      BW_CREATE_TAB,
												      url,
												      NULL,
												      gwin->gw->bw,
												      NULL);
										
									}
									nsurl_unref(url);
								}
							}
						}
						free(filename);
					}
				}
			}
		}
		ReplyMsg((struct Message *)appmsg);
	}
}

static void ami_handle_applib(void)
{
#ifdef __amigaos4__
	struct ApplicationMsg *applibmsg;
	struct browser_window *bw;
	nsurl *url;
	nserror error;

	if(!applibport) return;

	while((applibmsg=(struct ApplicationMsg *)GetMsg(applibport)))
	{
		switch (applibmsg->type)
		{
			case APPLIBMT_NewBlankDoc:
			{

				error = nsurl_create(nsoption_charp(homepage_url), &url);
				if (error == NSERROR_OK) {
					error = browser_window_create(BW_CREATE_HISTORY,
								      url,
								      NULL,
								      NULL,
								      &bw);
					nsurl_unref(url);
				}
				if (error != NSERROR_OK) {
					amiga_warn_user(messages_get_errorcode(error), 0);
				}
			}
			break;

			case APPLIBMT_OpenDoc:
			{
				struct ApplicationOpenPrintDocMsg *applibopdmsg =
					(struct ApplicationOpenPrintDocMsg *)applibmsg;

				error = netsurf_path_to_nsurl(applibopdmsg->fileName, &url);
				if (error == NSERROR_OK) {
					error = browser_window_create(BW_CREATE_HISTORY,
								      url,
								      NULL,
								      NULL,
								      &bw);
					nsurl_unref(url);
				}
				if (error != NSERROR_OK) {
					amiga_warn_user(messages_get_errorcode(error), 0);
				}
			}
			break;

			case APPLIBMT_ToFront:
				if(cur_gw)
				{
					ScreenToFront(scrn);
					WindowToFront(cur_gw->shared->win);
					ActivateWindow(cur_gw->shared->win);
				}
			break;

			case APPLIBMT_OpenPrefs:
				ScreenToFront(scrn);
				ami_gui_opts_open();
			break;

			case APPLIBMT_Quit:
			case APPLIBMT_ForceQuit:
				ami_quit_netsurf();
			break;

			case APPLIBMT_CustomMsg:
			{
				struct ApplicationCustomMsg *applibcustmsg =
					(struct ApplicationCustomMsg *)applibmsg;
				NSLOG(netsurf, INFO,
				      "Ringhio BackMsg received: %s",
				      applibcustmsg->customMsg);

				ami_download_parse_backmsg(applibcustmsg->customMsg);
			}
			break;
		}
		ReplyMsg((struct Message *)applibmsg);
	}
#endif
}

void ami_get_msg(void)
{
	ULONG winsignal = 1L << sport->mp_SigBit;
	ULONG appsig = 1L << appport->mp_SigBit;
	ULONG schedulesig = 1L << schedulermsgport->mp_SigBit;
	ULONG ctrlcsig = SIGBREAKF_CTRL_C;
	uint32 signal = 0;
	fd_set read_fd_set, write_fd_set, except_fd_set;
	int max_fd = -1;
	struct MsgPort *printmsgport = ami_print_get_msgport();
	ULONG printsig = 0;
	ULONG helpsignal = ami_help_signal();
	if(printmsgport) printsig = 1L << printmsgport->mp_SigBit;
	uint32 signalmask = winsignal | appsig | schedulesig | rxsig |
				printsig | applibsig | helpsignal;

	if ((fetch_fdset(&read_fd_set, &write_fd_set, &except_fd_set, &max_fd) == NSERROR_OK) &&
			(max_fd != -1)) {
		/* max_fd is the highest fd in use, but waitselect() needs to know how many
		 * are in use, so we add 1. */

		if (waitselect(max_fd + 1, &read_fd_set, &write_fd_set, &except_fd_set,
				NULL, (unsigned int *)&signalmask) != -1) {
			signal = signalmask;
		} else {
			NSLOG(netsurf, INFO, "waitselect() returned error");
			/* \todo Fix Ctrl-C handling.
			 * WaitSelect() from bsdsocket.library returns -1 if the task was
			 * signalled with a Ctrl-C.  waitselect() from newlib.library does not.
			 * Adding the Ctrl-C signal to our user signal mask causes a Ctrl-C to
			 * occur sporadically.  Otherwise we never get a -1 except on error.
			 * NetSurf still terminates at the Wait() when network activity is over.
			 */
		}
	} else {
		/* If fetcher_fdset fails or no network activity, do it the old fashioned way. */
		signalmask |= ctrlcsig;
		signal = Wait(signalmask);
	}

	if(signal & winsignal)
		while(ami_handle_msg());

	if(signal & appsig)
		ami_handle_appmsg();

	if(signal & rxsig)
		ami_arexx_handle();

	if(signal & applibsig)
		ami_handle_applib();

	if(signal & printsig) {
		while(GetMsg(printmsgport));  //ReplyMsg
		ami_print_cont();
	}

	if(signal & schedulesig) {
		ami_schedule_handle(schedulermsgport);
	}

	if(signal & helpsignal)
		ami_help_process();

	if(signal & ctrlcsig)
		ami_quit_netsurf_delayed();
}

static void ami_change_tab(struct gui_window_2 *gwin, int direction)
{
	struct Node *tab_node = gwin->gw->tab_node;
	struct Node *ptab = NULL;

	if(gwin->tabs <= 1) return;

	if(direction > 0) {
		ptab = GetSucc(tab_node);
	} else {
		ptab = GetPred(tab_node);
	}

	if(!ptab) return;

	RefreshSetGadgetAttrs((struct Gadget *)gwin->objects[GID_TABS], gwin->win, NULL,
						CLICKTAB_CurrentNode, ptab,
						TAG_DONE);

	ami_switch_tab(gwin, true);
}


static void gui_window_set_title(struct gui_window *g, const char *restrict title)
{
	struct Node *node;
	char *restrict utf8title;

	if(!g) return;
	if(!title) return;

	utf8title = ami_utf8_easy((char *)title);

	if(g->tab_node) {
		node = g->tab_node;

		if((g->tabtitle == NULL) || (strcmp(utf8title, g->tabtitle)))
		{
			SetGadgetAttrs((struct Gadget *)g->shared->objects[GID_TABS],
							g->shared->win, NULL,
							CLICKTAB_Labels, ~0,
							TAG_DONE);

			if(g->tabtitle) free(g->tabtitle);
			g->tabtitle = strdup(utf8title);

			SetClickTabNodeAttrs(node, TNA_Text, g->tabtitle,
							TNA_HintInfo, g->tabtitle,
							TAG_DONE);

			RefreshSetGadgetAttrs((struct Gadget *)g->shared->objects[GID_TABS],
								g->shared->win, NULL,
								CLICKTAB_Labels, &g->shared->tab_list,
								TAG_DONE);

			if(ClickTabBase->lib_Version < 53)
				RethinkLayout((struct Gadget *)g->shared->objects[GID_TABLAYOUT],
					g->shared->win, NULL, TRUE);
		}
	}

	if(g == g->shared->gw) {
		if((g->shared->wintitle == NULL) || (strcmp(utf8title, g->shared->wintitle)))
		{
			if(g->shared->wintitle) free(g->shared->wintitle);
			g->shared->wintitle = strdup(utf8title);
			SetWindowTitles(g->shared->win, g->shared->wintitle, ami_gui_get_screen_title());
		}
	}

	ami_utf8_free(utf8title);
}

static void gui_window_update_extent(struct gui_window *g)
{
	struct IBox *bbox;

	if(!g || !g->bw) return;
	if(browser_window_has_content(g->bw) == false) return;

	if(g == g->shared->gw) {
		int width, height;
		if(ami_gui_get_space_box((Object *)g->shared->objects[GID_BROWSER], &bbox) != NSERROR_OK) {
			amiga_warn_user("NoMemory", "");
			return;
		}

		if(g->shared->objects[GID_VSCROLL]) {
			browser_window_get_extents(g->bw, true, &width, &height);
			RefreshSetGadgetAttrs((struct Gadget *)(APTR)g->shared->objects[GID_VSCROLL],g->shared->win,NULL,
				SCROLLER_Total, (ULONG)(height),
				SCROLLER_Visible, bbox->Height,
			TAG_DONE);
		}

		if(g->shared->objects[GID_HSCROLL])
		{
			browser_window_get_extents(g->bw, true, &width, &height);
			RefreshSetGadgetAttrs((struct Gadget *)(APTR)g->shared->objects[GID_HSCROLL],
				g->shared->win, NULL,
				SCROLLER_Total, (ULONG)(width),
				SCROLLER_Visible, bbox->Width,
				TAG_DONE);
		}

		ami_gui_free_space_box(bbox);
	}

	ami_gui_scroller_update(g->shared);
	g->shared->new_content = true;
}


/**
 * Invalidates an area of an amiga browser window
 *
 * \param g gui_window
 * \param rect area to redraw or NULL for the entire window area
 * \return NSERROR_OK on success or appropriate error code
 */
static nserror amiga_window_invalidate_area(struct gui_window *g,
					    const struct rect *restrict rect)
{
	struct nsObject *nsobj;
	struct rect *restrict deferred_rect;

	if(!g) return NSERROR_BAD_PARAMETER;

	if (rect == NULL) {
		if (g != g->shared->gw) {
			return NSERROR_OK;
		}
	} else {
		if (ami_gui_window_update_box_deferred_check(g->deferred_rects, rect,
							    g->deferred_rects_pool)) {
			deferred_rect = ami_memory_itempool_alloc(g->deferred_rects_pool,
								  sizeof(struct rect));
			CopyMem(rect, deferred_rect, sizeof(struct rect));
			nsobj = AddObject(g->deferred_rects, AMINS_RECT);
			nsobj->objstruct = deferred_rect;
		} else {
			NSLOG(netsurf, INFO,
			      "Ignoring duplicate or subset of queued box redraw");
		}
	}
	ami_schedule_redraw(g->shared, false);

	return NSERROR_OK;
}


static void ami_switch_tab(struct gui_window_2 *gwin, bool redraw)
{
	struct Node *tabnode;
	struct IBox *bbox;

	/* Clear the last new tab list */
	gwin->last_new_tab = NULL;

	if(gwin->tabs == 0) return;

	gui_window_get_scroll(gwin->gw,
		&gwin->gw->scrollx, &gwin->gw->scrolly);

	GetAttr(CLICKTAB_CurrentNode, (Object *)gwin->objects[GID_TABS],
				(ULONG *)&tabnode);
	GetClickTabNodeAttrs(tabnode,
				TNA_UserData, &gwin->gw,
				TAG_DONE);
	cur_gw = gwin->gw;

	ami_gui_console_log_switch(gwin->gw);

	if(ami_gui_get_space_box((Object *)gwin->objects[GID_BROWSER], &bbox) != NSERROR_OK) {
		amiga_warn_user("NoMemory", "");
		return;
	}

	if((gwin->gw->bw == NULL) || (browser_window_has_content(gwin->gw->bw)) == false) {
		RefreshSetGadgetAttrs((struct Gadget *)gwin->objects[GID_URL],
			gwin->win, NULL, STRINGA_TextVal, "", TAG_DONE);

		ami_plot_clear_bbox(gwin->win->RPort, bbox);
		ami_gui_free_space_box(bbox);
		return;
	}

	ami_plot_release_pens(gwin->shared_pens);
	ami_update_buttons(gwin);
	ami_gui_menu_update_disabled(gwin->gw, browser_window_get_content(gwin->gw->bw));

	if(redraw)
	{
		struct rect rect;

		ami_plot_clear_bbox(gwin->win->RPort, bbox);
		gui_window_set_title(gwin->gw,
				     browser_window_get_title(gwin->gw->bw));
		gui_window_update_extent(gwin->gw);
		amiga_window_invalidate_area(gwin->gw, NULL);

		rect.x0 = rect.x1 = gwin->gw->scrollx;
		rect.y0 = rect.y1 = gwin->gw->scrolly;

		gui_window_set_scroll(gwin->gw, &rect);
		gwin->redraw_scroll = false;

		browser_window_refresh_url_bar(gwin->gw->bw);
		ami_gui_update_hotlist_button(gwin);
		ami_gui_scroller_update(gwin);
		ami_throbber_redraw_schedule(0, gwin->gw);

		gui_window_set_icon(gwin->gw, gwin->gw->favicon);
		gui_page_info_change(gwin->gw);
	}

	ami_gui_free_space_box(bbox);
}

void ami_quit_netsurf(void)
{
	struct nsObject *node;
	struct nsObject *nnode;
	struct ami_generic_window *w;

	/* Disable the multiple tabs open warning */
	nsoption_set_bool(tab_close_warn, false);

	if(!IsMinListEmpty(window_list)) {
		node = (struct nsObject *)GetHead((struct List *)window_list);

		do {
			nnode=(struct nsObject *)GetSucc((struct Node *)node);
			w = node->objstruct;

			if(w->tbl->close != NULL) {
				if(node->Type == AMINS_WINDOW) {
					struct gui_window_2 *gwin = (struct gui_window_2 *)w;
					ShowWindow(gwin->win, WINDOW_BACKMOST); // do we need this??
				}
				w->tbl->close(w);
			}
		} while((node = nnode));

		win_destroyed = true;
	}

	if(IsMinListEmpty(window_list)) {
		/* last window closed, so exit */
		ami_quit = true;
	}
}

static void ami_quit_netsurf_delayed(void)
{
	int res = -1;
#ifdef __amigaos4__
	char *utf8text = ami_utf8_easy(messages_get("TCPIPShutdown"));
	char *utf8gadgets = ami_utf8_easy(messages_get("AbortShutdown"));

	DisplayBeep(NULL);
	
	res = TimedDosRequesterTags(TDR_ImageType, TDRIMAGE_INFO,
		TDR_TitleString, messages_get("NetSurf"),
		TDR_FormatString, utf8text,
		TDR_GadgetString, utf8gadgets,
		TDR_Timeout, 5,
		TDR_Inactive, TRUE,
		TAG_DONE);
	
	free(utf8text);
	free(utf8gadgets);
#endif
	if(res == -1) { /* Requester timed out */
		ami_quit_netsurf();
	}
}

static void ami_gui_close_screen(struct Screen *scrn, BOOL locked_screen, BOOL donotwait)
{
	if(scrn == NULL) return;

	if(locked_screen) {
		UnlockPubScreen(NULL,scrn);
		locked_screen = FALSE;
	}

	/* If this is our own screen, wait for visitor windows to close */
	if(screen_signal == -1) return;

	if(CloseScreen(scrn) == TRUE) {
		if(screen_signal != -1) {
			FreeSignal(screen_signal);
			screen_signal = -1;
			scrn = NULL;
		}
		return;
	}
	if(donotwait == TRUE) return;

	ULONG scrnsig = 1 << screen_signal;
	NSLOG(netsurf, INFO,
	      "Waiting for visitor windows to close... (signal)");
	Wait(scrnsig);

	while (CloseScreen(scrn) == FALSE) {
		NSLOG(netsurf, INFO,
		      "Waiting for visitor windows to close... (polling)");
		Delay(50);
	}

	FreeSignal(screen_signal);
	screen_signal = -1;
	scrn = NULL;
}

void ami_try_quit(void)
{
	if(!IsMinListEmpty(window_list)) return;

	if(nsoption_bool(close_no_quit) == false)
	{
		ami_quit = true;
		return;
	}
	else
	{
		ami_gui_close_screen(scrn, locked_screen, TRUE);
	}
}

static void gui_quit(void)
{
	ami_theme_throbber_free();

	urldb_save(nsoption_charp(url_file));
	urldb_save_cookies(nsoption_charp(cookie_file));
	hotlist_fini();
#ifdef __amigaos4__
	if(IApplication && ami_appid)
		UnregisterApplication(ami_appid, NULL);
#endif
	ami_arexx_cleanup();

	ami_plot_ra_free(browserglob);

	ami_font_fini();
	ami_help_free();
	
	NSLOG(netsurf, INFO, "Freeing menu items");
	ami_ctxmenu_free();
	ami_menu_free_glyphs();

	NSLOG(netsurf, INFO, "Freeing mouse pointers");
	ami_mouse_pointers_free();

	ami_file_req_free();
	ami_openurl_close();
#ifdef __amigaos4__
	FreeStringClass(urlStringClass);
#endif

	FreeObjList(window_list);

	ami_clipboard_free();
	ami_gui_resources_free();

	NSLOG(netsurf, INFO, "Closing screen");
	ami_gui_close_screen(scrn, locked_screen, FALSE);
	if(nsscreentitle) FreeVec(nsscreentitle);
}

char *ami_gui_get_cache_favicon_name(nsurl *url, bool only_if_avail)
{
	STRPTR filename = NULL;

	if ((filename = ASPrintf("%s/%x", current_user_faviconcache, nsurl_hash(url)))) {
		NSLOG(netsurf, INFO, "favicon cache location: %s", filename);

		if (only_if_avail == true) {
			BPTR lock = 0;
			if((lock = Lock(filename, ACCESS_READ))) {
				UnLock(lock);
				return filename;
			}
		} else {
			return filename;
		}
	}
	return NULL;
}

static void ami_gui_cache_favicon(nsurl *url, struct bitmap *favicon)
{
	STRPTR filename = NULL;

	if ((filename = ami_gui_get_cache_favicon_name(url, false))) {
		if(favicon) amiga_bitmap_save(favicon, filename, AMI_BITMAP_SCALE_ICON);
		FreeVec(filename);
	}
}

void ami_gui_update_hotlist_button(struct gui_window_2 *gwin)
{
	char *url;
	nsurl *nsurl;

	GetAttr(STRINGA_TextVal,
		(Object *)gwin->objects[GID_URL],
		(ULONG *)&url);

	if(nsurl_create(url, &nsurl) == NSERROR_OK) {
		if(hotlist_has_url(nsurl)) {
			RefreshSetGadgetAttrs((struct Gadget *)gwin->objects[GID_FAVE], gwin->win, NULL,
				BUTTON_RenderImage, gwin->objects[GID_FAVE_RMV], TAG_DONE);

			if (gwin->gw->favicon)
				ami_gui_cache_favicon(nsurl, content_get_bitmap(gwin->gw->favicon));
		} else {
			RefreshSetGadgetAttrs((struct Gadget *)gwin->objects[GID_FAVE], gwin->win, NULL,
				BUTTON_RenderImage, gwin->objects[GID_FAVE_ADD], TAG_DONE);
		}
		
		nsurl_unref(nsurl);
	}
}

static bool ami_gui_hotlist_add(void *userdata, int level, int item,
		const char *title, nsurl *url, bool is_folder)
{
	struct ami_gui_tb_userdata *tb_userdata = (struct ami_gui_tb_userdata *)userdata;
	struct Node *speed_button_node;
	char menu_icon[1024];
	char *utf8title = NULL;

	if(level != 1) return false;
	if(item > AMI_GUI_TOOLBAR_MAX) return false;
	if(is_folder == true) return false;

	if(utf8_from_local_encoding(title,
		(strlen(title) < NSA_MAX_HOTLIST_BUTTON_LEN) ? strlen(title) : NSA_MAX_HOTLIST_BUTTON_LEN,
		&utf8title) != NSERROR_OK)
		return false;

	char *iconname = ami_gui_get_cache_favicon_name(url, true);
	if (iconname == NULL) iconname = ASPrintf("icons/content.png");
	ami_locate_resource(menu_icon, iconname);

	tb_userdata->gw->hotlist_toolbar_lab[item] = BitMapObj,
						IA_Scalable, TRUE,
						BITMAP_Screen, scrn,
						BITMAP_SourceFile, menu_icon,
						BITMAP_Masking, TRUE,
					BitMapEnd;

	/* \todo make this scale the bitmap to these dimensions */
	SetAttrs(tb_userdata->gw->hotlist_toolbar_lab[item],
				BITMAP_Width, 16,
				BITMAP_Height, 16,
			TAG_DONE);

	Object *lab_item = LabelObj,
				//		LABEL_DrawInfo, dri,
						LABEL_DisposeImage, TRUE,
						LABEL_Image, tb_userdata->gw->hotlist_toolbar_lab[item],
						LABEL_Text, " ",
						LABEL_Text, utf8title,
					LabelEnd;

	free(utf8title);

	speed_button_node = AllocSpeedButtonNode(item,
					SBNA_Image, lab_item,
					SBNA_HintInfo, nsurl_access(url),
					SBNA_UserData, (void *)url,
					TAG_DONE);
			
	AddTail(tb_userdata->sblist, speed_button_node);

	tb_userdata->items++;
	return true;
}

static int ami_gui_hotlist_scan(struct List *speed_button_list, struct gui_window_2 *gwin)
{
	struct ami_gui_tb_userdata userdata;
	userdata.gw = gwin;
	userdata.sblist = speed_button_list;
	userdata.items = 0;

	ami_hotlist_scan((void *)&userdata, 0, messages_get("HotlistToolbar"), ami_gui_hotlist_add);
	return userdata.items;
}

static void ami_gui_hotlist_toolbar_add(struct gui_window_2 *gwin)
{
	struct TagItem attrs[2];

	attrs[0].ti_Tag = CHILD_MinWidth;
	attrs[0].ti_Data = 0;
	attrs[1].ti_Tag = TAG_DONE;
	attrs[1].ti_Data = 0;

	NewList(&gwin->hotlist_toolbar_list);

	if(ami_gui_hotlist_scan(&gwin->hotlist_toolbar_list, gwin) > 0) {
		gwin->objects[GID_HOTLIST] =
				SpeedBarObj,
					GA_ID, GID_HOTLIST,
					GA_RelVerify, TRUE,
					ICA_TARGET, ICTARGET_IDCMP,
					SPEEDBAR_BevelStyle, BVS_NONE,
					SPEEDBAR_Buttons, &gwin->hotlist_toolbar_list,
				SpeedBarEnd;
				
		gwin->objects[GID_HOTLISTSEPBAR] =
				BevelObj,
					BEVEL_Style, BVS_SBAR_VERT,
				BevelEnd;
#ifdef __amigaos4__
		IDoMethod(gwin->objects[GID_HOTLISTLAYOUT], LM_ADDCHILD,
				gwin->win, gwin->objects[GID_HOTLIST], attrs);

		IDoMethod(gwin->objects[GID_HOTLISTLAYOUT], LM_ADDIMAGE,
				gwin->win, gwin->objects[GID_HOTLISTSEPBAR], NULL);

#else
		SetAttrs(gwin->objects[GID_HOTLISTLAYOUT],
			LAYOUT_AddChild, gwin->objects[GID_HOTLIST], TAG_MORE, &attrs);
		SetAttrs(gwin->objects[GID_HOTLISTLAYOUT],
			LAYOUT_AddChild, gwin->objects[GID_HOTLISTSEPBAR], TAG_DONE);
#endif

		FlushLayoutDomainCache((struct Gadget *)gwin->objects[GID_MAIN]);

		RethinkLayout((struct Gadget *)gwin->objects[GID_MAIN],
				gwin->win, NULL, TRUE);
		
		ami_schedule_redraw(gwin, true);
	}
}

static void ami_gui_hotlist_toolbar_free(struct gui_window_2 *gwin, struct List *speed_button_list)
{
	int i;
	struct Node *node;
	struct Node *nnode;

	if(nsoption_bool(kiosk_mode) == true) return;

	if(IsListEmpty(speed_button_list)) return;
	node = GetHead(speed_button_list);

	do {
		nnode = GetSucc(node);
		Remove(node);
		FreeSpeedButtonNode(node);
	} while((node = nnode));

	for(i = 0; i < AMI_GUI_TOOLBAR_MAX; i++) {
		if(gwin->hotlist_toolbar_lab[i]) {
			DisposeObject(gwin->hotlist_toolbar_lab[i]);
			gwin->hotlist_toolbar_lab[i] = NULL;
		}
	}
}

static void ami_gui_hotlist_toolbar_remove(struct gui_window_2 *gwin)
{
#ifdef __amigaos4__
	IDoMethod(gwin->objects[GID_HOTLISTLAYOUT], LM_REMOVECHILD,
			gwin->win, gwin->objects[GID_HOTLIST]);

	IDoMethod(gwin->objects[GID_HOTLISTLAYOUT], LM_REMOVECHILD,
			gwin->win, gwin->objects[GID_HOTLISTSEPBAR]);
#else
	SetAttrs(gwin->objects[GID_HOTLISTLAYOUT],
		LAYOUT_RemoveChild, gwin->objects[GID_HOTLIST], TAG_DONE);
	SetAttrs(gwin->objects[GID_HOTLISTLAYOUT],
		LAYOUT_RemoveChild, gwin->objects[GID_HOTLISTSEPBAR], TAG_DONE);
#endif
	FlushLayoutDomainCache((struct Gadget *)gwin->objects[GID_MAIN]);

	RethinkLayout((struct Gadget *)gwin->objects[GID_MAIN],
			gwin->win, NULL, TRUE);

	ami_schedule_redraw(gwin, true);
}

static void ami_gui_hotlist_toolbar_update(struct gui_window_2 *gwin)
{
	if(IsListEmpty(&gwin->hotlist_toolbar_list)) {
		ami_gui_hotlist_toolbar_add(gwin);
		return;
	}

	/* Below should be SetAttr according to Autodocs */
	SetGadgetAttrs((struct Gadget *)gwin->objects[GID_HOTLIST],
						gwin->win, NULL,
						SPEEDBAR_Buttons, ~0,
						TAG_DONE);

	ami_gui_hotlist_toolbar_free(gwin, &gwin->hotlist_toolbar_list);

	if(ami_gui_hotlist_scan(&gwin->hotlist_toolbar_list, gwin) > 0) {
		SetGadgetAttrs((struct Gadget *)gwin->objects[GID_HOTLIST],
						gwin->win, NULL,
						SPEEDBAR_Buttons, &gwin->hotlist_toolbar_list,
						TAG_DONE);
	} else {
		ami_gui_hotlist_toolbar_remove(gwin);
	}
}

/**
 * Update hotlist toolbar and recreate the menu for all windows
 */
void ami_gui_hotlist_update_all(void)
{
	struct nsObject *node;
	struct nsObject *nnode;
	struct gui_window_2 *gwin;

	if(IsMinListEmpty(window_list))	return;

	ami_gui_menu_refresh_hotlist();

	node = (struct nsObject *)GetHead((struct List *)window_list);

	do {
		nnode=(struct nsObject *)GetSucc((struct Node *)node);
		gwin = node->objstruct;

		if(node->Type == AMINS_WINDOW) {
			ami_gui_hotlist_toolbar_update(gwin);
		}
	} while((node = nnode));
}

static void ami_toggletabbar(struct gui_window_2 *gwin, bool show)
{
	if(ClickTabBase->lib_Version < 53) return;

	if(show) {
		struct TagItem attrs[3];

		attrs[0].ti_Tag = CHILD_WeightedWidth;
		attrs[0].ti_Data = 0;
		attrs[1].ti_Tag = CHILD_WeightedHeight;
		attrs[1].ti_Data = 0;
		attrs[2].ti_Tag = TAG_DONE;
		attrs[2].ti_Data = 0;

		gwin->objects[GID_TABS] = ClickTabObj,
					GA_ID, GID_TABS,
					GA_RelVerify, TRUE,
					GA_Underscore, 13, // disable kb shortcuts
					GA_ContextMenu, ami_ctxmenu_clicktab_create(gwin, &gwin->clicktab_ctxmenu),
					CLICKTAB_Labels, &gwin->tab_list,
					CLICKTAB_LabelTruncate, TRUE,
					CLICKTAB_CloseImage, gwin->objects[GID_CLOSETAB_BM],
					CLICKTAB_FlagImage, gwin->objects[GID_TABS_FLAG],
					ClickTabEnd;

		gwin->objects[GID_ADDTAB] = ButtonObj,
					GA_ID, GID_ADDTAB,
					GA_RelVerify, TRUE,
					GA_HintInfo, gwin->helphints[GID_ADDTAB],
					GA_Text, "+",
					BUTTON_RenderImage, gwin->objects[GID_ADDTAB_BM],
					ButtonEnd;
#ifdef __amigaos4__
		IDoMethod(gwin->objects[GID_TABLAYOUT], LM_ADDCHILD,
				gwin->win, gwin->objects[GID_TABS], NULL);

		IDoMethod(gwin->objects[GID_TABLAYOUT], LM_ADDCHILD,
				gwin->win, gwin->objects[GID_ADDTAB], attrs);
#else
		SetAttrs(gwin->objects[GID_TABLAYOUT],
				LAYOUT_AddChild, gwin->objects[GID_TABS], TAG_DONE);
		SetAttrs(gwin->objects[GID_TABLAYOUT],
				LAYOUT_AddChild, gwin->objects[GID_ADDTAB], TAG_MORE, &attrs);
#endif
	} else {
#ifdef __amigaos4__
		IDoMethod(gwin->objects[GID_TABLAYOUT], LM_REMOVECHILD,
				gwin->win, gwin->objects[GID_TABS]);

		IDoMethod(gwin->objects[GID_TABLAYOUT], LM_REMOVECHILD,
				gwin->win, gwin->objects[GID_ADDTAB]);
#else
		SetAttrs(gwin->objects[GID_TABLAYOUT],
				LAYOUT_RemoveChild, gwin->objects[GID_TABS], TAG_DONE);
		SetAttrs(gwin->objects[GID_TABLAYOUT],
				LAYOUT_RemoveChild, gwin->objects[GID_ADDTAB], TAG_DONE);
#endif

		gwin->objects[GID_TABS] = NULL;
		gwin->objects[GID_ADDTAB] = NULL;
	}

	FlushLayoutDomainCache((struct Gadget *)gwin->objects[GID_MAIN]);

	RethinkLayout((struct Gadget *)gwin->objects[GID_MAIN],
			gwin->win, NULL, TRUE);

	if (gwin->gw && gwin->gw->bw) {
		gui_window_set_title(gwin->gw,
				     browser_window_get_title(gwin->gw->bw));
		gui_window_update_extent(gwin->gw);
		amiga_window_invalidate_area(gwin->gw, NULL);
	}
}

void ami_gui_tabs_toggle_all(void)
{
	struct nsObject *node;
	struct nsObject *nnode;
	struct gui_window_2 *gwin;

	if(IsMinListEmpty(window_list))	return;

	node = (struct nsObject *)GetHead((struct List *)window_list);

	do {
		nnode=(struct nsObject *)GetSucc((struct Node *)node);
		gwin = node->objstruct;

		if(node->Type == AMINS_WINDOW)
		{
			if(gwin->tabs == 1) {
				if(nsoption_bool(tab_always_show) == true) {
					ami_toggletabbar(gwin, true);
				} else {
					ami_toggletabbar(gwin, false);
				}
			}
		}
	} while((node = nnode));
}

static void ami_gui_search_ico_refresh(void *p)
{
	search_web_select_provider(-1);
}

/**
 * Count windows, and optionally tabs.
 *
 * \param  window    window to count tabs of
 * \param  tabs      if window > 0, will be updated to contain the number of tabs
 *                   in that window, unchanged otherwise
 * \return number of windows currently open
 */
int ami_gui_count_windows(int window, int *tabs)
{
	int windows = 0;
	struct nsObject *node, *nnode;
	struct gui_window_2 *gwin;

	if(!IsMinListEmpty(window_list)) {
		node = (struct nsObject *)GetHead((struct List *)window_list);
		do {
			nnode=(struct nsObject *)GetSucc((struct Node *)node);

			gwin = node->objstruct;

			if(node->Type == AMINS_WINDOW) {
				windows++;
				if(window == windows) *tabs = gwin->tabs;
			}
		} while((node = nnode));
	}
	return windows;
}

/**
 * Set the scale of a gui window
 *
 * \param gw	gui_window to set scale for
 * \param scale	scale to set
 */
void ami_gui_set_scale(struct gui_window *gw, float scale)
{
	browser_window_set_scale(gw->bw, scale, true);
	ami_schedule_redraw(gw->shared, true);
}

void ami_gui_adjust_scale(struct gui_window *gw, float adjustment)
{
	browser_window_set_scale(gw->bw, adjustment, false);
	ami_schedule_redraw(gw->shared, true);
}

void ami_gui_switch_to_new_tab(struct gui_window_2 *gwin)
{
	if(nsoption_bool(new_tab_is_active) == true) return;

	/* Switch to the just-opened tab (if new_tab_is_active, we already did!) */
	RefreshSetGadgetAttrs((struct Gadget *)gwin->objects[GID_TABS],
							gwin->win, NULL,
							CLICKTAB_CurrentNode, gwin->last_new_tab,
							TAG_DONE);

	ami_switch_tab(gwin, false);
}

nserror ami_gui_new_blank_tab(struct gui_window_2 *gwin)
{
	nsurl *url;
	nserror error;
	struct browser_window *bw = NULL;

	error = nsurl_create(nsoption_charp(homepage_url), &url);
	if (error == NSERROR_OK) {
		error = browser_window_create(BW_CREATE_HISTORY |
					      BW_CREATE_TAB,
					      url,
					      NULL,
					      gwin->gw->bw,
					      &bw);
		nsurl_unref(url);
	}
	if (error != NSERROR_OK) {
		amiga_warn_user(messages_get_errorcode(error), 0);
		return error;
	}

	ami_gui_switch_to_new_tab(gwin);

	return NSERROR_OK;
}

static void ami_do_redraw_tiled(struct gui_window_2 *gwin, bool busy,
	int left, int top, int width, int height,
	int sx, int sy, struct IBox *bbox, struct redraw_context *ctx)
{
	struct gui_globals *glob = (struct gui_globals *)ctx->priv;
	int x, y;
	struct rect clip;
	int tile_size_x;
	int tile_size_y;

	ami_plot_ra_get_size(glob, &tile_size_x, &tile_size_y);
	ami_plot_ra_set_pen_list(glob, gwin->shared_pens);
	
	if(top < 0) {
		height += top;
		top = 0;
	}

	if(left < 0) {
		width += left;
		left = 0;
	}

	if(top < sy) {
		height += (top - sy);
		top = sy;
	}
	if(left < sx) {
		width += (left - sx);
		left = sx;
	}

	if(((top - sy) + height) > bbox->Height)
		height = bbox->Height - (top - sy);

	if(((left - sx) + width) > bbox->Width)
		width = bbox->Width - (left - sx);

	if(width <= 0) return;
	if(height <= 0) return;

	if(busy) ami_set_pointer(gwin, GUI_POINTER_WAIT, false);

	for(y = top; y < (top + height); y += tile_size_y) {
		clip.y0 = 0;
		clip.y1 = tile_size_y;
		if(clip.y1 > height) clip.y1 = height;
		if(((y - sy) + clip.y1) > bbox->Height)
			clip.y1 = bbox->Height - (y - sy);

		for(x = left; x < (left + width); x += tile_size_x) {
			clip.x0 = 0;
			clip.x1 = tile_size_x;
			if(clip.x1 > width) clip.x1 = width;
			if(((x - sx) + clip.x1) > bbox->Width)
				clip.x1 = bbox->Width - (x - sx);

			if(browser_window_redraw(gwin->gw->bw,
				clip.x0 - (int)x,
				clip.y0 - (int)y,
				&clip, ctx))
			{
				ami_clearclipreg(glob);
#ifdef __amigaos4__
				BltBitMapTags(BLITA_SrcType, BLITT_BITMAP, 
					BLITA_Source, ami_plot_ra_get_bitmap(glob),
					BLITA_SrcX, 0,
					BLITA_SrcY, 0,
					BLITA_DestType, BLITT_RASTPORT, 
					BLITA_Dest, gwin->win->RPort,
					BLITA_DestX, bbox->Left + (int)(x - sx),
					BLITA_DestY, bbox->Top + (int)(y - sy),
					BLITA_Width, (int)(clip.x1),
					BLITA_Height, (int)(clip.y1),
					TAG_DONE);
#else
				BltBitMapRastPort(ami_plot_ra_get_bitmap(glob), 0, 0, gwin->win->RPort,
					bbox->Left + (int)(x - sx),
					bbox->Top + (int)(y - sy),
					(int)(clip.x1), (int)(clip.y1), 0xC0);
#endif
			}
		}
	}
	
	if(busy) ami_reset_pointer(gwin);
}


/**
 * Redraw an area of the browser window - Amiga-specific function
 *
 * \param  g   a struct gui_window 
 * \param  bw  a struct browser_window
 * \param  busy  busy flag passed to tiled redraw.
 * \param  x0  top-left co-ordinate (in document co-ordinates)
 * \param  y0  top-left co-ordinate (in document co-ordinates)
 * \param  x1  bottom-right co-ordinate (in document co-ordinates)
 * \param  y1  bottom-right co-ordinate (in document co-ordinates)
 */

static void ami_do_redraw_limits(struct gui_window *g, struct browser_window *bw, bool busy,
		int x0, int y0, int x1, int y1)
{
	struct IBox *bbox;
	ULONG sx, sy;

	struct redraw_context ctx = {
		.interactive = true,
		.background_images = true,
		.plot = &amiplot,
		.priv = browserglob
	};

	if(!g) return;
	if(browser_window_redraw_ready(bw) == false) return;

	sx = g->scrollx;
	sy = g->scrolly;

	if(g != g->shared->gw) return;

	if(ami_gui_get_space_box((Object *)g->shared->objects[GID_BROWSER], &bbox) != NSERROR_OK) {
		amiga_warn_user("NoMemory", "");
		return;
	}

	ami_do_redraw_tiled(g->shared, busy, x0, y0,
		x1 - x0, y1 - y0, sx, sy, bbox, &ctx);

	ami_gui_free_space_box(bbox);

	return;
}


static void ami_refresh_window(struct gui_window_2 *gwin)
{
	/* simplerefresh only */

	struct IBox *bbox;
	int sx, sy;
	struct RegionRectangle *regrect;
	struct rect r;

	sx = gwin->gw->scrollx;
	sy = gwin->gw->scrolly;

	ami_set_pointer(gwin, GUI_POINTER_WAIT, false);

	if(ami_gui_get_space_box((Object *)gwin->objects[GID_BROWSER], &bbox) != NSERROR_OK) {
		amiga_warn_user("NoMemory", "");
		return;
	}
	
	BeginRefresh(gwin->win);

	r.x0 = (gwin->win->RPort->Layer->DamageList->bounds.MinX - bbox->Left) + sx - 1;
	r.x1 = (gwin->win->RPort->Layer->DamageList->bounds.MaxX - bbox->Left) + sx + 2;
	r.y0 = (gwin->win->RPort->Layer->DamageList->bounds.MinY - bbox->Top) + sy - 1;
	r.y1 = (gwin->win->RPort->Layer->DamageList->bounds.MaxY - bbox->Top) + sy + 2;

	regrect = gwin->win->RPort->Layer->DamageList->RegionRectangle;

	amiga_window_invalidate_area(gwin->gw, &r);

	while(regrect)
	{
		r.x0 = (regrect->bounds.MinX - bbox->Left) + sx - 1;
		r.x1 = (regrect->bounds.MaxX - bbox->Left) + sx + 2;
		r.y0 = (regrect->bounds.MinY - bbox->Top) + sy - 1;
		r.y1 = (regrect->bounds.MaxY - bbox->Top) + sy + 2;

		regrect = regrect->Next;

		amiga_window_invalidate_area(gwin->gw, &r);
	}

	EndRefresh(gwin->win, TRUE);

	ami_gui_free_space_box(bbox);	
	ami_reset_pointer(gwin);
}

HOOKF(void, ami_scroller_hook, Object *, object, struct IntuiMessage *)
{
	ULONG gid;
	struct gui_window_2 *gwin = hook->h_Data;
	struct IntuiWheelData *wheel;
	struct Node *node = NULL;
	nsurl *url;

	switch(msg->Class)
	{
		case IDCMP_IDCMPUPDATE:
			gid = GetTagData( GA_ID, 0, msg->IAddress );

			switch( gid ) 
			{
				case GID_HSCROLL:
 				case GID_VSCROLL:
					if(nsoption_bool(faster_scroll) == true) gwin->redraw_scroll = true;
						else gwin->redraw_scroll = false;

					ami_schedule_redraw(gwin, true);
 				break;
				
				case GID_HOTLIST:
					if((node = (struct Node *)GetTagData(SPEEDBAR_SelectedNode, 0, msg->IAddress))) {
						GetSpeedButtonNodeAttrs(node, SBNA_UserData, (ULONG *)&url, TAG_DONE);

						if(gwin->key_state & BROWSER_MOUSE_MOD_2) {
							browser_window_create(BW_CREATE_TAB,
										      url,
										      NULL,
										      gwin->gw->bw,
										      NULL);
						} else {
							browser_window_navigate(gwin->gw->bw,
									url,
									NULL,
									BW_NAVIGATE_HISTORY,
									NULL,
									NULL,
									NULL);

						}
					}
				break;
			} 
		break;
#ifdef __amigaos4__
		case IDCMP_EXTENDEDMOUSE:
			if(msg->Code == IMSGCODE_INTUIWHEELDATA)
			{
				wheel = (struct IntuiWheelData *)msg->IAddress;

				ami_gui_scroll_internal(gwin, wheel->WheelX * 50, wheel->WheelY * 50);
			}
		break;
#endif
		case IDCMP_SIZEVERIFY:
		break;

		case IDCMP_REFRESHWINDOW:
			ami_refresh_window(gwin);
		break;

		default:
			NSLOG(netsurf, INFO,
			      "IDCMP hook unhandled event: %ld", msg->Class);
		break;
	}
//	ReplyMsg((struct Message *)msg);
} 

/* exported function documented in gui.h */
nserror ami_gui_win_list_add(void *win, int type, const struct ami_win_event_table *table)
{
	struct nsObject *node = AddObject(window_list, type);
	if(node == NULL) return NSERROR_NOMEM;
	node->objstruct = win;

	struct ami_generic_window *w = (struct ami_generic_window *)win;
	w->tbl = table;
	w->node = node;

	return NSERROR_OK;
}

/* exported function documented in gui.h */
void ami_gui_win_list_remove(void *win)
{
	struct ami_generic_window *w = (struct ami_generic_window *)win;

	if(w->node->Type == AMINS_TVWINDOW) {
		DelObjectNoFree(w->node);
	} else {
		DelObject(w->node);
	}
}

static const struct ami_win_event_table ami_gui_table = {
	ami_gui_event,
	ami_gui_close_window,
};

static struct gui_window *
gui_window_create(struct browser_window *bw,
		struct gui_window *existing,
		gui_window_create_flags flags)
{
	struct gui_window *g = NULL;
	ULONG offset = 0;
	ULONG curx = nsoption_int(window_x), cury = nsoption_int(window_y);
	ULONG curw = nsoption_int(window_width), curh = nsoption_int(window_height);
	char nav_west[100],nav_west_s[100],nav_west_g[100];
	char nav_east[100],nav_east_s[100],nav_east_g[100];
	char stop[100],stop_s[100],stop_g[100];
	char reload[100],reload_s[100],reload_g[100];
	char home[100],home_s[100],home_g[100];
	char closetab[100],closetab_s[100],closetab_g[100];
	char addtab[100],addtab_s[100],addtab_g[100];
	char fave[100], unfave[100];
	char pi_insecure[100], pi_internal[100], pi_local[100], pi_secure[100], pi_warning[100];
	char tabthrobber[100];
	ULONG refresh_mode = WA_SmartRefresh;
	ULONG defer_layout = TRUE;
	ULONG idcmp_sizeverify = IDCMP_SIZEVERIFY;

	NSLOG(netsurf, INFO, "Creating window");

	if (!scrn) ami_openscreenfirst();

	if (nsoption_bool(kiosk_mode)) flags &= ~GW_CREATE_TAB;
	if (nsoption_bool(resize_with_contents)) idcmp_sizeverify = 0;

	/* Offset the new window by titlebar + 1 as per AmigaOS style guide.
	 * If we don't have a clone window we offset by all windows open. */
	offset = scrn->WBorTop + scrn->Font->ta_YSize + 1;

	if(existing) {
		curx = existing->shared->win->LeftEdge;
		cury = existing->shared->win->TopEdge + offset;
		curw = existing->shared->win->Width;
		curh = existing->shared->win->Height;
	} else {
		if(nsoption_bool(kiosk_mode) == false) {
			cury += offset * ami_gui_count_windows(0, NULL);
		}
	}

	if(curh > (scrn->Height - cury)) curh = scrn->Height - cury;

	g = calloc(1, sizeof(struct gui_window));

	if(!g)
	{
		amiga_warn_user("NoMemory","");
		return NULL;
	}

	NewList(&g->dllist);
	g->deferred_rects = NewObjList();
	g->deferred_rects_pool = ami_memory_itempool_create(sizeof(struct rect));
	g->bw = bw;

	NewList(&g->loglist);
#ifdef __amigaos4__
	g->logcolumns = AllocLBColumnInfo(4,
			LBCIA_Column, 0,
			//	LBCIA_CopyTitle, TRUE,
				LBCIA_Title, "time", /**\TODO: add these to Messages */
				LBCIA_Weight, 10,
				LBCIA_DraggableSeparator, TRUE,
				LBCIA_Separator, TRUE,
			LBCIA_Column, 1,
			//	LBCIA_CopyTitle, TRUE,
				LBCIA_Title, "source", /**\TODO: add these to Messages */
				LBCIA_Weight, 10,
				LBCIA_DraggableSeparator, TRUE,
				LBCIA_Separator, TRUE,
			LBCIA_Column, 2,
			//	LBCIA_CopyTitle, TRUE,
				LBCIA_Title, "level", /**\TODO: add these to Messages */
				LBCIA_Weight, 5,
				LBCIA_DraggableSeparator, TRUE,
				LBCIA_Separator, TRUE,
			LBCIA_Column, 3,
			//	LBCIA_CopyTitle, TRUE,
				LBCIA_Title, "message", /**\TODO: add these to Messages */
				LBCIA_Weight, 75,
				LBCIA_DraggableSeparator, TRUE,
				LBCIA_Separator, TRUE,
		TAG_DONE);
#else
	/**\TODO write OS3-compatible version */
#endif

	if((flags & GW_CREATE_TAB) && existing)
	{
		g->shared = existing->shared;
		g->tab = g->shared->next_tab;
		g->shared->tabs++; /* do this early so functions know to update the tabs */

		if((g->shared->tabs == 2) && (nsoption_bool(tab_always_show) == false)) {
			ami_toggletabbar(g->shared, true);
		}

		SetGadgetAttrs((struct Gadget *)g->shared->objects[GID_TABS],
						g->shared->win, NULL,
						CLICKTAB_Labels, ~0,
						TAG_DONE);

		g->tab_node = AllocClickTabNode(TNA_Text, messages_get("NetSurf"),
								TNA_Number, g->tab,
								TNA_UserData, g,
								TNA_CloseGadget, TRUE,
								TAG_DONE);

		if(nsoption_bool(new_tab_last)) {
			AddTail(&g->shared->tab_list, g->tab_node);
		} else {
			struct Node *insert_after = existing->tab_node;

			if(g->shared->last_new_tab)
				insert_after = g->shared->last_new_tab;
			Insert(&g->shared->tab_list, g->tab_node, insert_after);
		}

		g->shared->last_new_tab = g->tab_node;

		RefreshSetGadgetAttrs((struct Gadget *)g->shared->objects[GID_TABS],
							g->shared->win, NULL,
							CLICKTAB_Labels, &g->shared->tab_list,
							TAG_DONE);

		if(nsoption_bool(new_tab_is_active)) {
			RefreshSetGadgetAttrs((struct Gadget *)g->shared->objects[GID_TABS],
							g->shared->win, NULL,
							CLICKTAB_Current, g->tab,
							TAG_DONE);
		}

		if(ClickTabBase->lib_Version < 53) {
			RethinkLayout((struct Gadget *)g->shared->objects[GID_TABLAYOUT],
				g->shared->win, NULL, TRUE);
		}

		g->shared->next_tab++;

		if(nsoption_bool(new_tab_is_active)) ami_switch_tab(g->shared,false);

		ami_update_buttons(g->shared);
		ami_schedule(0, ami_gui_refresh_favicon, g->shared);

		return g;
	}

	g->shared = calloc(1, sizeof(struct gui_window_2));

	if(!g->shared)
	{
		amiga_warn_user("NoMemory","");
		return NULL;
	}

	g->shared->shared_pens = ami_AllocMinList();

	g->shared->scrollerhook.h_Entry = (void *)ami_scroller_hook;
	g->shared->scrollerhook.h_Data = g->shared;

	g->shared->favicon_hook.h_Entry = (void *)ami_set_favicon_render_hook;
	g->shared->favicon_hook.h_Data = g->shared;

	g->shared->throbber_hook.h_Entry = (void *)ami_set_throbber_render_hook;
	g->shared->throbber_hook.h_Data = g->shared;

	g->shared->browser_hook.h_Entry = (void *)ami_gui_browser_render_hook;
	g->shared->browser_hook.h_Data = g->shared;

	newprefs_hook.h_Entry = (void *)ami_gui_newprefs_hook;
	newprefs_hook.h_Data = 0;
	
	g->shared->ctxmenu_hook = ami_ctxmenu_get_hook(g->shared);
	g->shared->history_ctxmenu[AMI_CTXMENU_HISTORY_BACK] = NULL;
	g->shared->history_ctxmenu[AMI_CTXMENU_HISTORY_FORWARD] = NULL;
	g->shared->clicktab_ctxmenu = NULL;

	if(nsoption_bool(window_simple_refresh) == true) {
		refresh_mode = WA_SimpleRefresh;
		defer_layout = FALSE; /* testing reveals this does work with SimpleRefresh,
								but the docs say it doesn't so err on the side of caution. */
	} else {
		refresh_mode = WA_SmartRefresh;
		defer_layout = TRUE;
	}

	if(!nsoption_bool(kiosk_mode))
	{
		ULONG addtabclosegadget = TAG_IGNORE;
		ULONG iconifygadget = FALSE;

#ifdef __amigaos4__
		if (nsoption_charp(pubscreen_name) && 
		    (locked_screen == TRUE) &&
		    (strcmp(nsoption_charp(pubscreen_name), "Workbench") == 0))
				iconifygadget = TRUE;
#endif

		NSLOG(netsurf, INFO, "Creating menu");
		struct Menu *menu = ami_gui_menu_create(g->shared);

		NewList(&g->shared->tab_list);
		g->tab_node = AllocClickTabNode(TNA_Text,messages_get("NetSurf"),
											TNA_Number, 0,
											TNA_UserData, g,
											TNA_CloseGadget, TRUE,
											TAG_DONE);
		AddTail(&g->shared->tab_list,g->tab_node);

		g->shared->web_search_list = ami_gui_opts_websearch();
		g->shared->search_bm = NULL;

		g->shared->tabs=1;
		g->shared->next_tab=1;

		g->shared->svbuffer = calloc(1, 2000);

		g->shared->helphints[GID_BACK] =
			translate_escape_chars(messages_get("HelpToolbarBack"));
		g->shared->helphints[GID_FORWARD] =
			translate_escape_chars(messages_get("HelpToolbarForward"));
		g->shared->helphints[GID_STOP] =
			translate_escape_chars(messages_get("HelpToolbarStop"));
		g->shared->helphints[GID_RELOAD] =
			translate_escape_chars(messages_get("HelpToolbarReload"));
		g->shared->helphints[GID_HOME] =
			translate_escape_chars(messages_get("HelpToolbarHome"));
		g->shared->helphints[GID_URL] =
			translate_escape_chars(messages_get("HelpToolbarURL"));
		g->shared->helphints[GID_SEARCHSTRING] =
			translate_escape_chars(messages_get("HelpToolbarWebSearch"));
		g->shared->helphints[GID_ADDTAB] =
			translate_escape_chars(messages_get("HelpToolbarAddTab"));

		g->shared->helphints[GID_PAGEINFO_INSECURE_BM] = ami_utf8_easy(messages_get("PageInfoInsecure"));
		g->shared->helphints[GID_PAGEINFO_LOCAL_BM] = ami_utf8_easy(messages_get("PageInfoLocal"));
		g->shared->helphints[GID_PAGEINFO_SECURE_BM] = ami_utf8_easy(messages_get("PageInfoSecure"));
		g->shared->helphints[GID_PAGEINFO_WARNING_BM] = ami_utf8_easy(messages_get("PageInfoWarning"));
		g->shared->helphints[GID_PAGEINFO_INTERNAL_BM] = ami_utf8_easy(messages_get("PageInfoInternal"));

		ami_get_theme_filename(nav_west, "theme_nav_west", false);
		ami_get_theme_filename(nav_west_s, "theme_nav_west_s", false);
		ami_get_theme_filename(nav_west_g, "theme_nav_west_g", false);
		ami_get_theme_filename(nav_east, "theme_nav_east", false);
		ami_get_theme_filename(nav_east_s, "theme_nav_east_s", false);
		ami_get_theme_filename(nav_east_g, "theme_nav_east_g", false);
		ami_get_theme_filename(stop, "theme_stop", false);
		ami_get_theme_filename(stop_s, "theme_stop_s", false);
		ami_get_theme_filename(stop_g, "theme_stop_g", false);
		ami_get_theme_filename(reload, "theme_reload", false);
		ami_get_theme_filename(reload_s, "theme_reload_s", false);
		ami_get_theme_filename(reload_g, "theme_reload_g", false);
		ami_get_theme_filename(home, "theme_home", false);
		ami_get_theme_filename(home_s, "theme_home_s", false);
		ami_get_theme_filename(home_g, "theme_home_g", false);
		ami_get_theme_filename(closetab, "theme_closetab", false);
		ami_get_theme_filename(closetab_s, "theme_closetab_s", false);
		ami_get_theme_filename(closetab_g, "theme_closetab_g", false);
		ami_get_theme_filename(addtab, "theme_addtab", false);
		ami_get_theme_filename(addtab_s, "theme_addtab_s", false);
		ami_get_theme_filename(addtab_g, "theme_addtab_g", false);
		ami_get_theme_filename(tabthrobber, "theme_tab_loading", false);
		ami_get_theme_filename(fave, "theme_fave", false);
		ami_get_theme_filename(unfave, "theme_unfave", false);
		ami_get_theme_filename(pi_insecure, "theme_pageinfo_insecure", false);
		ami_get_theme_filename(pi_internal, "theme_pageinfo_internal", false);
		ami_get_theme_filename(pi_local, "theme_pageinfo_local", false);
		ami_get_theme_filename(pi_secure, "theme_pageinfo_secure", false);
		ami_get_theme_filename(pi_warning, "theme_pageinfo_warning", false);

		g->shared->objects[GID_FAVE_ADD] = BitMapObj,
					BITMAP_SourceFile, fave,
					BITMAP_Screen, scrn,
					BITMAP_Masking, TRUE,
					BitMapEnd;

		g->shared->objects[GID_FAVE_RMV] = BitMapObj,
					BITMAP_SourceFile, unfave,
					BITMAP_Screen, scrn,
					BITMAP_Masking, TRUE,
					BitMapEnd;

		g->shared->objects[GID_ADDTAB_BM] = BitMapObj,
					BITMAP_SourceFile, addtab,
					BITMAP_SelectSourceFile, addtab_s,
					BITMAP_DisabledSourceFile, addtab_g,
					BITMAP_Screen, scrn,
					BITMAP_Masking, TRUE,
					BitMapEnd;

		g->shared->objects[GID_CLOSETAB_BM] = BitMapObj,
					BITMAP_SourceFile, closetab,
					BITMAP_SelectSourceFile, closetab_s,
					BITMAP_DisabledSourceFile, closetab_g,
					BITMAP_Screen, scrn,
					BITMAP_Masking, TRUE,
					BitMapEnd;

		g->shared->objects[GID_PAGEINFO_INSECURE_BM] = BitMapObj,
					BITMAP_SourceFile, pi_insecure,
					BITMAP_Screen, scrn,
					BITMAP_Masking, TRUE,
					BitMapEnd;

		g->shared->objects[GID_PAGEINFO_INTERNAL_BM] = BitMapObj,
					BITMAP_SourceFile, pi_internal,
					BITMAP_Screen, scrn,
					BITMAP_Masking, TRUE,
					BitMapEnd;

		g->shared->objects[GID_PAGEINFO_LOCAL_BM] = BitMapObj,
					BITMAP_SourceFile, pi_local,
					BITMAP_Screen, scrn,
					BITMAP_Masking, TRUE,
					BitMapEnd;

		g->shared->objects[GID_PAGEINFO_SECURE_BM] = BitMapObj,
					BITMAP_SourceFile, pi_secure,
					BITMAP_Screen, scrn,
					BITMAP_Masking, TRUE,
					BitMapEnd;

		g->shared->objects[GID_PAGEINFO_WARNING_BM] = BitMapObj,
					BITMAP_SourceFile, pi_warning,
					BITMAP_Screen, scrn,
					BITMAP_Masking, TRUE,
					BitMapEnd;


		if(ClickTabBase->lib_Version < 53)
		{
			addtabclosegadget = LAYOUT_AddChild;
			g->shared->objects[GID_CLOSETAB] = ButtonObj,
					GA_ID, GID_CLOSETAB,
					GA_RelVerify, TRUE,
					BUTTON_RenderImage, g->shared->objects[GID_CLOSETAB_BM],
					ButtonEnd;

			g->shared->objects[GID_TABS] = ClickTabObj,
					GA_ID,GID_TABS,
					GA_RelVerify,TRUE,
					GA_Underscore,13, // disable kb shortcuts
					CLICKTAB_Labels,&g->shared->tab_list,
					CLICKTAB_LabelTruncate,TRUE,
					ClickTabEnd;

			g->shared->objects[GID_ADDTAB] = ButtonObj,
					GA_ID, GID_ADDTAB,
					GA_RelVerify, TRUE,
					GA_Text, "+",
					BUTTON_RenderImage, g->shared->objects[GID_ADDTAB_BM],
					ButtonEnd;
		}
		else
		{
			g->shared->objects[GID_TABS_FLAG] = BitMapObj,
					BITMAP_SourceFile, tabthrobber,
					BITMAP_Screen,scrn,
					BITMAP_Masking,TRUE,
					BitMapEnd;
		}

		NSLOG(netsurf, INFO, "Creating window object");

		g->shared->objects[OID_MAIN] = WindowObj,
			WA_ScreenTitle, ami_gui_get_screen_title(),
			WA_Activate, TRUE,
			WA_DepthGadget, TRUE,
			WA_DragBar, TRUE,
			WA_CloseGadget, TRUE,
			WA_SizeGadget, TRUE,
			WA_Top,cury,
			WA_Left,curx,
			WA_Width,curw,
			WA_Height,curh,
			WA_PubScreen,scrn,
			WA_ReportMouse,TRUE,
			refresh_mode, TRUE,
			WA_SizeBBottom, TRUE,
			WA_ContextMenuHook, g->shared->ctxmenu_hook,
			WA_IDCMP, IDCMP_MENUPICK | IDCMP_MOUSEMOVE |
				IDCMP_MOUSEBUTTONS | IDCMP_NEWSIZE |
				IDCMP_RAWKEY | idcmp_sizeverify |
				IDCMP_GADGETUP | IDCMP_IDCMPUPDATE |
				IDCMP_REFRESHWINDOW |
				IDCMP_ACTIVEWINDOW | IDCMP_EXTENDEDMOUSE,
			WINDOW_IconifyGadget, iconifygadget,
			WINDOW_MenuStrip, menu,
			WINDOW_MenuUserData, WGUD_HOOK,
			WINDOW_NewPrefsHook, &newprefs_hook,
			WINDOW_IDCMPHook, &g->shared->scrollerhook,
			WINDOW_IDCMPHookBits, IDCMP_IDCMPUPDATE | IDCMP_REFRESHWINDOW |
						IDCMP_EXTENDEDMOUSE | IDCMP_SIZEVERIFY,
			WINDOW_SharedPort, sport,
			WINDOW_BuiltInScroll, TRUE,
			WINDOW_GadgetHelp, TRUE,
			WINDOW_UserData, g->shared,
  			WINDOW_ParentGroup, g->shared->objects[GID_MAIN] = LayoutVObj,
				LAYOUT_DeferLayout, defer_layout,
				LAYOUT_SpaceOuter, TRUE,
				LAYOUT_AddChild, g->shared->objects[GID_TOOLBARLAYOUT] = LayoutHObj,
					LAYOUT_VertAlignment, LALIGN_CENTER,
					LAYOUT_AddChild, g->shared->objects[GID_BACK] = ButtonObj,
						GA_ID, GID_BACK,
						GA_RelVerify, TRUE,
						GA_Disabled, TRUE,
						GA_ContextMenu, ami_ctxmenu_history_create(AMI_CTXMENU_HISTORY_BACK, g->shared),
						GA_HintInfo, g->shared->helphints[GID_BACK],
						BUTTON_RenderImage,BitMapObj,
							BITMAP_SourceFile,nav_west,
							BITMAP_SelectSourceFile,nav_west_s,
							BITMAP_DisabledSourceFile,nav_west_g,
							BITMAP_Screen,scrn,
							BITMAP_Masking,TRUE,
						BitMapEnd,
					ButtonEnd,
					CHILD_WeightedWidth,0,
					CHILD_WeightedHeight,0,
					LAYOUT_AddChild, g->shared->objects[GID_FORWARD] = ButtonObj,
						GA_ID, GID_FORWARD,
						GA_RelVerify, TRUE,
						GA_Disabled, TRUE,
						GA_ContextMenu, ami_ctxmenu_history_create(AMI_CTXMENU_HISTORY_FORWARD, g->shared),
						GA_HintInfo, g->shared->helphints[GID_FORWARD],
						BUTTON_RenderImage,BitMapObj,
							BITMAP_SourceFile,nav_east,
							BITMAP_SelectSourceFile,nav_east_s,
							BITMAP_DisabledSourceFile,nav_east_g,
							BITMAP_Screen,scrn,
							BITMAP_Masking,TRUE,
						BitMapEnd,
					ButtonEnd,
					CHILD_WeightedWidth,0,
					CHILD_WeightedHeight,0,
					LAYOUT_AddChild, g->shared->objects[GID_STOP] = ButtonObj,
						GA_ID,GID_STOP,
						GA_RelVerify,TRUE,
						GA_HintInfo, g->shared->helphints[GID_STOP],
						BUTTON_RenderImage,BitMapObj,
							BITMAP_SourceFile,stop,
							BITMAP_SelectSourceFile,stop_s,
							BITMAP_DisabledSourceFile,stop_g,
							BITMAP_Screen,scrn,
							BITMAP_Masking,TRUE,
						BitMapEnd,
					ButtonEnd,
					CHILD_WeightedWidth,0,
					CHILD_WeightedHeight,0,
					LAYOUT_AddChild, g->shared->objects[GID_RELOAD] = ButtonObj,
						GA_ID,GID_RELOAD,
						GA_RelVerify,TRUE,
						GA_HintInfo, g->shared->helphints[GID_RELOAD],
						BUTTON_RenderImage,BitMapObj,
							BITMAP_SourceFile,reload,
							BITMAP_SelectSourceFile,reload_s,
							BITMAP_DisabledSourceFile,reload_g,
							BITMAP_Screen,scrn,
							BITMAP_Masking,TRUE,
						BitMapEnd,
					ButtonEnd,
					CHILD_WeightedWidth,0,
					CHILD_WeightedHeight,0,
					LAYOUT_AddChild, g->shared->objects[GID_HOME] = ButtonObj,
						GA_ID,GID_HOME,
						GA_RelVerify,TRUE,
						GA_HintInfo, g->shared->helphints[GID_HOME],
						BUTTON_RenderImage,BitMapObj,
							BITMAP_SourceFile,home,
							BITMAP_SelectSourceFile,home_s,
							BITMAP_DisabledSourceFile,home_g,
							BITMAP_Screen,scrn,
							BITMAP_Masking,TRUE,
						BitMapEnd,
					ButtonEnd,
					CHILD_WeightedWidth,0,
					CHILD_WeightedHeight,0,
					LAYOUT_AddChild, LayoutHObj, // FavIcon, URL bar and hotlist star
						LAYOUT_VertAlignment, LALIGN_CENTER,
						LAYOUT_AddChild, g->shared->objects[GID_ICON] = SpaceObj,
							GA_ID, GID_ICON,
							SPACE_MinWidth, 16,
							SPACE_MinHeight, 16,
							SPACE_Transparent, TRUE,
						//	SPACE_RenderHook, &g->shared->favicon_hook,
						SpaceEnd,
						CHILD_WeightedWidth, 0,
						CHILD_WeightedHeight, 0,
						LAYOUT_AddChild, g->shared->objects[GID_PAGEINFO] = ButtonObj,
							GA_ID, GID_PAGEINFO,
							GA_RelVerify, TRUE,
							GA_ReadOnly, FALSE,
							BUTTON_RenderImage, g->shared->objects[GID_PAGEINFO_INTERNAL_BM],
						ButtonEnd,
						CHILD_WeightedWidth, 0,
						CHILD_WeightedHeight, 0,
						LAYOUT_AddChild, g->shared->objects[GID_URL] =
#ifdef __amigaos4__
							NewObject(urlStringClass, NULL,
#else
							StringObj,
#endif
									STRINGA_MaxChars, 2000,
									GA_ID, GID_URL,
									GA_RelVerify, TRUE,
									GA_HintInfo, g->shared->helphints[GID_URL],
									GA_TabCycle, TRUE,
									STRINGA_Buffer, g->shared->svbuffer,
#ifdef __amigaos4__
									STRINGVIEW_Header, URLHistory_GetList(),
#endif
							TAG_DONE),
						LAYOUT_AddChild, g->shared->objects[GID_FAVE] = ButtonObj,
							GA_ID, GID_FAVE,
							GA_RelVerify, TRUE,
						//	GA_HintInfo, g->shared->helphints[GID_FAVE],
							BUTTON_RenderImage, g->shared->objects[GID_FAVE_ADD],
						ButtonEnd,
						CHILD_WeightedWidth, 0,
						CHILD_WeightedHeight, 0,
					LayoutEnd,
				//	GA_ID, GID_TOOLBARLAYOUT,
				//	GA_RelVerify, TRUE,
				//	LAYOUT_RelVerify, TRUE,
					LAYOUT_WeightBar, TRUE,
					LAYOUT_AddChild, LayoutHObj,
						LAYOUT_VertAlignment, LALIGN_CENTER,
						LAYOUT_AddChild, g->shared->objects[GID_SEARCH_ICON] = ChooserObj,
							GA_ID, GID_SEARCH_ICON,
							GA_RelVerify, TRUE,
							CHOOSER_DropDown, TRUE,
							CHOOSER_Labels, g->shared->web_search_list,
							CHOOSER_MaxLabels, 40, /* Same as options GUI */
						ChooserEnd,
						CHILD_WeightedWidth,0,
						CHILD_WeightedHeight,0,
						LAYOUT_AddChild, g->shared->objects[GID_SEARCHSTRING] = StringObj,
							GA_ID,GID_SEARCHSTRING,
                 					STRINGA_TextVal, NULL,
							GA_RelVerify,TRUE,
							GA_HintInfo, g->shared->helphints[GID_SEARCHSTRING],
						StringEnd,
					LayoutEnd,
					CHILD_WeightedWidth, nsoption_int(web_search_width),
					LAYOUT_AddChild, g->shared->objects[GID_THROBBER] = SpaceObj,
						GA_ID,GID_THROBBER,
						SPACE_MinWidth, ami_theme_throbber_get_width(),
						SPACE_MinHeight, ami_theme_throbber_get_height(),
						SPACE_Transparent,TRUE,
					//	SPACE_RenderHook, &g->shared->throbber_hook,
					SpaceEnd,
					CHILD_WeightedWidth,0,
					CHILD_WeightedHeight,0,
				LayoutEnd,
				CHILD_WeightedHeight,0,
				LAYOUT_AddImage, BevelObj,
					BEVEL_Style, BVS_SBAR_VERT,
				BevelEnd,
				CHILD_WeightedHeight, 0,
				LAYOUT_AddChild, g->shared->objects[GID_HOTLISTLAYOUT] = LayoutVObj,
					LAYOUT_SpaceInner, FALSE,
				LayoutEnd,
				CHILD_WeightedHeight,0,
				LAYOUT_AddChild, g->shared->objects[GID_TABLAYOUT] = LayoutHObj,
					LAYOUT_SpaceInner,FALSE,
					addtabclosegadget, g->shared->objects[GID_CLOSETAB],
					CHILD_WeightedWidth,0,
					CHILD_WeightedHeight,0,

					addtabclosegadget, g->shared->objects[GID_TABS],
					CHILD_CacheDomain,FALSE,

					addtabclosegadget, g->shared->objects[GID_ADDTAB],
					CHILD_WeightedWidth,0,
					CHILD_WeightedHeight,0,
				LayoutEnd,
				CHILD_WeightedHeight,0,
				LAYOUT_AddChild, LayoutVObj,
					LAYOUT_AddChild, g->shared->objects[GID_VSCROLLLAYOUT] = LayoutHObj,
						LAYOUT_AddChild, LayoutVObj,
							LAYOUT_AddChild, g->shared->objects[GID_HSCROLLLAYOUT] = LayoutVObj,
								LAYOUT_AddChild, g->shared->objects[GID_BROWSER] = SpaceObj,
									GA_ID,GID_BROWSER,
									SPACE_Transparent,TRUE,
									SPACE_RenderHook, &g->shared->browser_hook,
								SpaceEnd,
							EndGroup,
						EndGroup,
					EndGroup,
//					LAYOUT_WeightBar, TRUE,
					LAYOUT_AddChild, g->shared->objects[GID_LOGLAYOUT] = LayoutVObj,
					EndGroup,
					CHILD_WeightedHeight, 0,
#ifndef __amigaos4__
					LAYOUT_AddChild, g->shared->objects[GID_STATUS] = StringObj,
						GA_ID, GID_STATUS,
							GA_ReadOnly, TRUE,
             				STRINGA_TextVal, NULL,
						GA_RelVerify, TRUE,
					StringEnd,
#endif
				EndGroup,
			EndGroup,
		EndWindow;
	}
	else
	{
		/* borderless kiosk mode window */
		g->tab = 0;
		g->shared->tabs = 0;
		g->tab_node = NULL;

		g->shared->objects[OID_MAIN] = WindowObj,
       	    WA_ScreenTitle, ami_gui_get_screen_title(),
           	WA_Activate, TRUE,
           	WA_DepthGadget, FALSE,
       	   	WA_DragBar, FALSE,
           	WA_CloseGadget, FALSE,
			WA_Borderless,TRUE,
			WA_RMBTrap,TRUE,
			WA_Top,0,
			WA_Left,0,
			WA_Width, scrn->Width,
			WA_Height, scrn->Height,
           	WA_SizeGadget, FALSE,
			WA_PubScreen, scrn,
			WA_ReportMouse, TRUE,
			refresh_mode, TRUE,
       	   	WA_IDCMP, IDCMP_MENUPICK | IDCMP_MOUSEMOVE |
					IDCMP_MOUSEBUTTONS | IDCMP_NEWSIZE |
					IDCMP_RAWKEY | IDCMP_REFRESHWINDOW |
					IDCMP_GADGETUP | IDCMP_IDCMPUPDATE |
					IDCMP_EXTENDEDMOUSE,
			WINDOW_IDCMPHook,&g->shared->scrollerhook,
			WINDOW_IDCMPHookBits, IDCMP_IDCMPUPDATE |
					IDCMP_EXTENDEDMOUSE | IDCMP_REFRESHWINDOW,
			WINDOW_SharedPort,sport,
			WINDOW_UserData,g->shared,
			WINDOW_BuiltInScroll,TRUE,
			WINDOW_ParentGroup, g->shared->objects[GID_MAIN] = LayoutHObj,
			LAYOUT_DeferLayout, defer_layout,
			LAYOUT_SpaceOuter, TRUE,
				LAYOUT_AddChild, g->shared->objects[GID_VSCROLLLAYOUT] = LayoutHObj,
					LAYOUT_AddChild, g->shared->objects[GID_HSCROLLLAYOUT] = LayoutVObj,
						LAYOUT_AddChild, g->shared->objects[GID_BROWSER] = SpaceObj,
							GA_ID,GID_BROWSER,
							SPACE_Transparent,TRUE,
						SpaceEnd,
					EndGroup,
				EndGroup,
			EndGroup,
		EndWindow;
	}

	NSLOG(netsurf, INFO, "Opening window");

	g->shared->win = (struct Window *)RA_OpenWindow(g->shared->objects[OID_MAIN]);

	NSLOG(netsurf, INFO, "Window opened, adding border gadgets");

	if(!g->shared->win)
	{
		amiga_warn_user("NoMemory","");
		free(g->shared);
		free(g);
		return NULL;
	}

	if(nsoption_bool(kiosk_mode) == false)
	{
#ifdef __amigaos4__
		ULONG width, height;
		struct DrawInfo *dri = GetScreenDrawInfo(scrn);
		
		ami_get_border_gadget_size(g->shared,
				(ULONG *)&width, (ULONG *)&height);

		g->shared->objects[GID_STATUS] = NewObject(
				NULL,
				"frbuttonclass",
				GA_ID, GID_STATUS,
				GA_Left, scrn->WBorLeft + 2,
				GA_RelBottom, scrn->WBorBottom - (height/2),
				GA_BottomBorder, TRUE,
				GA_Width, width,
				GA_Height, 1 + height - scrn->WBorBottom,
				GA_DrawInfo, dri,
				GA_ReadOnly, TRUE,
				GA_Disabled, TRUE,
				GA_Image, (struct Image *)NewObject(
					NULL,
					"gaugeiclass",
					GAUGEIA_Level, 0,
					IA_Top, (int)(- ceil((scrn->WBorBottom + height) / 2)),
					IA_Left, -4,
					IA_Height, 2 + height - scrn->WBorBottom, 
					IA_Label, NULL,
					IA_InBorder, TRUE,
					IA_Screen, scrn,
					TAG_DONE),
				TAG_DONE);

		AddGList(g->shared->win, (struct Gadget *)g->shared->objects[GID_STATUS],
				(UWORD)~0, -1, NULL);

		/* Apparently you can't set GA_Width on creation time for frbuttonclass */

		SetGadgetAttrs((struct Gadget *)g->shared->objects[GID_STATUS],
			g->shared->win, NULL,
			GA_Width, width,
			TAG_DONE);

		RefreshGadgets((APTR)g->shared->objects[GID_STATUS],
				g->shared->win, NULL);
				
		FreeScreenDrawInfo(scrn, dri);
#endif //__amigaos4__				
		ami_gui_hotlist_toolbar_add(g->shared); /* is this the right place for this? */
		if(nsoption_bool(tab_always_show)) ami_toggletabbar(g->shared, true);
	}

	g->shared->gw = g;
	cur_gw = g;

	g->shared->appwin = AddAppWindowA((ULONG)g->shared->objects[OID_MAIN],
							(ULONG)g->shared, g->shared->win, appport, NULL);

	ami_gui_win_list_add(g->shared, AMINS_WINDOW, &ami_gui_table);

	if(locked_screen) {
		UnlockPubScreen(NULL,scrn);
		locked_screen = FALSE;
	}

	ami_schedule(0, ami_gui_search_ico_refresh, NULL);

	ScreenToFront(scrn);

	return g;
}

static void ami_gui_close_tabs(struct gui_window_2 *gwin, bool other_tabs)
{
	struct Node *tab;
	struct Node *ntab;
	struct gui_window *gw;

	if((gwin->tabs > 1) && (nsoption_bool(tab_close_warn) == true)) {
		int32 res = amiga_warn_user_multi(messages_get("MultiTabClose"), "Yes", "No", gwin->win);

		if(res == 0) return;
	}

	if(gwin->tabs) {
		tab = GetHead(&gwin->tab_list);

		do {
			ntab=GetSucc(tab);
			GetClickTabNodeAttrs(tab,
								TNA_UserData,&gw,
								TAG_DONE);

			if((other_tabs == false) || (gwin->gw != gw)) {
				browser_window_destroy(gw->bw);
			}
		} while((tab=ntab));
	} else {
		if(other_tabs == false) browser_window_destroy(gwin->gw->bw);
	}
}

void ami_gui_close_window(void *w)
{
	struct gui_window_2 *gwin = (struct gui_window_2 *)w;
	ami_gui_close_tabs(gwin, false);
}

void ami_gui_close_inactive_tabs(struct gui_window_2 *gwin)
{
	ami_gui_close_tabs(gwin, true);
}

static void gui_window_destroy(struct gui_window *g)
{
	struct Node *ptab = NULL;
	int gid;

	if(!g) return;

	if (ami_search_get_gwin(g->shared->searchwin) == g)
	{
		ami_search_close();
		win_destroyed = true;
	}

	if(g->hw)
	{
		ami_history_local_destroy(g->hw);
		win_destroyed = true;
	}

	ami_free_download_list(&g->dllist);
	FreeObjList(g->deferred_rects);
	ami_memory_itempool_delete(g->deferred_rects_pool);
	gui_window_stop_throbber(g);

	cur_gw = NULL;

	if(g->shared->tabs > 1) {
		SetGadgetAttrs((struct Gadget *)g->shared->objects[GID_TABS],g->shared->win,NULL,
						CLICKTAB_Labels,~0,
						TAG_DONE);

		GetAttr(CLICKTAB_CurrentNode, g->shared->objects[GID_TABS], (ULONG *)&ptab);

		if(ptab == g->tab_node) {
			ptab = GetSucc(g->tab_node);
			if(!ptab) ptab = GetPred(g->tab_node);
		}

		Remove(g->tab_node);
		FreeClickTabNode(g->tab_node);
		RefreshSetGadgetAttrs((struct Gadget *)g->shared->objects[GID_TABS], g->shared->win, NULL,
						CLICKTAB_Labels, &g->shared->tab_list,
						CLICKTAB_CurrentNode, ptab,
						TAG_DONE);

		if(ClickTabBase->lib_Version < 53)
			RethinkLayout((struct Gadget *)g->shared->objects[GID_TABLAYOUT],
				g->shared->win, NULL, TRUE);

		g->shared->tabs--;
		ami_switch_tab(g->shared,true);
		ami_schedule(0, ami_gui_refresh_favicon, g->shared);

		if((g->shared->tabs == 1) && (nsoption_bool(tab_always_show) == false))
			ami_toggletabbar(g->shared, false);

		FreeListBrowserList(&g->loglist);
#ifdef __amigaos4__
		FreeLBColumnInfo(g->logcolumns);
#endif

		if(g->tabtitle) free(g->tabtitle);
		free(g);
		return;
	}

	ami_plot_release_pens(g->shared->shared_pens);
	free(g->shared->shared_pens);
	ami_schedule_redraw_remove(g->shared);
	ami_schedule(-1, ami_gui_refresh_favicon, g->shared);

	DisposeObject(g->shared->objects[OID_MAIN]);
	ami_gui_appicon_remove(g->shared);
	if(g->shared->appwin) RemoveAppWindow(g->shared->appwin);
	ami_gui_hotlist_toolbar_free(g->shared, &g->shared->hotlist_toolbar_list);

	/* These aren't freed by the above.
	 * TODO: nav_west etc need freeing too? */
	DisposeObject(g->shared->objects[GID_ADDTAB_BM]);
	DisposeObject(g->shared->objects[GID_CLOSETAB_BM]);
	DisposeObject(g->shared->objects[GID_TABS_FLAG]);
	DisposeObject(g->shared->objects[GID_FAVE_ADD]);
	DisposeObject(g->shared->objects[GID_FAVE_RMV]);
	DisposeObject(g->shared->objects[GID_PAGEINFO_INSECURE_BM]);
	DisposeObject(g->shared->objects[GID_PAGEINFO_INTERNAL_BM]);
	DisposeObject(g->shared->objects[GID_PAGEINFO_LOCAL_BM]);
	DisposeObject(g->shared->objects[GID_PAGEINFO_SECURE_BM]);
	DisposeObject(g->shared->objects[GID_PAGEINFO_WARNING_BM]);

	ami_gui_opts_websearch_free(g->shared->web_search_list);
	if(g->shared->search_bm) DisposeObject(g->shared->search_bm);

	/* This appears to be disposed along with the ClickTab object
	if(g->shared->clicktab_ctxmenu) DisposeObject((Object *)g->shared->clicktab_ctxmenu); */
	DisposeObject((Object *)g->shared->history_ctxmenu[AMI_CTXMENU_HISTORY_BACK]);
	DisposeObject((Object *)g->shared->history_ctxmenu[AMI_CTXMENU_HISTORY_FORWARD]);
	ami_ctxmenu_release_hook(g->shared->ctxmenu_hook);
	ami_gui_menu_free(g->shared);

	FreeListBrowserList(&g->loglist);
#ifdef __amigaos4__
	FreeLBColumnInfo(g->logcolumns);
#endif

	free(g->shared->wintitle);
	ami_utf8_free(g->shared->status);
	free(g->shared->svbuffer);

	for(gid = 0; gid < GID_LAST; gid++)
		ami_utf8_free(g->shared->helphints[gid]);

	ami_gui_win_list_remove(g->shared);
	if(g->tab_node) {
		Remove(g->tab_node);
		FreeClickTabNode(g->tab_node);
	}
	if(g->tabtitle) free(g->tabtitle);
	free(g); // g->shared should be freed by DelObject()

	if(IsMinListEmpty(window_list))
	{
		/* last window closed, so exit */
		ami_try_quit();
	}

	win_destroyed = true;
}


static void ami_redraw_callback(void *p)
{
	struct gui_window_2 *gwin = (struct gui_window_2 *)p;

	if(gwin->redraw_required) {
		ami_do_redraw(gwin);
	}

	ami_gui_window_update_box_deferred(gwin->gw, true);

	if(gwin->gw->c_h)
	{
		gui_window_place_caret(gwin->gw, gwin->gw->c_x,
		gwin->gw->c_y, gwin->gw->c_h, NULL);
	}
}

/**
 * Schedule a redraw of the browser window - Amiga-specific function
 *
 * \param  gwin         a struct gui_window_2
 * \param  full_redraw  set to true to schedule a full redraw,
                        should only be set to false when called from amiga_window_invalidate_area()
 */
void ami_schedule_redraw(struct gui_window_2 *gwin, bool full_redraw)
{
	int ms = 1;

	if(full_redraw) gwin->redraw_required = true;
	ami_schedule(ms, ami_redraw_callback, gwin);
}

static void ami_schedule_redraw_remove(struct gui_window_2 *gwin)
{
	ami_schedule(-1, ami_redraw_callback, gwin);
}

static void ami_gui_window_update_box_deferred(struct gui_window *g, bool draw)
{
	struct nsObject *node;
	struct nsObject *nnode;
	struct rect *rect;
	
	if(!g) return;
	if(IsMinListEmpty(g->deferred_rects)) return;

	if(draw == true) {
		ami_set_pointer(g->shared, GUI_POINTER_WAIT, false);
	} else {
		NSLOG(netsurf, INFO, "Ignoring deferred box redraw queue");
	}

	node = (struct nsObject *)GetHead((struct List *)g->deferred_rects);

	do {
		if(draw == true) {
			rect = (struct rect *)node->objstruct;
			ami_do_redraw_limits(g, g->bw, false,
				rect->x0, rect->y0, rect->x1, rect->y1);
		}
		nnode=(struct nsObject *)GetSucc((struct Node *)node);
		ami_memory_itempool_free(g->deferred_rects_pool, node->objstruct, sizeof(struct rect));
		DelObjectNoFree(node);
	} while((node = nnode));

	if(draw == true) ami_reset_pointer(g->shared);
}

bool ami_gui_window_update_box_deferred_check(struct MinList *deferred_rects,
				const struct rect *restrict new_rect, APTR mempool)
{
	struct nsObject *node;
	struct nsObject *nnode;
	struct rect *restrict rect;
	
	if(IsMinListEmpty(deferred_rects)) return true;

	node = (struct nsObject *)GetHead((struct List *)deferred_rects);

	do {
		nnode=(struct nsObject *)GetSucc((struct Node *)node);
		rect = (struct rect *)node->objstruct;
		
		if((rect->x0 <= new_rect->x0) &&
			(rect->y0 <= new_rect->y0) &&
			(rect->x1 >= new_rect->x1) &&
			(rect->y1 >= new_rect->y1)) {
			return false;
		}
		
		if ((new_rect->x0 <= rect->x0) &&
			(new_rect->y0 <= rect->y0) &&
			(new_rect->x1 >= rect->x1) &&
			(new_rect->y1 >= rect->y1)) {
			NSLOG(netsurf, INFO,
			      "Removing queued redraw that is a subset of new box redraw");
			ami_memory_itempool_free(mempool, node->objstruct, sizeof(struct rect));
			DelObjectNoFree(node);
			/* Don't return - we might find more */
		}
	} while((node = nnode));

	return true;
}


static void ami_do_redraw(struct gui_window_2 *gwin)
{
	ULONG hcurrent,vcurrent,xoffset,yoffset,width=800,height=600;
	struct IBox *bbox;
	ULONG oldh = gwin->oldh, oldv=gwin->oldv;

	if(browser_window_redraw_ready(gwin->gw->bw) == false) return;

	ami_get_hscroll_pos(gwin, (ULONG *)&hcurrent);
	ami_get_vscroll_pos(gwin, (ULONG *)&vcurrent);

	gwin->gw->scrollx = hcurrent;
	gwin->gw->scrolly = vcurrent;

	if(ami_gui_get_space_box((Object *)gwin->objects[GID_BROWSER], &bbox) != NSERROR_OK) {
		amiga_warn_user("NoMemory", "");
		return;
	}

	width=bbox->Width;
	height=bbox->Height;
	xoffset=bbox->Left;
	yoffset=bbox->Top;

	if(gwin->redraw_scroll)
	{
		if((abs(vcurrent-oldv) > height) ||	(abs(hcurrent-oldh) > width))
			gwin->redraw_scroll = false;

 		if(gwin->new_content) gwin->redraw_scroll = false;
	}

	if(gwin->redraw_scroll)
	{
		struct rect rect;
		
		gwin->gw->c_h_temp = gwin->gw->c_h;
		gui_window_remove_caret(gwin->gw);

		ScrollWindowRaster(gwin->win, hcurrent - oldh, vcurrent - oldv,
				xoffset, yoffset, xoffset + width - 1, yoffset + height - 1);

		gwin->gw->c_h = gwin->gw->c_h_temp;

		if(vcurrent>oldv) /* Going down */
		{
			ami_spacebox_to_ns_coords(gwin, &rect.x0, &rect.y0, 0, height - (vcurrent - oldv) - 1);
			ami_spacebox_to_ns_coords(gwin, &rect.x1, &rect.y1, width + 1, height + 1);
			amiga_window_invalidate_area(gwin->gw, &rect);
		}
		else if(vcurrent<oldv) /* Going up */
		{
			ami_spacebox_to_ns_coords(gwin, &rect.x0, &rect.y0, 0, 0);
			ami_spacebox_to_ns_coords(gwin, &rect.x1, &rect.y1, width + 1, oldv - vcurrent + 1);
			amiga_window_invalidate_area(gwin->gw, &rect);
		}

		if(hcurrent>oldh) /* Going right */
		{
			ami_spacebox_to_ns_coords(gwin, &rect.x0, &rect.y0, width - (hcurrent - oldh), 0);
			ami_spacebox_to_ns_coords(gwin, &rect.x1, &rect.y1, width + 1, height + 1);
			amiga_window_invalidate_area(gwin->gw, &rect);
		}
		else if(hcurrent<oldh) /* Going left */
		{
			ami_spacebox_to_ns_coords(gwin, &rect.x0, &rect.y0, 0, 0);
			ami_spacebox_to_ns_coords(gwin, &rect.x1, &rect.y1, oldh - hcurrent + 1, height + 1);
			amiga_window_invalidate_area(gwin->gw, &rect);
		}
	}
	else
	{
		struct redraw_context ctx = {
			.interactive = true,
			.background_images = true,
			.plot = &amiplot,
			.priv = browserglob
		};

		ami_do_redraw_tiled(gwin, true, hcurrent, vcurrent, width, height, hcurrent, vcurrent, bbox, &ctx);

		/* Tell NetSurf not to bother with the next queued box redraw, as we've redrawn everything. */
		ami_gui_window_update_box_deferred(gwin->gw, false);
	}

	ami_update_buttons(gwin);

	gwin->oldh = hcurrent;
	gwin->oldv = vcurrent;

	gwin->redraw_scroll = false;
	gwin->redraw_required = false;
	gwin->new_content = false;

	ami_gui_free_space_box(bbox);
}


static void ami_get_hscroll_pos(struct gui_window_2 *gwin, ULONG *xs)
{
	if(gwin->objects[GID_HSCROLL])
	{
		GetAttr(SCROLLER_Top, (Object *)gwin->objects[GID_HSCROLL], xs);
	} else {
		*xs = 0;
	}
}

static void ami_get_vscroll_pos(struct gui_window_2 *gwin, ULONG *ys)
{
	if(gwin->objects[GID_VSCROLL]) {
		GetAttr(SCROLLER_Top, gwin->objects[GID_VSCROLL], ys);
	} else {
		*ys = 0;
	}
}

static bool gui_window_get_scroll(struct gui_window *g, int *restrict sx, int *restrict sy)
{
	ami_get_hscroll_pos(g->shared, (ULONG *)sx);
	ami_get_vscroll_pos(g->shared, (ULONG *)sy);

	return true;
}

/**
 * Set the scroll position of a amiga browser window.
 *
 * Scrolls the viewport to ensure the specified rectangle of the
 *   content is shown. The amiga implementation scrolls the contents so
 *   the specified point in the content is at the top of the viewport.
 *
 * \param g gui_window to scroll
 * \param rect The rectangle to ensure is shown.
 * \return NSERROR_OK on success or apropriate error code.
 */
static nserror
gui_window_set_scroll(struct gui_window *g, const struct rect *rect)
{
	struct IBox *bbox;
	int width, height;
	nserror res;
	int sx = 0, sy = 0;

	if(!g) {
		return NSERROR_BAD_PARAMETER;
	}
	if(!g->bw || browser_window_has_content(g->bw) == false) {
		return NSERROR_BAD_PARAMETER;
	}

	res = ami_gui_get_space_box((Object *)g->shared->objects[GID_BROWSER], &bbox);
	if(res != NSERROR_OK) {
		amiga_warn_user("NoMemory", "");
		return res;
	}

	if (rect->x0 > 0) {
		sx = rect->x0;
	}
	if (rect->y0 > 0) {
		sy = rect->y0;
	}

	browser_window_get_extents(g->bw, false, &width, &height);

	if(sx >= width - bbox->Width)
		sx = width - bbox->Width;
	if(sy >= height - bbox->Height)
		sy = height - bbox->Height;

	if(width <= bbox->Width) sx = 0;
	if(height <= bbox->Height) sy = 0;

	ami_gui_free_space_box(bbox);

	if(g == g->shared->gw) {
		if(g->shared->objects[GID_VSCROLL]) {
			RefreshSetGadgetAttrs((struct Gadget *)(APTR)g->shared->objects[GID_VSCROLL],
				g->shared->win, NULL,
				SCROLLER_Top, (ULONG)(sy),
			TAG_DONE);
		}

		if(g->shared->objects[GID_HSCROLL])
		{
			RefreshSetGadgetAttrs((struct Gadget *)(APTR)g->shared->objects[GID_HSCROLL],
				g->shared->win, NULL,
				SCROLLER_Top, (ULONG)(sx),
				TAG_DONE);
		}

		ami_schedule_redraw(g->shared, true);

		if(nsoption_bool(faster_scroll) == true) g->shared->redraw_scroll = true;
			else g->shared->redraw_scroll = false;

		g->scrollx = sx;
		g->scrolly = sy;
	}
	return NSERROR_OK;
}

static void gui_window_set_status(struct gui_window *g, const char *text)
{
	char *utf8text;
	ULONG size;
	UWORD chars;
	struct TextExtent textex;

	if(!g) return;
	if(!text) return;
	if(!g->shared->objects[GID_STATUS]) return;

	if(g == g->shared->gw) {
		utf8text = ami_utf8_easy((char *)text);
		if(utf8text == NULL) return;

		GetAttr(GA_Width, g->shared->objects[GID_STATUS], (ULONG *)&size);
		chars = TextFit(&scrn->RastPort, utf8text, (UWORD)strlen(utf8text),
					&textex, NULL, 1, size - 4, scrn->RastPort.TxHeight);

		utf8text[chars] = 0;

		SetGadgetAttrs((struct Gadget *)g->shared->objects[GID_STATUS],
			g->shared->win, NULL,
			NSA_STATUS_TEXT, utf8text,
			TAG_DONE);

		RefreshGList((struct Gadget *)g->shared->objects[GID_STATUS],
				g->shared->win, NULL, 1);

		if(g->shared->status) ami_utf8_free(g->shared->status);
		g->shared->status = utf8text;
	}
}

static nserror gui_window_set_url(struct gui_window *g, nsurl *url)
{
	size_t idn_url_l;
	char *idn_url_s = NULL;
	char *url_lc = NULL;

	if(!g) return NSERROR_OK;

	if(g == g->shared->gw) {
		if(nsoption_bool(display_decoded_idn) == true) {
			if (nsurl_get_utf8(url, &idn_url_s, &idn_url_l) == NSERROR_OK) {
				url_lc = ami_utf8_easy(idn_url_s);
			}
		}

		RefreshSetGadgetAttrs((struct Gadget *)g->shared->objects[GID_URL],
								g->shared->win, NULL,
								STRINGA_TextVal, url_lc ? url_lc : nsurl_access(url),
								TAG_DONE);

		if(url_lc) {
			ami_utf8_free(url_lc);
			if(idn_url_s) free(idn_url_s);
		}
	}

	ami_update_buttons(g->shared);

	return NSERROR_OK;
}

HOOKF(uint32, ami_set_favicon_render_hook, APTR, space, struct gpRender *)
{
	ami_schedule(0, ami_gui_refresh_favicon, hook->h_Data);
	return 0;
}

/**
 * Gui callback when search provider details are updated.
 *
 * \param provider_name The providers name.
 * \param ico_bitmap The icon bitmap representing the provider.
 * \return NSERROR_OK on success else error code.
 */
static nserror gui_search_web_provider_update(const char *provider_name,
	struct bitmap *ico_bitmap)
{
	struct BitMap *bm = NULL;
	struct nsObject *node;
	struct nsObject *nnode;
	struct gui_window_2 *gwin;

	if(IsMinListEmpty(window_list))	return NSERROR_BAD_PARAMETER;
	if(nsoption_bool(kiosk_mode) == true) return NSERROR_BAD_PARAMETER;

	if (ico_bitmap != NULL) {
		bm = ami_bitmap_get_native(ico_bitmap, 16, 16, ami_plot_screen_is_palettemapped(), NULL);
	}

	if(bm == NULL) return NSERROR_BAD_PARAMETER;

	node = (struct nsObject *)GetHead((struct List *)window_list);

	do {
		nnode=(struct nsObject *)GetSucc((struct Node *)node);
		gwin = node->objstruct;

		if(node->Type == AMINS_WINDOW)
		{
			if(gwin->search_bm != NULL)
				DisposeObject(gwin->search_bm);

			ULONG bm_masking_tag = TAG_IGNORE;

			if(LIB_IS_AT_LEAST((struct Library *)ChooserBase, 53, 21)) {
				/* Broken in earlier versions */
				bm_masking_tag = BITMAP_Masking;
			}

			gwin->search_bm = BitMapObj,
						BITMAP_Screen, scrn,
						BITMAP_Width, 16,
						BITMAP_Height, 16,
						BITMAP_BitMap, bm,
						BITMAP_HasAlpha, TRUE,
						bm_masking_tag, TRUE,
					BitMapEnd;

			RefreshSetGadgetAttrs((struct Gadget *)gwin->objects[GID_SEARCH_ICON],
				gwin->win, NULL,
				GA_HintInfo, provider_name,
				GA_Image, gwin->search_bm,
				TAG_DONE);
		}
	} while((node = nnode));

	return NSERROR_OK;
}

HOOKF(uint32, ami_set_throbber_render_hook, APTR, space, struct gpRender *)
{
	struct gui_window_2 *gwin = hook->h_Data;
	ami_throbber_redraw_schedule(0, gwin->gw);
	return 0;
}

HOOKF(uint32, ami_gui_browser_render_hook, APTR, space, struct gpRender *)
{
	struct gui_window_2 *gwin = hook->h_Data;

	NSLOG(netsurf, DEBUG, "Render hook called with %ld (REDRAW=1)", msg->gpr_Redraw);

	if(msg->gpr_Redraw != GREDRAW_REDRAW) return 0;

	ami_schedule_redraw(gwin, true);

	return 0;
}

static void gui_window_place_caret(struct gui_window *g, int x, int y, int height,
		const struct rect *clip)
{
	struct IBox *bbox;
	int xs,ys;

	if(!g) return;

	gui_window_remove_caret(g);

	xs = g->scrollx;
	ys = g->scrolly;

	SetAPen(g->shared->win->RPort,3);

	if(ami_gui_get_space_box((Object *)g->shared->objects[GID_BROWSER], &bbox) != NSERROR_OK) {
		amiga_warn_user("NoMemory", "");
		return;
	}

	if((y-ys+height) > (bbox->Height)) height = bbox->Height-y+ys;

	if(((x-xs) <= 0) || ((x-xs+2) >= (bbox->Width)) || ((y-ys) <= 0) || ((y-ys) >= (bbox->Height))) {
		ami_gui_free_space_box(bbox);
		return;
	}

	g->c_w = 2;

	SetDrMd(g->shared->win->RPort,COMPLEMENT);
	RectFill(g->shared->win->RPort, x + bbox->Left - xs, y + bbox->Top - ys,
		x + bbox->Left + g->c_w - xs, y+bbox->Top + height - ys);
	SetDrMd(g->shared->win->RPort,JAM1);

	ami_gui_free_space_box(bbox);

	g->c_x = x;
	g->c_y = y;
	g->c_h = height;

	if((nsoption_bool(kiosk_mode) == false))
		ami_gui_menu_set_disabled(g->shared->win, g->shared->imenu, M_PASTE, false);
}

static void gui_window_remove_caret(struct gui_window *g)
{
	if(!g) return;
	if(g->c_h == 0) return;

	if((nsoption_bool(kiosk_mode) == false))
		ami_gui_menu_set_disabled(g->shared->win, g->shared->imenu, M_PASTE, true);

	ami_do_redraw_limits(g, g->bw, false, g->c_x, g->c_y,
		g->c_x + g->c_w + 1, g->c_y + g->c_h + 1);

	g->c_h = 0;
}

static void gui_window_new_content(struct gui_window *g)
{
	struct hlcache_handle *c;

	if(g && g->shared && g->bw && browser_window_has_content(g->bw))
		c = browser_window_get_content(g->bw);
	else return;

	ami_clearclipreg(browserglob);
	g->shared->new_content = true;
	g->scrollx = 0;
	g->scrolly = 0;
	g->shared->oldh = 0;
	g->shared->oldv = 0;
	g->favicon = NULL;
	ami_plot_release_pens(g->shared->shared_pens);
	ami_gui_menu_update_disabled(g, c);
	ami_gui_update_hotlist_button(g->shared);
	ami_gui_scroller_update(g->shared);
}

static bool gui_window_drag_start(struct gui_window *g, gui_drag_type type,
		const struct rect *rect)
{
#ifdef __amigaos4__
	g->shared->drag_op = type;
	if(rect) g->shared->ptr_lock = ami_ns_rect_to_ibox(g->shared, rect);

	if(type == GDRAGGING_NONE)
	{
		SetWindowAttrs(g->shared->win, WA_GrabFocus, 0,
			WA_MouseLimits, NULL, TAG_DONE);

		if(g->shared->ptr_lock)
		{
			free(g->shared->ptr_lock);
			g->shared->ptr_lock = NULL;
		}
	}
#endif
	return true;
}

/* return the text box at posn x,y in window coordinates
   x,y are updated to be document co-ordinates */

bool ami_text_box_at_point(struct gui_window_2 *gwin, ULONG *restrict x, ULONG *restrict y)
{
	struct IBox *bbox;
	ULONG xs, ys;
	struct browser_window_features data;

	if(ami_gui_get_space_box((Object *)gwin->objects[GID_BROWSER], &bbox) != NSERROR_OK) {
		amiga_warn_user("NoMemory", "");
		return false;
	}

	ami_get_hscroll_pos(gwin, (ULONG *)&xs);
	*x = *x - (bbox->Left) +xs;

	ami_get_vscroll_pos(gwin, (ULONG *)&ys);
	*y = *y - (bbox->Top) + ys;

	ami_gui_free_space_box(bbox);

	browser_window_get_features(gwin->gw->bw, *x, *y, &data);

	if (data.form_features == CTX_FORM_TEXT)
		return true;

	return false;
}

BOOL ami_gadget_hit(Object *obj, int x, int y)
{
	int top, left, width, height;

	GetAttrs(obj,
		GA_Left, &left,
		GA_Top, &top,
		GA_Width, &width,
		GA_Height, &height,
		TAG_DONE);

	if((x >= left) && (x <= (left + width)) && (y >= top) && (y <= (top + height)))
		return TRUE;
	else return FALSE;
}

static Object *ami_gui_splash_open(void)
{
	Object *restrict win_obj, *restrict bm_obj;
	struct Window *win;
	struct Screen *wbscreen = LockPubScreen("Workbench");
	uint32 top = 0, left = 0;
	struct TextAttr tattr;
	struct TextFont *tfont;

	win_obj = WindowObj,
#ifdef __amigaos4__
				WA_ToolBox, TRUE,
#endif
				WA_Borderless, TRUE,
				WA_BusyPointer, TRUE,
				WINDOW_Position, WPOS_CENTERSCREEN,
				WINDOW_LockWidth, TRUE,
				WINDOW_LockHeight, TRUE,
				WINDOW_ParentGroup, LayoutVObj,
					LAYOUT_AddImage, bm_obj = BitMapObj,
						BITMAP_SourceFile, "PROGDIR:Resources/splash.png",
						BITMAP_Screen, wbscreen,
						BITMAP_Precision, PRECISION_IMAGE,
					BitMapEnd,
				LayoutEnd,
			EndWindow;

	if(win_obj == NULL) {
		NSLOG(netsurf, INFO, "Splash window object not created");
		return NULL;
	}

	NSLOG(netsurf, INFO, "Attempting to open splash window...");
	win = RA_OpenWindow(win_obj);

	if(win == NULL) {
		NSLOG(netsurf, INFO, "Splash window did not open");
		return NULL;
	}

	if(bm_obj == NULL) {
		NSLOG(netsurf, INFO, "BitMap object not created");
		return NULL;
	}

	GetAttrs(bm_obj, IA_Top, &top,
				IA_Left, &left,
				TAG_DONE);

	SetDrMd(win->RPort, JAM1);
#ifdef __amigaos4__
	SetRPAttrs(win->RPort, RPTAG_APenColor, 0xFF3F6DFE, TAG_DONE);
	tattr.ta_Name = "DejaVu Serif Italic.font";
#else
	SetAPen(win->RPort, 3); /* Pen 3 is usually blue */
	tattr.ta_Name = "ruby.font";
#endif
	tattr.ta_YSize = 24;
	tattr.ta_Style = 0;
	tattr.ta_Flags = 0;

	if((tfont = ami_font_open_disk_font(&tattr)))
	{
		SetFont(win->RPort, tfont);
	}
	else
	{
		tattr.ta_Name = "DejaVu Serif Oblique.font";
		if((tfont = ami_font_open_disk_font(&tattr)))
			SetFont(win->RPort, tfont);
	}

	Move(win->RPort, left + 5, top + 25);
	Text(win->RPort, "Initialising...", strlen("Initialising..."));

	if(tfont) ami_font_close_disk_font(tfont);

#ifdef __amigaos4__
	tattr.ta_Name = "DejaVu Sans.font";
#else
	tattr.ta_Name = "helvetica.font";
#endif
	tattr.ta_YSize = 16;
	tattr.ta_Style = 0;
	tattr.ta_Flags = 0;

	if((tfont = ami_font_open_disk_font(&tattr)))
		SetFont(win->RPort, tfont);

	Move(win->RPort, left + 185, top + 220);
	Text(win->RPort, netsurf_version, strlen(netsurf_version));

	if(tfont) ami_font_close_disk_font(tfont);

	UnlockPubScreen(NULL, wbscreen);

	return win_obj;
}

static void ami_gui_splash_close(Object *win_obj)
{
	if(win_obj == NULL) return;

	NSLOG(netsurf, INFO, "Closing splash window");
	DisposeObject(win_obj);
}

static void gui_file_gadget_open(struct gui_window *g, struct hlcache_handle *hl, 
	struct form_control *gadget)
{
	NSLOG(netsurf, INFO, "File open dialog request for %p/%p", g, gadget);

	if(AslRequestTags(filereq,
			ASLFR_Window, g->shared->win,
			ASLFR_SleepWindow, TRUE,
			ASLFR_TitleText, messages_get("NetSurf"),
			ASLFR_Screen, scrn,
			ASLFR_DoSaveMode, FALSE,
			TAG_DONE)) {
		char fname[1024];
		strlcpy(fname, filereq->fr_Drawer, 1024);
		AddPart(fname, filereq->fr_File, 1024);
		browser_window_set_gadget_filename(g->bw, gadget, fname);
	}
}

/* exported function documented in amiga/gui.h */
uint32 ami_gui_get_app_id(void)
{
	return ami_appid;
}

/* Get current user directory for user-specific NetSurf data
 * Returns NULL on error
 */
static char *ami_gui_get_user_dir(STRPTR current_user)
{
	BPTR lock = 0;
	char temp[1024];
	int32 user = 0;

	if(current_user == NULL) {
		user = GetVar("user", temp, 1024, GVF_GLOBAL_ONLY);
		current_user = ASPrintf("%s", (user == -1) ? "Default" : temp);
	}
	NSLOG(netsurf, INFO, "User: %s", current_user);

	if(users_dir == NULL) {
		users_dir = ASPrintf("%s", USERS_DIR);
		if(users_dir == NULL) {
			ami_misc_fatal_error("Failed to allocate memory");
			FreeVec(current_user);
			return NULL;
		}
	}

	if(LIB_IS_AT_LEAST((struct Library *)DOSBase, 51, 96)) {
#ifdef __amigaos4__
		struct InfoData *infodata = AllocDosObject(DOS_INFODATA, 0);
		if(infodata == NULL) {
			ami_misc_fatal_error("Failed to allocate memory");
			FreeVec(current_user);
			return NULL;
		}
		GetDiskInfoTags(GDI_StringNameInput, users_dir,
					GDI_InfoData, infodata,
					TAG_DONE);
		if(infodata->id_DiskState == ID_DISKSTATE_WRITE_PROTECTED) {
			FreeDosObject(DOS_INFODATA, infodata);
			ami_misc_fatal_error("User directory MUST be on a writeable volume");
			FreeVec(current_user);
			return NULL;
		}
		FreeDosObject(DOS_INFODATA, infodata);
#else
#warning FIXME for OS3 and older OS4
#endif
	} else {
//TODO: check volume write status using old API
	}

	int len = strlen(current_user);
	len += strlen(users_dir);
	len += 2; /* for poss path sep and NULL term */

	current_user_dir = malloc(len);
	if(current_user_dir == NULL) {
		ami_misc_fatal_error("Failed to allocate memory");
		FreeVec(current_user);
		return NULL;
	}

	strlcpy(current_user_dir, users_dir, len);
	AddPart(current_user_dir, current_user, len);
	FreeVec(users_dir);
	FreeVec(current_user);

	NSLOG(netsurf, INFO, "User dir: %s", current_user_dir);

	if((lock = CreateDirTree(current_user_dir)))
		UnLock(lock);

	ami_nsoption_set_location(current_user_dir);

	current_user_faviconcache = ASPrintf("%s/IconCache", current_user_dir);
	if((lock = CreateDirTree(current_user_faviconcache))) UnLock(lock);

	return current_user_dir;
}


/**
 * process miscellaneous window events
 *
 * \param gw The window receiving the event.
 * \param event The event code.
 * \return NSERROR_OK when processed ok
 */
static nserror
gui_window_event(struct gui_window *gw, enum gui_window_event event)
{
	switch (event) {
	case GW_EVENT_UPDATE_EXTENT:
		gui_window_update_extent(gw);
		break;

	case GW_EVENT_REMOVE_CARET:
		gui_window_remove_caret(gw);
		break;

	case GW_EVENT_NEW_CONTENT:
		gui_window_new_content(gw);
		break;

	case GW_EVENT_START_SELECTION:
		gui_start_selection(gw);
		break;

	case GW_EVENT_START_THROBBER:
		gui_window_start_throbber(gw);
		break;

	case GW_EVENT_STOP_THROBBER:
		gui_window_stop_throbber(gw);
		break;

	case GW_EVENT_PAGE_INFO_CHANGE:
		gui_page_info_change(gw);
		break;

	default:
		break;
	}
	return NSERROR_OK;
}


static struct gui_window_table amiga_window_table = {
	.create = gui_window_create,
	.destroy = gui_window_destroy,
	.invalidate = amiga_window_invalidate_area,
	.get_scroll = gui_window_get_scroll,
	.set_scroll = gui_window_set_scroll,
	.get_dimensions = gui_window_get_dimensions,
	.event = gui_window_event,

	.set_icon = gui_window_set_icon,
	.set_title = gui_window_set_title,
	.set_url = gui_window_set_url,
	.set_status = gui_window_set_status,
	.place_caret = gui_window_place_caret,
	.drag_start = gui_window_drag_start,
	.create_form_select_menu = gui_create_form_select_menu,
	.file_gadget_open = gui_file_gadget_open,
	.drag_save_object = gui_drag_save_object,
	.drag_save_selection = gui_drag_save_selection,

	.console_log = gui_window_console_log,

	/* from theme */
	.set_pointer = gui_window_set_pointer,

	/* from download */
	.save_link = gui_window_save_link,
};


static struct gui_fetch_table amiga_fetch_table = {
	.filetype = fetch_filetype,

	.get_resource_url = gui_get_resource_url,
};

static struct gui_search_web_table amiga_search_web_table = {
	.provider_update = gui_search_web_provider_update,
};

static struct gui_misc_table amiga_misc_table = {
	.schedule = ami_schedule,

	.quit = gui_quit,
	.launch_url = gui_launch_url,
	.present_cookies = ami_cookies_present,
};

/** Normal entry point from OS */
int main(int argc, char** argv)
{
	setbuf(stderr, NULL);
	char messages[100];
	char script[1024];
	char temp[1024];
	STRPTR current_user_cache = NULL;
	STRPTR current_user = NULL;
	BPTR lock = 0;
	nserror ret;
	int nargc = 0;
	char *nargv = NULL;

	struct netsurf_table amiga_table = {
		.misc = &amiga_misc_table,
		.window = &amiga_window_table,
		.clipboard = amiga_clipboard_table,
		.download = amiga_download_table,
		.fetch = &amiga_fetch_table,
		.file = amiga_file_table,
		.utf8 = amiga_utf8_table,
		.search = amiga_search_table,
		.search_web = &amiga_search_web_table,
		.llcache = filesystem_llcache_table,
		.bitmap = amiga_bitmap_table,
		.layout = ami_layout_table,
	};

#ifdef __amigaos4__
	signal(SIGINT, SIG_IGN);
#endif
	ret = netsurf_register(&amiga_table);
	if (ret != NSERROR_OK) {
		ami_misc_fatal_error("NetSurf operation table failed registration");
		return RETURN_FAIL;
	}

	/* initialise logging. Not fatal if it fails but not much we
	 * can do about it either.
	 */
	nslog_init(NULL, &argc, argv);

	/* Need to do this before opening any splash windows etc... */
	if ((ami_libs_open() == false)) {
		return RETURN_FAIL;
	}

	/* Open splash window */
	Object *splash_window = ami_gui_splash_open();

#ifndef __amigaos4__
	/* OS3 low memory handler */
	struct Interupt *memhandler = ami_memory_init();
#endif

	if (ami_gui_resources_open() == false) { /* alloc msgports, objects and other miscelleny */
		ami_misc_fatal_error("Unable to allocate resources");
		ami_gui_splash_close(splash_window);
		ami_libs_close();
		return RETURN_FAIL;
	}

	current_user = ami_gui_read_all_tooltypes(argc, argv);
	struct RDArgs *args = ami_gui_commandline(&argc, argv, &nargc, &nargv);

	current_user_dir = ami_gui_get_user_dir(current_user);
	if(current_user_dir == NULL) {
		ami_gui_resources_free();
		ami_gui_splash_close(splash_window);
		ami_libs_close();
		return RETURN_FAIL;
	}

	ami_mime_init("PROGDIR:Resources/mimetypes");
	sprintf(temp, "%s/mimetypes.user", current_user_dir);
	ami_mime_init(temp);

#ifdef __amigaos4__
	amiga_plugin_hack_init();

	/* DataTypes loader needs datatypes.library v45,
	 * but for some reason that's not in OS3.9.
	 * Skip it to ensure it isn't causing other problems. */
	ret = amiga_datatypes_init();
#endif

	/* user options setup */
	ret = nsoption_init(ami_set_options, &nsoptions, &nsoptions_default);
	if (ret != NSERROR_OK) {
		ami_misc_fatal_error("Options failed to initialise");
		ami_gui_resources_free();
		ami_gui_splash_close(splash_window);
		ami_libs_close();
		return RETURN_FAIL;
	}
	ami_nsoption_read();
	if(args != NULL) {
		nsoption_commandline(&nargc, &nargv, NULL);
		FreeArgs(args);
	}

	if (ami_locate_resource(messages, "Messages") == false) {
		ami_misc_fatal_error("Cannot open Messages file");
		ami_nsoption_free();
		nsoption_finalise(nsoptions, nsoptions_default);
		ami_gui_resources_free();
		ami_gui_splash_close(splash_window);
		ami_libs_close();
		return RETURN_FAIL;
	}

	ret = messages_add_from_file(messages);

	current_user_cache = ASPrintf("%s/Cache", current_user_dir);
	if((lock = CreateDirTree(current_user_cache))) UnLock(lock);

	ret = netsurf_init(current_user_cache);

	if(current_user_cache != NULL) FreeVec(current_user_cache);

	if (ret != NSERROR_OK) {
		ami_misc_fatal_error("NetSurf failed to initialise");
		ami_nsoption_free();
		nsoption_finalise(nsoptions, nsoptions_default);
		ami_gui_resources_free();
		ami_gui_splash_close(splash_window);
		ami_libs_close();
		return RETURN_FAIL;
	}

	ret = amiga_icon_init();

	search_web_init(nsoption_charp(search_engines_file));
	ami_clipboard_init();
	ami_openurl_open();
	ami_amiupdate(); /* set env-vars for AmiUpdate */
	ami_font_init();
	save_complete_init();
	ami_theme_init();
	ami_init_mouse_pointers();
	ami_file_req_init();

	win_destroyed = false;
	ami_font_setdevicedpi(0); /* for early font requests, eg treeview init */

	window_list = NewObjList();

	urldb_load(nsoption_charp(url_file));
	urldb_load_cookies(nsoption_charp(cookie_file));

	gui_init2(argc, argv);

	ami_ctxmenu_init(); /* Requires screen pointer */

	ami_gui_splash_close(splash_window);

	strlcpy(script, nsoption_charp(arexx_dir), 1024);
	AddPart(script, nsoption_charp(arexx_startup), 1024);
	ami_arexx_execute(script);

	NSLOG(netsurf, INFO, "Entering main loop");

	while (!ami_quit) {
		ami_get_msg();
	}

	strlcpy(script, nsoption_charp(arexx_dir), 1024);
	AddPart(script, nsoption_charp(arexx_shutdown), 1024);
	ami_arexx_execute(script);

	ami_mime_free();

	netsurf_exit();

	nsoption_finalise(nsoptions, nsoptions_default);
	ami_nsoption_free();
	free(current_user_dir);
	FreeVec(current_user_faviconcache);

	/* finalise logging */
	nslog_finalise();

#ifndef __amigaos4__
	/* OS3 low memory handler */
	ami_memory_fini(memhandler);
#endif

	ami_bitmap_fini();
	ami_libs_close();

	return RETURN_OK;
}

