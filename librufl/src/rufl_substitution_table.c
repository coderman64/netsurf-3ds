/*
 * This file is part of RUfl
 * Licensed under the MIT License,
 *                http://www.opensource.org/licenses/mit-license
 * Copyright 2006 James Bursa <james@semichrome.net>
 */

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "rufl_internal.h"

#undef RUFL_SUBSTITUTION_TABLE_DEBUG

/**
 * Base type for a substitution table.
 */
struct rufl_substitution_table {
	/** Description of table implementation */
	const char *desc;
	/** Look up a Unicode codepoint. */
	unsigned int (*lookup)(const struct rufl_substitution_table *t,
			uint32_t u);
	/** Free the resources used by this table */
	void (*free)(struct rufl_substitution_table *t);
	/** Dump the contents of this table to stdout */
	void (*dump)(const struct rufl_substitution_table *t,
			unsigned int plane);
	/** Compute the storage size of this table */
	size_t (*size)(const struct rufl_substitution_table *t,
			unsigned int *glyph_count);
};

/**
 * Implementation of a substitution table using direct lookup.
 */
struct rufl_substitution_table_direct {
	struct rufl_substitution_table base;

	/** Table index.
	 *
	 * Each entry represents a block of 256 codepoints, so i[k] refers
	 * to codepoints [256*k, 256*(k+1)). The value is an offset into
	 * the block table.
	 */
	uint8_t index[256];

	/** Bits per block table entry. Will be 8 or 16. */
	uint8_t bits_per_entry;

	/** Substitution table.
	 *
	 * Entries are the index into rufl_font_list of a font providing a
	 * substitution glyph for this codepoint or NOT_AVAILABLE.
	 *
	 * Note that, although this is defined as a 16bit type,
	 * the actual field width is indicated by bits_per_entry (and,
	 * if bits_per_entry is 8, then (NOT_AVAILABLE & 0xff) represents
	 * a missing glyph).
	 */
	uint16_t *table;
};

/**
 * Implementation of a substitution table using a perfect hash.
 *
 * A perfect hash constructed at library initialisation time using the
 * CHD algorithm. Hash entries are found via a two-step process:
 *
 *   1. apply a first-stage hash to the key to find the bucket
 *      in which the corresponding entry should be found.
 *   2. apply a second-stage hash to the key and the stored
 *      displacement value for the bucket to find the index
 *      into the substitution table.
 */
struct rufl_substitution_table_chd {
	struct rufl_substitution_table base;

	uint32_t num_buckets; /**< Number of buckets in the hash */
	uint32_t num_slots; /**< Number of slots in the table */
	/** Substitution table.
	 *
	 * Fields in the substitution table have the following format:
	 *
	 *    3                   2                   1                   0
	 *  1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
	 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	 * |       Unicode codepoint       |        Font identifier        |
	 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	 *
	 * where:
	 *
	 *   unicode codepoint: 16 bits
	 *     The low 16 bits of the Unicode codepoint value.
	 *
	 *   font identifier: 16 bits
	 *     The index into rufl_font_list of a font providing a
	 *     substitution glyph for this codepoint or NOT_AVAILABLE.
	 *
	 * Note that, as the substitution table is sparse and may not be
	 * fully populated, it is necessary to verify that the Unicode
	 * codepoint matches the key being hashed and that the font
	 * identifier is not NOT_AVAILABLE. If either of these tests
	 * fail, no font provides a suitable glyph and the not available
	 * path should be taken.
	 */
	uint32_t *table;
	uint8_t bits_per_entry; /**< Bits per displacement bitmap entry */
	/** Displacement bitmap.
	 *
	 * The displacement values are stored in a bitmap of num_buckets
	 * fields each being bits_per_entry wide. Both values are computed
	 * at runtime.
	 */
	uint8_t displacement_map[];
};
/** Font substitution tables -- one per plane */
static struct rufl_substitution_table *rufl_substitution_table[17];

/**
 * Round an unsigned 32bit value up to the next power of 2
 */
static uint32_t ceil2(uint32_t val)
{
	val--;
	val |= (val >> 1);
	val |= (val >> 2);
	val |= (val >> 4);
	val |= (val >> 8);
	val |= (val >> 16);
	val++;
	val += (val == 0);
	return val;
}

/**
 * Compute the number of bits needed to store a value
 */
