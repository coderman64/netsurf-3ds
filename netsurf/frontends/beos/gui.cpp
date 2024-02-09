/*
 * Copyright 2015 Adrián Arroyo Calle <adrian.arroyocalle@gmail.com>
 * Copyright 2008 François Revol <mmu_man@users.sourceforge.net>
 * Copyright 2005 James Bursa <bursa@users.sourceforge.net>
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

#define __STDBOOL_H__	1
#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <Alert.h>
#include <Application.h>
#include <BeBuild.h>
#include <FindDirectory.h>
#include <Mime.h>
#include <Path.h>
#include <PathFinder.h>
#include <Resources.h>
#include <Roster.h>
#include <Screen.h>
#include <String.h>
#ifdef __HAIKU__
#include <LocaleRoster.h>
#endif

extern "C" {

#include "utils/nsoption.h"
#include "utils/filename.h"
#include "utils/log.h"
#include "utils/messages.h"
#include "utils/url.h"
#include "utils/corestrings.h"
#include "utils/utf8.h"
#include "utils/utils.h"
#include "utils/nsurl.h"
#include "netsurf/misc.h"
#include "netsurf/clipboard.h"
#include "netsurf/search.h"
#include "netsurf/fetch.h"
#include "netsurf/netsurf.h"
#include "netsurf/content.h"
#include "netsurf/browser_window.h"
#include "netsurf/cookie_db.h"
#include "netsurf/url_db.h"
#include "content/fetch.h"

}

#include "beos/gui.h"
#include "beos/gui_options.h"
//#include "beos/completion.h"
#include "beos/window.h"
#include "beos/throbber.h"
#include "beos/filetype.h"
#include "beos/download.h"
#include "beos/schedule.h"
#include "beos/fetch_rsrc.h"
#include "beos/scaffolding.h"
#include "beos/bitmap.h"
#include "beos/font.h"

//TODO: use resources
// enable using resources instead of files
#define USE_RESOURCES 1

bool nsbeos_done = false;

bool replicated = false; /**< if we are running as a replicant */

char *options_file_location;
char *glade_file_location;

struct gui_window *search_current_window = 0;

BWindow *wndAbout;
BWindow *wndWarning;
//GladeXML *gladeWindows;
BWindow *wndTooltip;
//beosLabel *labelTooltip;
BFilePanel *wndOpenFile;

static thread_id sBAppThreadID;

static BMessage *gFirstRefsReceived = NULL;

static int sEventPipe[2];

// #pragma mark - class NSBrowserFrameView


/* exported function defined in beos/gui.h */
nserror beos_warn_user(const char *warning, const char *detail)
{
	NSLOG(netsurf, INFO, "warn_user: %s (%s)", warning, detail);
	BAlert *alert;
	BString text(warning);
	if (detail)
		text << ":\n" << detail;

	alert = new BAlert("NetSurf Warning", text.String(), "Debug", "Ok",
                           NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
	if (alert->Go() < 1) {
		debugger("warn_user");
        }
        
        return NSERROR_OK;
}

NSBrowserApplication::NSBrowserApplication()
	: BApplication("application/x-vnd.NetSurf")
{
}


NSBrowserApplication::~NSBrowserApplication()
{
}


void
NSBrowserApplication::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case B_REFS_RECEIVED:
		case B_UI_SETTINGS_CHANGED:
		// messages for top-level
		// we'll just send them to the first window
		case 'back':
		case 'forw':
		case 'stop':
		case 'relo':
		case 'home':
		case 'urlc':
		case 'urle':
		case 'sear':
		case 'menu':
		// NetPositive messages
		case B_NETPOSITIVE_OPEN_URL:
		case B_NETPOSITIVE_BACK:
		case B_NETPOSITIVE_FORWARD:
		case B_NETPOSITIVE_HOME:
		case B_NETPOSITIVE_RELOAD:
		case B_NETPOSITIVE_STOP:
		case B_NETPOSITIVE_DOWN:
		case B_NETPOSITIVE_UP:
			//DetachCurrentMessage();
			//nsbeos_pipe_message(message, this, fGuiWindow);
			break;
		default:
			BApplication::MessageReceived(message);
	}
}


void
NSBrowserApplication::ArgvReceived(int32 argc, char **argv)
{
	NSBrowserWindow *win = nsbeos_find_last_window();
	if (!win) {
		return;
	}
	win->Unlock();
	BMessage *message = DetachCurrentMessage();
	nsbeos_pipe_message_top(message, win, win->Scaffolding());
}


void
NSBrowserApplication::RefsReceived(BMessage *message)
{
	DetachCurrentMessage();
	NSBrowserWindow *win = nsbeos_find_last_window();
	if (!win) {
		gFirstRefsReceived = message;
		return;
	}
	win->Unlock();
	nsbeos_pipe_message_top(message, win, win->Scaffolding());
}


