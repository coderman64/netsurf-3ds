/*
 * Copyright 2007 Rob Kendrick <rjek@netsurf-browser.org>
 * Copyright 2007 Vincent Sanders <vince@debian.org>
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

/**
 * \file
 * file extension to mimetype mapping for the monkey frontend
 *
 * allows monkey frontend to map file extension to mime types using a
 * default builtin list and /etc/mime.types file if present.
 *
 * mime type and content type handling is derived from the BNF in
 * RFC822 section 3.3, RFC2045 section 5.1 and RFC6838 section
 * 4.2. Upshot is their charset and parsing is all a strict subset of
 * ASCII hence not using locale dependant ctype functions for parsing.
 */

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "utils/log.h"
#include "utils/ascii.h"
#include "utils/hashtable.h"

#include "monkey/filetype.h"

#define HASH_SIZE 117
#define MAX_LINE_LEN 256

static struct hash_table *mime_hash = NULL;

void monkey_fetch_filetype_init(const char *mimefile)
{
	struct stat statbuf;
	FILE *fh = NULL;

	mime_hash = hash_create(HASH_SIZE);

	/* Some OSes (mentioning no Solarises) have a worthlessly tiny
	 * /etc/mime.types that don't include essential things, so we
	 * pre-seed our hash with the essentials.  These will get
	 * over-ridden if they are mentioned in the mime.types file.
	 */

	hash_add(mime_hash, "css", "text/css");
	hash_add(mime_hash, "htm", "text/html");
	hash_add(mime_hash, "html", "text/html");
	hash_add(mime_hash, "jpg", "image/jpeg");
	hash_add(mime_hash, "jpeg", "image/jpeg");
	hash_add(mime_hash, "bmp", "image/bmp");
	hash_add(mime_hash, "gif", "image/gif");
	hash_add(mime_hash, "png", "image/png");
	hash_add(mime_hash, "ico", "image/ico");
	hash_add(mime_hash, "jng", "image/jng");
	hash_add(mime_hash, "mng", "image/mng");
	hash_add(mime_hash, "webp", "image/webp");
	hash_add(mime_hash, "spr", "image/x-riscos-sprite");

	/* first, check to see if /etc/mime.types in preference */
	if ((stat("/etc/mime.types", &statbuf) == 0) &&
			S_ISREG(statbuf.st_mode)) {
		mimefile = "/etc/mime.types";

	}

	fh = fopen(mimefile, "r");

	if (fh == NULL) {
		NSLOG(netsurf, INFO,
		      "Unable to open a mime.types file, so using a minimal one for you.");
		return;
	}

	while (!feof(fh)) {
		char line[MAX_LINE_LEN], *ptr, *type, *ext;
		if (fgets(line, MAX_LINE_LEN, fh) == NULL)
                	break;
		if (!feof(fh) && line[0] != '#') {
			ptr = line;

			/* search for the first non-whitespace character */
			while (ascii_is_space(*ptr))
				ptr++;

			/* is this line empty other than leading whitespace? */
			if (*ptr == '\n' || *ptr == '\0')
				continue;

			type = ptr;

			/* search for the first whitespace char or NUL or
			 * NL */
			while (*ptr && (!ascii_is_space(*ptr)))
				ptr++;

			if (*ptr == '\0' || *ptr == '\n') {
				/* this mimetype has no extensions - read next
				 * line.
				 */
				continue;
			}

			*ptr++ = '\0';

			/* search for the first non-whitespace character which
			 * will be the first filename extenion */
			while (ascii_is_space(*ptr))
				ptr++;

			while(true) {
				ext = ptr;

				/* search for the first whitespace char or
				 * NUL or NL which is the end of the ext.
				 */
				while (*ptr && (!ascii_is_space(*ptr)))
					ptr++;

				if (*ptr == '\0' || *ptr == '\n') {
					/* special case for last extension on
					 * the line
					 */
					*ptr = '\0';
					hash_add(mime_hash, ext, type);
					break;
				}

				*ptr++ = '\0';
				hash_add(mime_hash, ext, type);

				/* search for the first non-whitespace char or
				 * NUL or NL, to find start of next ext.
				 */
				while (*ptr &&
				       (ascii_is_space(*ptr)) &&
				       *ptr != '\n') {
					ptr++;
				}
			}
		}
	}

	fclose(fh);
}

void monkey_fetch_filetype_fin(void)
{
	hash_destroy(mime_hash);
}

/**
 * Determine the MIME type of a local file.
 *
 * @note used in file fetcher
 *
 * \param unix_path Unix style path to file on disk
 * \return Pointer to static MIME type string (should not be freed) not NULL.
 *	   invalidated on next call to fetch_filetype.
 */
const char *monkey_fetch_filetype(const char *unix_path)
{
	struct stat statbuf;
	char *ext;
	const char *ptr;
	char *lowerchar;
	const char *type;
	int l;

	if (stat(unix_path, &statbuf) != 0) {
		/* error calling stat, the file has probably become
		 * inacessible, this routine cannot fail so just
		 * return the default mime type.
		 */
		return "text/plain";
	}
	if (S_ISDIR(statbuf.st_mode)) {
		return "application/x-netsurf-directory";
	}

	l = strlen(unix_path);
	if ((3 < l) && (strcasecmp(unix_path + l - 4, ",f79") == 0)) {
		return "text/css";
	}

	if (strchr(unix_path, '.') == NULL) {
		/* no extension anywhere! */
		return "text/plain";
	}

	ptr = unix_path + strlen(unix_path);
	while (*ptr != '.' && *ptr != '/')
		ptr--;

	if (*ptr != '.') {
		return "text/plain";
	}

	ext = strdup(ptr + 1);	/* skip the . */

	/* the hash table only contains lower-case versions - make sure this
	 * copy is lower case too.
	 */
	lowerchar = ext;
	while(*lowerchar) {
		*lowerchar = ascii_to_lower(*lowerchar);
		lowerchar++;
	}

	type = hash_get(mime_hash, ext);
	free(ext);

	return type != NULL ? type : "text/plain";
}
