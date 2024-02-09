/*
 * Copyright 2016 Chris Young <chris@unsatisfactorysoftware.co.uk>
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

#ifndef __amigaos4__
#include <proto/dos.h>
#include <proto/exec.h>
#include <exec/interrupts.h>
#include <stdlib.h>
#include "amiga/memory.h"
#include "amiga/os3support.h"
#include "amiga/schedule.h"
#include "content/llcache.h"
#include "utils/log.h"

ULONG __slab_max_size = 2048; /* Enable clib2's slab allocator */

enum {
	PURGE_NONE = 0,
	PURGE_STEP1,
	PURGE_STEP2,
	PURGE_DONE_STEP1,
	PURGE_DONE_STEP2
};
static int low_mem_status = PURGE_NONE;

/* Special clear (ie. non-zero) */
void *ami_memory_clear_alloc(size_t size, UBYTE value)
{
	void *mem = malloc(size);
	if (mem) memset(mem, value, size);
	return mem;
}

/* clib2 slab allocator stats */
static int ami_memory_slab_usage_cb(const struct __slab_usage_information * sui)
{
	if(sui->sui_slab_index <= 1) {
		NSLOG(netsurf, INFO, "clib2 slab usage:");
		NSLOG(netsurf, INFO,
		      "  The size of all slabs, in bytes: %ld",
		      sui->sui_slab_size);
		NSLOG(netsurf, INFO,
		      "  Number of allocations which are not managed by slabs: %ld",
		      sui->sui_num_single_allocations);
		NSLOG(netsurf, INFO,
		      "  Total number of bytes allocated for memory not managed by slabs: %ld",
		      sui->sui_total_single_allocation_size);
		NSLOG(netsurf, INFO,
		      "  Number of slabs currently in play: %ld",
		      sui->sui_num_slabs);
		NSLOG(netsurf, INFO,
		      "  Number of currently unused slabs: %ld",
		      sui->sui_num_empty_slabs);
		NSLOG(netsurf, INFO,
		      "  Number of slabs in use which are completely filled with data: %ld",
		      sui->sui_num_full_slabs);
		NSLOG(netsurf, INFO,
		      "  Total number of bytes allocated for all slabs: %ld",
		      sui->sui_total_slab_allocation_size);
	}
	NSLOG(netsurf, INFO, "Slab %d", sui->sui_slab_index);
	NSLOG(netsurf, INFO, "  Memory chunk size managed by this slab: %ld",
	      sui->sui_chunk_size);
	NSLOG(netsurf, INFO,
	      "  Number of memory chunks that fit in this slab: %ld",
	      sui->sui_num_chunks);
	NSLOG(netsurf, INFO,
	      "  Number of memory chunks used in this slab: %ld",
	      sui->sui_num_chunks_used);

	return 0;
}

static int ami_memory_slab_alloc_cb(const struct __slab_allocation_information *sai)
{
	if(sai->sai_allocation_index <= 1) {
		NSLOG(netsurf, INFO, "clib2 allocation usage:");
		NSLOG(netsurf, INFO,
		      "  Number of allocations which are not managed by slabs: %ld",
		      sai->sai_num_single_allocations);
		NSLOG(netsurf, INFO,
		      "  Total number of bytes allocated for memory not managed by slabs: %ld",
		      sai->sai_total_single_allocation_size);
	}
	NSLOG(netsurf, INFO, "Alloc %d", sai->sai_allocation_index);
	NSLOG(netsurf, INFO, "  Size of this allocation, as requested: %ld",
	      sai->sai_allocation_size);
	NSLOG(netsurf, INFO,
	      "  Total size of this allocation, including management data: %ld",
	      sai->sai_total_allocation_size);

	return 0;
}

static int ami_memory_slab_stats_cb(void *user_data, const char *line, size_t line_length)
{
	BPTR fh = (BPTR)user_data;
	long err = FPuts(fh, line);

	if(err != 0) {
		return -1;
	} else {
		return 0;
	}
}

void ami_memory_slab_dump(BPTR fh)
{
	__get_slab_usage(ami_memory_slab_usage_cb);
	__get_slab_allocations(ami_memory_slab_alloc_cb);
	__get_slab_stats(fh, ami_memory_slab_stats_cb);
}

/* Low memory handler */
static void ami_memory_low_mem_handler(void *p)
{
	if(low_mem_status == PURGE_STEP1) {
		NSLOG(netsurf, INFO, "Purging llcache");
		llcache_clean(true);
		low_mem_status = PURGE_DONE_STEP1;
	}

	if(low_mem_status == PURGE_STEP2) {
		NSLOG(netsurf, INFO, "Purging unused slabs");
		__free_unused_slabs();
		low_mem_status = PURGE_DONE_STEP2;
	}
}

static ASM ULONG ami_memory_handler(REG(a0, struct MemHandlerData *mhd), REG(a1, void *userdata), REG(a6, struct ExecBase *execbase))
{
	if(low_mem_status == PURGE_DONE_STEP2) {
		low_mem_status = PURGE_NONE;
		return MEM_ALL_DONE;
	}

	if(low_mem_status == PURGE_DONE_STEP1) {
		low_mem_status = PURGE_STEP2;
	}

	if(low_mem_status == PURGE_NONE) {
		low_mem_status = PURGE_STEP1;
	}

	ami_schedule(1, ami_memory_low_mem_handler, NULL);

	return MEM_TRY_AGAIN;
}
 
struct Interrupt *ami_memory_init(void)
{
	struct Interrupt *memhandler = malloc(sizeof(struct Interrupt));
	if(memhandler == NULL) return NULL; // we're screwed

	memhandler->is_Node.ln_Pri = -127; // low down as will be slow
	memhandler->is_Node.ln_Name = "NetSurf low memory handler";
	memhandler->is_Data = NULL;
	memhandler->is_Code = (APTR)&ami_memory_handler;
	AddMemHandler(memhandler);

	return memhandler;
}

void ami_memory_fini(struct Interrupt *memhandler)
{
	if(memhandler != NULL) {
		RemMemHandler(memhandler);
		free(memhandler);
	}
}

#endif