void
NSBrowserApplication::AboutRequested()
{
	nsbeos_pipe_message(new BMessage(B_ABOUT_REQUESTED), NULL, NULL);
}


bool
NSBrowserApplication::QuitRequested()
{
	// let it notice it
	nsbeos_pipe_message(new BMessage(B_QUIT_REQUESTED), NULL, NULL);
	// we'll let the main thread Quit() ourselves when it's done.
	return false;
}


// #pragma mark - implementation



/* realpath fallback on R5 */
#if !defined(__HAIKU__) && !defined(B_BEOS_VERSION_DANO)
extern "C" char *realpath(const char *f, char *buf);
char *realpath(const char *f, char *buf)
{
	BPath path(f, NULL, true);
	if (path.InitCheck() < 0) {
		strncpy(buf, f, MAXPATHLEN);
		return NULL;
	}
	//printf("RP: '%s'\n", path.Path());
	strncpy(buf, path.Path(), MAXPATHLEN);
	return buf;
}
#endif

/* finds the NetSurf binary image ID and path
 * 
 */
image_id nsbeos_find_app_path(char *path)
{
	image_info info;
	int32 cookie = 0;
	while (get_next_image_info(0, &cookie, &info) == B_OK) {
//fprintf(stderr, "%p <> %p, %p\n", (char *)&find_app_resources, (char *)info.text, (char *)info.text + info.text_size);
		if (((char *)&nsbeos_find_app_path >= (char *)info.text)
		 && ((char *)&nsbeos_find_app_path < (char *)info.text + info.text_size)) {
//fprintf(stderr, "match\n");
			if (path) {
				memset(path, 0, B_PATH_NAME_LENGTH);
				strncpy(path, info.name, B_PATH_NAME_LENGTH-1);
			}
			return info.id;
		}
	}
	return B_ERROR;
}

/**
 * Locate a shared resource file by searching known places in order.
 *
 * Search order is: ~/config/settings/NetSurf/, ~/.netsurf/, $NETSURFRES/
 * (where NETSURFRES is an environment variable), and finally the path
 * specified by the macro at the top of this file.
 *
 * \param  buf      buffer to write to.  must be at least PATH_MAX chars
 * \param  filename file to look for
 * \param  def      default to return if file not found
 * \return path to resource.
 */
char *find_resource(char *buf, const char *filename, const char *def)
{
	const char *cdir = NULL;
	status_t err;
	BPath path;
	char t[PATH_MAX];

	err = find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	path.Append("NetSurf");
	if (err >= B_OK)
		cdir = path.Path();
	if (cdir != NULL) {
		strcpy(t, cdir);
		strcat(t, "/");
		strcat(t, filename);
		realpath(t, buf);
		if (access(buf, R_OK) == 0)
			return buf;
	}

	cdir = getenv("HOME");
	if (cdir != NULL) {
		strcpy(t, cdir);
		strcat(t, "/.netsurf/");
		strcat(t, filename);
		realpath(t, buf);
		if (access(buf, R_OK) == 0)
			return buf;
	}

	cdir = getenv("NETSURFRES");

	if (cdir != NULL) {
		realpath(cdir, buf);
		strcat(buf, "/");
		strcat(buf, filename);
		if (access(buf, R_OK) == 0)
			return buf;
	}


	BPathFinder f((void*)find_resource);

	BPath p;
	if (f.FindPath(B_FIND_PATH_APPS_DIRECTORY, "netsurf/res", p) == B_OK) {
		strcpy(t, p.Path());
		strcat(t, filename);
		realpath(t, buf);
		if (access(buf, R_OK) == 0)
			return buf;
	}

	if (def[0] == '%') {
		snprintf(t, PATH_MAX, "%s%s", path.Path(), def + 1);
		if (realpath(t, buf) == NULL) {
			strcpy(buf, t);
		}
	} else if (def[0] == '~') {
		snprintf(t, PATH_MAX, "%s%s", getenv("HOME"), def + 1);
		if (realpath(t, buf) == NULL) {
			strcpy(buf, t);
		}
	} else {
		if (realpath(def, buf) == NULL) {
			strcpy(buf, def);
		}
	}

	return buf;
}

/**
 * Check that ~/.netsurf/ exists, and if it doesn't, create it.
 */
static void check_homedir(void)
{
	status_t err;

	BPath path;
	err = find_directory(B_USER_SETTINGS_DIRECTORY, &path, true);

	if (err < B_OK) {
		/* we really can't continue without a home directory. */
		NSLOG(netsurf, INFO,
		      "Can't find user settings directory - nowhere to store state!");
		die("NetSurf needs to find the user settings directory in order to run.\n");
	}

	path.Append("NetSurf");
	err = create_directory(path.Path(), 0644); 
	if (err < B_OK) {
		NSLOG(netsurf, INFO, "Unable to create %s", path.Path());
		die("NetSurf could not create its settings directory.\n");
	}
}