static uint32_t bits_needed(uint32_t val)
{
	int32_t result = 0;

	if (val == 0)
		return 1;

	if ((val & (val - 1))) {
		/* Not a power of 2: round up */
		val = ceil2(val);
		/* Will need one fewer bit than we're about to count */
		result = -1;
	}

	while (val > 0) {
		result += 1;
		val >>= 1;
	}

	return (uint32_t) result;
}

/**
 * Perform one round of MurmurHash2
 */
static uint32_t mround(uint32_t val, uint32_t s)
{
	val *= 0x5db1e995;
	val ^= (val >> 24);
	val *= 0x5db1e995;
	val ^= (s * 0x5db1e995);

	return val;
}

/**
 * Perform the MurmurHash2 mixing step
 */
static uint32_t mmix(uint32_t val)
{
	val ^= (val >> 13);
	val *= 0x5db1e995;
	val ^= (val >> 15);

	return val;
}

/**
 * First-stage hash (i.e. g(x)) for substitution table.
 *
 * As we know that the input values are Unicode codepoints,
 * do some trivial bit manipulation, which has reasonable
 * distribution properties.
 */
static uint32_t hash1(uint32_t val)
{
	val ^= (val >> 7);
	val ^= (val << 3);
	val ^= (val >> 4);
	return val;
}

/**
 * Second-stage hash (i.e. f(d, x)) for substitution table.
 *
 * Apply MurmurHash2 to the value and displacement
 */
static uint32_t hash2(uint32_t val, uint32_t d)
{
	return mmix(mround(val, mround(d, 4)));
}

/**
 * Comparison function for table entries.
 *
 * We use this when sorting the intermediate table for CHD.
 */
static int table_chd_cmp(const void *a, const void *b)
{
	/* We're only interested in the CHD metadata here.
	 * (i.e. the computed value of g(x) and the bucket size) */
	const uint64_t aa = (*(const uint64_t *) a) & 0x000fffff00000000llu;
	const uint64_t bb = (*(const uint64_t *) b) & 0x000fffff00000000llu;

	if (aa > bb)
		return -1;
	else if (aa < bb)
		return 1;
	return 0;
}

/**
 * Test that all specified bits in a bit map are clear and set them if so.
 *
 * \param bitmap Bit map to inspect
 * \param idx    Table of indices to inspect
 * \param len    Number of entries in index table
 * \return True if all bits were clear. False otherwise.
 */
static bool test_and_set_bits(uint8_t *bitmap, const uint32_t *idx, size_t len)
{
	unsigned int i;
	bool result = true;

	/* Test if all specified bits are clear */
	for (i = 0; i != len; i++) {
		const uint32_t byte = (idx[i] >> 3);
		const uint32_t bit = (idx[i] & 0x7);

		result &= ((bitmap[byte] & (1 << bit)) == 0);
	}

	if (result) {
		/* They are, so set them */
		for (i = 0; i != len; i++) {
			const uint32_t byte = (idx[i] >> 3);
			const uint32_t bit = (idx[i] & 0x7);

			bitmap[byte] |= (1 << bit);
		}
	}

	return result;
}

static void rufl_substitution_table_free_chd(
		struct rufl_substitution_table *t)
{
	free(((struct rufl_substitution_table_chd *)t)->table);
	free(t);
}


static unsigned int rufl_substitution_table_lookup_chd(
		const struct rufl_substitution_table *ts, uint32_t u)
{
	const struct rufl_substitution_table_chd *t = (const void *) ts;
	uint32_t displacement = 0;
	uint32_t f, g = hash1(u & 0xffff) & (t->num_buckets - 1);
	uint32_t bits_to_read = t->bits_per_entry;
	uint32_t offset_bits = g * bits_to_read;
	const uint8_t *pread = &t->displacement_map[offset_bits >> 3];

	offset_bits &= 7;

	while (bits_to_read > 0) {
		uint32_t space_available = (8 - offset_bits);
		if (space_available > bits_to_read)
			space_available = bits_to_read;

		displacement <<= space_available;
		displacement |= (*pread & (0xff >> offset_bits)) >>
				(8 - space_available - offset_bits);

		offset_bits += space_available;
		if (offset_bits >= 8) {
			pread++;
			offset_bits = 0;
		}
		bits_to_read -= space_available;
	}

	f = hash2((u & 0xffff), displacement) & (t->num_slots - 1);

	if ((t->table[f] & 0xffff) != NOT_AVAILABLE &&
			((t->table[f] >> 16) & 0xffff) == (u & 0xffff))
		return t->table[f] & 0xffff;

	return NOT_AVAILABLE;
}

