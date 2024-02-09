/*
 * Copyright 2008-2010, 2016 Chris Young <chris@unsatisfactorysoftware.co.uk>
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

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <proto/intuition.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/clicktab.h>
#include <gadgets/clicktab.h>
#include <reaction/reaction_macros.h>

#include "utils/log.h"
#include "utils/nsoption.h"
#include "utils/nsurl.h"
#include "netsurf/content.h"
#include "netsurf/browser_window.h"
#include "desktop/version.h"

#include "amiga/arexx.h"
#include "amiga/gui.h"
#include "amiga/download.h"
#include "amiga/hotlist.h"
#include "amiga/libs.h"
#include "amiga/misc.h"
#include "amiga/theme.h"

#ifndef __amigaos4__
#include "amiga/memory.h"
#endif

extern const char * const verarexx;
extern const char * const wt_revid;

enum
{
	RX_OPEN=0,
	RX_QUIT,
	RX_TOFRONT,
	RX_GETURL,
	RX_GETTITLE,
	RX_VERSION,
	RX_SAVE,
	RX_PUBSCREEN,
	RX_BACK,
	RX_FORWARD,
	RX_HOME,
	RX_RELOAD,
	RX_WINDOWS,
	RX_ACTIVE,
	RX_CLOSE,
	RX_HOTLIST,
	RX_SLABSTATS,
	RX_EXEC
};

static Object *arexx_obj = NULL;
STATIC char result[100];

#ifdef __amigaos4__
#define RXHOOKF(func) static VOID func(struct ARexxCmd *cmd, struct RexxMsg *rxm __attribute__((unused)))
#else
#define RXHOOKF(func) static ASM VOID func(REG(a0, struct ARexxCmd* cmd), REG(a1, struct RexxMsg* msg))
#endif

RXHOOKF(rx_open);
RXHOOKF(rx_quit);
RXHOOKF(rx_tofront);
RXHOOKF(rx_geturl);
RXHOOKF(rx_gettitle);
RXHOOKF(rx_version);
RXHOOKF(rx_save);
RXHOOKF(rx_pubscreen);
RXHOOKF(rx_back);
RXHOOKF(rx_forward);
RXHOOKF(rx_home);
RXHOOKF(rx_reload);
RXHOOKF(rx_windows);
RXHOOKF(rx_active);
RXHOOKF(rx_close);
RXHOOKF(rx_hotlist);
RXHOOKF(rx_slabstats);
RXHOOKF(rx_exec);

STATIC struct ARexxCmd Commands[] =
{
	{"OPEN",RX_OPEN,rx_open,"URL/A,NEW=NEWWINDOW/S,NEWTAB/S,SAVEAS/K,W=WINDOW/K/N,T=TAB/K/N,ACTIVE/S", 		0, 	NULL, 	0, 	0, 	NULL },
	{"QUIT",RX_QUIT,rx_quit,NULL, 		0, 	NULL, 	0, 	0, 	NULL },
	{"TOFRONT",RX_TOFRONT,rx_tofront,NULL, 		0, 	NULL, 	0, 	0, 	NULL },
	{"GETURL",RX_GETURL,rx_geturl,	"W=WINDOW/K/N,T=TAB/K/N", 		0, 	NULL, 	0, 	0, 	NULL },
	{"GETTITLE",RX_GETTITLE,rx_gettitle,	"W=WINDOW/K/N,T=TAB/K/N", 		0, 	NULL, 	0, 	0, 	NULL },
	{"VERSION",RX_VERSION,rx_version,"VERSION/N,SVN=REVISION/N,RELEASE/S", 		0, 	NULL, 	0, 	0, 	NULL },
	{"SAVE",RX_SAVE,rx_save,"FILENAME/A,W=WINDOW/K/N,T=TAB/K/N", 		0, 	NULL, 	0, 	0, 	NULL },
	{"GETSCREENNAME",RX_PUBSCREEN,rx_pubscreen,NULL, 		0, 	NULL, 	0, 	0, 	NULL },
	{"BACK",	RX_BACK,	rx_back,	"W=WINDOW/K/N,T=TAB/K/N", 		0, 	NULL, 	0, 	0, 	NULL },
	{"FORWARD",	RX_FORWARD,	rx_forward,	"W=WINDOW/K/N,T=TAB/K/N", 		0, 	NULL, 	0, 	0, 	NULL },
	{"HOME",	RX_HOME,	rx_home,	"W=WINDOW/K/N,T=TAB/K/N", 		0, 	NULL, 	0, 	0, 	NULL },
	{"RELOAD",	RX_RELOAD,	rx_reload,	"FORCE/S,W=WINDOW/K/N,T=TAB/K/N", 		0, 	NULL, 	0, 	0, 	NULL },
	{"WINDOWS",	RX_WINDOWS,	rx_windows,	"W=WINDOW/K/N", 		0, 	NULL, 	0, 	0, 	NULL },
	{"ACTIVE",	RX_ACTIVE,	rx_active,	"T=TAB/S", 		0, 	NULL, 	0, 	0, 	NULL },
	{"CLOSE",	RX_CLOSE,	rx_close,	"W=WINDOW/K/N,T=TAB/K/N", 		0, 	NULL, 	0, 	0, 	NULL },
	{"HOTLIST",	RX_HOTLIST,	rx_hotlist,	"A=ACTION/A", 		0, 	NULL, 	0, 	0, 	NULL },
	{"SLABSTATS",	RX_SLABSTATS,	rx_slabstats,	"FILE", 		0, 	NULL, 	0, 	0, 	NULL },
	{"EXEC",	RX_EXEC,	rx_exec,	"W=WINDOW/K/N,T=TAB/K/N,COMMAND/A/F", 		0, 	NULL, 	0, 	0, 	NULL },
	{ NULL, 		0, 				NULL, 		NULL, 		0, 	NULL, 	0, 	0, 	NULL }
};

bool ami_arexx_init(ULONG *rxsig)
{
	if((arexx_obj = ARexxObj,
			AREXX_HostName,"NETSURF",
			AREXX_Commands,Commands,
			AREXX_NoSlot,TRUE,
			AREXX_ReplyHook,NULL,
			AREXX_DefExtension,"nsrx",
			End))
	{
		GetAttr(AREXX_SigMask, arexx_obj, rxsig);
		return true;
	}
	else
	{
/* Create a temporary ARexx port so we can send commands to the NetSurf which
 * is already running */
		arexx_obj = ARexxObj,
			AREXX_HostName,"NETSURF",
			AREXX_Commands,Commands,
			AREXX_NoSlot,FALSE,
			AREXX_ReplyHook,NULL,
			AREXX_DefExtension,"nsrx",
			End;

		return false;
	}
}

