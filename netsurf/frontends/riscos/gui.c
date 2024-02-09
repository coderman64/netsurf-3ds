/*
 * Copyright 2003 Phil Mellor <monkeyson@users.sourceforge.net>
 * Copyright 2004-2008 James Bursa <bursa@users.sourceforge.net>
 * Copyright 2003 John M Bell <jmb202@ecs.soton.ac.uk>
 * Copyright 2005 Richard Wilson <info@tinct.net>
 * Copyright 2004 Andrew Timmins <atimmins@blueyonder.co.uk>
 * Copyright 2004-2009 John Tytgat <joty@netsurf-browser.org>
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
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <unixlib/local.h>
#include <fpu_control.h>
#include <oslib/help.h>
#include <oslib/uri.h>
#include <oslib/inetsuite.h>
#include <oslib/pdriver.h>
#include <oslib/osfile.h>
#include <oslib/hourglass.h>
#include <oslib/osgbpb.h>
#include <oslib/osbyte.h>
#include <oslib/osmodule.h>
#include <oslib/osfscontrol.h>

#include "utils/utils.h"
#include "utils/nsoption.h"
#include "utils/log.h"
#include "utils/messages.h"
#include "utils/file.h"
#include "utils/filename.h"
#include "utils/url.h"
#include "utils/corestrings.h"
#include "netsurf/fetch.h"
#include "netsurf/misc.h"
#include "netsurf/content.h"
#include "netsurf/netsurf.h"
#include "netsurf/browser_window.h"
#include "netsurf/cookie_db.h"
#include "netsurf/url_db.h"
#include "desktop/save_complete.h"
#include "desktop/hotlist.h"
#include "content/backing_store.h"

#include "riscos/gui.h"
#include "riscos/bitmap.h"
#include "riscos/wimputils.h"
#include "riscos/hotlist.h"
#include "riscos/buffer.h"
#include "riscos/textselection.h"
#include "riscos/print.h"
#include "riscos/save.h"
#include "riscos/dialog.h"
#include "riscos/wimp.h"
#include "riscos/message.h"
#include "riscos/help.h"
#include "riscos/query.h"
#include "riscos/window.h"
#include "riscos/iconbar.h"
#include "riscos/local_history.h"
#include "riscos/global_history.h"
#include "riscos/cookies.h"
#include "riscos/pageinfo.h"
#include "riscos/wimp_event.h"
#include "riscos/uri.h"
#include "riscos/url_protocol.h"
#include "riscos/mouse.h"
#include "riscos/ucstables.h"
#include "riscos/filetype.h"
#include "riscos/font.h"
#include "riscos/toolbar.h"
#include "riscos/content-handlers/artworks.h"
#include "riscos/content-handlers/draw.h"
#include "riscos/content-handlers/sprite.h"

bool riscos_done = false;

extern bool ro_plot_patterned_lines;

int os_version = 0;
bool os_alpha_sprite_supported = false;

const char * const __dynamic_da_name = "NetSurf";	/**< For UnixLib. */
int __dynamic_da_max_size = 128 * 1024 * 1024;	/**< For UnixLib. */
int __feature_imagefs_is_file = 1;		/**< For UnixLib. */
/* default filename handling */
int __riscosify_control = __RISCOSIFY_NO_SUFFIX |
			__RISCOSIFY_NO_REVERSE_SUFFIX;
#ifndef __ELF__
extern int __dynamic_num;
#endif

const char * NETSURF_DIR;

static const char *task_name = "NetSurf";
#define CHOICES_PREFIX "<Choices$Write>.WWW.NetSurf."

ro_gui_drag_type gui_current_drag_type;
wimp_t task_handle; /**< RISC OS wimp task handle. */
static clock_t gui_last_poll; /**< Time of last wimp_poll. */
osspriteop_area *gui_sprites; /**< Sprite area containing pointer and hotlist sprites */

#define DIR_SEP ('.')

/**
 * Accepted wimp user messages.
 */
static ns_wimp_message_list task_messages = {
	message_HELP_REQUEST,
	{
		message_DATA_SAVE,
		message_DATA_SAVE_ACK,
		message_DATA_LOAD,
		message_DATA_LOAD_ACK,
		message_DATA_OPEN,
		message_PRE_QUIT,
		message_SAVE_DESKTOP,
		message_MENU_WARNING,
		message_MENUS_DELETED,
		message_WINDOW_INFO,
		message_CLAIM_ENTITY,
		message_DATA_REQUEST,
		message_DRAGGING,
		message_DRAG_CLAIM,
		message_MODE_CHANGE,
		message_PALETTE_CHANGE,
		message_FONT_CHANGED,
		message_URI_PROCESS,
		message_URI_RETURN_RESULT,
		message_INET_SUITE_OPEN_URL,
		message_PRINT_SAVE,
		message_PRINT_ERROR,
		message_PRINT_TYPE_ODD,
		message_HOTLIST_ADD_URL,
		message_HOTLIST_CHANGED,
		0
	}
};


static struct
{
	int width; /* in OS units */
	int height;
} screen_info;


/**
 * Callback to translate resource to full url for RISC OS.
 *
 * Transforms a resource: path into a full URL. The returned URL is
 * used as the target for a redirect. The caller takes ownership of
 * the returned nsurl including unrefing it when finished with it.
 *
 * \param path The path of the resource to locate.
 * \return A string containing the full URL of the target object or
 *         NULL if no suitable resource can be found.
 */
static nsurl *gui_get_resource_url(const char *path)
{
	static const char base_url[] = "file:///NetSurf:/Resources/";
	const char *lang;
	size_t path_len, length;
	char *raw;
	nsurl *url = NULL;

	/* Map paths first */
	if (strcmp(path, "adblock.css") == 0) {
		path = "AdBlock";

	} else if (strcmp(path, "default.css") == 0) {
		path = "CSS";

	} else if (strcmp(path, "quirks.css") == 0) {
		path = "Quirks";

	} else if (strcmp(path, "favicon.ico") == 0) {
		path = "Icons/content.png";

	} else if (strcmp(path, "user.css") == 0) {
		/* Special case; this file comes from Choices: */
		nsurl_create("file:///Choices:WWW/NetSurf/User", &url);
		return url;
	}

	path_len = strlen(path);

	lang = ro_gui_default_language();

	/* Find max URL length */
	length = SLEN(base_url) +
			strlen(lang) + 1 + /* <lang> + / */
			path_len + 1; /* + NUL */

	raw = malloc(length);
	if (raw != NULL) {
		/* Insert base URL */
		char *ptr = memcpy(raw, base_url, SLEN(base_url));
		ptr += SLEN(base_url);

		/* Add language directory to URL, for translated files */
		/* TODO: handle non-html translated files */
		if (path_len > SLEN(".html") &&
				strncmp(path + path_len - SLEN(".html"),
					".html", SLEN(".html")) == 0) {
			ptr += sprintf(ptr, "%s/", lang);
		}

		/* Add filename to URL */
		memcpy(ptr, path, path_len);
		ptr += path_len;

		/* Terminate string */
		*ptr = '\0';

		nsurl_create(raw, &url);
		free(raw);
	}

	return url;
}


/**
 * Set colour option from wimp.
 *
 * \param opts The netsurf options.
 * \param wimp wimp colour value
 * \param option the netsurf option enum.
 * \param def_colour The default colour value to use.
 * \return NSERROR_OK on success or error code.
 */
static nserror
set_colour_from_wimp(struct nsoption_s *opts,
		   wimp_colour wimp,
		   enum nsoption_e option,
		   colour def_colour)
{
	os_error *error;
	os_PALETTE(20) palette;

	error = xwimp_read_true_palette((os_palette *) &palette);
	if (error != NULL) {
		NSLOG(netsurf, INFO,
		      "xwimp_read_palette: 0x%x: %s",
		      error->errnum,
		      error->errmess);
	} else {
		/* entries are in B0G0R0LL */
		def_colour = palette.entries[wimp] >> 8;
	}

	opts[option].value.c = def_colour;

	return NSERROR_OK;
}


/**
 * Set option defaults for riscos frontend
 *
 * @param defaults The option table to update.
 * @return error status.
 *
 * @todo The wimp_COLOUR_... values here map the colour definitions to
 * parts of the RISC OS desktop palette. In places this is fairly
 * arbitrary, and could probably do with re-checking.
 */