static int table_dump_cmp(const void *a, const void *b)
{
	const uint32_t aa = (*(const uint32_t *) a);
	const uint32_t bb = (*(const uint32_t *) b);

	if (aa > bb)
		return 1;
	else if (aa < bb)
		return -1;
	return 0;
}

static void rufl_substitution_table_dump_chd(
		const struct rufl_substitution_table *ts, unsigned int plane)
{
	const struct rufl_substitution_table_chd *t = (const void *) ts;
	unsigned int font;
	unsigned int u, prev;
	uint32_t *table;

	table = malloc(t->num_slots * sizeof(*table));
	if (table == NULL)
		return;

	memcpy(table, t->table, t->num_slots * sizeof(*table));

	qsort(table, t->num_slots, sizeof(*table), table_dump_cmp);

	u = 0;
	while (u < t->num_slots) {
		prev = u;
		font = table[prev] & 0xffff;
		while (u < t->num_slots && font == (table[u] & 0xffff) &&
				((u == prev) ||
				 ((table[u - 1] >> 16) ==
				  ((table[u] >> 16) - 1))))
			u++;
		if (font != NOT_AVAILABLE)
			printf("  %x-%x => %u \"%s\"\n",
					(plane << 16) | (table[prev] >> 16),
					(plane << 16) |	(table[u - 1] >> 16),
					font, rufl_font_list[font].identifier);
	}

	free(table);
}

static size_t rufl_substitution_table_size_chd(
		const struct rufl_substitution_table *ts,
		unsigned int *glyph_count)
{
	const struct rufl_substitution_table_chd *t = (const void *) ts;
	size_t size = sizeof(*t);
	unsigned int count = 0;
	uint32_t i;

	/* Add on displacement map size */
	size += (t->num_buckets * t->bits_per_entry + 7) >> 3;

	/* Add on table size */
	size += t->num_slots * sizeof(*t->table);

	/* Count glyphs */
	for (i = 0; i < t->num_slots; i++) {
		if ((t->table[i] & 0xffff) != NOT_AVAILABLE)
			count++;
	}
	if (glyph_count != NULL)
		*glyph_count = count;

	return size;
}

/**
 * Create the final substitution table from the intermediate parts
 *
 * \param table               Substitution table
 * \param table_entries       Number of entries in table
 * \param buckets             Number of CHD buckets
 * \param range               Number of slots in final table
 * \param max_displacement    max(displacements)
 * \param displacements       Table of displacement values. One per bucket.
 * \param substitution_table  Location to receive result.
 */
static rufl_code create_substitution_table_chd(uint64_t *table,
		size_t table_entries, uint32_t buckets, uint32_t range,
		uint32_t max_displacement, uint32_t *displacements,
		struct rufl_substitution_table **substitution_table)
{
	struct rufl_substitution_table_chd *subst_table;
	uint64_t *t64;
	size_t subst_table_size;
	unsigned int i;

#ifdef RUFL_SUBSTITUTION_TABLE_DEBUG
	LOG("max displacement of %u requires %u bits",
			max_displacement, bits_needed(max_displacement));
#endif

	subst_table_size = offsetof(struct rufl_substitution_table_chd,
			displacement_map) +
			((buckets * bits_needed(max_displacement) + 7) >> 3);

	subst_table = calloc(subst_table_size, 1);
	if (!subst_table)
		return rufl_OUT_OF_MEMORY;

	/* We know there are at least table_entries in the table, but
	 * we should now resize it to the size of the target hashtable.
	 * We still want each entry to be 64bits wide at this point. */
	t64 = realloc(table, range * sizeof(*t64));
	if (!t64) {
		free(subst_table);
		return rufl_OUT_OF_MEMORY;
	}
	/* Initialise unused slots */
	for (i = table_entries; i < range; i++) {
		t64[i] = NOT_AVAILABLE;
	}

	subst_table->base.desc = "CHD";
	subst_table->base.lookup = rufl_substitution_table_lookup_chd;
	subst_table->base.free = rufl_substitution_table_free_chd;
	subst_table->base.dump = rufl_substitution_table_dump_chd;
	subst_table->base.size = rufl_substitution_table_size_chd;
	subst_table->num_buckets = buckets;
	subst_table->num_slots = range;
	subst_table->bits_per_entry = bits_needed(max_displacement);
	subst_table->table = (uint32_t *) t64;