void ami_arexx_handle(void)
{
	RA_HandleRexx(arexx_obj);
}

static void ami_arexx_command(const char *cmd, const char *port)
{
	if(arexx_obj == NULL) return;
	IDoMethod(arexx_obj, AM_EXECUTE, cmd, port, NULL, NULL, NULL, NULL);
}

void ami_arexx_self(const char *cmd)
{
	ami_arexx_command(cmd, "NETSURF");
}

void ami_arexx_execute(char *script)
{
	char full_script_path[1025];
	BPTR lock;

	if((lock = Lock(script, ACCESS_READ))) {
		DevNameFromLock(lock, full_script_path, 1024, DN_FULLPATH);
		NSLOG(netsurf, INFO, "Executing script: %s", full_script_path);
		ami_arexx_command(full_script_path, NULL);
		UnLock(lock);
	}
}

void ami_arexx_cleanup(void)
{
	if(arexx_obj) DisposeObject(arexx_obj);
}

static struct gui_window *ami_find_tab_gwin(struct gui_window_2 *gwin, int tab)
{
	int tabs = 0;
	struct Node *ctab;
	struct Node *ntab;
	struct gui_window *gw;

	if((tab == 0) || (ami_gui2_get_tabs(gwin) == 0)) return ami_gui2_get_gui_window(gwin);