static int32 bapp_thread(void *arg)
{
	be_app->Lock();
	be_app->Run();
	return 0;
}

static nsurl *gui_get_resource_url(const char *path)
{
	nsurl *url = NULL;
	BString u("rsrc:///");

	/* default.css -> beosdefault.css */
	if (strcmp(path, "default.css") == 0)
		path = "beosdefault.css";

	/* favicon.ico -> favicon.png */
	if (strcmp(path, "favicon.ico") == 0)
		path = "favicon.png";

	u << path;
	NSLOG(netsurf, INFO, "(%s) -> '%s'\n", path, u.String());
	nsurl_create(u.String(), &url);
	return url;
}



#if !defined(__HAIKU__) && !defined(B_BEOS_VERSION_DANO)
/* more ui_colors, R5 only had a few defined... */
#define B_PANEL_TEXT_COLOR ((color_which)10)
#define B_DOCUMENT_BACKGROUND_COLOR ((color_which)11)
#define B_DOCUMENT_TEXT_COLOR ((color_which)12)
#define B_CONTROL_BACKGROUND_COLOR ((color_which)13)
#define B_CONTROL_TEXT_COLOR ((color_which)14)
#define B_CONTROL_BORDER_COLOR ((color_which)15)
#define B_CONTROL_HIGHLIGHT_COLOR ((color_which)16)
#define B_NAVIGATION_BASE_COLOR ((color_which)4)
#define B_NAVIGATION_PULSE_COLOR ((color_which)17)
#define B_SHINE_COLOR ((color_which)18)
#define B_SHADOW_COLOR ((color_which)19)
#define B_MENU_SELECTED_BORDER_COLOR ((color_which)9)
#define B_TOOL_TIP_BACKGROUND_COLOR ((color_which)20)
#define B_TOOL_TIP_TEXT_COLOR ((color_which)21)
#define B_SUCCESS_COLOR ((color_which)100)
#define B_FAILURE_COLOR ((color_which)101)
#define B_MENU_SELECTED_BACKGROUND_COLOR B_MENU_SELECTION_BACKGROUND_COLOR
#define B_RANDOM_COLOR ((color_which)0x80000000)
#define B_MICHELANGELO_FAVORITE_COLOR ((color_which)0x80000001)
#define B_DSANDLER_FAVORITE_SKY_COLOR ((color_which)0x80000002)
#define B_DSANDLER_FAVORITE_INK_COLOR ((color_which)0x80000003)
#define B_DSANDLER_FAVORITE_SHOES_COLOR ((color_which)0x80000004)
#define B_DAVE_BROWN_FAVORITE_COLOR ((color_which)0x80000005)
#endif
#if defined(B_BEOS_VERSION_DANO)
#define B_TOOL_TIP_BACKGROUND_COLOR B_TOOLTIP_BACKGROUND_COLOR
#define B_TOOL_TIP_TEXT_COLOR B_TOOLTIP_TEXT_COLOR
#define
#endif
#define NOCOL ((color_which)0)

/**
 * set option from pen
 */
static nserror
set_colour_from_ui(struct nsoption_s *opts,
                   color_which ui,
                   enum nsoption_e option,
                   colour def_colour)
{
	if (ui != NOCOL) {
		rgb_color c;
		if (ui == B_DESKTOP_COLOR) {
			BScreen s;
			c = s.DesktopColor();
		} else {
			c = ui_color(ui);
		}

		def_colour = ((((uint32_t)c.blue << 16) & 0xff0000) |
					  ((c.green << 8) & 0x00ff00) |
					  ((c.red) & 0x0000ff));
	}

	opts[option].value.c = def_colour;

	return NSERROR_OK;
}

/**
 * Set option defaults for framebuffer frontend
 *
 * @param defaults The option table to update.
 * @return error status.
 */