static nserror set_defaults(struct nsoption_s *defaults)
{
	/* Set defaults for absent option strings */
	nsoption_setnull_charp(ca_bundle, strdup("<NetSurf$CABundle>"));
	nsoption_setnull_charp(cookie_file, strdup("NetSurf:Cookies"));
	nsoption_setnull_charp(cookie_jar, strdup(CHOICES_PREFIX "Cookies"));

	if (nsoption_charp(ca_bundle) == NULL ||
	    nsoption_charp(cookie_file) == NULL ||
	    nsoption_charp(cookie_jar) == NULL) {
		NSLOG(netsurf, INFO, "Failed initialising default options");
		return NSERROR_BAD_PARAMETER;
	}

	/* RISC OS platform does not generally benefit from disc cache
	 * so the default should be off.
	 */
	nsoption_set_uint(disc_cache_size, 0);

	/* Override core default treeview font size with 12 pt.
	 * TODO: 12 is the normal desktop font size, but users might run
	 *       with something different.
	 */
	nsoption_set_int(treeview_font_size, 12 * 10);

	/* set default system colours for riscos ui */
	set_colour_from_wimp(defaults, wimp_COLOUR_BLACK, NSOPTION_sys_colour_ActiveBorder, 0x00000000);
	set_colour_from_wimp(defaults, wimp_COLOUR_CREAM, NSOPTION_sys_colour_ActiveCaption, 0x00dddddd);
	set_colour_from_wimp(defaults, wimp_COLOUR_VERY_LIGHT_GREY, NSOPTION_sys_colour_AppWorkspace, 0x00eeeeee);
	set_colour_from_wimp(defaults, wimp_COLOUR_VERY_LIGHT_GREY, NSOPTION_sys_colour_Background, 0x00aa0000);/* \TODO -- Check */
	set_colour_from_wimp(defaults, wimp_COLOUR_VERY_LIGHT_GREY, NSOPTION_sys_colour_ButtonFace, 0x00aaaaaa);
	set_colour_from_wimp(defaults, wimp_COLOUR_DARK_GREY, NSOPTION_sys_colour_ButtonHighlight, 0x00cccccc);/* \TODO -- Check */
	set_colour_from_wimp(defaults, wimp_COLOUR_MID_DARK_GREY, NSOPTION_sys_colour_ButtonShadow, 0x00bbbbbb);
	set_colour_from_wimp(defaults, wimp_COLOUR_BLACK, NSOPTION_sys_colour_ButtonText, 0x00000000);
	set_colour_from_wimp(defaults, wimp_COLOUR_BLACK, NSOPTION_sys_colour_CaptionText, 0x00000000);
	set_colour_from_wimp(defaults, wimp_COLOUR_MID_LIGHT_GREY, NSOPTION_sys_colour_GrayText, 0x00777777);/* \TODO -- Check */
	set_colour_from_wimp(defaults, wimp_COLOUR_BLACK, NSOPTION_sys_colour_Highlight, 0x00ee0000);
	set_colour_from_wimp(defaults, wimp_COLOUR_WHITE, NSOPTION_sys_colour_HighlightText, 0x00000000);
	set_colour_from_wimp(defaults, wimp_COLOUR_BLACK, NSOPTION_sys_colour_InactiveBorder, 0x00000000);
	set_colour_from_wimp(defaults, wimp_COLOUR_LIGHT_GREY, NSOPTION_sys_colour_InactiveCaption, 0x00ffffff);
	set_colour_from_wimp(defaults, wimp_COLOUR_BLACK, NSOPTION_sys_colour_InactiveCaptionText, 0x00cccccc);
	set_colour_from_wimp(defaults, wimp_COLOUR_CREAM, NSOPTION_sys_colour_InfoBackground, 0x00aaaaaa);
	set_colour_from_wimp(defaults, wimp_COLOUR_BLACK, NSOPTION_sys_colour_InfoText, 0x00000000);
	set_colour_from_wimp(defaults, wimp_COLOUR_WHITE, NSOPTION_sys_colour_Menu, 0x00aaaaaa);
	set_colour_from_wimp(defaults, wimp_COLOUR_BLACK, NSOPTION_sys_colour_MenuText, 0x00000000);
	set_colour_from_wimp(defaults, wimp_COLOUR_LIGHT_GREY, NSOPTION_sys_colour_Scrollbar, 0x00aaaaaa);/* \TODO -- Check */
	set_colour_from_wimp(defaults, wimp_COLOUR_MID_DARK_GREY, NSOPTION_sys_colour_ThreeDDarkShadow, 0x00555555);
	set_colour_from_wimp(defaults, wimp_COLOUR_VERY_LIGHT_GREY, NSOPTION_sys_colour_ThreeDFace, 0x00dddddd);
	set_colour_from_wimp(defaults, wimp_COLOUR_WHITE, NSOPTION_sys_colour_ThreeDHighlight, 0x00aaaaaa);
	set_colour_from_wimp(defaults, wimp_COLOUR_WHITE, NSOPTION_sys_colour_ThreeDLightShadow, 0x00999999);
	set_colour_from_wimp(defaults, wimp_COLOUR_MID_DARK_GREY, NSOPTION_sys_colour_ThreeDShadow, 0x00777777);
	set_colour_from_wimp(defaults, wimp_COLOUR_VERY_LIGHT_GREY, NSOPTION_sys_colour_Window, 0x00aaaaaa);
	set_colour_from_wimp(defaults, wimp_COLOUR_BLACK, NSOPTION_sys_colour_WindowFrame, 0x00000000);
	set_colour_from_wimp(defaults, wimp_COLOUR_BLACK, NSOPTION_sys_colour_WindowText, 0x00000000);

	return NSERROR_OK;
}




/**
 * Create intermediate directories for Choices and User Data files
 */
static void ro_gui_create_dirs(void)
{
	char buf[256];
	char *path;

	/* Choices */
	path = getenv("NetSurf$ChoicesSave");
	if (!path)
		die("Failed to find NetSurf Choices save path");

	snprintf(buf, sizeof(buf), "%s", path);
	netsurf_mkdir_all(buf);

	/* URL */
	snprintf(buf, sizeof(buf), "%s", nsoption_charp(url_save));
	netsurf_mkdir_all(buf);

	/* Hotlist */
	snprintf(buf, sizeof(buf), "%s", nsoption_charp(hotlist_save));
	netsurf_mkdir_all(buf);

	/* Recent */
	snprintf(buf, sizeof(buf), "%s", nsoption_charp(recent_save));
	netsurf_mkdir_all(buf);

	/* Theme */
	snprintf(buf, sizeof(buf), "%s", nsoption_charp(theme_save));
	netsurf_mkdir_all(buf);
	/* and the final directory part (as theme_save is a directory) */
	xosfile_create_dir(buf, 0);
}


/**
 * Ensures the gui exits cleanly.
 */
static void ro_gui_cleanup(void)
{
	ro_gui_buffer_close();
	xhourglass_off();
	/* Uninstall NetSurf-specific fonts */
	xos_cli("FontRemove NetSurf:Resources.Fonts.");
}


/**
 * Handles a signal
 */
static void ro_gui_signal(int sig)
{
	static const os_error error = { 1, "NetSurf has detected a serious "
			"error and must exit. Please submit a bug report, "
			"attaching the browser log file." };
	os_colour old_sand, old_glass;

	ro_gui_cleanup();

	xhourglass_on();
	xhourglass_colours(0x0000ffff, 0x000000ff, &old_sand, &old_glass);
	nsoption_dump(stderr, NULL);
	/*rufl_dump_state();*/

#ifndef __ELF__
	/* save WimpSlot and DA to files if NetSurf$CoreDump exists */
	int used;
	xos_read_var_val_size("NetSurf$CoreDump", 0, 0, &used, 0, 0);
	if (used) {
		int curr_slot;
		xwimp_slot_size(-1, -1, &curr_slot, 0, 0);
		NSLOG(netsurf, INFO, "saving WimpSlot, size 0x%x", curr_slot);
		xosfile_save("$.NetSurf_Slot", 0x8000, 0,
				(byte *) 0x8000,
				(byte *) 0x8000 + curr_slot);

		if (__dynamic_num != -1) {
			int size;
			byte *base_address;
			xosdynamicarea_read(__dynamic_num, &size,
					&base_address, 0, 0, 0, 0, 0);
			NSLOG(netsurf, INFO,
			      "saving DA %i, base %p, size 0x%x",
			      __dynamic_num,
			      base_address,
			      size);
			xosfile_save("$.NetSurf_DA",
					(bits) base_address, 0,
					base_address,
					base_address + size);
		}
	}
#else
	/* Save WimpSlot and UnixLib managed DAs when UnixEnv$coredump
	 * defines a coredump directory.  */
	const _kernel_oserror *err = __unixlib_write_coredump (NULL);
	if (err != NULL)
		NSLOG(netsurf, INFO, "Coredump failed: %s", err->errmess);
#endif

	xhourglass_colours(old_sand, old_glass, 0, 0);
	xhourglass_off();

	__write_backtrace(sig);

	xwimp_report_error_by_category(&error,
			wimp_ERROR_BOX_GIVEN_CATEGORY |
			wimp_ERROR_BOX_CATEGORY_ERROR <<
				wimp_ERROR_BOX_CATEGORY_SHIFT,
			"NetSurf", "!netsurf",
			(osspriteop_area *) 1, "Quit", 0);
	xos_cli("Filer_Run <Wimp$ScrapDir>.WWW.NetSurf.Log");

	_Exit(sig);
}


/**
 * Read a "line" from an Acorn URI file.
 *
 * \param fp file pointer to read from
 * \param b buffer for line, size 400 bytes
 * \return true on success, false on EOF
 */
static bool ro_gui_uri_file_parse_line(FILE *fp, char *b)
{
	int c;
	unsigned int i = 0;

	c = getc(fp);
	if (c == EOF)
		return false;

	/* skip comment lines */
	while (c == '#') {
		do { c = getc(fp); } while (c != EOF && 32 <= c);
		if (c == EOF)
			return false;
		do { c = getc(fp); } while (c != EOF && c < 32);
		if (c == EOF)
			return false;
	}

	/* read "line" */
	do {
		if (i == 399)
			return false;
		b[i++] = c;
		c = getc(fp);
	} while (c != EOF && 32 <= c);

	/* skip line ending control characters */
	while (c != EOF && c < 32)
		c = getc(fp);

	if (c != EOF)
		ungetc(c, fp);

	b[i] = 0;
	return true;
}


/**
 * Parse an Acorn URI file.
 *
 * \param file_name file to read
 * \param uri_title pointer to receive title data, or NULL for no data
 * \return URL from file, or 0 on error and error reported
 */