	/* Fill in displacement map */
	//XXX: compress map using Fredriksson-Nikitin encoding?
	for (i = 0; i < buckets; i++) {
		uint32_t offset_bits = i * subst_table->bits_per_entry;
		uint32_t bits_to_write = subst_table->bits_per_entry;
		uint8_t *pwrite =
			&subst_table->displacement_map[offset_bits >> 3];

		offset_bits &= 7;

		while (bits_to_write > 0) {
			uint32_t space_available = (8 - offset_bits);
			uint32_t mask = 0, mask_idx;

			if (space_available > bits_to_write)
				space_available = bits_to_write;

			for (mask_idx = 0; mask_idx != space_available;
					mask_idx++) {
				mask <<= 1;
				mask |= 1;
			}

			*pwrite |= ((displacements[i] >>
				 (bits_to_write - space_available)) & mask) <<
				(8 - offset_bits - space_available);
			pwrite++;
			offset_bits = 0;
			bits_to_write -= space_available;
		}
	}

	/* Shuffle table data so the indices match the hash values */
	for (i = 0; i < table_entries; ) {
		uint32_t f, g;
		uint64_t tmp;

		if (t64[i] == NOT_AVAILABLE) {
			i++;
			continue;
		}

		g = ((t64[i] >> 32) & 0xffff);
		f = hash2((t64[i] >> 16) & 0xffff,
				displacements[g]) & (range - 1);

		/* Exchange this entry with the one in the slot at f.*/
		if (f != i) {
			tmp = t64[f];
			t64[f] = t64[i];
			t64[i] = tmp;
		} else {
			/* Reconsider this slot unless it already
			 * had the correct entry */
			i++;
		}
	}
	/* Strip all the CHD metadata out of the final table.
	 * We can simply drop the top 32bits of each entry by
	 * compacting the entries. */
	for (i = 0; i < range; i++) {
		subst_table->table[i] = t64[i] & 0xffffffffu;
	}

	/* Shrink the table to its final size. If this fails, leave
	 * the existing data intact as it's correct -- we just have
	 * twice the storage usage we need. */
	subst_table->table = realloc(t64,
			range * sizeof(*subst_table->table));
	if (!subst_table->table)
		subst_table->table = (uint32_t *) t64;

	*substitution_table = &subst_table->base;

#ifdef RUFL_SUBSTITUTION_TABLE_DEBUG
	LOG("table size(%zu) entries %zu buckets(%u@%ubpe => %u)",
			subst_table->num_slots * sizeof(*subst_table->table),
			table_entries,
			subst_table->num_buckets,
			subst_table->bits_per_entry,
			(subst_table->num_buckets *
			 subst_table->bits_per_entry + 7) >> 3);
#endif

	return rufl_OK;
}

/**
 * Compute a perfect hash to address the substitution table.
 *
 * We use the CHD algorithm to do this.
 * (https://doi.org/10.1007/978-3-642-04128-0_61 ;
 *  http://cmph.sourceforge.net/papers/esa09.pdf)
 *
 * A more recent alternative might be RecSplit
 * (https://arxiv.org/abs/1910.06416v2).
 *
 * \param table               Pre-filled table of raw substitution data
 * \param table_entries       Number of entries in the table
 * \param substitution_table  Location to receive result
 */
static rufl_code chd(uint64_t *table, size_t table_entries,
		struct rufl_substitution_table **substitution_table)
{
	/** Number of buckets assuming an average bucket size of 4 */
	const uint32_t buckets = ceil2((table_entries + 3) & ~3);
	/** Number of output hash slots assuming a load factor of 1 */
	const uint32_t range = ceil2(table_entries);
	uint32_t bucket_size, max_displacement = 0;
	unsigned int i;
	uint8_t *entries_per_bucket, *bitmap;
	uint32_t *displacements;
	rufl_code result = rufl_OK;

#ifdef RUFL_SUBSTITUTION_TABLE_DEBUG
	LOG("hashing %zu entries into %u buckets with range %u",
			table_entries, buckets, range);
#endif

	entries_per_bucket = calloc(buckets, sizeof(*entries_per_bucket));
	if (!entries_per_bucket)
		return rufl_OUT_OF_MEMORY;

	/* Round up bitmap size to the next byte boundary */
	bitmap = calloc(((range + 7) & ~7) >> 3, 1);
	if (!bitmap) {
		free(entries_per_bucket);
		return rufl_OUT_OF_MEMORY;
	}

	displacements = calloc(buckets, sizeof(*displacements));
	if (!displacements) {
		free(bitmap);
		free(entries_per_bucket);
		return rufl_OUT_OF_MEMORY;
	}