	ctab = GetHead(ami_gui2_get_tab_list(gwin));

	do
	{
		tabs++;
		ntab=GetSucc(ctab);
		GetClickTabNodeAttrs(ctab,
							TNA_UserData, &gw,
							TAG_DONE);
		if(tabs == tab) return gw;
	} while((ctab=ntab));

	return NULL;
}

static int ami_find_tab_bw(struct gui_window_2 *gwin, struct browser_window *bw)
{
	int tabs = 0;
	struct Node *ctab;
	struct Node *ntab;
	struct gui_window *tgw = NULL;

	if((bw == NULL) || (ami_gui2_get_tabs(gwin) == 0)) return 1;

	ctab = GetHead(ami_gui2_get_tab_list(gwin));

	do
	{
		tabs++;
		ntab=GetSucc(ctab);
		GetClickTabNodeAttrs(ctab,
							TNA_UserData, &tgw,
							TAG_DONE);
		if(ami_gui_get_browser_window(tgw) == bw) return tabs;
	} while((ctab=ntab));

	return 0;
}

static struct gui_window *ami_find_tab(int window, int tab)
{
	struct nsObject *node, *nnode;
	struct MinList *window_list = ami_gui_get_window_list();

	if(!IsMinListEmpty(window_list))
	{
		int windows = 0;

		node = (struct nsObject *)GetHead((struct List *)window_list);

		do
		{
			nnode=(struct nsObject *)GetSucc((struct Node *)node);

			if(node->Type == AMINS_WINDOW)
			{
				windows++;
				if(windows == window)
					return ami_find_tab_gwin(node->objstruct, tab);
			}
		} while((node = nnode));
	}
	return NULL;
}

RXHOOKF(rx_open)
{
	struct dlnode *dln;
	struct gui_window *gw = ami_gui_get_active_gw();
	nsurl *url;

	cmd->ac_RC = 0;

	if((cmd->ac_ArgList[4]) && (cmd->ac_ArgList[5]))
		gw = ami_find_tab(*(ULONG *)cmd->ac_ArgList[4], *(ULONG *)cmd->ac_ArgList[5]);

	if (nsurl_create((char *)cmd->ac_ArgList[0], &url) != NSERROR_OK) {
		amiga_warn_user("NoMemory", 0);
		return;
	}

	if(cmd->ac_ArgList[3])
	{
		if(!gw) return;

		dln = calloc(1, sizeof(struct dlnode));
		dln->filename = strdup((char *)cmd->ac_ArgList[3]);
		dln->node.ln_Name = strdup((char *)cmd->ac_ArgList[0]);
		dln->node.ln_Type = NT_USER;
		AddTail(ami_gui_get_download_list(gw), (struct Node *)dln);
		browser_window_navigate(ami_gui_get_browser_window(gw),
				url,
				NULL,
				BW_NAVIGATE_DOWNLOAD,
				NULL,
				NULL,
				NULL);
	}
	else if(cmd->ac_ArgList[2])
	{
		if(gw == NULL) {
			browser_window_create(BW_CREATE_HISTORY,
				      url,
				      NULL,
				      NULL,
				      NULL);
		} else {
			browser_window_create(BW_CREATE_HISTORY |
				      BW_CREATE_TAB,
				      url,
				      NULL,
				      ami_gui_get_browser_window(gw),
				      NULL);

			if(cmd->ac_ArgList[6]) {
				ami_gui_switch_to_new_tab(ami_gui_get_gui_window_2(gw));
			}
		}
	}
	else if(cmd->ac_ArgList[1])
	{
		browser_window_create(BW_CREATE_HISTORY,
				      url,
				      NULL,
				      NULL,
				      NULL);
	}
	else
	{
		if(gw)
		{
			browser_window_navigate(ami_gui_get_browser_window(gw),
					url,
					NULL,
					BW_NAVIGATE_HISTORY,
					NULL,
					NULL,
					NULL);
		}
		else
		{
			browser_window_create(BW_CREATE_HISTORY,
					      url,
					      NULL,
					      NULL,
					      NULL);
		}
	}
	nsurl_unref(url);
}