static char *ro_gui_uri_file_parse(const char *file_name, char **uri_title)
{
	/* See the "Acorn URI Handler Functional Specification" for the
	 * definition of the URI file format. */
	char line[400];
	char *url = NULL;
	FILE *fp;

	*uri_title = NULL;
	fp = fopen(file_name, "rb");
	if (!fp) {
		NSLOG(netsurf, INFO, "fopen(\"%s\", \"rb\"): %i: %s",
		      file_name, errno, strerror(errno));
		ro_warn_user("LoadError", strerror(errno));
		return 0;
	}

	/* "URI" */
	if (!ro_gui_uri_file_parse_line(fp, line) || strcmp(line, "URI") != 0)
		goto uri_syntax_error;

	/* version */
	if (!ro_gui_uri_file_parse_line(fp, line) ||
			strspn(line, "0123456789") != strlen(line))
		goto uri_syntax_error;

	/* URI */
	if (!ro_gui_uri_file_parse_line(fp, line))
		goto uri_syntax_error;

	url = strdup(line);
	if (!url) {
		ro_warn_user("NoMemory", 0);
		fclose(fp);
		return 0;
	}

	/* title */
	if (!ro_gui_uri_file_parse_line(fp, line))
		goto uri_free;
	if (uri_title && line[0] && ((line[0] != '*') || line[1])) {
		*uri_title = strdup(line);
		if (!*uri_title) /* non-fatal */
			ro_warn_user("NoMemory", 0);
	}
	fclose(fp);

	return url;

uri_free:
	free(url);

uri_syntax_error:
	fclose(fp);
	ro_warn_user("URIError", 0);
	return 0;
}


/**
 * Parse an ANT URL file.
 *
 * \param  file_name  file to read
 * \return  URL from file, or 0 on error and error reported
 */
static char *ro_gui_url_file_parse(const char *file_name)
{
	char line[400];
	char *url;
	FILE *fp;

	fp = fopen(file_name, "r");
	if (!fp) {
		NSLOG(netsurf, INFO, "fopen(\"%s\", \"r\"): %i: %s",
		      file_name, errno, strerror(errno));
		ro_warn_user("LoadError", strerror(errno));
		return 0;
	}

	if (!fgets(line, sizeof line, fp)) {
		if (ferror(fp)) {
			NSLOG(netsurf, INFO, "fgets: %i: %s", errno,
			      strerror(errno));
			ro_warn_user("LoadError", strerror(errno));
		} else
			ro_warn_user("LoadError", messages_get("EmptyError"));
		fclose(fp);
		return 0;
	}

	fclose(fp);

	if (line[strlen(line) - 1] == '\n')
		line[strlen(line) - 1] = '\0';

	url = strdup(line);
	if (!url) {
		ro_warn_user("NoMemory", 0);
		return 0;
	}

	return url;
}


/**
 * Parse an IEURL file.
 *
 * \param  file_name  file to read
 * \return  URL from file, or 0 on error and error reported
 */
static char *ro_gui_ieurl_file_parse(const char *file_name)
{
	char line[400];
	char *url = 0;
	FILE *fp;

	fp = fopen(file_name, "r");
	if (!fp) {
		NSLOG(netsurf, INFO, "fopen(\"%s\", \"r\"): %i: %s",
		      file_name, errno, strerror(errno));
		ro_warn_user("LoadError", strerror(errno));
		return 0;
	}

	while (fgets(line, sizeof line, fp)) {
		if (strncmp(line, "URL=", 4) == 0) {
			if (line[strlen(line) - 1] == '\n')
				line[strlen(line) - 1] = '\0';
			url = strdup(line + 4);
			if (!url) {
				fclose(fp);
				ro_warn_user("NoMemory", 0);
				return 0;
			}
			break;
		}
	}
	if (ferror(fp)) {
		NSLOG(netsurf, INFO, "fgets: %i: %s", errno, strerror(errno));
		ro_warn_user("LoadError", strerror(errno));
		fclose(fp);
		return 0;
	}

	fclose(fp);

	if (!url)
		ro_warn_user("URIError", 0);

	return url;
}


/**
 * Handle Message_DataOpen (double-click on file in the Filer).
 *
 * \param message The wimp message to open.
 */
static void ro_msg_dataopen(wimp_message *message)
{
	int file_type = message->data.data_xfer.file_type;
	char *url = 0;
	os_error *oserror;
	nsurl *urlns;
	nserror error;
	size_t len;

	switch (file_type) {
	case 0xb28:			/* ANT URL file */
		url = ro_gui_url_file_parse(message->data.data_xfer.file_name);
		error = nsurl_create(url, &urlns);
		free(url);
		break;

	case 0xfaf:		/* HTML file */
		error = netsurf_path_to_nsurl(message->data.data_xfer.file_name,
					      &urlns);
		break;

	case 0x1ba:		/* IEURL file */
		url = ro_gui_ieurl_file_parse(message->
				data.data_xfer.file_name);
		error = nsurl_create(url, &urlns);
		free(url);
		break;

	case 0x2000: 		/* application */
		len = strlen(message->data.data_xfer.file_name);
		if (len < 9 || strcmp(".!NetSurf",
				message->data.data_xfer.file_name + len - 9))
			return;

		if (nsoption_charp(homepage_url) &&
		    nsoption_charp(homepage_url)[0]) {
			error = nsurl_create(nsoption_charp(homepage_url),
					     &urlns);
		} else {
			error = nsurl_create(NETSURF_HOMEPAGE, &urlns);
		}
		break;

	default:
		return;
	}

	/* send DataLoadAck */
	message->action = message_DATA_LOAD_ACK;
	message->your_ref = message->my_ref;
	oserror = xwimp_send_message(wimp_USER_MESSAGE, message, message->sender);
	if (oserror) {
		NSLOG(netsurf, INFO, "xwimp_send_message: 0x%x: %s",
		      oserror->errnum, oserror->errmess);
		ro_warn_user("WimpError", oserror->errmess);
		return;
	}

	if (error != NSERROR_OK) {
		ro_warn_user(messages_get_errorcode(error), 0);
		return;
	}

	/* create a new window with the file */
	error = browser_window_create(BW_CREATE_HISTORY,
				      urlns,
				      NULL,
				      NULL,
				      NULL);
	nsurl_unref(urlns);
	if (error != NSERROR_OK) {
		ro_warn_user(messages_get_errorcode(error), 0);
	}
}


/**
 * Handle Message_DataLoad (file dragged in).
 */
static void ro_msg_dataload(wimp_message *message)
{
	int file_type = message->data.data_xfer.file_type;
	char *urltxt = NULL;
	char *title = NULL;
	struct gui_window *g;
	os_error *oserror;
	nsurl *url;
	nserror error;

	g = ro_gui_window_lookup(message->data.data_xfer.w);
	if (g) {
		if (ro_gui_window_dataload(g, message))
			return;
	}
	else {
		g = ro_gui_toolbar_lookup(message->data.data_xfer.w);
		if (g && ro_gui_toolbar_dataload(g, message))
			return;
	}

	switch (file_type) {
		case FILETYPE_ACORN_URI:
			urltxt = ro_gui_uri_file_parse(message->data.data_xfer.file_name,
					&title);
			error = nsurl_create(urltxt, &url);
			free(urltxt);
			break;

		case FILETYPE_ANT_URL:
			urltxt = ro_gui_url_file_parse(message->data.data_xfer.file_name);
			error = nsurl_create(urltxt, &url);
			free(urltxt);
			break;

		case FILETYPE_IEURL:
			urltxt = ro_gui_ieurl_file_parse(message->data.data_xfer.file_name);
			error = nsurl_create(urltxt, &url);
			free(urltxt);
			break;

		case FILETYPE_HTML:
		case FILETYPE_JNG:
		case FILETYPE_CSS:
		case FILETYPE_MNG:
		case FILETYPE_GIF:
		case FILETYPE_BMP:
		case FILETYPE_ICO:
		case osfile_TYPE_DRAW:
		case FILETYPE_PNG:
		case FILETYPE_JPEG:
		case osfile_TYPE_SPRITE:
		case osfile_TYPE_TEXT:
		case FILETYPE_ARTWORKS:
		case FILETYPE_SVG:
		case FILETYPE_WEBP:
			/* display the actual file */
			error = netsurf_path_to_nsurl(message->data.data_xfer.file_name, &url);
			break;

		default:
			return;
	}

	/* report error to user */
	if (error != NSERROR_OK) {
		ro_warn_user(messages_get_errorcode(error), 0);
		return;
	}


	if (g) {
		error = browser_window_navigate(g->bw,
						url,
						NULL,
						BW_NAVIGATE_HISTORY,
						NULL,
						NULL,
						NULL);
	} else {
		error = browser_window_create(BW_CREATE_HISTORY,
					      url,
					      NULL,
					      NULL,
					      NULL);
	}
	nsurl_unref(url);
	if (error != NSERROR_OK) {
		ro_warn_user(messages_get_errorcode(error), 0);
	}


	/* send DataLoadAck */
	message->action = message_DATA_LOAD_ACK;
	message->your_ref = message->my_ref;
	oserror = xwimp_send_message(wimp_USER_MESSAGE, message,
			message->sender);
	if (oserror) {
		NSLOG(netsurf, INFO, "xwimp_send_message: 0x%x: %s",
		      oserror->errnum, oserror->errmess);
		ro_warn_user("WimpError", oserror->errmess);
		return;
	}

}


/**
 * Ensure that the filename in a data transfer message is NULL terminated
 * (some applications, especially BASIC programs use CR)
 *
 * \param message message to be corrected
 */
static void ro_msg_terminate_filename(wimp_full_message_data_xfer *message)
{
	const char *ep = (char*)message + message->size;
	char *p = message->file_name;

	if ((size_t)message->size >= sizeof(*message))
		ep = (char*)message + sizeof(*message) - 1;

	while (p < ep && *p >= ' ') p++;
	*p = '\0';
}


/**
 * Handle Message_DataSave
 */
