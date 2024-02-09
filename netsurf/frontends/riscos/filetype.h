/*
 * Copyright 2014 Vincent Sanders <vince@netsurf-browser.org>
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

/** \file riscos/filetype.h
 * RISC OS filetpe interface.
 */

#ifndef _NETSURF_RISCOS_FILETYPE_H_
#define _NETSURF_RISCOS_FILETYPE_H_

#include "netsurf/content_type.h"

#ifndef FILETYPE_ACORN_URI
#define FILETYPE_ACORN_URI 0xf91
#endif
#ifndef FILETYPE_ANT_URL
#define FILETYPE_ANT_URL 0xb28
#endif
#ifndef FILETYPE_IEURL
#define FILETYPE_IEURL 0x1ba
#endif
#ifndef FILETYPE_HTML
#define FILETYPE_HTML 0xfaf
#endif
#ifndef FILETYPE_JNG
#define FILETYPE_JNG 0xf78
#endif
#ifndef FILETYPE_CSS
#define FILETYPE_CSS 0xf79
#endif
#ifndef FILETYPE_MNG
#define FILETYPE_MNG 0xf83
#endif
#ifndef FILETYPE_GIF
#define FILETYPE_GIF 0x695
#endif
#ifndef FILETYPE_BMP
#define FILETYPE_BMP 0x69c
#endif
#ifndef FILETYPE_ICO
#define FILETYPE_ICO 0x132
#endif
#ifndef FILETYPE_PNG
#define FILETYPE_PNG 0xb60
#endif
#ifndef FILETYPE_JPEG
#define FILETYPE_JPEG 0xc85
#endif
#ifndef FILETYPE_ARTWORKS
#define FILETYPE_ARTWORKS 0xd94
#endif
#ifndef FILETYPE_SVG
#define FILETYPE_SVG 0xaad
#endif
#ifndef FILETYPE_WEBP
#define FILETYPE_WEBP 0xa66
#endif

/**
 * Determine the MIME type of a local file.
 *
 * \param unix_path Unix style path to file on disk
 * \return Pointer to MIME type string (should not be freed) - invalidated
 *	   on next call to fetch_filetype.
 */
const char *fetch_filetype(const char *unix_path);

/**
 * Find a MIME type for a local file
 *
 * \param ro_path RISC OS style path to file on disk
 * \return MIME type string (on heap, caller should free), or NULL
 */
char *fetch_mimetype(const char *ro_path);

/**
 * Determine the RISC OS filetype for a content.
 *
 * \param h The handle of the content to examine.
 * \return The RISC OS filetype corresponding to the content
 */
int ro_content_filetype(struct hlcache_handle *h);

/**
 * Determine the native RISC OS filetype to export a content as
 *
 * \param c  The content to examine
 * \return Native RISC OS filetype for export
 */
int ro_content_native_type(struct hlcache_handle *c);

/**
 * Determine the RISC OS filetype for a MIME type
 *
 * \param mime_type  MIME type to consider
 * \return Corresponding RISC OS filetype
 */
int ro_content_filetype_from_mime_type(lwc_string *mime_type);

/**
 * Determine the RISC OS filetype from a content type.
 *
 * \param type The content type to examine.
 * \return The RISC OS filetype corresponding to the content, or 0 for unknown
 */
int ro_content_filetype_from_type(content_type type);

/**
 * Determine the type of a local file.
 *
 * \param unix_path Unix style path to file on disk
 * \return File type
 */
bits ro_filetype_from_unix_path(const char *unix_path);

#endif