RXHOOKF(rx_save)
{
	BPTR fh = 0;
	struct gui_window *gw = ami_gui_get_active_gw();

	cmd->ac_RC = 0;

	if((cmd->ac_ArgList[1]) && (cmd->ac_ArgList[2]))
		gw = ami_find_tab(*(ULONG *)cmd->ac_ArgList[1], *(ULONG *)cmd->ac_ArgList[2]);

	if(!gw) return;

	ami_set_pointer(ami_gui_get_gui_window_2(gw), GUI_POINTER_WAIT, false);
					
	if((fh = FOpen((char *)cmd->ac_ArgList[0], MODE_NEWFILE, 0)))
	{
		const uint8_t *source_data;
		size_t source_size;
		struct hlcache_handle *h = browser_window_get_content(ami_gui_get_browser_window(gw));
		source_data = content_get_source_data(h, &source_size);
		if (source_data != NULL) {
			FWrite(fh, source_data, 1, source_size);
		}

		FClose(fh);
		SetComment((char *)cmd->ac_ArgList[0], nsurl_access(browser_window_access_url(ami_gui_get_browser_window(gw))));
	}

	ami_reset_pointer(ami_gui_get_gui_window_2(gw));
}

RXHOOKF(rx_quit)
{
	cmd->ac_RC = 0;
	ami_quit_netsurf();
}

RXHOOKF(rx_tofront)
{
	cmd->ac_RC = 0;
	ScreenToFront(ami_gui_get_screen());
}

RXHOOKF(rx_geturl)
{
	struct gui_window *gw = ami_gui_get_active_gw();

	cmd->ac_RC = 0;

	if((cmd->ac_ArgList[0]) && (cmd->ac_ArgList[1]))
		gw = ami_find_tab(*(ULONG *)cmd->ac_ArgList[0], *(ULONG *)cmd->ac_ArgList[1]);

	if(gw && ami_gui_get_browser_window(gw))
	{
		strcpy(result, nsurl_access(browser_window_access_url(ami_gui_get_browser_window(gw))));
	}
	else
	{
		strcpy(result,"");
	}

	cmd->ac_Result = result;
}

RXHOOKF(rx_gettitle)
{
	struct gui_window *gw = ami_gui_get_active_gw();

	cmd->ac_RC = 0;

	if((cmd->ac_ArgList[0]) && (cmd->ac_ArgList[1]))
		gw = ami_find_tab(*(ULONG *)cmd->ac_ArgList[0], *(ULONG *)cmd->ac_ArgList[1]);

	if(gw)
	{
		if(ami_gui2_get_tabs(ami_gui_get_gui_window_2(gw)) > 1)
			strcpy(result, ami_gui_get_tab_title(gw));
		else
			strcpy(result, ami_gui_get_win_title(gw));
	}
	else
	{
		strcpy(result,"");
	}

	cmd->ac_Result = result;
}