static void ro_msg_datasave(wimp_message *message)
{
	wimp_full_message_data_xfer *dataxfer = (wimp_full_message_data_xfer*)message;

	/* remove ghost caret if drag-and-drop protocol was used */
//	ro_gui_selection_drag_reset();

	ro_msg_terminate_filename(dataxfer);

	if (ro_gui_selection_prepare_paste_datasave(dataxfer))
		return;

	switch (dataxfer->file_type) {
		case FILETYPE_ACORN_URI:
		case FILETYPE_ANT_URL:
		case FILETYPE_IEURL:
		case FILETYPE_HTML:
		case FILETYPE_JNG:
		case FILETYPE_CSS:
		case FILETYPE_MNG:
		case FILETYPE_GIF:
		case FILETYPE_BMP:
		case FILETYPE_ICO:
		case osfile_TYPE_DRAW:
		case FILETYPE_PNG:
		case FILETYPE_JPEG:
		case osfile_TYPE_SPRITE:
		case osfile_TYPE_TEXT:
		case FILETYPE_ARTWORKS:
		case FILETYPE_SVG:
		case FILETYPE_WEBP: {
			os_error *error;

			dataxfer->your_ref = dataxfer->my_ref;
			dataxfer->size = offsetof(wimp_full_message_data_xfer, file_name) + 16;
			dataxfer->action = message_DATA_SAVE_ACK;
			dataxfer->est_size = -1;
			memcpy(dataxfer->file_name, "<Wimp$Scrap>", 13);

			error = xwimp_send_message(wimp_USER_MESSAGE, (wimp_message*)dataxfer, message->sender);
			if (error) {
				NSLOG(netsurf, INFO,
				      "xwimp_send_message: 0x%x: %s",
				      error->errnum,
				      error->errmess);
				ro_warn_user("WimpError", error->errmess);
			}
		}
		break;
	}
}


/**
 * Handle Message_DataSaveAck.
 */
static void ro_msg_datasave_ack(wimp_message *message)
{
	ro_msg_terminate_filename((wimp_full_message_data_xfer*)message);

	if (ro_print_ack(message))
		return;

	switch (gui_current_drag_type) {
		case GUI_DRAG_DOWNLOAD_SAVE:
			ro_gui_download_datasave_ack(message);
			break;

		case GUI_DRAG_SAVE:
			ro_gui_save_datasave_ack(message);
			gui_current_drag_type = GUI_DRAG_NONE;
			break;

		default:
			break;
	}

	gui_current_drag_type = GUI_DRAG_NONE;
}


/**
 * Handle PreQuit message
 *
 * \param  message  PreQuit message from Wimp
 */
static void ro_msg_prequit(wimp_message *message)
{
	if (!ro_gui_prequit()) {
		os_error *error;

		/* we're objecting to the close down */
		message->your_ref = message->my_ref;
		error = xwimp_send_message(wimp_USER_MESSAGE_ACKNOWLEDGE,
						message, message->sender);
		if (error) {
			NSLOG(netsurf, INFO, "xwimp_send_message: 0x%x:%s",
			      error->errnum, error->errmess);
			ro_warn_user("WimpError", error->errmess);
		}
	}
}


/**
 * Handle SaveDesktop message.
 *
 * \param message SaveDesktop message from Wimp.
 */
static void ro_msg_save_desktop(wimp_message *message)
{
	os_error *error;

	error = xosgbpb_writew(message->data.save_desktopw.file,
				(const byte*)"Run ", 4, NULL);
	if (!error) {
		error = xosgbpb_writew(message->data.save_desktopw.file,
					(const byte*)NETSURF_DIR, strlen(NETSURF_DIR), NULL);
		if (!error)
			error = xos_bputw('\n', message->data.save_desktopw.file);
	}

	if (error) {
		NSLOG(netsurf, INFO, "xosgbpb_writew/xos_bputw: 0x%x:%s",
		      error->errnum, error->errmess);
		ro_warn_user("SaveError", error->errmess);

		/* we must cancel the save by acknowledging the message */
		message->your_ref = message->my_ref;
		error = xwimp_send_message(wimp_USER_MESSAGE_ACKNOWLEDGE,
						message, message->sender);
		if (error) {
			NSLOG(netsurf, INFO, "xwimp_send_message: 0x%x:%s",
			      error->errnum, error->errmess);
			ro_warn_user("WimpError", error->errmess);
		}
	}
}


/**
 * Handle WindowInfo message (part of the iconising protocol)
 *
 * \param  message  WindowInfo message from the Iconiser
 */
static void ro_msg_window_info(wimp_message *message)
{
	wimp_full_message_window_info *wi;
	struct gui_window *g;

	/* allow the user to turn off thumbnail icons */
	if (!nsoption_bool(thumbnail_iconise))
		return;

	wi = (wimp_full_message_window_info*)message;
	g = ro_gui_window_lookup(wi->w);

	/* ic_<task name> will suffice for our other windows */
	if (g) {
		ro_gui_window_iconise(g, wi);
		ro_gui_dialog_close_persistent(wi->w);
	}
}


/**
 * Get screen properties following a mode change.
 */
static void ro_gui_get_screen_properties(void)
{
	static const ns_os_vdu_var_list vars = {
		os_MODEVAR_XWIND_LIMIT,
		{
			os_MODEVAR_YWIND_LIMIT,
			os_MODEVAR_XEIG_FACTOR,
			os_MODEVAR_YEIG_FACTOR,
			os_VDUVAR_END_LIST
		}
	};
	os_error *error;
	int vals[4];

	error = xos_read_vdu_variables(PTR_OS_VDU_VAR_LIST(&vars), vals);
	if (error) {
		NSLOG(netsurf, INFO, "xos_read_vdu_variables: 0x%x: %s",
		      error->errnum, error->errmess);
		ro_warn_user("MiscError", error->errmess);
		return;
	}
	screen_info.width  = (vals[0] + 1) << vals[2];
	screen_info.height = (vals[1] + 1) << vals[3];
}


/**
 * Warn the user if Inet$Resolvers is not set.
 */
static void ro_gui_check_resolvers(void)
{
	char *resolvers;
	resolvers = getenv("Inet$Resolvers");
	if (resolvers && resolvers[0]) {
		NSLOG(netsurf, INFO, "Inet$Resolvers '%s'", resolvers);
	} else {
		NSLOG(netsurf, INFO, "Inet$Resolvers not set or empty");
		ro_warn_user("Resolvers", 0);
	}
}

/**
 * Determine whether the OS version supports alpha channels.
 *
 * \return true iff alpha channels are supported, false otherwise.
 */
static bool ro_gui__os_alpha_sprites_supported(void)
{
	os_error *error;
	int var_val;
	bits psr;

	psr = 0;
	error = xos_read_mode_variable(alpha_SPRITE_MODE,
			os_MODEVAR_MODE_FLAGS, &var_val, &psr);
	if (error) {
		NSLOG(netsurf, ERROR, "xos_read_mode_variable: 0x%x: %s",
				error->errnum, error->errmess);
		return false;
	}

	return (var_val == (1 << 15));
}

/**
 * Initialise the RISC OS specific GUI.
 *
 * \param argc The number of command line arguments.
 * \param argv The string vector of command line arguments.
 */