static nserror set_defaults(struct nsoption_s *defaults)
{
	/* set system colours for beos ui */
	set_colour_from_ui(defaults, NOCOL, NSOPTION_sys_colour_ActiveBorder, 0x00000000);
	set_colour_from_ui(defaults, B_WINDOW_TAB_COLOR, NSOPTION_sys_colour_ActiveCaption, 0x00dddddd);
	set_colour_from_ui(defaults, B_PANEL_BACKGROUND_COLOR, NSOPTION_sys_colour_AppWorkspace, 0x00eeeeee);
	set_colour_from_ui(defaults, B_DESKTOP_COLOR, NSOPTION_sys_colour_Background, 0x00aa0000);
	set_colour_from_ui(defaults, B_CONTROL_BACKGROUND_COLOR, NSOPTION_sys_colour_ButtonFace, 0x00aaaaaa);
	set_colour_from_ui(defaults, B_CONTROL_HIGHLIGHT_COLOR, NSOPTION_sys_colour_ButtonHighlight, 0x00cccccc);
	set_colour_from_ui(defaults, NOCOL, NSOPTION_sys_colour_ButtonShadow, 0x00bbbbbb);
	set_colour_from_ui(defaults, B_CONTROL_TEXT_COLOR, NSOPTION_sys_colour_ButtonText, 0x00000000);
	set_colour_from_ui(defaults, NOCOL, NSOPTION_sys_colour_CaptionText, 0x00000000);
	set_colour_from_ui(defaults, NOCOL, NSOPTION_sys_colour_GrayText, 0x00777777);
	set_colour_from_ui(defaults, NOCOL, NSOPTION_sys_colour_Highlight, 0x00ee0000);
	set_colour_from_ui(defaults, NOCOL, NSOPTION_sys_colour_HighlightText, 0x00000000);
	set_colour_from_ui(defaults, NOCOL, NSOPTION_sys_colour_InactiveBorder, 0x00000000);
	set_colour_from_ui(defaults, NOCOL, NSOPTION_sys_colour_InactiveCaption, 0x00ffffff);
	set_colour_from_ui(defaults, NOCOL, NSOPTION_sys_colour_InactiveCaptionText, 0x00cccccc);
	set_colour_from_ui(defaults, B_TOOL_TIP_BACKGROUND_COLOR, NSOPTION_sys_colour_InfoBackground, 0x00aaaaaa);
	set_colour_from_ui(defaults, B_TOOL_TIP_TEXT_COLOR, NSOPTION_sys_colour_InfoText, 0x00000000);
	set_colour_from_ui(defaults, B_MENU_BACKGROUND_COLOR, NSOPTION_sys_colour_Menu, 0x00aaaaaa);
	set_colour_from_ui(defaults, B_MENU_ITEM_TEXT_COLOR, NSOPTION_sys_colour_MenuText, 0x00000000);
	set_colour_from_ui(defaults, NOCOL, NSOPTION_sys_colour_Scrollbar, 0x00aaaaaa);
	set_colour_from_ui(defaults, NOCOL, NSOPTION_sys_colour_ThreeDDarkShadow, 0x00555555);
	set_colour_from_ui(defaults, NOCOL, NSOPTION_sys_colour_ThreeDFace, 0x00dddddd);
	set_colour_from_ui(defaults, NOCOL, NSOPTION_sys_colour_ThreeDHighlight, 0x00aaaaaa);
	set_colour_from_ui(defaults, NOCOL, NSOPTION_sys_colour_ThreeDLightShadow, 0x00999999);
	set_colour_from_ui(defaults, NOCOL, NSOPTION_sys_colour_ThreeDShadow, 0x00777777);
	set_colour_from_ui(defaults, B_DOCUMENT_BACKGROUND_COLOR, NSOPTION_sys_colour_Window, 0x00aaaaaa);
	set_colour_from_ui(defaults, NOCOL, NSOPTION_sys_colour_WindowFrame, 0x00000000);
	set_colour_from_ui(defaults, B_DOCUMENT_TEXT_COLOR, NSOPTION_sys_colour_WindowText, 0x00000000);

	return NSERROR_OK;
}

void nsbeos_update_system_ui_colors(void)
{
	set_defaults(nsoptions);
}

/**
 * Ensures output logging stream is correctly configured
 */
static bool nslog_stream_configure(FILE *fptr)
{
        /* set log stream to be non-buffering */
	setbuf(fptr, NULL);

	return true;
}

static BPath get_messages_path()
{
	BPathFinder f((void*)get_messages_path);

	BPath p;
	f.FindPath(B_FIND_PATH_APPS_DIRECTORY, "netsurf/res", p);
	BString lang;
#ifdef __HAIKU__
	BMessage preferredLangs;
	if (BLocaleRoster::Default()->GetPreferredLanguages(&preferredLangs) == B_OK) {
		preferredLangs.FindString("language", 0, &lang);
		lang.Truncate(2);
	}
#endif
	if (lang.Length() < 1) {
		lang.SetTo(getenv("LC_MESSAGES"));
		lang.Truncate(2);
	}
	BDirectory d(p.Path());
	if (!d.Contains(lang.String(), B_DIRECTORY_NODE))
		lang = "en";
	p.Append(lang.String());
	p.Append("Messages");
	return p;
}