	/* Compute g(x) for each entry, placing them into buckets */
	for (i = 0; i < table_entries; i++) {
		uint64_t g = hash1((table[i] >> 16) & 0xffff) & (buckets - 1);

		/* Insert hash into entry (it's 16 bits at most,
		 * so use bits 32-47) */
		table[i] |= ((g & 0xffff) << 32);

		entries_per_bucket[g]++;
	}

	/* Inject bucket size into entries */
	for (i = 0; i < table_entries; i++) {
		uint32_t g = ((table[i] >> 32) & 0xffff);

		/* With a target bucket size of 4, do not expect
		 * >= twice that number of entries in the largest
		 * bucket. If there are, the hash function needs
		 * work (we allocate 4 bits for the bucket size,
		 * so should have sufficient headroom). */
		if (entries_per_bucket[g] >= 8)
			LOG("unexpectedly large bucket %u",
					entries_per_bucket[g]);

		/* Stash bucket size into bits 48-51 of the entry */
		table[i] |= ((uint64_t)entries_per_bucket[g] << 48);
	}

	/* Bits 52-63 of table entries are currently unused */

	free(entries_per_bucket);

	/* Sort entries in descending bucket size order */
	qsort(table, table_entries, sizeof(*table), table_chd_cmp);

	/* Compute f(x) for each bucket, finding a unique mapping */
	for (i = 0; i < table_entries; i += bucket_size) {
		const uint32_t g = ((table[i] >> 32) & 0xffff);
		uint32_t hashes[8], num_hashes;
		uint32_t d = 0;

		bucket_size = ((table[i] >> 48) & 0xf);

		do {
			uint32_t j, k;

			d++;
			num_hashes = 0;

			for (j = 0; j != bucket_size; j++) {
				uint32_t f = hash2(
					(table[i+j] >> 16) & 0xffff, d) &
					(range - 1);
				for (k = 0; k < num_hashes; k++) {
					if (f == hashes[k])
						break;
				}
				if (k == num_hashes) {
					hashes[num_hashes] = f;
					num_hashes++;
				}
			}
		} while (num_hashes != bucket_size || !test_and_set_bits(
				bitmap, hashes, num_hashes));

		displacements[g] = d;
		if (d > max_displacement)
			max_displacement = d;
	}

	free(bitmap);

	result = create_substitution_table_chd(table, table_entries,
			buckets, range, max_displacement, displacements,
			substitution_table);
	free(displacements);

	return result;
}

static size_t rufl_substitution_table_estimate_size_chd(size_t table_entries,
		size_t blocks_used)
{
	size_t size = sizeof(struct rufl_substitution_table_chd);

	(void) blocks_used;

	/** Number of buckets assuming an average bucket size of 4 */
	const uint32_t buckets = ceil2((table_entries + 3) & ~3);
	/** Number of output hash slots assuming a load factor of 1 */
	const uint32_t range = ceil2(table_entries);

	/* Conservatively assume 6 bits per displacement map entry */
	size += (buckets * 6 + 7) >> 3;

	/* Add on table size */
	size += range * 4;

	return size;
}

/****************************************************************************/

static void rufl_substitution_table_free_direct(
		struct rufl_substitution_table *t)
{
	free(((struct rufl_substitution_table_direct *)t)->table);
	free(t);
}


static unsigned int rufl_substitution_table_lookup_direct(
		const struct rufl_substitution_table *ts, uint32_t u)
{
	const struct rufl_substitution_table_direct *t = (const void *) ts;
	uint32_t block = (u >> 8) & 0xff;
	uint32_t slot = (u & 0xff);
	unsigned int font;

	if (t->bits_per_entry == 8) {
		font = ((uint8_t *) t->table)[t->index[block] * 256 + slot];
		if (font == (NOT_AVAILABLE & 0xff))
			font = NOT_AVAILABLE;
	} else
		font = t->table[t->index[block] * 256 + slot];

	return font;
}