RXHOOKF(rx_version)
{
	cmd->ac_RC = 0;

	if(cmd->ac_ArgList[2])
	{
		if(cmd->ac_ArgList[1])
		{
			if((netsurf_version_major > *(int *)cmd->ac_ArgList[0]) || ((netsurf_version_minor >= *(int *)cmd->ac_ArgList[1]) && (netsurf_version_major == *(int *)cmd->ac_ArgList[0])))
			{
				strcpy(result,"1");
			}
			else
			{
				strcpy(result,"0");
			}
		}
		else if(cmd->ac_ArgList[0])
		{
			if((netsurf_version_major >= *(int *)cmd->ac_ArgList[0]))
			{
				strcpy(result,"1");
			}
			else
			{
				strcpy(result,"0");
			}
		}
		else
		{
			strcpy(result,netsurf_version);
		}
	}
	else
	{
		if(cmd->ac_ArgList[1])
		{
			if((netsurf_version_major > *(int *)cmd->ac_ArgList[0]) || ((atoi(wt_revid) >= *(int *)cmd->ac_ArgList[1]) && (netsurf_version_major == *(int *)cmd->ac_ArgList[0])))
			{
				strcpy(result,"1");
			}
			else
			{
				strcpy(result,"0");
			}
		}
		else if(cmd->ac_ArgList[0])
		{
			if((netsurf_version_major >= *(int *)cmd->ac_ArgList[0]))
			{
				strcpy(result,"1");
			}
			else
			{
				strcpy(result,"0");
			}
		}
		else
		{
			strcpy(result,verarexx);
		}
	}

	cmd->ac_Result = result;
}

RXHOOKF(rx_pubscreen)
{
	cmd->ac_RC = 0;

	if(nsoption_charp(pubscreen_name) == NULL)
	{
		strcpy(result,"NetSurf");
	}
	else
	{
		strcpy(result, nsoption_charp(pubscreen_name));
	}

	cmd->ac_Result = result;
}

RXHOOKF(rx_back)
{
	struct gui_window *gw = ami_gui_get_active_gw();

	cmd->ac_RC = 0;

	if((cmd->ac_ArgList[0]) && (cmd->ac_ArgList[1]))
		gw = ami_find_tab(*(ULONG *)cmd->ac_ArgList[0], *(ULONG *)cmd->ac_ArgList[1]);

	if(gw) ami_gui_history(ami_gui_get_gui_window_2(gw), true);
}

RXHOOKF(rx_forward)
{
	struct gui_window *gw = ami_gui_get_active_gw();

	cmd->ac_RC = 0;

	if((cmd->ac_ArgList[0]) && (cmd->ac_ArgList[1]))
		gw = ami_find_tab(*(ULONG *)cmd->ac_ArgList[0], *(ULONG *)cmd->ac_ArgList[1]);

	if(gw) ami_gui_history(ami_gui_get_gui_window_2(gw), false);

}

RXHOOKF(rx_home)
{
	struct gui_window *gw = ami_gui_get_active_gw();
	nsurl *url;

	cmd->ac_RC = 0;

	if((cmd->ac_ArgList[0]) && (cmd->ac_ArgList[1]))
		gw = ami_find_tab(*(ULONG *)cmd->ac_ArgList[0], *(ULONG *)cmd->ac_ArgList[1]);

	if(gw == NULL) return;

	if (nsurl_create(nsoption_charp(homepage_url), &url) != NSERROR_OK) {
		amiga_warn_user("NoMemory", 0);
	} else {
		browser_window_navigate(ami_gui_get_browser_window(gw),
					url,
					NULL,
					BW_NAVIGATE_HISTORY,
					NULL,
					NULL,
					NULL);
		nsurl_unref(url);
	}
}

RXHOOKF(rx_reload)
{
	struct gui_window *gw = ami_gui_get_active_gw();

	cmd->ac_RC = 0;

	if((cmd->ac_ArgList[1]) && (cmd->ac_ArgList[2]))
		gw = ami_find_tab(*(ULONG *)cmd->ac_ArgList[1], *(ULONG *)cmd->ac_ArgList[2]);

	if(gw)
	{
		if(cmd->ac_ArgList[0]) /* FORCE */
		{
			browser_window_reload(ami_gui_get_browser_window(gw), true);
		}
		else
		{
			browser_window_reload(ami_gui_get_browser_window(gw), false);
		}
	}
}

