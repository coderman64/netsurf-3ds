/*
 * Copyright 2008-2010 Chris Young <chris@unsatisfactorysoftware.co.uk>
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

#include <string.h>

#include <proto/wb.h>
#include <proto/asl.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/icon.h>
#ifdef __amigaos4__
#include <proto/application.h>
#endif

#include <workbench/icon.h>

#include <proto/window.h>
#include <proto/layout.h>

#include <proto/fuelgauge.h>
#include <classes/window.h>
#include <gadgets/fuelgauge.h>
#include <gadgets/layout.h>

#include <reaction/reaction_macros.h>

#include "utils/errors.h"
#include "utils/nsurl.h"
#include "utils/log.h"
#include "utils/messages.h"
#include "utils/utils.h"
#include "utils/nsoption.h"
#include "utils/string.h"
#include "netsurf/browser_window.h"
#include "netsurf/mouse.h"
#include "netsurf/window.h"
#include "netsurf/download.h"
#include "content/handlers/image/ico.h"
#include "desktop/download.h"
#include "desktop/save_complete.h"

#include "amiga/gui.h"
#include "amiga/download.h"
#include "amiga/object.h"
#include "amiga/bitmap.h"
#include "amiga/icon.h"
#include "amiga/file.h"
#include "amiga/iff_dr2d.h"
#include "amiga/libs.h"
#include "amiga/misc.h"
#include "amiga/theme.h"
#include "amiga/utf8.h"

#ifndef APPNOTIFY_DisplayTime
#define APPNOTIFY_DisplayTime   ( TAG_USER + 13 )
#endif

#ifndef APPNOTIFY_Percentage
#define APPNOTIFY_Percentage    ( TAG_USER + 14 )
#endif

#ifndef APPNOTIFY_StopBackMsg
#define APPNOTIFY_StopBackMsg   ( TAG_USER + 17 )
#endif

enum {
	OID_D_MAIN = 0,
	GID_D_MAIN,
	GID_D_STATUS,
	GID_D_CANCEL,
	GID_D_LAST
};

struct gui_download_window {
	struct ami_generic_window w;
	struct Window *win;
	Object *objects[GID_D_LAST];
	BPTR fh;
	uint32 size;
	uint32 downloaded;
	uint32 progress;
	struct dlnode *dln;
	struct browser_window *bw;
	struct download_context *ctx;
	const char *url;
	char fname[1024];
	int result;
};

enum {
	AMINS_DLOAD_PROGRESS = 0,
	AMINS_DLOAD_OK,
	AMINS_DLOAD_ERROR,
	AMINS_DLOAD_ABORT,
};

static void ami_download_window_abort(void *w);
static BOOL ami_download_window_event(void *w);

static const struct ami_win_event_table ami_download_table = {
	ami_download_window_event,
	ami_download_window_abort,
};

static int downloads_in_progress = 0;

static struct gui_download_window *gui_download_window_create(download_context *ctx,
		struct gui_window *gui)
{
	const char *url = nsurl_access(download_context_get_url(ctx));
	unsigned long total_size = download_context_get_total_length(ctx);
	struct gui_download_window *dw;
	char *dl_filename = ami_utf8_easy(download_context_get_filename(ctx));
	APTR va[3];

	dw = calloc(1, sizeof(struct gui_download_window));

	if(gui && (!IsListEmpty(ami_gui_get_download_list(gui)) &&
		(dw->dln = (struct dlnode *)FindName(ami_gui_get_download_list(gui), url)))) {
		strcpy(dw->fname, dw->dln->filename);
		free(dw->dln->node.ln_Name);
		dw->dln->node.ln_Name = NULL;
	}
	else
	{
		if(AslRequestTags(savereq,
			ASLFR_Window, ami_gui_get_window(gui),
			ASLFR_SleepWindow, TRUE,
			ASLFR_TitleText, messages_get("NetSurf"),
			ASLFR_Screen, ami_gui_get_screen(),
			ASLFR_InitialFile, dl_filename,
			TAG_DONE))
		{
			strlcpy(dw->fname, savereq->fr_Drawer, 1024);
			AddPart((STRPTR)&dw->fname,savereq->fr_File,1024);
			if(!ami_download_check_overwrite(dw->fname, ami_gui_get_window(gui), total_size))
			{
				free(dw);
				return NULL;
			}
		}
		else
		{
			free(dw);
			return NULL;
		}
	}

	if(dl_filename) ami_utf8_free(dl_filename);
	dw->size = total_size;
	dw->downloaded = 0;
	if(gui) dw->bw = ami_gui_get_browser_window(gui);
	dw->url = url;

	va[0] = (APTR)dw->downloaded;
	va[1] = (APTR)dw->size;
	va[2] = 0;

	if(!(dw->fh = FOpen((STRPTR)&dw->fname,MODE_NEWFILE,0)))
	{
		free(dw);
		return NULL;
	}

	if((nsoption_bool(download_notify_progress) == true)) {
		char bkm[1030];
		snprintf(bkm, 1030, "STOP %p", dw);

		Notify(ami_gui_get_app_id(), APPNOTIFY_Title, messages_get("amiDownloading"),
				APPNOTIFY_PubScreenName, "FRONT",
				APPNOTIFY_Text, dw->fname,
				APPNOTIFY_DisplayTime, TRUE,     
				APPNOTIFY_Percentage, 0,
				APPNOTIFY_StopBackMsg, bkm,
				TAG_DONE);
	} else {
		dw->objects[OID_D_MAIN] = WindowObj,
      	    WA_ScreenTitle, ami_gui_get_screen_title(),
           	WA_Title, dw->url,
           	WA_Activate, TRUE,
           	WA_DepthGadget, TRUE,
           	WA_DragBar, TRUE,
           	WA_CloseGadget, FALSE,
           	WA_SizeGadget, TRUE,
			WA_PubScreen, ami_gui_get_screen(),
			WINDOW_SharedPort, ami_gui_get_shared_msgport(),
			WINDOW_UserData,dw,
			WINDOW_IconifyGadget, FALSE,
			WINDOW_LockHeight,TRUE,
         	WINDOW_Position, WPOS_CENTERSCREEN,
           	WINDOW_ParentGroup, dw->objects[GID_D_MAIN] = LayoutVObj,
				LAYOUT_AddChild, dw->objects[GID_D_STATUS] = FuelGaugeObj,
					GA_ID,GID_D_STATUS,
					GA_Text,messages_get("amiDownload"),
					FUELGAUGE_Min,0,
					FUELGAUGE_Max,total_size,
					FUELGAUGE_Level,0,
					FUELGAUGE_Ticks,11,
					FUELGAUGE_ShortTicks,TRUE,
					FUELGAUGE_VarArgs,va,
					FUELGAUGE_Percent,FALSE,
					FUELGAUGE_Justification,FGJ_CENTER,
				FuelGaugeEnd,
				CHILD_NominalSize,TRUE,
				CHILD_WeightedHeight,0,
				LAYOUT_AddChild, dw->objects[GID_D_CANCEL] = ButtonObj,
					GA_ID,GID_D_CANCEL,
					GA_RelVerify,TRUE,
					GA_Text,messages_get("Abort"),
					GA_TabCycle,TRUE,
				ButtonEnd,
			EndGroup,
		EndWindow;

		dw->win = (struct Window *)RA_OpenWindow(dw->objects[OID_D_MAIN]);
	}

	dw->ctx = ctx;
	dw->result = AMINS_DLOAD_PROGRESS;

	ami_gui_win_list_add(dw, AMINS_DLWINDOW, &ami_download_table);

	downloads_in_progress++;

	return dw;
}

static nserror gui_download_window_data(struct gui_download_window *dw, 
		const char *data, unsigned int size)
{
	APTR va[3];
	if(!dw) return NSERROR_SAVE_FAILED;

	FWrite(dw->fh,data,1,size);

	dw->downloaded = dw->downloaded + size;

	va[0] = (APTR)dw->downloaded;
	va[1] = (APTR)dw->size;
	va[2] = 0;

	if(dw->size) {
		if((nsoption_bool(download_notify_progress) == true) &&
			(((dw->downloaded * 100) / dw->size) > dw->progress)) {
			dw->progress = (uint32)((dw->downloaded * 100) / dw->size);
			Notify(ami_gui_get_app_id(),
					APPNOTIFY_Percentage, dw->progress,
					TAG_DONE);
		} else {
			RefreshSetGadgetAttrs((struct Gadget *)dw->objects[GID_D_STATUS], dw->win, NULL,
						FUELGAUGE_Level,   dw->downloaded,
						GA_Text,           messages_get("amiDownload"),
						FUELGAUGE_VarArgs, va,
						TAG_DONE);
		}
	}
	else
	{
		if((nsoption_bool(download_notify_progress) == true)) {
			/* unknown size, not entirely sure how to deal with this atm... */
			Notify(ami_gui_get_app_id(),
					APPNOTIFY_Percentage, 100,
					TAG_DONE);
		} else {
			RefreshSetGadgetAttrs((struct Gadget *)dw->objects[GID_D_STATUS], dw->win, NULL,
						FUELGAUGE_Level,   dw->downloaded,
						GA_Text,           messages_get("amiDownloadU"),
						FUELGAUGE_VarArgs, va,
						TAG_DONE);
		}
	}

	return NSERROR_OK;
}