static void rufl_substitution_table_dump_direct(
		const struct rufl_substitution_table *ts, unsigned int plane)
{
	const struct rufl_substitution_table_direct *t = (const void *) ts;
	unsigned int font, na;
	unsigned int u, prev;
	uint8_t *t8 = (uint8_t *) t->table;

	na = NOT_AVAILABLE & ((t->bits_per_entry == 8) ? 0xff : 0xffff);

#define LOOKUP(u) (t->bits_per_entry == 8 \
		? t8[t->index[(u >> 8) & 0xff] * 256 + (u & 0xff)] \
		: t->table[t->index[(u >> 8) & 0xff] * 256 + (u & 0xff)])

	u = 0;
	while (u < 0x10000) {
		prev = u;
		font = LOOKUP(u);
		while (u < 0x10000 && font == LOOKUP(u))
			u++;
		if (font != na)
			printf("  %x-%x => %u \"%s\"\n",
					(plane << 16) | prev,
					(plane << 16) | (u - 1),
					font, rufl_font_list[font].identifier);
	}

#undef LOOKUP
}

static size_t rufl_substitution_table_size_direct(
		const struct rufl_substitution_table *ts,
		unsigned int *glyph_count)
{
	const struct rufl_substitution_table_direct *t = (const void *) ts;
	size_t size = sizeof(*t);
	unsigned int i, block_idx = 0;
	unsigned int count = 0, na;

	/* Find the largest block index (blocks are contiguous) */
	for (i = 0; i < 256; i++)
		if (t->index[i] > block_idx)
			block_idx = t->index[i];

	/* Add on table size */
	size += (t->bits_per_entry * (block_idx + 1) * 256) >> 3;

	/* Count glyphs */
	na = NOT_AVAILABLE & ((t->bits_per_entry == 8) ? 0xff : 0xffff);
	for (i = 0; i < 0x10000; i++) {
		const uint8_t *t8 = (const uint8_t *) t->table;

#define LOOKUP(u) (t->bits_per_entry == 8 \
		? t8[t->index[(u >> 8) & 0xff] * 256 + (u & 0xff)] \
		: t->table[t->index[(u >> 8) & 0xff] * 256 + (u & 0xff)])

		if (LOOKUP(i) != na)
			count++;
#undef LOOKUP
	}
	if (glyph_count != NULL)
		*glyph_count = count;

	return size;
}

/**
 * Construct a direct-mapped substitution table
 */
static rufl_code direct(uint64_t *table, size_t table_entries,
		size_t blocks_used, const uint8_t block_histogram[256],
		struct rufl_substitution_table **substitution_table)
{
	struct rufl_substitution_table_direct *subst_table;
	size_t blocks_needed, table_size;
	unsigned int i, block;

	subst_table = calloc(sizeof(*subst_table), 1);
	if (!subst_table)
		return rufl_OUT_OF_MEMORY;

	subst_table->base.desc = "Direct";
	subst_table->base.lookup = rufl_substitution_table_lookup_direct;
	subst_table->base.free = rufl_substitution_table_free_direct;
	subst_table->base.dump = rufl_substitution_table_dump_direct;
	subst_table->base.size = rufl_substitution_table_size_direct;
	/* We can use 8bits per entry if there are fewer than 255 fonts */
	subst_table->bits_per_entry = rufl_font_list_entries < 255 ? 8 : 16;

	/* Need one extra block if there's at least one free */
	blocks_needed = (blocks_used < 256) ?
			(blocks_used + 1) : blocks_used;

	table_size = (256 * blocks_needed * subst_table->bits_per_entry) >> 3;

	/* Populate block index */
	for (i = 0, block = 0; i != 256; i++) {
		if (block_histogram[i] == 0) {
			subst_table->index[i] = blocks_used;
		} else {
			subst_table->index[i] = block;
			block++;
		}
	}

#ifdef RUFL_SUBSTITUTION_TABLE_DEBUG
	LOG("blocks-used = %zu blocks-needed = %zu"
		       " bits-per-entry = %u size = %zu",
		       blocks_used, blocks_needed, subst_table->bits_per_entry,
		       table_size);
#endif

	/* Allocate table */
	//XXX: can we just rearrange the existing one in-place?
	subst_table->table = malloc(table_size * (subst_table->bits_per_entry >> 3));
	if (!subst_table->table) {
		free(subst_table);
		return rufl_OUT_OF_MEMORY;
	}
	/* Fill it with NOT_AVAILABLE */
	if (subst_table->bits_per_entry == 8) {
		memset(subst_table->table, (NOT_AVAILABLE & 0xff), table_size);
	} else {
		for (i = 0; i < table_size; i++)
			subst_table->table[i] = NOT_AVAILABLE;
	}