static nserror gui_init(int argc, char** argv)
{
	struct {
		void (*sigabrt)(int);
		void (*sigfpe)(int);
		void (*sigill)(int);
		void (*sigint)(int);
		void (*sigsegv)(int);
		void (*sigterm)(int);
		void (*sigoserror)(int);
	} prev_sigs;
	char path[40];
	os_error *error;
	int length;
	char *nsdir_temp;
	byte *base;
	nsurl *url;
	nserror ret;
	bool open_window;

	/* re-enable all FPU exceptions/traps except inexact operations,
	 * which we're not interested in, and underflow which is incorrectly
	 * raised when converting an exact value of 0 from double-precision
	 * to single-precision on FPEmulator v4.09-4.11 (MVFD F0,#0:MVFS F0,F0)
	 * - UnixLib disables all FP exceptions by default */

	_FPU_SETCW(_FPU_IEEE & ~(_FPU_MASK_PM | _FPU_MASK_UM));

	xhourglass_start(1);

	/* read OS version for code that adapts to conform to the OS
	 * (remember that it's preferable to check for specific features
	 * being present) */
	xos_byte(osbyte_IN_KEY, 0, 0xff, &os_version, NULL);

	os_alpha_sprite_supported = ro_gui__os_alpha_sprites_supported();
	NSLOG(netsurf, INFO, "OS supports alpha sprites: %s",
			os_alpha_sprite_supported ? "yes" : "no");

	/* the first release version of the A9home OS is incapable of
	   plotting patterned lines (presumably a fault in the hw acceleration) */
	if (!xosmodule_lookup("VideoHWSMI", NULL, NULL, &base, NULL, NULL)) {
#if 0   // this fault still hasn't been fixed, so disable patterned lines for all versions until it has
		const char *help = (char*)base + ((int*)base)[5];
		while (*help > 9) help++;
		while (*help == 9) help++;
		if (!memcmp(help, "0.55", 4))
#endif
			ro_plot_patterned_lines = false;
	}

	/* Create our choices directories */
	ro_gui_create_dirs();

	/* Register exit and signal handlers */
	atexit(ro_gui_cleanup);
	prev_sigs.sigabrt = signal(SIGABRT, ro_gui_signal);
	prev_sigs.sigfpe = signal(SIGFPE, ro_gui_signal);
	prev_sigs.sigill = signal(SIGILL, ro_gui_signal);
	prev_sigs.sigint = signal(SIGINT, ro_gui_signal);
	prev_sigs.sigsegv = signal(SIGSEGV, ro_gui_signal);
	prev_sigs.sigterm = signal(SIGTERM, ro_gui_signal);
	prev_sigs.sigoserror = signal(SIGOSERROR, ro_gui_signal);

	if (prev_sigs.sigabrt == SIG_ERR || prev_sigs.sigfpe == SIG_ERR ||
			prev_sigs.sigill == SIG_ERR ||
			prev_sigs.sigint == SIG_ERR ||
			prev_sigs.sigsegv == SIG_ERR ||
			prev_sigs.sigterm == SIG_ERR ||
			prev_sigs.sigoserror == SIG_ERR)
		die("Failed registering signal handlers");

	/* Load in UI sprites */
	gui_sprites = ro_gui_load_sprite_file("NetSurf:Resources.Sprites");
	if (!gui_sprites)
		die("Unable to load Sprites.");

	/* Find NetSurf directory */
	nsdir_temp = getenv("NetSurf$Dir");
	if (!nsdir_temp)
		die("Failed to locate NetSurf directory");
	NETSURF_DIR = strdup(nsdir_temp);
	if (!NETSURF_DIR)
		die("Failed duplicating NetSurf directory string");

	/* Initialise filename allocator */
	filename_initialise();

	/* Initialise save complete functionality */
	save_complete_init();

	/* Load in visited URLs and Cookies */
	urldb_load(nsoption_charp(url_path));
	urldb_load_cookies(nsoption_charp(cookie_file));

	/* Initialise with the wimp */
	error = xwimp_initialise(wimp_VERSION_RO38, task_name,
			PTR_WIMP_MESSAGE_LIST(&task_messages), 0,
			&task_handle);
	if (error) {
		NSLOG(netsurf, INFO, "xwimp_initialise: 0x%x: %s",
		      error->errnum, error->errmess);
		die(error->errmess);
	}
	/* Register message handlers */
	ro_message_register_route(message_HELP_REQUEST,
			ro_gui_interactive_help_request);
	ro_message_register_route(message_DATA_OPEN,
			ro_msg_dataopen);
	ro_message_register_route(message_DATA_SAVE,
			ro_msg_datasave);
	ro_message_register_route(message_DATA_SAVE_ACK,
			ro_msg_datasave_ack);
	ro_message_register_route(message_PRE_QUIT,
			ro_msg_prequit);
	ro_message_register_route(message_SAVE_DESKTOP,
			ro_msg_save_desktop);
	ro_message_register_route(message_DRAGGING,
			ro_gui_selection_dragging);
	ro_message_register_route(message_DRAG_CLAIM,
			ro_gui_selection_drag_claim);
	ro_message_register_route(message_WINDOW_INFO,
			ro_msg_window_info);

	/* Initialise the font subsystem (must be after Wimp_Initialise) */
	nsfont_init();

	/* Initialise the hotlist (must be after fonts) */
	hotlist_init(nsoption_charp(hotlist_path),
			nsoption_bool(external_hotlists) ?
					NULL :
					nsoption_charp(hotlist_save));

	/* Initialise global information */
	ro_gui_get_screen_properties();
	ro_gui_wimp_get_desktop_font();

	/* Issue a *Desktop to poke AcornURI into life */
	if (getenv("NetSurf$Start_URI_Handler"))
		xwimp_start_task("Desktop", 0);

	/* Open the templates */
	if ((length = snprintf(path, sizeof(path),
			"NetSurf:Resources.%s.Templates",
			nsoption_charp(language))) < 0 || length >= (int)sizeof(path))
		die("Failed to locate Templates resource.");
	error = xwimp_open_template(path);
	if (error) {
		NSLOG(netsurf, INFO, "xwimp_open_template failed: 0x%x: %s",
		      error->errnum, error->errmess);
		die(error->errmess);
	}

	/* Initialise themes before dialogs */
	ro_gui_theme_initialise();

	/* Initialise dialog windows (must be after UI sprites are loaded) */
	ro_gui_dialog_init();

	/* Initialise download window */
	ro_gui_download_init();

	/* Initialise menus */
	ro_gui_menu_init();

	/* Initialise query windows */
	ro_gui_query_init();

	/* Initialise toolbars */
	ro_toolbar_init();

	/* Initialise url bar module */
	ro_gui_url_bar_init();

	/* Initialise browser windows */
	ro_gui_window_initialise();

	/* Done with the templates file */
	wimp_close_template();

	/* Create Iconbar icon and menus */
	ro_gui_iconbar_initialise();

	/* Finally, check Inet$Resolvers for sanity */
	ro_gui_check_resolvers();

	open_window = nsoption_bool(open_browser_at_startup);

	/* parse command-line arguments */
	if (argc == 2) {
		NSLOG(netsurf, INFO, "parameters: '%s'", argv[1]);
		/* this is needed for launching URI files */
		if (strcasecmp(argv[1], "-nowin") == 0) {
			return NSERROR_OK;
		}
		ret = nsurl_create(NETSURF_HOMEPAGE, &url);
	}
	else if (argc == 3) {
		NSLOG(netsurf, INFO, "parameters: '%s' '%s'", argv[1],
		      argv[2]);
		open_window = true;

		/* HTML files */
		if (strcasecmp(argv[1], "-html") == 0) {
			ret = netsurf_path_to_nsurl(argv[2], &url);
		}
		/* URL files */
		else if (strcasecmp(argv[1], "-urlf") == 0) {
			char *urlf = ro_gui_url_file_parse(argv[2]);
			if (!urlf) {
				NSLOG(netsurf, INFO, "allocation failed");
				die("Insufficient memory for URL");
			}
			ret = nsurl_create(urlf, &url);
			free(urlf);
		}
		/* ANT URL Load */
		else if (strcasecmp(argv[1], "-url") == 0) {
			ret = nsurl_create(argv[2], &url);
		}
		/* Unknown => exit here. */
		else {
			NSLOG(netsurf, INFO, "Unknown parameters: '%s' '%s'",
			      argv[1], argv[2]);
			return NSERROR_BAD_PARAMETER;
		}
	}
	/* get user's homepage (if configured) */
	else if (nsoption_charp(homepage_url) &&
		 nsoption_charp(homepage_url)[0]) {
		ret = nsurl_create(nsoption_charp(homepage_url), &url);
	}
	/* default homepage */
	else {
		ret = nsurl_create(NETSURF_HOMEPAGE, &url);
	}

	/* check for url creation error */
	if (ret != NSERROR_OK) {
		return ret;
	}

	if (open_window) {
		ret = browser_window_create(BW_CREATE_HISTORY,
					    url,
					    NULL,
					    NULL,
					    NULL);
	}
	nsurl_unref(url);

	return ret;
}


/**
 * Determine the default language to use.
 *
 * RISC OS has no standard way of determining which language the user prefers.
 * We have to guess from the 'Country' setting.
 */
const char *ro_gui_default_language(void)
{
	char path[40];
	const char *lang;
	int country;
	os_error *error;

	/* choose a language from the configured country number */
	error = xosbyte_read(osbyte_VAR_COUNTRY_NUMBER, &country);
	if (error) {
		NSLOG(netsurf, INFO, "xosbyte_read failed: 0x%x: %s",
		      error->errnum, error->errmess);
		country = 1;
	}
	switch (country) {
		case 7: /* Germany */
		case 30: /* Austria */
		case 35: /* Switzerland (70% German-speaking) */
			lang = "de";
			break;
		case 6: /* France */
		case 18: /* Canada2 (French Canada?) */
			lang = "fr";
			break;
		case 34: /* Netherlands */
			lang = "nl";
			break;
		default:
			lang = "en";
			break;
	}
	sprintf(path, "NetSurf:Resources.%s", lang);
	if (is_dir(path))
		return lang;
	return "en";
}


/**
 * Create a nsurl from a RISC OS pathname.
 *
 * Perform the necessary operations on a path to generate a nsurl.
 *
 * @param[in] path The RISC OS pathname to convert.
 * @param[out] url_out pointer to recive the nsurl, The returned url must be
 *                 unreferenced by the caller.
 * @return NSERROR_OK and the url is placed in \a url or error code on faliure.
 */
static nserror ro_path_to_nsurl(const char *path, struct nsurl **url_out)
{
	int spare;
	char *canonical_path; /* canonicalised RISC OS path */
	char *unix_path; /* unix path */
	char *escaped_path;
	os_error *error;
	nserror ret;
	int urllen;
	char *url; /* resulting url */

	/* calculate the canonical risc os path */
	error = xosfscontrol_canonicalise_path(path, 0, 0, 0, 0, &spare);
	if (error) {
		NSLOG(netsurf, INFO,
		      "xosfscontrol_canonicalise_path failed: 0x%x: %s",
		      error->errnum,
		      error->errmess);
		ro_warn_user("PathToURL", error->errmess);
		return NSERROR_NOT_FOUND;
	}

	canonical_path = malloc(1 - spare);
	if (canonical_path == NULL) {
		free(canonical_path);
		return NSERROR_NOMEM;
	}

	error = xosfscontrol_canonicalise_path(path, canonical_path, 0, 0, 1 - spare, 0);
	if (error) {
		NSLOG(netsurf, INFO,
		      "xosfscontrol_canonicalise_path failed: 0x%x: %s",
		      error->errnum,
		      error->errmess);
		ro_warn_user("PathToURL", error->errmess);
		free(canonical_path);
		return NSERROR_NOT_FOUND;
	}

	/* create a unix path from the cananocal risc os one */
	unix_path = __unixify(canonical_path, __RISCOSIFY_NO_REVERSE_SUFFIX, NULL, 0, 0);

	if (unix_path == NULL) {
		NSLOG(netsurf, INFO, "__unixify failed: %s", canonical_path);
		free(canonical_path);
		return NSERROR_BAD_PARAMETER;
	}
	free(canonical_path);

	/* url escape the unix path */
	ret = url_escape(unix_path, false, "/", &escaped_path);
	if (ret != NSERROR_OK) {
		free(unix_path);
		return ret;
	}
	free(unix_path);

	/* convert the escaped unix path into a url */
	urllen = strlen(escaped_path) + FILE_SCHEME_PREFIX_LEN + 1;
	url = malloc(urllen);
	if (url == NULL) {
		NSLOG(netsurf, INFO, "Unable to allocate url");
		free(escaped_path);
		return NSERROR_NOMEM;
	}

	if (*escaped_path == '/') {
		snprintf(url, urllen, "%s%s",
				FILE_SCHEME_PREFIX, escaped_path + 1);
	} else {
		snprintf(url, urllen, "%s%s",
				FILE_SCHEME_PREFIX, escaped_path);
	}
	free(escaped_path);

	ret = nsurl_create(url, url_out);
	free(url);

	return ret;
}