static void gui_init(int argc, char** argv)
{
	const char *addr;
	nsurl *url;
	nserror error;
	char buf[PATH_MAX];

	if (pipe(sEventPipe) < 0)
		return;
	if (!replicated) {
		sBAppThreadID = spawn_thread(bapp_thread, "BApplication(NetSurf)", B_NORMAL_PRIORITY, (void *)find_thread(NULL));
		if (sBAppThreadID < B_OK)
			return; /* #### handle errors */
		if (resume_thread(sBAppThreadID) < B_OK)
			return;
	}

	nsbeos_update_system_ui_colors();

	fetch_rsrc_register();

	check_homedir();

	// make sure the cache dir exists
	create_directory(TEMP_FILENAME_PREFIX, 0700);

	//nsbeos_completion_init();


	/* This is an ugly hack to just get the new-style throbber going.
	 * It, along with the PNG throbber loader, need making more generic.
	 */
	{
#define STROF(n) #n
#define FIND_THROB(n) filenames[(n)] = \
				"throbber/throbber" STROF(n) ".png";
		const char *filenames[9];
		FIND_THROB(0);
		FIND_THROB(1);
		FIND_THROB(2);
		FIND_THROB(3);
		FIND_THROB(4);
		FIND_THROB(5);
		FIND_THROB(6);
		FIND_THROB(7);
		FIND_THROB(8);
		nsbeos_throbber_initialise_from_png(9,
			filenames[0], filenames[1], filenames[2], filenames[3],
			filenames[4], filenames[5], filenames[6], filenames[7], 
			filenames[8]);
#undef FIND_THROB
#undef STROF
	}

	if (nsbeos_throbber == NULL)
		die("Unable to load throbber image.\n");

	find_resource(buf, "Choices", "%/Choices");
	NSLOG(netsurf, INFO, "Using '%s' as Preferences file", buf);
	options_file_location = strdup(buf);
	nsoption_read(buf, NULL);


	/* check what the font settings are, setting them to a default font
	 * if they're not set - stops Pango whinging
	 */
#define SETFONTDEFAULT(OPTION,y) if (nsoption_charp(OPTION) == NULL) nsoption_set_charp(OPTION, strdup((y)))

	//XXX: use be_plain_font & friends, when we can check if font is serif or not.
/*
	font_family family;
	font_style style;
	be_plain_font->GetFamilyAndStyle(&family, &style);
	SETFONTDEFAULT(font_sans, family);
	SETFONTDEFAULT(font_serif, family);
	SETFONTDEFAULT(font_mono, family);
	SETFONTDEFAULT(font_cursive, family);
	SETFONTDEFAULT(font_fantasy, family);
*/
#ifdef __HAIKU__
	SETFONTDEFAULT(font_sans, "DejaVu Sans");
	SETFONTDEFAULT(font_serif, "DejaVu Serif");
	SETFONTDEFAULT(font_mono, "DejaVu Mono");
	SETFONTDEFAULT(font_cursive, "DejaVu Sans");
	SETFONTDEFAULT(font_fantasy, "DejaVu Sans");
#else
	SETFONTDEFAULT(font_sans, "Bitstream Vera Sans");
	SETFONTDEFAULT(font_serif, "Bitstream Vera Serif");
	SETFONTDEFAULT(font_mono, "Bitstream Vera Sans Mono");
	SETFONTDEFAULT(font_cursive, "Bitstream Vera Serif");
	SETFONTDEFAULT(font_fantasy, "Bitstream Vera Serif");
#endif

	nsbeos_options_init();

	/* We don't yet have an implementation of "select" form elements (they should use a popup menu)
	 * So we use the cross-platform code instead. */
	nsoption_set_bool(core_select_menu, true);

	if (nsoption_charp(cookie_file) == NULL) {
		find_resource(buf, "Cookies", "%/Cookies");
		NSLOG(netsurf, INFO, "Using '%s' as Cookies file", buf);
		nsoption_set_charp(cookie_file, strdup(buf));
	}
	if (nsoption_charp(cookie_jar) == NULL) {
		find_resource(buf, "Cookies", "%/Cookies");
		NSLOG(netsurf, INFO, "Using '%s' as Cookie Jar file", buf);
		nsoption_set_charp(cookie_jar, strdup(buf));
	}
	if ((nsoption_charp(cookie_file) == NULL) || 
	    (nsoption_charp(cookie_jar) == NULL))
		die("Failed initialising cookie options");

	if (nsoption_charp(url_file) == NULL) {
		find_resource(buf, "URLs", "%/URLs");
		NSLOG(netsurf, INFO, "Using '%s' as URL file", buf);
		nsoption_set_charp(url_file, strdup(buf));
	}

        if (nsoption_charp(ca_path) == NULL) {
                find_resource(buf, "certs", "/etc/ssl/certs");
                NSLOG(netsurf, INFO, "Using '%s' as certificate path", buf);
                nsoption_set_charp(ca_path, strdup(buf));
        }

	//find_resource(buf, "mime.types", "/etc/mime.types");
	beos_fetch_filetype_init();

	urldb_load(nsoption_charp(url_file));
	urldb_load_cookies(nsoption_charp(cookie_file));

	//nsbeos_download_initialise();

	if (!replicated)
		be_app->Unlock();

	if (argc > 1) {
		addr = argv[1];
	} else if (nsoption_charp(homepage_url) != NULL) {
		addr = nsoption_charp(homepage_url);
	} else {
		addr = NETSURF_HOMEPAGE;
	}

	/* create an initial browser window */
	error = nsurl_create(addr, &url);
	if (error == NSERROR_OK) {
		error = browser_window_create(
			BW_CREATE_HISTORY,
			url,
			NULL,
			NULL,
			NULL);
		nsurl_unref(url);
	}
	if (error != NSERROR_OK) {
		beos_warn_user(messages_get_errorcode(error), 0);
	}

	if (gFirstRefsReceived) {
		// resend the refs we got before having a window to send them to
		be_app_messenger.SendMessage(gFirstRefsReceived);
		delete gFirstRefsReceived;
		gFirstRefsReceived = NULL;
	}

}