	/* Populate the table */
	for (i = 0; i < table_entries; i++) {
		uint64_t val = table[i];
		uint32_t slot = (val >> 16) & 0xff;
		block = subst_table->index[(val >> 24) & 0xff];

		if (subst_table->bits_per_entry == 8) {
			uint8_t *t8 = (uint8_t *) subst_table->table;
			t8[256 * block + slot] = (val & 0xff);
		} else {
			subst_table->table[256 * block + slot] = (val & 0xffff);
		}
	}

	free(table);

	*substitution_table = &subst_table->base;

	return rufl_OK;
}

static size_t rufl_substitution_table_estimate_size_direct(size_t table_entries,
		size_t blocks_used)
{
	size_t size = sizeof(struct rufl_substitution_table_direct);

	(void) table_entries;

	/* Add one for empty block */
	if (blocks_used < 256)
		blocks_used += 1;

	/* We can use 8bits per entry if there are fewer than 255 fonts */
	size += blocks_used * 256 * (rufl_font_list_entries < 255 ? 1 : 2);

	return size;
}

/****************************************************************************/

/**
 * Populate the substitution map for a given block
 */
static void fill_map_for_block(const struct rufl_character_set **charsets,
		uint32_t block, uint16_t map_for_block[256])
{
	unsigned int i, u;

	for (i = 0; i != rufl_font_list_entries; i++) {
		if (!charsets[i])
			continue;

		if (charsets[i]->index[block] == BLOCK_FULL) {
			for (u = 0; u != 256; u++)
				if (map_for_block[u] == NOT_AVAILABLE)
					map_for_block[u] = i;
		} else if (charsets[i]->index[block] != BLOCK_EMPTY) {
			const uint8_t *blk = charsets[i]->block[
					charsets[i]->index[block]];
			for (u = 0; u != 256; u++) {
				if (map_for_block[u] == NOT_AVAILABLE &&
						(blk[(u>>3)] & (1 << (u&7)))) {
					map_for_block[u] = i;
				}
			}
		}
	}
}

/**
 * Create a substitution table for the plane specified
 */
static rufl_code create_substitution_table_for_plane(unsigned int plane)
{
	unsigned int i;
	unsigned int block;
	unsigned int u;
	const struct rufl_character_set **charsets;
	const struct rufl_character_set *charset;
	unsigned int num_charsets;
	uint64_t *table;
	size_t table_size;
	size_t table_entries;
	uint8_t block_histogram[256];
	size_t blocks_used;
	size_t direct_size, chd_size;
	rufl_code result;

	charsets = malloc(rufl_font_list_entries * sizeof(*charsets));
	if (!charsets) {
		LOG("malloc(%zu) failed",
				rufl_font_list_entries * sizeof(*charsets));
		return rufl_OUT_OF_MEMORY;
	}

	/* Find fonts that have charsets for this plane */
	num_charsets = 0;
	for (i = 0; i != rufl_font_list_entries; i++) {
		charset = rufl_font_list[i].charset;
		if (!charset) {
			charsets[i] = NULL;
			continue;
		}

		while (PLANE_ID(charset->metadata) != plane &&
				EXTENSION_FOLLOWS(charset->metadata)) {
			charset = (void *)(((uint8_t *)charset) +
					PLANE_SIZE(charset->metadata));
		}
		if (PLANE_ID(charset->metadata) != plane)
			charset = NULL;
		charsets[i] = charset;
		num_charsets++;
	}
	if (num_charsets == 0) {
#ifdef RUFL_SUBSTITUTION_TABLE_DEBUG
		LOG("no charsets for plane %u", plane);
#endif
		rufl_substitution_table[plane] = NULL;
		free(charsets);
		return rufl_OK;
	}

	table = malloc(1024 * sizeof(*table));
	if (!table) {
		LOG("malloc(%zu) failed", 1024 * sizeof(*table));
		free(charsets);
		return rufl_OUT_OF_MEMORY;
	}
	table_size = 1024;
	table_entries = 0;

	/* Process each block, finding fonts that have glyphs */
	blocks_used = 0;
	memset(block_histogram, 0, 256);
	for (block = 0; block != 256; block++) {
		size_t prev_table_entries = table_entries;
		uint16_t map_for_block[256];
		for (i = 0; i != 256; i++)
			map_for_block[i] = NOT_AVAILABLE;

		fill_map_for_block(charsets, block, map_for_block);

		/* Merge block map into table */
		for (i = 0; i != 256; i++) {
			if (map_for_block[i] == NOT_AVAILABLE)
				continue;

			u = (block << 8) | i;
			table[table_entries] = (u << 16) | map_for_block[i];
			if (++table_entries == table_size) {
				uint64_t *tmp = realloc(table,
						2 * table_size *
						sizeof(*table));
				if (!tmp) {
					LOG("realloc(%zu) failed",
						2 * table_size *
							sizeof(*table));
					free(table);
					return rufl_OUT_OF_MEMORY;
				}

				table = tmp;
				table_size *= 2;
			}
		}

		block_histogram[block] = (table_entries != prev_table_entries);
		if (block_histogram[block] != 0)
			blocks_used++;
	}

	if (table_entries == 0) {
#ifdef RUFL_SUBSTITUTION_TABLE_DEBUG
		LOG("no glyphs for plane %u", plane);
#endif
		rufl_substitution_table[plane] = NULL;
		free(table);
		free(charsets);
		return rufl_OK;
	}

	/* Build final substitution table using whichever implementation
	 * estimates the smallest storage requirements. */
	direct_size = rufl_substitution_table_estimate_size_direct(
			table_entries, blocks_used);
	chd_size = rufl_substitution_table_estimate_size_chd(
			table_entries, blocks_used);
	if (direct_size <= chd_size) {
		result = direct(table, table_entries, blocks_used,
				block_histogram,
				&rufl_substitution_table[plane]);
	} else {
		result = chd(table, table_entries,
				&rufl_substitution_table[plane]);
	}

#ifdef RUFL_SUBSTITUTION_TABLE_DEBUG
	LOG("plane %u: table-entries = %zu blocks-used = %zu"
		       " estimated-direct-size = %zu estimated-chd-size = %zu"
		       " actual-size = %zu",
			plane, table_entries, blocks_used,
			direct_size, chd_size,
			rufl_substitution_table[plane] ?
				rufl_substitution_table[plane]->size(
					rufl_substitution_table[plane],
					NULL) : 0);
#endif

	free(charsets);

	return result;
}

