/*
 * Copyright 2015 Chris Young <chris@unsatisfactorysoftware.co.uk>
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

#include <proto/timer.h>
#include <proto/utility.h>

#include "utils/log.h"

#include "amiga/font.h"
#include "amiga/font_bullet.h"
#include "amiga/font_cache.h"
#include "amiga/schedule.h"

#ifdef __amigaos4__
#include "amiga/hash/xxhash.h"
#else
#include "amiga/object.h"
#endif

#ifdef __amigaos4__
static struct SkipList *ami_font_cache_list = NULL;
static struct Hook ami_font_cache_hook;
#else
static struct MinList *ami_font_cache_list = NULL;
#endif



#ifdef __amigaos4__
static LONG ami_font_cache_sort(struct Hook *hook, APTR key1, APTR key2)
{
	if(key1 == key2) return 0;
	if(key1 < key2) return -1;
	return 1;
}
#endif

#ifdef __amigaos4__
static void ami_font_cache_cleanup(struct SkipList *skiplist)
{
	struct ami_font_cache_node *node;
	struct ami_font_cache_node *nnode;
	struct TimeVal curtime;

	node = (struct ami_font_cache_node *)GetFirstSkipNode(skiplist);
	if(node == NULL) return;

	do {
		nnode = (struct ami_font_cache_node *)GetNextSkipNode(skiplist, (struct SkipNode *)node);
		GetSysTime(&curtime);
		SubTime(&curtime, &node->lastused);
		if(curtime.Seconds > 300)
		{
			NSLOG(netsurf, INFO,
			      "Freeing font %p not used for %ld seconds",
			      node->skip_node.sn_Key,
			      curtime.Seconds);
			ami_font_bullet_close(node);
			RemoveSkipNode(skiplist, node->skip_node.sn_Key);
		}
	} while((node = nnode));

	/* reschedule to run in five minutes */
	ami_schedule(300000, (void *)ami_font_cache_cleanup, ami_font_cache_list);
}
#else
static void ami_font_cache_cleanup(struct MinList *ami_font_cache_list)
{
	struct nsObject *node;
	struct nsObject *nnode;
	struct ami_font_cache_node *fnode;
	struct TimeVal curtime;

	if(IsMinListEmpty(ami_font_cache_list)) return;

	node = (struct nsObject *)GetHead((struct List *)ami_font_cache_list);

	do
	{
		nnode=(struct nsObject *)GetSucc((struct Node *)node);
		fnode = node->objstruct;
		GetSysTime(&curtime);
		SubTime(&curtime, &fnode->lastused);
		if(curtime.Seconds > 300)
		{
			NSLOG(netsurf, INFO,
			      "Freeing %s not used for %ld seconds",
			      node->dtz_Node.ln_Name,
			      curtime.Seconds);
			DelObject(node);
		}
	} while((node=nnode));

	/* reschedule to run in five minutes */
	ami_schedule(300000, (void *)ami_font_cache_cleanup, ami_font_cache_list);
}
#endif

#ifdef __amigaos4__
static void ami_font_cache_del_skiplist(struct SkipList *skiplist)
{
	struct SkipNode *node;
	struct SkipNode *nnode;

	node = GetFirstSkipNode(skiplist);
	if(node == NULL) return;

	do {
		nnode = GetNextSkipNode(skiplist, node);
		ami_font_bullet_close((struct ami_font_cache_node *)node);
		
	} while((node = nnode));

	DeleteSkipList(skiplist);
}
#endif


struct ami_font_cache_node *ami_font_cache_locate(const char *font)
{
	struct ami_font_cache_node *nodedata = NULL;
	uint32 hash = 0;

#ifdef __amigaos4__
	hash = XXH32(font, strlen(font), 0);
	nodedata = (struct ami_font_cache_node *)FindSkipNode(ami_font_cache_list, (APTR)hash);		
#else
	struct nsObject *node = (struct nsObject *)FindIName((struct List *)ami_font_cache_list, font);
	if(node) nodedata = node->objstruct;
#endif

	if(nodedata) {
		GetSysTime(&nodedata->lastused);
		return nodedata;
	}

	NSLOG(netsurf, INFO, "Font cache miss: %s (%lx)", font, hash);
	return NULL;
}

struct ami_font_cache_node *ami_font_cache_alloc_entry(const char *font)
{
	struct ami_font_cache_node *nodedata;

#ifdef __amigaos4__
	uint32 hash = XXH32(font, strlen(font), 0);
	nodedata = (struct ami_font_cache_node *)InsertSkipNode(ami_font_cache_list, (APTR)hash, sizeof(struct ami_font_cache_node));
#else
	nodedata = malloc(sizeof(struct ami_font_cache_node));
#endif

	GetSysTime(&nodedata->lastused);

	return nodedata;
}

void ami_font_cache_insert(struct ami_font_cache_node *nodedata, const char *font)
{
#ifndef __amigaos4__
	struct nsObject *node = AddObject(ami_font_cache_list, AMINS_FONT);
	if(node) {
		ObjectCallback(node, ami_font_bullet_close);
		node->objstruct = nodedata;
		node->dtz_Node.ln_Name = strdup(font);
	}
#endif
}

void ami_font_cache_fini(void)
{
	NSLOG(netsurf, INFO, "Cleaning up font cache");
	ami_schedule(-1, (void *)ami_font_cache_cleanup, ami_font_cache_list);
#ifdef __amigaos4__
	ami_font_cache_del_skiplist(ami_font_cache_list);
#else
	FreeObjList(ami_font_cache_list);
#endif
	ami_font_cache_list = NULL;
}

void ami_font_cache_init(void)
{
#ifdef __amigaos4__
	ami_font_cache_hook.h_Entry = (HOOKFUNC)ami_font_cache_sort;
	ami_font_cache_hook.h_Data = 0;
	ami_font_cache_list = CreateSkipList(&ami_font_cache_hook, 8);
#else
	ami_font_cache_list = NewObjList();
#endif

	/* run first cleanup in ten minutes */
	ami_schedule(600000, (void *)ami_font_cache_cleanup, ami_font_cache_list);
}