void nsbeos_pipe_message(BMessage *message, BView *_this, struct gui_window *gui)
{
	if (message == NULL) {
		fprintf(stderr, "%s(NULL)!\n", __FUNCTION__);
		return;
	}
	if (_this)
		message->AddPointer("View", _this);
	if (gui)
		message->AddPointer("gui_window", gui);
	write(sEventPipe[1], &message, sizeof(void *));
}


void nsbeos_pipe_message_top(BMessage *message, BWindow *_this, struct beos_scaffolding *scaffold)
{
	if (message == NULL) {
		fprintf(stderr, "%s(NULL)!\n", __FUNCTION__);
		return;
	}
	if (_this)
		message->AddPointer("Window", _this);
	if (scaffold)
		message->AddPointer("scaffolding", scaffold);
	write(sEventPipe[1], &message, sizeof(void *));
}


void nsbeos_gui_poll(void)
{
	fd_set read_fd_set, write_fd_set, exc_fd_set;
	int max_fd;
	struct timeval timeout;
	unsigned int fd_count = 0;
	bigtime_t next_schedule = 0;

        /* run the scheduler */
	schedule_run();

        /* get any active fetcher fd */
	fetch_fdset(&read_fd_set, &write_fd_set, &exc_fd_set, &max_fd);

	// our own event pipe
	FD_SET(sEventPipe[0], &read_fd_set);

	// max of all the fds in the set, plus one for select()
	max_fd = MAX(max_fd, sEventPipe[0]) + 1;

	// compute schedule timeout
        if (earliest_callback_timeout != B_INFINITE_TIMEOUT) {
                next_schedule = earliest_callback_timeout - system_time();
        } else {
                next_schedule = earliest_callback_timeout;
        }

        // we're quite late already...
        if (next_schedule < 0)
                next_schedule = 0;

	timeout.tv_sec = (long)(next_schedule / 1000000LL);
	timeout.tv_usec = (long)(next_schedule % 1000000LL);

	NSLOG(netsurf, DEEPDEBUG,
	      "gui_poll: select(%d, ..., %Ldus",
	      max_fd, next_schedule);
	fd_count = select(max_fd, &read_fd_set, &write_fd_set, &exc_fd_set, 
		&timeout);
	NSLOG(netsurf, DEEPDEBUG, "select: %d\n", fd_count);

	if (fd_count > 0 && FD_ISSET(sEventPipe[0], &read_fd_set)) {
		BMessage *message;
		int len = read(sEventPipe[0], &message, sizeof(void *));
		NSLOG(netsurf, DEEPDEBUG, "gui_poll: BMessage ? %d read", len);
		if (len == sizeof(void *)) {
			NSLOG(netsurf, DEEPDEBUG,
			      "gui_poll: BMessage.what %-4.4s\n",
			      (char *)&(message->what));
			nsbeos_dispatch_event(message);
		}
	}
}


static void gui_quit(void)
{
	urldb_save_cookies(nsoption_charp(cookie_jar));
	urldb_save(nsoption_charp(url_file));
	//options_save_tree(hotlist,nsoption_charp(hotlist_file),messages_get("TreeHotlist"));

	free(nsoption_charp(cookie_file));
	free(nsoption_charp(cookie_jar));
	beos_fetch_filetype_fin();
	fetch_rsrc_unregister();
}

static char *url_to_path(const char *url)
{
	char *url_path;
	char *path = NULL;

	if (url_unescape(url, 0, NULL, &url_path) == NSERROR_OK) {
		/* return the absolute path including leading / */
		path = strdup(url_path + (FILE_SCHEME_PREFIX_LEN - 1));
		free(url_path);
	}

	return path;
}

/**
 * Send the source of a content to a text editor.
 */