/**
 * Construct the font substitution table.
 */

rufl_code rufl_substitution_table_init(void)
{
	unsigned int plane;
	rufl_code rc;

	for (plane = 0; plane < 17; plane++) {
		rc = create_substitution_table_for_plane(plane);
		if (rc != rufl_OK) {
			while (plane > 0) {
				plane--;
				if (!rufl_substitution_table[plane])
					continue;
				rufl_substitution_table[plane]->free(
						rufl_substitution_table[plane]);
			}
			return rc;
		}
	}

	return rufl_OK;
}

/**
 * Destroy the substitution table and clean up its resources
 */

void rufl_substitution_table_fini(void)
{
	unsigned int plane;

	for (plane = 0; plane < 17; plane++) {
		if (rufl_substitution_table[plane] != NULL)
			rufl_substitution_table[plane]->free(
					rufl_substitution_table[plane]);
		rufl_substitution_table[plane] = NULL;
	}
}

/**
 * Look up a Unicode codepoint in the substitution table
 */

unsigned int rufl_substitution_table_lookup(uint32_t u)
{
	unsigned int plane = (u >> 16) & 0x1f;

	if (17 <= plane || !rufl_substitution_table[plane])
		return NOT_AVAILABLE;

	return rufl_substitution_table[plane]->lookup(
			rufl_substitution_table[plane], u);
}

/**
 * Dump a representation of the substitution table to stdout.
 */

void rufl_substitution_table_dump(void)
{
	unsigned int plane, glyphs = 0;
	size_t size = 0;

	for (plane = 0; plane < 17; plane++) {
		if (!rufl_substitution_table[plane])
			continue;
		rufl_substitution_table[plane]->dump(
				rufl_substitution_table[plane], plane);
	}

	for (plane = 0; plane < 17; plane++) {
		size_t plane_size;
		unsigned int plane_glyphs;
		const char *plane_desc;

		if (!rufl_substitution_table[plane]) {
			plane_size = 0;
			plane_glyphs = 0;
			plane_desc = "None";
		} else {
			plane_size = rufl_substitution_table[plane]->size(
				rufl_substitution_table[plane], &plane_glyphs);
			plane_desc = rufl_substitution_table[plane]->desc;
		}
		size += plane_size;
		glyphs += plane_glyphs;
		printf("              Storage for plane %2d: %8zu bytes %7u glyphs (%s)\n",
				plane + 1, plane_size, plane_glyphs, plane_desc);
	}

	printf("  Total substitution table storage: %8zu bytes %7u glyphs\n",
			size + sizeof(rufl_substitution_table), glyphs);
}