/**
 * Create a path from a nsurl using posix file handling.
 *
 * @param[in] url The url to encode.
 * @param[out] path_out A string containing the result path which should
 *                      be freed by the caller.
 * @return NSERROR_OK and the path is written to \a path or error code
 *         on faliure.
 */
static nserror ro_nsurl_to_path(struct nsurl *url, char **path_out)
{
	lwc_string *urlpath;
	size_t unpath_len;
	char *unpath;
	char *path;
	bool match;
	lwc_string *scheme;
	nserror res;
	char *r;

	if ((url == NULL) || (path_out == NULL)) {
		return NSERROR_BAD_PARAMETER;
	}

	scheme = nsurl_get_component(url, NSURL_SCHEME);

	if (lwc_string_caseless_isequal(scheme, corestring_lwc_file,
					&match) != lwc_error_ok)
	{
		return NSERROR_BAD_PARAMETER;
	}
	lwc_string_unref(scheme);
	if (match == false) {
		return NSERROR_BAD_PARAMETER;
	}

	urlpath = nsurl_get_component(url, NSURL_PATH);
	if (urlpath == NULL) {
		return NSERROR_BAD_PARAMETER;
	}

	res = url_unescape(lwc_string_data(urlpath),
			   lwc_string_length(urlpath),
			   &unpath_len,
			   &unpath);
	lwc_string_unref(urlpath);
	if (res != NSERROR_OK) {
		return res;
	}

	/* RISC OS path should not be more than 100 characters longer */
	path = malloc(unpath_len + 100);
	if (path == NULL) {
		free(unpath);
		return NSERROR_NOMEM;
	}

	r = __riscosify(unpath, 0, __RISCOSIFY_NO_SUFFIX,
			path, unpath_len + 100, 0);
	free(unpath);
	if (r == NULL) {
		free(path);
		return NSERROR_NOMEM;
	}

	*path_out = path;

	return NSERROR_OK;
}


/**
 * Ensures output logging stream is correctly configured.
 */
static bool nslog_stream_configure(FILE *fptr)
{
	/* set log stream to be non-buffering */
	setbuf(fptr, NULL);

	return true;
}


/**
 * Close down the gui (RISC OS).
 */
static void gui_quit(void)
{
	urldb_save_cookies(nsoption_charp(cookie_jar));
	urldb_save(nsoption_charp(url_save));
	ro_gui_window_quit();
	ro_gui_local_history_finalise();
	ro_gui_global_history_finalise();
	ro_gui_pageinfo_finalise();
	ro_gui_hotlist_finalise();
	ro_gui_cookies_finalise();
	ro_gui_saveas_quit();
	ro_gui_url_bar_fini();
	rufl_quit();
	free(gui_sprites);
	xwimp_close_down(task_handle);
	xhourglass_off();
}


/**
 * Handle Close_Window_Request events.
 */
static void ro_gui_close_window_request(wimp_close *close)
{
	if (ro_gui_alt_pressed())
		ro_gui_window_close_all();
	else {
		if (ro_gui_wimp_event_close_window(close->w))
			return;
		ro_gui_dialog_close(close->w);
	}
}


/**
 * Handle key press paste callback.
 */
static void ro_gui_keypress_cb(void *pw)
{
	wimp_key *key = (wimp_key *) pw;

	if (ro_gui_wimp_event_keypress(key) == false) {
		os_error *error = xwimp_process_key(key->c);
		if (error) {
			NSLOG(netsurf, INFO, "xwimp_process_key: 0x%x: %s",
			      error->errnum, error->errmess);
			ro_warn_user("WimpError", error->errmess);
		}
	}

	free(key);
}


/**
 * Handle gui keypress.
 */
static void ro_gui_keypress(wimp_key *key)
{
	if (key->c == wimp_KEY_ESCAPE &&
		(gui_current_drag_type == GUI_DRAG_SAVE ||
		 gui_current_drag_type == GUI_DRAG_DOWNLOAD_SAVE)) {

		/* Allow Escape key to be used for cancelling a drag
		 * save (easier than finding somewhere safe to abort
		 * the drag)
		 */
		ro_gui_drag_box_cancel();
		gui_current_drag_type = GUI_DRAG_NONE;
	} else if (key->c == 22 /* Ctrl-V */) {
		wimp_key *copy;

		/* Must copy the keypress as it's on the stack */
		copy = malloc(sizeof(wimp_key));
		if (copy == NULL)
			return;
		memcpy(copy, key, sizeof(wimp_key));

		ro_gui_selection_prepare_paste(key->w, ro_gui_keypress_cb, copy);
	} else if (ro_gui_wimp_event_keypress(key) == false) {
		os_error *error = xwimp_process_key(key->c);
		if (error) {
			NSLOG(netsurf, INFO, "xwimp_process_key: 0x%x: %s",
			      error->errnum, error->errmess);
			ro_warn_user("WimpError", error->errmess);
		}
	}
}


/**
 * Handle the three User_Message events.
 */
static void ro_gui_user_message(wimp_event_no event, wimp_message *message)
{
	/* attempt automatic routing */
	if (ro_message_handle_message(event, message))
		return;

	switch (message->action) {
		case message_DATA_LOAD:
			ro_msg_terminate_filename((wimp_full_message_data_xfer*)message);

			if (event == wimp_USER_MESSAGE_ACKNOWLEDGE) {
				if (ro_print_current_window)
					ro_print_dataload_bounce(message);
			} else if (ro_gui_selection_prepare_paste_dataload(
					(wimp_full_message_data_xfer *) message) == false) {
				ro_msg_dataload(message);
			}
			break;

		case message_DATA_LOAD_ACK:
			if (ro_print_current_window)
				ro_print_cleanup();
			break;

		case message_MENU_WARNING:
			ro_gui_menu_warning((wimp_message_menu_warning *)
					&message->data);
			break;

		case message_MENUS_DELETED:
			ro_gui_menu_message_deleted((wimp_message_menus_deleted *)
					&message->data);
			break;

		case message_CLAIM_ENTITY:
			ro_gui_selection_claim_entity((wimp_full_message_claim_entity*)message);
			break;

		case message_DATA_REQUEST:
			ro_gui_selection_data_request((wimp_full_message_data_request*)message);
			break;

		case message_MODE_CHANGE:
			ro_gui_get_screen_properties();
			rufl_invalidate_cache();
			break;

		case message_PALETTE_CHANGE:
			break;

		case message_FONT_CHANGED:
			ro_gui_wimp_get_desktop_font();
			break;

		case message_URI_PROCESS:
			if (event != wimp_USER_MESSAGE_ACKNOWLEDGE)
				ro_uri_message_received(message);
			break;
		case message_URI_RETURN_RESULT:
			ro_uri_bounce(message);
			break;
		case message_INET_SUITE_OPEN_URL:
			if (event == wimp_USER_MESSAGE_ACKNOWLEDGE) {
				ro_url_bounce(message);
			}
			else {
				ro_url_message_received(message);
			}
			break;
		case message_PRINT_SAVE:
			if (event == wimp_USER_MESSAGE_ACKNOWLEDGE)
				ro_print_save_bounce(message);
			break;
		case message_PRINT_ERROR:
			ro_print_error(message);
			break;
		case message_PRINT_TYPE_ODD:
			ro_print_type_odd(message);
			break;
		case message_HOTLIST_CHANGED:
			ro_gui_hotlist_add_cleanup();
			break;
		case message_QUIT:
			riscos_done = true;
			break;
	}
}


/**
 * Process a Wimp_Poll event.
 *
 * \param event wimp event number
 * \param block parameter block
 */
static void ro_gui_handle_event(wimp_event_no event, wimp_block *block)
{
	switch (event) {
		case wimp_NULL_REASON_CODE:
			ro_gui_throb();
			ro_mouse_poll();
			break;

		case wimp_REDRAW_WINDOW_REQUEST:
			ro_gui_wimp_event_redraw_window(&block->redraw);
			break;

		case wimp_OPEN_WINDOW_REQUEST:
			ro_gui_open_window_request(&block->open);
			break;

		case wimp_CLOSE_WINDOW_REQUEST:
			ro_gui_close_window_request(&block->close);
			break;

		case wimp_POINTER_LEAVING_WINDOW:
			ro_mouse_pointer_leaving_window(&block->leaving);
			break;

		case wimp_POINTER_ENTERING_WINDOW:
			ro_gui_wimp_event_pointer_entering_window(&block->entering);
			break;

		case wimp_MOUSE_CLICK:
			ro_gui_wimp_event_mouse_click(&block->pointer);
			break;

		case wimp_USER_DRAG_BOX:
			ro_mouse_drag_end(&block->dragged);
			break;

		case wimp_KEY_PRESSED:
			ro_gui_keypress(&(block->key));
			break;

		case wimp_MENU_SELECTION:
			ro_gui_menu_selection(&(block->selection));
			break;

		/* Scroll requests fall back to a generic handler because we
		 * might get these events for any window from a scroll-wheel.
		 */

		case wimp_SCROLL_REQUEST:
			if (!ro_gui_wimp_event_scroll_window(&(block->scroll)))
				ro_gui_scroll(&(block->scroll));
			break;

		case wimp_USER_MESSAGE:
		case wimp_USER_MESSAGE_RECORDED:
		case wimp_USER_MESSAGE_ACKNOWLEDGE:
			ro_gui_user_message(event, &(block->message));
			break;
	}
}


