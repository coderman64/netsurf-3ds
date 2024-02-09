/*
 * Copyright 2005, 2008, 2016 Chris Young <chris@unsatisfactorysoftware.co.uk>
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

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <proto/exec.h>
#include <exec/lists.h>
#include <exec/nodes.h>

#include "amiga/memory.h"
#include "amiga/object.h"

#ifdef __amigaos4__
#define nsList MinList
#define NewnsList NewMinList
#else
#define nsList List
#define NewnsList NewList
#endif

static APTR pool_nsobj = NULL;

bool ami_object_init(void)
{
	pool_nsobj = ami_memory_itempool_create(sizeof(struct nsObject));

	if(pool_nsobj == NULL) return false;
		else return true;
}

void ami_object_fini(void)
{
	ami_memory_itempool_delete(pool_nsobj);
}

/* Slightly abstract MinList initialisation */
static void ami_NewMinList(struct MinList *list)
{
	if(list == NULL) return;
	NewnsList((struct nsList *)list);
}

/* Allocate and initialise a new MinList */
struct MinList *ami_AllocMinList(void)
{
	struct MinList *objlist = (struct MinList *)malloc(sizeof(struct nsList));
	if(objlist == NULL) return NULL;
	ami_NewMinList(objlist);
	return objlist;
}

struct MinList *NewObjList(void)
{
	struct MinList *objlist = ami_AllocMinList();
	return(objlist);
}

struct nsObject *AddObject(struct MinList *objlist, ULONG otype)
{
	struct nsObject *dtzo;

	dtzo = (struct nsObject *)ami_memory_itempool_alloc(pool_nsobj, sizeof(struct nsObject));
	if(dtzo == NULL) return NULL;

	memset(dtzo, 0, sizeof(struct nsObject));
	AddTail((struct List *)objlist,(struct Node *)dtzo);

	dtzo->Type = otype;

	return(dtzo);
}

void ObjectCallback(struct nsObject *dtzo, void (*callback)(void *nso))
{
	dtzo->callback = callback;
}

static void DelObjectInternal(struct nsObject *dtzo, BOOL free_obj)
{
	Remove((struct Node *)dtzo);
	if(dtzo->callback != NULL) dtzo->callback(dtzo->objstruct);
	if(dtzo->objstruct && free_obj) free(dtzo->objstruct);
	if(dtzo->dtz_Node.ln_Name) free(dtzo->dtz_Node.ln_Name);
	ami_memory_itempool_free(pool_nsobj, dtzo, sizeof(struct nsObject));
	dtzo = NULL;
}

void DelObject(struct nsObject *dtzo)
{
	DelObjectInternal(dtzo, TRUE);
}

void DelObjectNoFree(struct nsObject *dtzo)
{
	DelObjectInternal(dtzo, FALSE);
}

void FreeObjList(struct MinList *objlist)
{
	struct nsObject *node;
	struct nsObject *nnode;

	if(IsMinListEmpty((struct MinList *)objlist) == FALSE) {
		node = (struct nsObject *)GetHead((struct List *)objlist);

		do {
			nnode = (struct nsObject *)GetSucc((struct Node *)node);
			if(node->Type == AMINS_RECT) {
				DelObjectNoFree(node);
			} else {
				DelObject(node);
			}
		} while((node = nnode));
	}
	free(objlist);
}