void nsbeos_gui_view_source(struct hlcache_handle *content)
{
	char *temp_name;
	bool done = false;
	BPath path;
	status_t err;
	size_t size;
	const uint8_t *source;

        source = content_get_source_data(content, &size);

	if (!content || !source) {
		beos_warn_user("MiscError", "No document source");
		return;
	}

	/* try to load local files directly. */
	temp_name = url_to_path(nsurl_access(hlcache_handle_get_url(content)));
	if (temp_name) {
		path.SetTo(temp_name);
		BEntry entry;
		if (entry.SetTo(path.Path()) >= B_OK 
			&& entry.Exists() && entry.IsFile())
			done = true;
	}
	if (!done) {
		/* We cannot release the requested filename until after it
		 * has finished being used. As we can't easily find out when
		 * this is, we simply don't bother releasing it and simply
		 * allow it to be re-used next time NetSurf is started. The
		 * memory overhead from doing this is under 1 byte per
		 * filename. */
		BString filename(filename_request());
		if (filename.IsEmpty()) {
			beos_warn_user("NoMemory", 0);
			return;
		}

		lwc_string *mime = content_get_mime_type(content);

		/* provide an extension, as Pe still doesn't sniff the MIME */
		if (mime) {
			BMimeType type(lwc_string_data(mime));
			BMessage extensions;
			if (type.GetFileExtensions(&extensions) == B_OK) {
				BString ext;
				if (extensions.FindString("extensions", &ext) == B_OK)
					filename << "." << ext;
			}
			/* we unref(mime) later on, we just leak on error */
		}

		path.SetTo(TEMP_FILENAME_PREFIX);
		path.Append(filename.String());
		BFile file(path.Path(), B_WRITE_ONLY | B_CREATE_FILE);
		err = file.InitCheck();
		if (err < B_OK) {
			beos_warn_user("IOError", strerror(err));
			return;
		}
		err = file.Write(source, size);
		if (err < B_OK) {
			beos_warn_user("IOError", strerror(err));
			return;
		}

		if (mime) {
			file.WriteAttr("BEOS:TYPE", B_MIME_STRING_TYPE, 0LL, 
				lwc_string_data(mime), lwc_string_length(mime) + 1);
			lwc_string_unref(mime);
		}
		
	}

	entry_ref ref;
	if (get_ref_for_path(path.Path(), &ref) < B_OK)
		return;

	BMessage m(B_REFS_RECEIVED);
	m.AddRef("refs", &ref);


	// apps to try
	const char *editorSigs[] = {
		"text/x-source-code",
		"application/x-vnd.beunited.pe",
		"application/x-vnd.XEmacs",
		"application/x-vnd.Haiku-StyledEdit",
		"application/x-vnd.Be-STEE",
		"application/x-vnd.yT-STEE",
		NULL
	};
	int i;
	for (i = 0; editorSigs[i]; i++) {
		team_id team = -1;
		{
			BMessenger msgr(editorSigs[i], team);
			if (msgr.SendMessage(&m) >= B_OK)
				break;

		}
		
		err = be_roster->Launch(editorSigs[i], (BMessage *)&m, &team);
		if (err >= B_OK || err == B_ALREADY_RUNNING)
			break;
	}
}

/**
 * Broadcast an URL that we can't handle.
 */

static nserror gui_launch_url(struct nsurl *url)
{
	status_t status;
	// try to open it as an URI
	BString mimeType = "application/x-vnd.Be.URL.";
	BString arg(nsurl_access(url));

	mimeType.Append(arg, arg.FindFirst(":"));

	// special case, text/x-email is used traditionally
	// use it instead
	if (arg.IFindFirst("mailto:") == 0)
		mimeType = "text/x-email";

	// the protocol should be alphanum
	// we just check if it's registered
	// if not there is likely no supporting app anyway
	if (!BMimeType::IsValid(mimeType.String()))
		return NSERROR_NO_FETCH_HANDLER;
	char *args[2] = { (char *)nsurl_access(url), NULL };
	status = be_roster->Launch(mimeType.String(), 1, args);
	if (status < B_OK)
		beos_warn_user("Cannot launch url", strerror(status));
        return NSERROR_OK;
}



void die(const char * const error)
{
	fprintf(stderr, "%s", error);
	BAlert *alert;
	BString text("Cannot continue:\n");
	text << error;

	alert = new BAlert("NetSurf Error", text.String(), "Debug", "Ok", NULL, 
		B_WIDTH_AS_USUAL, B_STOP_ALERT);
	if (alert->Go() < 1)
		debugger("die");

	exit(EXIT_FAILURE);
}


static struct gui_fetch_table beos_fetch_table = {
	fetch_filetype,
	gui_get_resource_url,
	NULL, // ???
	NULL, // release_resource_data
	NULL, // fetch_mimetype
};

