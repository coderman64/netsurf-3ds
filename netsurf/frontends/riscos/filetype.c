/*
 * Copyright 2004 James Bursa <bursa@users.sourceforge.net>
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

#include <stdlib.h>
#include <string.h>
#include <unixlib/local.h>
#include "oslib/mimemap.h"
#include "oslib/osfile.h"

#include "utils/config.h"
#include "utils/log.h"
#include "netsurf/content.h"

#include "riscos/filetype.h"
#include "riscos/gui.h"

/* type_map must be in sorted order by file_type */
struct type_entry {
	bits file_type;
	char mime_type[40];
};
static const struct type_entry type_map[] = {
	{0x132, "image/ico"},
	{0x188, "application/x-shockwave-flash"},
	{0x695, "image/gif"},
	{0x69c, "image/x-ms-bmp"},
	{0xa66, "image/webp"},
	{0xaad, "image/svg+xml"},
	{0xaff, "image/x-drawfile"},
	{0xb60, "image/png"},
	{0xc85, "image/jpeg"},
	{0xd94, "image/x-artworks"},
	{0xf78, "image/jng"},
	{0xf79, "text/css"},
	{0xf81, "application/javascript"},
	{0xf83, "image/mng"},
	{0xfaf, "text/html"},
	{0xff9, "image/x-riscos-sprite"},
	{0xfff, "text/plain"},
};
#define TYPE_MAP_COUNT (sizeof(type_map) / sizeof(type_map[0]))

#define BUF_SIZE (256)
static char type_buf[BUF_SIZE];


static int cmp_type(const void *x, const void *y);

/* exported interface documented in riscos/filetype.h */
const char *fetch_filetype(const char *unix_path)
{
	struct type_entry *t;
	unsigned int len = strlen(unix_path) + 100;
	char *path = calloc(len, 1);
	char *r;
	os_error *error;
	bits file_type, temp;
	int objtype;

	if (!path) {
		NSLOG(netsurf, INFO, "Insufficient memory for calloc");
		ro_warn_user("NoMemory", 0);
		return "application/riscos";
	}

	/* convert path to RISC OS format and read file type */
	r = __riscosify(unix_path, 0, __RISCOSIFY_NO_SUFFIX, path, len, 0);
	if (r == 0) {
		NSLOG(netsurf, INFO, "__riscosify failed");
		free(path);
		return "application/riscos";
	}

	error = xosfile_read_stamped_no_path(path, &objtype, 0, 0, 0, 0,
			&file_type);
	if (error) {
		NSLOG(netsurf, INFO,
		      "xosfile_read_stamped_no_path failed: %s",
		      error->errmess);
		free(path);
		return "application/riscos";
	}

	if (objtype == osfile_IS_DIR) {
		sprintf(type_buf, "application/x-netsurf-directory");
		free(path);
		return (const char *)type_buf;
	}

	/* If filetype is text or data, and the file has an extension, try to
	 * map the extension to a filetype via the MimeMap file. */
	if (file_type == osfile_TYPE_TEXT || file_type == osfile_TYPE_DATA) {
		char *slash = strrchr(path, '/');
		if (slash) {
			error = xmimemaptranslate_extension_to_filetype(
					slash+1, &temp);
			if (error)
				/* ignore error and leave file_type alone */
				NSLOG(netsurf, INFO,
				      "xmimemaptranslate_extension_to_filetype: ""0x%x %s",
				      error->errnum,
				      error->errmess);
			else
				file_type = temp;
		}
	}

	/* search for MIME type in our internal table */
	t = bsearch(&file_type, type_map, TYPE_MAP_COUNT,
				sizeof(type_map[0]), cmp_type);
	if (t) {
		/* found, so return it */
		free(path);
		return t->mime_type;
	}

	/* not in internal table, so ask MimeMap */
	error = xmimemaptranslate_filetype_to_mime_type(file_type, type_buf);
	if (error) {
		NSLOG(netsurf, INFO, "0x%x %s", error->errnum, error->errmess);
		free(path);
		return "application/riscos";
	}
	/* make sure we're NULL terminated. If we're not, the MimeMap
	 * module's probably written past the end of the buffer from
	 * SVC mode. Short of rewriting MimeMap with an incompatible API,
	 * there's nothing we can do about it.
	 */
	type_buf[BUF_SIZE - 1] = '\0';

	free(path);

	NSLOG(netsurf, INFO, "mime type '%s'", type_buf);
	return (const char *)type_buf;

}