static void gui_download_window_done(struct gui_download_window *dw)
{
	struct dlnode *dln,*dln2 = NULL;
	struct browser_window *bw;
	bool queuedl = false;

	if(!dw) return;
	bw = dw->bw;

	if(dw->result == AMINS_DLOAD_PROGRESS)
		dw->result = AMINS_DLOAD_OK;

	if((nsoption_bool(download_notify_progress) == true)) {
		Notify(ami_gui_get_app_id(),
				APPNOTIFY_Update, TRUE,
				TAG_DONE);
	}

	if((nsoption_bool(download_notify)) && (dw->result == AMINS_DLOAD_OK))
	{
		char bkm[1030];
		snprintf(bkm, 1030, "OPEN %s", dw->fname);

		Notify(ami_gui_get_app_id(), APPNOTIFY_Title, messages_get("amiDownloadComplete"),
				APPNOTIFY_PubScreenName, "FRONT",
				APPNOTIFY_BackMsg, bkm,
				APPNOTIFY_CloseOnDC, TRUE,
				APPNOTIFY_Text, dw->fname,
				TAG_DONE);
	}

	download_context_destroy(dw->ctx);

	if((dln = dw->dln))
	{
		dln2 = (struct dlnode *)GetSucc((struct Node *)dln);
		if((dln!=dln2) && (dln2)) queuedl = true;

		free(dln->filename);
		Remove((struct Node *)dln);
		free(dln);
	}

	FClose(dw->fh);
	SetComment(dw->fname, dw->url);

	downloads_in_progress--;

	if(dw->objects[OID_D_MAIN] != NULL) {
		DisposeObject(dw->objects[OID_D_MAIN]);
	}

	ami_gui_win_list_remove(dw);
	if(queuedl) {
		nsurl *url;
		if (nsurl_create(dln2->node.ln_Name, &url) != NSERROR_OK) {
			amiga_warn_user("NoMemory", 0);
		} else {
			browser_window_navigate(bw,
				url,
				NULL,
				BW_NAVIGATE_DOWNLOAD,
				NULL,
				NULL,
				NULL);
			nsurl_unref(url);
		}
	}
	ami_try_quit(); /* In case the only window open was this download */
}

