/*
 * Copyright 2008-10 Chris Young <chris@unsatisfactorysoftware.co.uk>
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

/** \file
 * Fetching of data from a file (implementation).
 */

#include "amiga/os3support.h"

#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/openurl.h>

#include "amiga/launch.h"
#include "amiga/object.h" /* for list abstraction */
#include "utils/nsoption.h"
#include "utils/nsurl.h"

struct Library *OpenURLBase = NULL;
struct OpenURLIFace *IOpenURL = NULL;

static struct MinList *ami_unsupportedprotocols;

struct ami_protocol
{
	struct MinNode node;
	lwc_string *protocol;
};

static struct ami_protocol *ami_openurl_add_protocol(const char *url)
{
	nsurl *ns_url;
	struct ami_protocol *ami_p =
		(struct ami_protocol *)malloc(sizeof(struct ami_protocol));

	if (nsurl_create(url, &ns_url) != NSERROR_OK) {
		free(ami_p);
		return NULL;
	}

	ami_p->protocol = nsurl_get_component(ns_url, NSURL_SCHEME);
	nsurl_unref(ns_url);
	if (ami_p->protocol == NULL)
	{
		free(ami_p);
		return NULL;
	}

	AddTail((struct List *)ami_unsupportedprotocols, (struct Node *)ami_p);
	return ami_p;
}

static void ami_openurl_free_list(struct MinList *list)
{
	struct ami_protocol *node;
	struct ami_protocol *nnode;

	if(!IsMinListEmpty(list)) {
		node = (struct ami_protocol *)GetHead((struct List *)list);

		do
		{
			nnode=(struct ami_protocol *)GetSucc((struct Node *)node);

			Remove((struct Node *)node);
			if (node->protocol) lwc_string_unref(node->protocol);
			free(node);
			node = NULL;
		}while((node=nnode));
	}
	free(list);
}

static BOOL ami_openurl_check_list(struct MinList *list, nsurl *url)
{
	struct ami_protocol *node;
	struct ami_protocol *nnode;
	lwc_string *url_scheme;
	bool match;

	if(IsMinListEmpty(list)) return FALSE;

	url_scheme = nsurl_get_component(url, NSURL_SCHEME);

	node = (struct ami_protocol *)GetHead((struct List *)list);

	do
	{
		nnode=(struct ami_protocol *)GetSucc((struct Node *)node);

		if ((lwc_string_isequal(url_scheme, node->protocol,
				&match) == lwc_error_ok) && (match == true)) {
			lwc_string_unref(url_scheme);
			return TRUE;
		}
	}while((node=nnode));

	lwc_string_unref(url_scheme);
	return FALSE;
}

/**
 * Initialise the fetcher.
 *
 * Must be called once before any other function.
 */

void ami_openurl_open(void)
{
	if(nsoption_bool(use_openurl_lib)) {
		if((OpenURLBase = OpenLibrary("openurl.library",0))) {
#ifdef __amigaos4__
			IOpenURL = (struct OpenURLIFace *)GetInterface(OpenURLBase,"main",1,NULL);
#endif
		}
	}

	ami_unsupportedprotocols = ami_AllocMinList();
}

void ami_openurl_close(void)
{
#ifdef __amigaos4__
	if(IOpenURL) DropInterface((struct Interface *)IOpenURL);
#endif
	if(OpenURLBase) CloseLibrary(OpenURLBase);

	ami_openurl_free_list(ami_unsupportedprotocols);
}

nserror gui_launch_url(struct nsurl *url)
{
#ifdef __amigaos4__
	APTR procwin = SetProcWindow((APTR)-1L);
#endif
	char *launchurl = NULL;

	if(ami_openurl_check_list(ami_unsupportedprotocols, url) == FALSE)
	{
		if(IOpenURL)
		{
			URL_OpenA((STRPTR)url,NULL);
		} else {
			if((launchurl = ASPrintf("URL:%s", nsurl_access(url)))) {
				BPTR fptr = Open(launchurl,MODE_OLDFILE);
				if(fptr)
				{
					Close(fptr);
				} else {
					ami_openurl_add_protocol(nsurl_access(url));
				}
				FreeVec(launchurl);
			}
		}
	}
#ifdef __amigaos4__
	SetProcWindow(procwin);
#endif
	return NSERROR_OK;
}