/* exported interface documented in riscos/filetype.h */
char *fetch_mimetype(const char *ro_path)
{
	os_error *e;
	bits filetype = 0, load;
	int objtype;
	char *mime = calloc(BUF_SIZE, sizeof(char));
	char *slash;
	struct type_entry *t;

	if (!mime) {
		NSLOG(netsurf, INFO, "Insufficient memory for calloc");
		ro_warn_user("NoMemory", 0);
		return 0;
	}

	e = xosfile_read_no_path(ro_path, &objtype, &load, 0, 0, 0);
	if (e) {
		NSLOG(netsurf, INFO, "xosfile_read_no_path: 0x%x: %s",
		      e->errnum, e->errmess);
		free(mime);
		return 0;
	}

	if (objtype == osfile_IS_DIR) {
		free(mime);
		return 0; /* directories are pointless */
	}

	if ((load >> 20) & 0xFFF) {
		filetype = (load>>8) & 0x000FFF;
	}
	else {
		free(mime);
		return 0; /* no idea */
	}

	/* If filetype is text and the file has an extension, try to map the
	 * extension to a filetype via the MimeMap file. */
	slash = strrchr(ro_path, '/');
	if (slash && filetype == osfile_TYPE_TEXT) {
		e = xmimemaptranslate_extension_to_filetype(slash+1, &load);
		if (e)
			/* if we get an error here, simply ignore it and
			 * leave filetype unchanged */
			NSLOG(netsurf, INFO, "0x%x %s", e->errnum, e->errmess);
		else
			filetype = load;
	}

	/* search for MIME type in our internal table */
	t = bsearch(&filetype, type_map, TYPE_MAP_COUNT,
				sizeof(type_map[0]), cmp_type);
	if (t) {
		/* found, so return it */
		strncpy(mime, t->mime_type, BUF_SIZE);
		return mime;
	}

	/* not in internal table, so ask MimeMap */
	e = xmimemaptranslate_filetype_to_mime_type(filetype, mime);
	if (e) {
		NSLOG(netsurf, INFO,
		      "xmimemaptranslate_filetype_to_mime_type: 0x%x: %s",
		      e->errnum,
		      e->errmess);
		free(mime);
		return 0;
	}
	/* make sure we're NULL terminated. If we're not, the MimeMap
	 * module's probably written past the end of the buffer from
	 * SVC mode. Short of rewriting MimeMap with an incompatible API,
	 * there's nothing we can do about it.
	 */
	mime[BUF_SIZE - 1] = '\0';

	return mime;
}

/**
 * Comparison function for bsearch
 */
int cmp_type(const void *x, const void *y)
{
	const bits *p = x;
	const struct type_entry *q = y;
	return *p < q->file_type ? -1 : (*p == q->file_type ? 0 : +1);
}

/* exported interface documented in riscos/filetype.h */
int ro_content_filetype(struct hlcache_handle *c)
{
	lwc_string *mime_type;
	int file_type;

	file_type = ro_content_filetype_from_type(content_get_type(c));
	if (file_type != 0)
		return file_type;

	mime_type = content_get_mime_type(c);

	file_type = ro_content_filetype_from_mime_type(mime_type);

	lwc_string_unref(mime_type);

	return file_type;
}


/* exported interface documented in riscos/filetype.h */
int ro_content_native_type(struct hlcache_handle *c)
{
	switch (ro_content_filetype(c)) {
	case FILETYPE_JPEG: /* jpeg */
	case FILETYPE_JNG: /* jng */
	case FILETYPE_MNG: /* mng */
	case FILETYPE_GIF: /* gif */
	case FILETYPE_BMP: /* bmp */
	case FILETYPE_ICO: /* ico */
	case FILETYPE_PNG: /* png */
	case FILETYPE_WEBP: /* webp */
	case 0xff9: /* sprite */
		return osfile_TYPE_SPRITE;
	case FILETYPE_SVG: /* svg */
	case 0xaff: /* draw */
		return osfile_TYPE_DRAW;
	default:
		break;
	}

	return osfile_TYPE_DATA;
}


/* exported interface documented in riscos/filetype.h */
int ro_content_filetype_from_mime_type(lwc_string *mime_type)
{
	int file_type, index;
	os_error *error;

	/* Search internal type map */
	for (index = TYPE_MAP_COUNT; index > 0; index--) {
		const struct type_entry *e = &type_map[index - 1];

		if (strlen(e->mime_type) == lwc_string_length(mime_type) &&
				strncasecmp(e->mime_type,
				lwc_string_data(mime_type),
				lwc_string_length(mime_type)) == 0)
			return e->file_type;
	}

	/* Ask MimeMap module */
	error = xmimemaptranslate_mime_type_to_filetype(
			lwc_string_data(mime_type), (bits *) &file_type);
	if (error)
		file_type = 0xffd;

	return file_type;
}


/* exported interface documented in riscos/filetype.h */
int ro_content_filetype_from_type(content_type type) {
	switch (type) {
	case CONTENT_HTML:	return FILETYPE_HTML;
	case CONTENT_TEXTPLAIN:	return 0xfff;
	case CONTENT_CSS:	return 0xf79;
	default:		break;
	}
	return 0;
}


/* exported interface documented in riscos/filetype.h */
bits ro_filetype_from_unix_path(const char *unix_path)
{
	unsigned int len = strlen(unix_path) + 100;
	char *path = calloc(len, 1);
	char *r;
	os_error *error;
	bits file_type;

	if (!path) {
		NSLOG(netsurf, INFO, "Insufficient memory for calloc");
		ro_warn_user("NoMemory", 0);
		return osfile_TYPE_DATA;
	}

	/* convert path to RISC OS format and read file type */
	r = __riscosify(unix_path, 0, __RISCOSIFY_NO_SUFFIX, path, len, 0);
	if (r == 0) {
		NSLOG(netsurf, INFO, "__riscosify failed");
		free(path);
		return osfile_TYPE_DATA;
	}

	error = xosfile_read_stamped_no_path(path, 0, 0, 0, 0, 0,
			&file_type);
	if (error) {
		NSLOG(netsurf, INFO,
		      "xosfile_read_stamped_no_path failed: %s",
		      error->errmess);
		free(path);
		return osfile_TYPE_DATA;
	}

	free(path);

	return file_type;
}