/**
 * Poll the RISC OS wimp for events.
 */
static void riscos_poll(void)
{
	wimp_event_no event;
	wimp_block block;
	const wimp_poll_flags mask = wimp_MASK_LOSE | wimp_MASK_GAIN | wimp_SAVE_FP;
	os_t track_poll_offset;

	/* Poll wimp. */
	xhourglass_off();
	track_poll_offset = ro_mouse_poll_interval();
	if (sched_active || (track_poll_offset > 0)) {
		os_t t = os_read_monotonic_time();

		if (track_poll_offset > 0) {
			t += track_poll_offset;
		} else {
			t += 10;
		}

		if (sched_active && (sched_time - t) < 0) {
			t = sched_time;
		}

		event = wimp_poll_idle(mask, &block, t, 0);
	} else {
		event = wimp_poll(wimp_MASK_NULL | mask, &block, 0);
	}

	xhourglass_on();
	gui_last_poll = clock();
	ro_gui_handle_event(event, &block);

	/* Only run scheduled callbacks on a null poll
	 * We cannot do this in the null event handler, as that may be called
	 * from gui_multitask(). Scheduled callbacks must only be run from the
	 * top-level.
	 */
	if (event == wimp_NULL_REASON_CODE) {
		schedule_run();
	}

	ro_gui_window_update_boxes();
}


/**
 * Handle Open_Window_Request events.
 */
void ro_gui_open_window_request(wimp_open *open)
{
	os_error *error;

	if (ro_gui_wimp_event_open_window(open))
		return;

	error = xwimp_open_window(open);
	if (error) {
		NSLOG(netsurf, INFO, "xwimp_open_window: 0x%x: %s",
		      error->errnum, error->errmess);
		ro_warn_user("WimpError", error->errmess);
		return;
	}
}


/**
 * source bounce callback.
 */
static void ro_gui_view_source_bounce(wimp_message *message)
{
	char *filename;
	os_error *error;
	char command[256];

	/* run the file as text */
	filename = ((wimp_full_message_data_xfer *)message)->file_name;
	sprintf(command, "@RunType_FFF %s", filename);
	error = xwimp_start_task(command, 0);
	if (error) {
		NSLOG(netsurf, INFO, "xwimp_start_task failed: 0x%x: %s",
		      error->errnum, error->errmess);
		ro_warn_user("WimpError", error->errmess);
	}
}


/**
 * Send the source of a content to a text editor.
 */
void ro_gui_view_source(struct hlcache_handle *c)
{
	os_error *error;
	char *temp_name;
	wimp_full_message_data_xfer message;
	int objtype;
	bool done = false;

	const uint8_t *source_data;
	size_t source_size;

	if (!c) {
		ro_warn_user("MiscError", "No document source");
		return;
	}

	source_data = content_get_source_data(c, &source_size);

	if (!source_data) {
		ro_warn_user("MiscError", "No document source");
		return;
	}

	/* try to load local files directly. */
	if (netsurf_nsurl_to_path(hlcache_handle_get_url(c), &temp_name) == NSERROR_OK) {
		error = xosfile_read_no_path(temp_name, &objtype, 0, 0, 0, 0);
		if ((!error) && (objtype == osfile_IS_FILE)) {
			snprintf(message.file_name, 212, "%s", temp_name);
			message.file_name[211] = '\0';
			done = true;
		}
		free(temp_name);
	}
	if (!done) {
		/* We cannot release the requested filename until after it
		 * has finished being used. As we can't easily find out when
		 * this is, we simply don't bother releasing it and simply
		 * allow it to be re-used next time NetSurf is started. The
		 * memory overhead from doing this is under 1 byte per
		 * filename. */
		char *r;
		char full_name[256];
		const char *filename = filename_request();
		if (!filename) {
			ro_warn_user("NoMemory", 0);
			return;
		}

		snprintf(full_name, 256, "%s/%s", TEMP_FILENAME_PREFIX,
				filename);
		full_name[255] = '\0';
		r = __riscosify(full_name, 0, __RISCOSIFY_NO_SUFFIX,
				message.file_name, 212, 0);
		if (r == 0) {
			NSLOG(netsurf, INFO, "__riscosify failed");
			return;
		}
		message.file_name[211] = '\0';

		error = xosfile_save_stamped(message.file_name,
				ro_content_filetype(c),
				(byte *) source_data,
				(byte *) source_data + source_size);
		if (error) {
			NSLOG(netsurf, INFO,
			      "xosfile_save_stamped failed: 0x%x: %s",
			      error->errnum,
			      error->errmess);
			ro_warn_user("MiscError", error->errmess);
			return;
		}
	}

	/* begin the DataOpen protocol */
	message.your_ref = 0;
	message.size = 44 + ((strlen(message.file_name) + 4) & (~3u));
	message.action = message_DATA_OPEN;
	message.w = 0;
	message.i = 0;
	message.pos.x = 0;
	message.pos.y = 0;
	message.est_size = 0;
	message.file_type = 0xfff;
	ro_message_send_message(wimp_USER_MESSAGE_RECORDED,
			(wimp_message*)&message, 0,
			ro_gui_view_source_bounce);
}


/**
 * Broadcast an URL that we can't handle.
 */
static nserror gui_launch_url(struct nsurl *url)
{
	/* Try ant broadcast */
	ro_url_broadcast(nsurl_access(url));
	return NSERROR_OK;
}


/**
 * Choose the language to use.
 */
static void ro_gui_choose_language(void)
{
	/* if option_language exists and is valid, use that */
	if (nsoption_charp(language)) {
		char path[40];
		if (2 < strlen(nsoption_charp(language)))
			nsoption_charp(language)[2] = 0;
		sprintf(path, "NetSurf:Resources.%s", nsoption_charp(language));

		if (is_dir(path)) {
			nsoption_setnull_charp(accept_language,
					strdup(nsoption_charp(language)));
			return;
		}
		nsoption_set_charp(language, NULL);
	}

	nsoption_set_charp(language, strdup(ro_gui_default_language()));
	if (nsoption_charp(language) == NULL)
		die("Out of memory");
	nsoption_set_charp(accept_language, strdup(nsoption_charp(language)));
	if (nsoption_charp(accept_language) == NULL)
		die("Out of memory");
}


/**
 * Display a warning for a serious problem (eg memory exhaustion).
 *
 * \param  warning  message key for warning message
 * \param  detail   additional message, or 0
 */
nserror ro_warn_user(const char *warning, const char *detail)
{
	NSLOG(netsurf, INFO, "%s %s", warning, detail);

	if (dialog_warning) {
		char warn_buffer[300];
		snprintf(warn_buffer, sizeof warn_buffer, "%s %s",
				messages_get(warning),
				detail ? detail : "");
		warn_buffer[sizeof warn_buffer - 1] = 0;
		ro_gui_set_icon_string(dialog_warning, ICON_WARNING_MESSAGE,
				warn_buffer, true);
		xwimp_set_icon_state(dialog_warning, ICON_WARNING_HELP,
				wimp_ICON_DELETED, wimp_ICON_DELETED);
		ro_gui_dialog_open(dialog_warning);
		xos_bell();
	} else {
		/* probably haven't initialised (properly), use a
		   non-multitasking error box */
		os_error error;
		snprintf(error.errmess, sizeof error.errmess, "%s %s",
				messages_get(warning),
				detail ? detail : "");
		error.errmess[sizeof error.errmess - 1] = 0;
		xwimp_report_error_by_category(&error,
				wimp_ERROR_BOX_OK_ICON |
				wimp_ERROR_BOX_GIVEN_CATEGORY |
				wimp_ERROR_BOX_CATEGORY_ERROR <<
					wimp_ERROR_BOX_CATEGORY_SHIFT,
				"NetSurf", "!netsurf",
				(osspriteop_area *) 1, 0, 0);
	}
	
	return NSERROR_OK;
}


/**
 * Display an error and exit.
 *
 * Should only be used during initialisation.
 */
void die(const char * const error)
{
	os_error warn_error;

	NSLOG(netsurf, INFO, "%s", error);

	warn_error.errnum = 1; /* \todo: reasonable ? */
	strncpy(warn_error.errmess, messages_get(error),
			sizeof(warn_error.errmess)-1);
	warn_error.errmess[sizeof(warn_error.errmess)-1] = '\0';
	xwimp_report_error_by_category(&warn_error,
			wimp_ERROR_BOX_OK_ICON |
			wimp_ERROR_BOX_GIVEN_CATEGORY |
			wimp_ERROR_BOX_CATEGORY_ERROR <<
				wimp_ERROR_BOX_CATEGORY_SHIFT,
			"NetSurf", "!netsurf",
			(osspriteop_area *) 1, 0, 0);
	exit(EXIT_FAILURE);
}


/**
 * Test whether it's okay to shutdown, prompting the user if not.
 *
 * \return true iff it's okay to shutdown immediately
 */
bool ro_gui_prequit(void)
{
	return ro_gui_download_prequit();
}


/**
 * Generate a riscos path from one or more component elemnts.
 *
 * Constructs a complete path element from passed components. The
 * second (and subsequent) components have a slash substituted for all
 * riscos directory separators.
 *
 * If a string is allocated it must be freed by the caller.
 *
 * @param[in,out] str pointer to string pointer if this is NULL enough
 *                    storage will be allocated for the complete path.
 * @param[in,out] size The size of the space available if \a str not
 *                     NULL on input and if not NULL set to the total
 *                     output length on output.
 * @param[in] nelm The number of elements.
 * @param[in] ap The elements of the path as string pointers.
 * @return NSERROR_OK and the complete path is written to str
 *         or error code on faliure.
 */