static struct gui_misc_table beos_misc_table = {
	beos_schedule,
	gui_quit,
	gui_launch_url,
	NULL, //401login
	NULL, // pdf_password (if we have Haru support)
	NULL, // present_cookies
};


/** Normal entry point from OS */
int main(int argc, char** argv)
{
	nserror ret;
	BPath options;
	struct netsurf_table beos_table = {
		&beos_misc_table,
		beos_window_table,
		beos_download_table,
		beos_clipboard_table,
                &beos_fetch_table,
                NULL, /* use POSIX file */
                NULL, /* default utf8 */
                NULL, /* default search */
                NULL, /* default web search */
                NULL, /* default low level cache persistant storage */
                beos_bitmap_table,
                beos_layout_table
	};

        ret = netsurf_register(&beos_table);
        if (ret != NSERROR_OK) {
		die("NetSurf operation table failed registration");
        }

	if (find_directory(B_USER_SETTINGS_DIRECTORY, &options, true) == B_OK) {
		options.Append("x-vnd.NetSurf");
	}

	if (!replicated) {
		// create the Application object before trying to use messages
		// so we can open an alert in case of error.
		new NSBrowserApplication;
	}

	/* initialise logging. Not fatal if it fails but not much we
	 * can do about it either.
	 */
	nslog_init(nslog_stream_configure, &argc, argv);

	/* user options setup */
	ret = nsoption_init(set_defaults, &nsoptions, &nsoptions_default);
	if (ret != NSERROR_OK) {
		die("Options failed to initialise");
	}
	nsoption_read(options.Path(), NULL);
	nsoption_commandline(&argc, argv, NULL);

	/* common initialisation */
	BResources resources;
	resources.SetToImage((const void*)main);
	size_t size = 0;

	BString lang;
#ifdef __HAIKU__
	BMessage preferredLangs;
	if (BLocaleRoster::Default()->GetPreferredLanguages(&preferredLangs) == B_OK) {
		preferredLangs.FindString("language", 0, &lang);
	}
#endif
	if (lang.Length() < 1)
		lang.SetTo(getenv("LC_MESSAGES"));

	char path[12];
	sprintf(path,"%.2s/Messages", lang.String());
	NSLOG(netsurf, INFO, "Loading messages from resource %s\n", path);

	const uint8_t* res = (const uint8_t*)resources.LoadResource('data', path, &size);
	if (size > 0 && res != NULL) {
		ret = messages_add_from_inline(res, size);
	} else {
		BPath messages = get_messages_path();
        ret = messages_add_from_file(messages.Path());
	}

        ret = netsurf_init(NULL);
	if (ret != NSERROR_OK) {
		die("NetSurf failed to initialise");
	}

	gui_init(argc, argv);

	while (!nsbeos_done) {
		nsbeos_gui_poll();
	}

	netsurf_exit();

	/* finalise options */
	nsoption_finalise(nsoptions, nsoptions_default);

	/* finalise logging */
	nslog_finalise();

	return 0;
}

/** called when replicated from NSBaseView::Instantiate() */
int gui_init_replicant(int argc, char** argv)
{
	nserror ret;
	BPath options;
	struct netsurf_table beos_table = {
		&beos_misc_table,
		beos_window_table,
		beos_download_table,
		beos_clipboard_table,
                &beos_fetch_table,
                NULL, /* use POSIX file */
                NULL, /* default utf8 */
                NULL, /* default search */
                NULL, /* default web search */
                NULL, /* default low level cache persistant storage */
                beos_bitmap_table,
                beos_layout_table
	};

        ret = netsurf_register(&beos_table);
        if (ret != NSERROR_OK) {
		die("NetSurf operation table failed registration");
        }

	if (find_directory(B_USER_SETTINGS_DIRECTORY, &options, true) == B_OK) {
		options.Append("x-vnd.NetSurf");
	}

	/* initialise logging. Not fatal if it fails but not much we
	 * can do about it either.
	 */
	nslog_init(nslog_stream_configure, &argc, argv);

	// FIXME: use options as readonly for replicants
	/* user options setup */
	ret = nsoption_init(set_defaults, &nsoptions, &nsoptions_default);
	if (ret != NSERROR_OK) {
		// FIXME: must not die when in replicant!
		die("Options failed to initialise");
	}
	nsoption_read(options.Path(), NULL);
	nsoption_commandline(&argc, argv, NULL);

	/* common initialisation */
	BPath messages = get_messages_path();
        ret = messages_add_from_file(messages.Path());

        ret = netsurf_init(NULL);
	if (ret != NSERROR_OK) {
		// FIXME: must not die when in replicant!
		die("NetSurf failed to initialise");
	}

	gui_init(argc, argv);

	return 0;
}