static void gui_download_window_error(struct gui_download_window *dw,
		const char *error_msg)
{
	amiga_warn_user("Unwritten","");
	dw->result = AMINS_DLOAD_ERROR;
	gui_download_window_done(dw);
}

static void ami_download_window_abort(void *w)
{
	struct gui_download_window *dw = (struct gui_download_window *)w;
	download_context_abort(dw->ctx);
	dw->result = AMINS_DLOAD_ABORT;
	gui_download_window_done(dw);
}

static BOOL ami_download_window_event(void *w)
{
	/* return TRUE if window destroyed */
	struct gui_download_window *dw = (struct gui_download_window *)w;
	ULONG result;
	uint16 code;

	if(dw == NULL) return FALSE; /* We may not have a real window */

	while((result = RA_HandleInput(dw->objects[OID_D_MAIN], &code)) != WMHI_LASTMSG)
	{
       	switch(result & WMHI_CLASSMASK) // class
		{
			case WMHI_GADGETUP:
				switch(result & WMHI_GADGETMASK)
				{
					case GID_D_CANCEL:
						ami_download_window_abort(dw);
						return TRUE;
					break;
				}
			break;
		}
	}
	return FALSE;
}

void ami_free_download_list(struct List *dllist)
{
	struct dlnode *node;
	struct dlnode *nnode;

	if(!dllist) return;
	if(IsListEmpty(dllist)) return;

	node = (struct dlnode *)GetHead((struct List *)dllist);

	do
	{
		nnode=(struct dlnode *)GetSucc((struct Node *)node);
		free(node->node.ln_Name);
		free(node->filename);
		Remove((struct Node *)node);
		free((struct Node *)node);
	}while((node=nnode));
}