RXHOOKF(rx_windows)
{
	int windows = 0, tabs = 0;
	int window = 0;

	if(cmd->ac_ArgList[0]) window = *(ULONG *)cmd->ac_ArgList[0];
	cmd->ac_RC = 0;

	windows = ami_gui_count_windows(window, &tabs);

	if(cmd->ac_ArgList[0]) sprintf(result, "%d", tabs);
		else sprintf(result, "%d", windows);
	cmd->ac_Result = result;
}

RXHOOKF(rx_active)
{
	int window = 0, tab = 0;
	struct gui_window *gw = ami_gui_get_active_gw();
	struct nsObject *node, *nnode;
	struct gui_window_2 *gwin = NULL;
	struct MinList *window_list = ami_gui_get_window_list();

	cmd->ac_RC = 0;

	if(!IsMinListEmpty(window_list))
	{
		int windows = 0;

		node = (struct nsObject *)GetHead((struct List *)window_list);

		do
		{
			nnode=(struct nsObject *)GetSucc((struct Node *)node);

			gwin = node->objstruct;

			if(node->Type == AMINS_WINDOW)
			{
				windows++;
				if(IS_CURRENT_GW(gwin,gw))
				{
					window = windows;
					break;
				}
			}
		} while((node = nnode));
	}

	if(cmd->ac_ArgList[0])
	{
		tab = ami_find_tab_bw(gwin, ami_gui_get_browser_window(gw));
	}

	if(cmd->ac_ArgList[0]) sprintf(result, "%d", tab);
		else sprintf(result, "%d", window);
	cmd->ac_Result = result;
}

RXHOOKF(rx_close)
{
	struct gui_window *gw = ami_gui_get_active_gw();

	cmd->ac_RC = 0;

	if((cmd->ac_ArgList[0]) && (cmd->ac_ArgList[1]))
		gw = ami_find_tab(*(ULONG *)cmd->ac_ArgList[0], *(ULONG *)cmd->ac_ArgList[1]);
	else if(cmd->ac_ArgList[0])
	{
		ami_gui_close_window(ami_gui_get_gui_window_2(gw));
		return;
	}

	if(gw) browser_window_destroy(ami_gui_get_browser_window(gw));
}

RXHOOKF(rx_hotlist)
{
	cmd->ac_RC = 0;

	if(strcasecmp((char *)cmd->ac_ArgList[0], "OPEN") == 0) {
		ami_hotlist_present();
	} else if(strcasecmp((char *)cmd->ac_ArgList[0], "CLOSE") == 0) {
		ami_hotlist_close();
	}
}

RXHOOKF(rx_exec)
{
	struct gui_window *gw = ami_gui_get_active_gw();
	bool res = false;

	if(nsoption_bool(arexx_allow_exec) == false) {
		cmd->ac_RC = RETURN_FAIL;
		return;
	}

	if((cmd->ac_ArgList[0]) && (cmd->ac_ArgList[1]))
		gw = ami_find_tab(*(ULONG *)cmd->ac_ArgList[0], *(ULONG *)cmd->ac_ArgList[1]);

	if(gw) {
		NSLOG(netsurf, WARNING, "Executing js: %s", (char *)cmd->ac_ArgList[2]);
		res = browser_window_exec(ami_gui_get_browser_window(gw),
			(char *)cmd->ac_ArgList[2], strlen((char *)cmd->ac_ArgList[2]));
	}

	if(res == false) {
		cmd->ac_RC = RETURN_ERROR;
	} else {
		cmd->ac_RC = RETURN_OK;
	}

}

RXHOOKF(rx_slabstats)
{
#ifndef __amigaos4__
	BPTR fh = 0;

	if(cmd->ac_ArgList[0] != NULL) {
		fh = Open((char *)cmd->ac_ArgList[0], MODE_NEWFILE);
	}
	ami_memory_slab_dump(fh);

	if(fh != 0) Close(fh);
#endif
}