static nserror riscos_mkpath(char **str, size_t *size, size_t nelm, va_list ap)
{
	const char *elm[16];
	size_t elm_len[16];
	size_t elm_idx;
	char *fname;
	size_t fname_len = 0;
	char *curp;
	size_t idx;

	/* check the parameters are all sensible */
	if ((nelm == 0) || (nelm > 16)) {
		return NSERROR_BAD_PARAMETER;
	}
	if ((*str != NULL) && (size == NULL)) {
		/* if the caller is providing the buffer they must say
		 * how much space is available.
		 */
		return NSERROR_BAD_PARAMETER;
	}

	/* calculate how much storage we need for the complete path
	 * with all the elements.
	 */
	for (elm_idx = 0; elm_idx < nelm; elm_idx++) {
		elm[elm_idx] = va_arg(ap, const char *);
		/* check the argument is not NULL */
		if (elm[elm_idx] == NULL) {
			return NSERROR_BAD_PARAMETER;
		}
		elm_len[elm_idx] = strlen(elm[elm_idx]);
		fname_len += elm_len[elm_idx];
	}
	fname_len += nelm; /* allow for separators and terminator */

	/* ensure there is enough space */
	fname = *str;
	if (fname != NULL) {
		if (fname_len > *size) {
			return NSERROR_NOSPACE;
		}
	} else {
		fname = malloc(fname_len);
		if (fname == NULL) {
			return NSERROR_NOMEM;
		}
	}

	/* copy the elements in with directory separator */
	curp = fname;

	/* first element is not altered */
	memmove(curp, elm[0], elm_len[0]);
	curp += elm_len[0];
	/* ensure there is a delimiter */
	if (curp[-1] != DIR_SEP) {
		*curp = DIR_SEP;
		curp++;
	}

	/* subsequent elemnts have slashes substituted with directory
	 * separators.
	 */
	for (elm_idx = 1; elm_idx < nelm; elm_idx++) {
		for (idx = 0; idx < elm_len[elm_idx]; idx++) {
			if (elm[elm_idx][idx] == DIR_SEP) {
				*curp = '/';
			} else {
				*curp = elm[elm_idx][idx];
			}
			curp++;
		}
		*curp = DIR_SEP;
		curp++;
	}
	curp[-1] = 0; /* NULL terminate */

	assert((curp - fname) <= (int)fname_len);

	*str = fname;
	if (size != NULL) {
		*size = fname_len;
	}

	return NSERROR_OK;

}


/**
 * Get the basename of a file using posix path handling.
 *
 * This gets the last element of a path and returns it. The returned
 * element has all forward slashes translated into riscos directory
 * separators.
 *
 * @param[in] path The path to extract the name from.
 * @param[in,out] str Pointer to string pointer if this is NULL enough
 *                    storage will be allocated for the path element.
 * @param[in,out] size The size of the space available if \a
 *                     str not NULL on input and set to the total
 *                     output length on output.
 * @return NSERROR_OK and the complete path is written to str
 *         or error code on faliure.
 */
static nserror riscos_basename(const char *path, char **str, size_t *size)
{
	const char *leafname;
	char *fname;
	char *temp;

	if (path == NULL) {
		return NSERROR_BAD_PARAMETER;
	}

	leafname = strrchr(path, DIR_SEP);
	if (!leafname) {
		leafname = path;
	} else {
		leafname += 1;
	}

	fname = strdup(leafname);
	if (fname == NULL) {
		return NSERROR_NOMEM;
	}

	/** @todo check this leafname translation is actually required */
	/* and s/\//\./g */
	for (temp = fname; *temp != 0; temp++) {
		if (*temp == '/') {
			*temp = DIR_SEP;
		}
	}

	*str = fname;
	if (size != NULL) {
		*size = strlen(fname);
	}
	return NSERROR_OK;
}


/**
 * Ensure that all directory elements needed to store a filename exist.
 *
 * Given a path of x.y.z directories x and x.y will be created.
 *
 * @param fname The filename to ensure the path to exists.
 * @return NSERROR_OK on success or error code on failure.
 */
static nserror riscos_mkdir_all(const char *fname)
{
	char *dname;
	char *cur;

	dname = strdup(fname);

	cur = dname;
	while ((cur = strchr(cur, '.'))) {
		*cur = '\0';
		xosfile_create_dir(dname, 0);
		*cur++ = '.';
	}

	free(dname);

	return NSERROR_OK;
}

/**
 * Find screen size in OS units.
 */
void ro_gui_screen_size(int *width, int *height)
{
	*width = screen_info.width;
	*height = screen_info.height;
}


/**
 * Send the debug dump of a content to a text editor.
 */
void ro_gui_dump_browser_window(struct browser_window *bw)
{
	os_error *error;

	/* open file for dump */
	FILE *stream = fopen("<Wimp$ScrapDir>.WWW.NetSurf.dump", "w");
	if (!stream) {
		NSLOG(netsurf, INFO, "fopen: errno %i", errno);
		ro_warn_user("SaveError", strerror(errno));
		return;
	}

	browser_window_debug_dump(bw, stream, CONTENT_DEBUG_RENDER);

	fclose(stream);

	/* launch file in editor */
	error = xwimp_start_task("Filer_Run <Wimp$ScrapDir>.WWW.NetSurf.dump",
			0);
	if (error) {
		NSLOG(netsurf, INFO, "xwimp_start_task failed: 0x%x: %s",
		      error->errnum, error->errmess);
		ro_warn_user("WimpError", error->errmess);
	}
}


static struct gui_file_table riscos_file_table = {
	.mkpath = riscos_mkpath,
	.basename = riscos_basename,
	.nsurl_to_path = ro_nsurl_to_path,
	.path_to_nsurl = ro_path_to_nsurl,
	.mkdir_all = riscos_mkdir_all,
};

static struct gui_fetch_table riscos_fetch_table = {
	.filetype = fetch_filetype,

	.get_resource_url = gui_get_resource_url,
	.mimetype = fetch_mimetype,
};

static struct gui_misc_table riscos_misc_table = {
	.schedule = riscos_schedule,

	.quit = gui_quit,
	.launch_url = gui_launch_url,
	.present_cookies = ro_gui_cookies_present,
};


static char *get_cachepath(void)
{
	char *cachedir;
	char *cachepath = NULL;
	nserror ret;

	cachedir = getenv("Cache$Dir");
	if ((cachedir == NULL) || (cachedir[0] == 0)) {
		NSLOG(netsurf, INFO, "cachedir was null");
		return NULL;
	}
	ret = netsurf_mkpath(&cachepath, NULL, 2, cachedir, "NetSurf");
	if (ret != NSERROR_OK) {
		return NULL;
	}
	return cachepath;
}

/**
 * Normal entry point from RISC OS.
 */
int main(int argc, char** argv)
{
	char *cachepath;
	char path[40];
	int length;
	os_var_type type;
	int used = -1;  /* slightly better with older OSLib versions */
	os_error *error;
	nserror ret;
	struct netsurf_table riscos_table = {
		.misc = &riscos_misc_table,
		.window = riscos_window_table,
		.clipboard = riscos_clipboard_table,
		.download = riscos_download_table,
		.fetch = &riscos_fetch_table,
		.file = &riscos_file_table,
		.utf8 = riscos_utf8_table,
		.search = riscos_search_table,
		.llcache = filesystem_llcache_table,
		.bitmap = riscos_bitmap_table,
		.layout = riscos_layout_table,
	};

	ret = netsurf_register(&riscos_table);
	if (ret != NSERROR_OK) {
		die("NetSurf operation table failed registration");
	}

	/* Consult NetSurf$Logging environment variable to decide if logging
	 * is required. */
	error = xos_read_var_val_size("NetSurf$Logging", 0, os_VARTYPE_STRING,
			&used, NULL, &type);
	if (error != NULL || type != os_VARTYPE_STRING || used != -2) {
		verbose_log = true;
	} else {
		char logging_env[2];
		error = xos_read_var_val("NetSurf$Logging", logging_env,
				sizeof(logging_env), 0, os_VARTYPE_STRING,
				&used, NULL, &type);
		if (error != NULL || logging_env[0] != '0') {
			verbose_log = true;
		} else {
			verbose_log = false;
		}
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
	nsoption_read("NetSurf:Choices", NULL);
	nsoption_commandline(&argc, argv, NULL);

	/* Choose the interface language to use */
	ro_gui_choose_language();

	/* select language-specific Messages */
	if (((length = snprintf(path,
				sizeof(path),
			       "NetSurf:Resources.%s.Messages",
				nsoption_charp(language))) < 0) ||
	    (length >= (int)sizeof(path))) {
		die("Failed to locate Messages resource.");
	}

	/* initialise messages */
	messages_add_from_file(path);

	/* obtain cache path */
	cachepath = get_cachepath();

	/* common initialisation */
	ret = netsurf_init(cachepath);
	free(cachepath);
	if (ret != NSERROR_OK) {
		die("NetSurf failed to initialise core");
	}

	artworks_init();
	draw_init();
	sprite_init();

	/* Load some extra RISC OS specific Messages */
	messages_add_from_file("NetSurf:Resources.LangNames");

	ret = gui_init(argc, argv);
	if (ret != NSERROR_OK) {
		ro_warn_user(messages_get_errorcode(ret), 0);
	}

	while (!riscos_done) {
		riscos_poll();
	}

	netsurf_exit();
	nsoption_finalise(nsoptions, nsoptions_default);

	/* finalise logging */
	nslog_finalise();

	return 0;
}