nserror
gui_window_save_link(struct gui_window *g, nsurl *url, const char *title)
{
	char fname[1024];
	STRPTR openurlstring,linkname;
	struct DiskObject *dobj = NULL;

	linkname = ASPrintf("Link_to_%s",FilePart(nsurl_access(url)));

	if(AslRequestTags(savereq,
		ASLFR_Window, ami_gui_get_window(g),
		ASLFR_SleepWindow, TRUE,
		ASLFR_TitleText,messages_get("NetSurf"),
		ASLFR_Screen, ami_gui_get_screen(),
		ASLFR_InitialFile, linkname,
		TAG_DONE))
	{
		strlcpy(fname, savereq->fr_Drawer, 1024);
		AddPart(fname,savereq->fr_File,1024);

		ami_set_pointer(ami_gui_get_gui_window_2(g), GUI_POINTER_WAIT, false);

		if(ami_download_check_overwrite(fname, ami_gui_get_window(g), 0))
		{
			BPTR fh;

			if((fh = FOpen(fname,MODE_NEWFILE,0)))
			{
				/* \todo Should be URLOpen on OS4.1 */
				openurlstring = ASPrintf("openurl \"%s\"\n",nsurl_access(url));
				FWrite(fh,openurlstring,1,strlen(openurlstring));
				FClose(fh);
				FreeVec(openurlstring);
				SetComment(fname, nsurl_access(url));

				dobj = GetIconTags(NULL,ICONGETA_GetDefaultName,"url",
									ICONGETA_GetDefaultType,WBPROJECT,
									TAG_DONE);		

				dobj->do_DefaultTool = "IconX";

				PutIconTags(fname,dobj,
							ICONPUTA_NotifyWorkbench,TRUE,
							TAG_DONE);

				FreeDiskObject(dobj);
			}
			FreeVec(linkname);
		}
		ami_reset_pointer(ami_gui_get_gui_window_2(g));
	}
	return NSERROR_OK;
}

BOOL ami_download_check_overwrite(const char *file, struct Window *win, ULONG size)
{
	/* Return TRUE if file can be (over-)written */
	int32 res = 0;
	BPTR lock = 0;
	char *overwritetext;

	if(nsoption_bool(ask_overwrite) == false) return TRUE;

	lock = Lock(file, ACCESS_READ);

	if(lock)
	{
		if(size) {
			BPTR fh;
			int64 oldsize = 0;

			if((fh = OpenFromLock(lock))) {
				oldsize = GetFileSize(fh);
				Close(fh);
			}
			overwritetext = ASPrintf("%s\n\n%s %s\n%s %s",
				messages_get("OverwriteFile"),
				messages_get("amiSizeExisting"), human_friendly_bytesize((ULONG)oldsize),
				messages_get("amiSizeNew"), human_friendly_bytesize(size));
		} else {
			UnLock(lock);
			overwritetext = ASPrintf(messages_get("OverwriteFile"));
		}

		res = amiga_warn_user_multi(overwritetext, "Replace", "DontReplace", win);
		FreeVec(overwritetext);
	}
	else return TRUE;

	if(res == 1) return TRUE;
		else return FALSE;
}

void ami_download_parse_backmsg(const char *backmsg)
{
	if((backmsg[0] == 'O') && (backmsg[1] == 'P') && (backmsg[2] == 'E') && (backmsg[3] == 'N')) {
		OpenWorkbenchObjectA((backmsg + 5), NULL);
	}
}

static struct gui_download_table download_table = {
	.create = gui_download_window_create,
	.data = gui_download_window_data,
	.error = gui_download_window_error,
	.done = gui_download_window_done,
};

struct gui_download_table *amiga_download_table = &download_table;
