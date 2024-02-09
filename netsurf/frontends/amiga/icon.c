/*
 * Copyright 2010 Chris Young <chris@unsatisfactorysoftware.co.uk>
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
 * Content for image/x-amiga-icon (icon.library implementation).
 *
 */

#include "utils/config.h"

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include <proto/exec.h>
#include <proto/icon.h>

#include <datatypes/pictureclass.h>
#ifdef __amigaos4__
#include <graphics/blitattr.h>
#endif
#include <workbench/icon.h>

#include "utils/log.h"
#include "utils/messages.h"
#include "utils/utils.h"
#include "utils/file.h"
#include "netsurf/plotters.h"
#include "netsurf/bitmap.h"
#include "netsurf/content.h"
#include "content/content.h"
#include "content/content_protected.h"
#include "content/content_factory.h"
#include "desktop/gui_internal.h"

#include "amiga/os3support.h"
#include "amiga/bitmap.h"
#include "amiga/icon.h"

#define THUMBNAIL_WIDTH 100 /* Icon sizes for thumbnails, usually the same as */
#define THUMBNAIL_HEIGHT 86 /* WIDTH/HEIGHT in desktop/thumbnail.c */

static ULONG *amiga_icon_convertcolouricon32(UBYTE *icondata, ULONG width, ULONG height,
		ULONG trans, ULONG pals1, struct ColorRegister *pal1, int alpha);

#ifdef WITH_AMIGA_ICON

typedef struct amiga_icon_content {
	struct content base;

	struct bitmap *bitmap;	/**< Created NetSurf bitmap */
} amiga_icon_content;

static nserror amiga_icon_create(const content_handler *handler,
		lwc_string *imime_type, const struct http_parameter *params,
		struct llcache_handle *llcache, const char *fallback_charset,
		bool quirks, struct content **c);
static bool amiga_icon_convert(struct content *c);
static void amiga_icon_destroy(struct content *c);
static bool amiga_icon_redraw(struct content *c,
		struct content_redraw_data *data, const struct rect *clip,
		const struct redraw_context *ctx);
static nserror amiga_icon_clone(const struct content *old, 
		struct content **newc);
static content_type amiga_icon_content_type(void);

static void *amiga_icon_get_internal(const struct content *c, void *context)
{
	amiga_icon_content *icon_c = (amiga_icon_content *)c;

	return icon_c->bitmap;
}

static bool amiga_icon_is_opaque(struct content *c)
{
	amiga_icon_content *icon_c = (amiga_icon_content *)c;

	if (icon_c->bitmap != NULL) {
		return guit->bitmap->get_opaque(icon_c->bitmap);
	}

	return false;
}

static const content_handler amiga_icon_content_handler = {
	.create = amiga_icon_create,
	.data_complete = amiga_icon_convert,
	.destroy = amiga_icon_destroy,
	.redraw = amiga_icon_redraw,
	.clone = amiga_icon_clone,
	.get_internal = amiga_icon_get_internal,
	.type = amiga_icon_content_type,
	.is_opaque = amiga_icon_is_opaque,
	.no_share = false,
};

static const char *amiga_icon_types[] = {
	"image/x-amiga-icon"
};

CONTENT_FACTORY_REGISTER_TYPES(amiga_icon, amiga_icon_types, 
		amiga_icon_content_handler)

nserror amiga_icon_create(const content_handler *handler,
		lwc_string *imime_type, const struct http_parameter *params,
		struct llcache_handle *llcache, const char *fallback_charset,
		bool quirks, struct content **c)
{
	amiga_icon_content *ai_content;
	nserror error;

	ai_content = calloc(1, sizeof(amiga_icon_content));
	if (ai_content == NULL)
		return NSERROR_NOMEM;

	error = content__init(&ai_content->base, handler, imime_type, params,
			llcache, fallback_charset, quirks);
	if (error != NSERROR_OK) {
		free(ai_content);
		return error;
	}

	*c = (struct content *)ai_content;

	return NSERROR_OK;
}

/**
 * Convert a CONTENT_AMIGA_ICON for display.
 *
 * No conversion is necessary. We merely read the icon dimensions.
 */

bool amiga_icon_convert(struct content *c)
{
	amiga_icon_content *icon_c = (amiga_icon_content *)c;	
	union content_msg_data msg_data;
	struct DiskObject *dobj;
	ULONG *imagebuf;
	unsigned char *imagebufptr = NULL;
	ULONG size;
	int width = 0, height = 0;
	long format = 0;
	uint8 r, g, b, a;
	ULONG offset;
	char *filename = NULL;
	char *p;
	ULONG trans, pals1;
	struct ColorRegister *pal1;

	netsurf_nsurl_to_path(content_get_url(c), &filename);
	/* This loader will only work on local files, so fail if not a local path */
	if(filename == NULL)
	{
		msg_data.errordata.errorcode = NSERROR_NOMEM;
		msg_data.errordata.errormsg = messages_get("NoMemory");
		content_broadcast(c, CONTENT_MSG_ERROR, &msg_data);
		return false;
	}

	p = strstr(filename, ".info");
	*p = '\0';

	dobj = GetIconTagList(filename, NULL);	

	if(dobj == NULL)
	{
		msg_data.errordata.errorcode = NSERROR_NOMEM;
		msg_data.errordata.errormsg = messages_get("NoMemory");
		content_broadcast(c, CONTENT_MSG_ERROR, &msg_data);
		return false;
	}

	IconControl(dobj,
			ICONCTRLA_GetImageDataFormat,&format,
			ICONCTRLA_GetWidth,&width,
			ICONCTRLA_GetHeight,&height,
			TAG_DONE);

	/* Check icon is direct mapped (truecolour) or palette-mapped colour.
	   We need additional code to handle planar icons */
	if((format != IDFMT_DIRECTMAPPED) && (format==IDFMT_PALETTEMAPPED)) {
		if(dobj) FreeDiskObject(dobj);
		return false;
	}

	icon_c->bitmap = amiga_bitmap_create(width, height, BITMAP_NONE);
	if (!icon_c->bitmap) {
		msg_data.errordata.errorcode = NSERROR_NOMEM;
		msg_data.errordata.errormsg = messages_get("NoMemory");
		content_broadcast(c, CONTENT_MSG_ERROR, &msg_data);
		if(dobj) FreeDiskObject(dobj);
		return false;
	}
	imagebuf = (ULONG *) amiga_bitmap_get_buffer(icon_c->bitmap);
	if (!imagebuf) {
		msg_data.errordata.errorcode = NSERROR_NOMEM;
		msg_data.errordata.errormsg = messages_get("NoMemory");
		content_broadcast(c, CONTENT_MSG_ERROR, &msg_data);
		if(dobj) FreeDiskObject(dobj);
		return false;
	}

	IconControl(dobj,
			ICONCTRLA_GetImageData1, &imagebufptr,
			TAG_DONE);

	if(format==IDFMT_PALETTEMAPPED)
	{
		IconControl(dobj, ICONCTRLA_GetTransparentColor1, &trans,
		            ICONCTRLA_GetPalette1, &pal1,
	    	        ICONCTRLA_GetPaletteSize1, &pals1,
    	    	    TAG_DONE);

		imagebufptr = (unsigned char *) amiga_icon_convertcolouricon32((UBYTE *)imagebufptr,
						width, height, trans, pals1, pal1, 0xff);
	}

	/* Decoded data is ARGB, so ensure correct byte order */

	size = width * height * 4;

	for (offset = 0; offset < size; offset += 4) {
		b = imagebufptr[offset+3];
		g = imagebufptr[offset+2];
		r = imagebufptr[offset+1];
		a = imagebufptr[offset];

		*imagebuf = r << 24 | g << 16 | b << 8 | a;
		imagebuf++;
	}

	c->width = width;
	c->height = height;

	amiga_bitmap_modified(icon_c->bitmap);
	content_set_ready(c);
	content_set_done(c);
	content_set_status(c, "");

	if(dobj) FreeDiskObject(dobj);

	if(format==IDFMT_PALETTEMAPPED)
		free(imagebufptr);

	return true;
}


/**
 * Destroy a CONTENT_AMIGA_ICON and free all resources it owns.
 */

void amiga_icon_destroy(struct content *c)
{
	amiga_icon_content *icon_c = (amiga_icon_content *)c;	

	if (icon_c->bitmap != NULL)
		amiga_bitmap_destroy(icon_c->bitmap);
}


/**
 * Redraw a CONTENT_AMIGA_ICON.
 */

bool amiga_icon_redraw(struct content *c,
		struct content_redraw_data *data, const struct rect *clip,
		const struct redraw_context *ctx)
{
	amiga_icon_content *icon_c = (amiga_icon_content *)c;	
	bitmap_flags_t flags = BITMAPF_NONE;

	if (data->repeat_x)
		flags |= BITMAPF_REPEAT_X;
	if (data->repeat_y)
		flags |= BITMAPF_REPEAT_Y;

	return (ctx->plot->bitmap(ctx,
				  icon_c->bitmap,
				  data->x,
				  data->y,
				  data->width,
				  data->height,
				  data->background_colour,
				  flags) == NSERROR_OK);
}


nserror amiga_icon_clone(const struct content *old, struct content **newc)
{
	amiga_icon_content *ai;
	nserror error;

	ai = calloc(1, sizeof(amiga_icon_content));
	if (ai == NULL)
		return NSERROR_NOMEM;

	error = content__clone(old, &ai->base);
	if (error != NSERROR_OK) {
		content_destroy(&ai->base);
		return error;
	}

	/* Simply replay convert */
	if (old->status == CONTENT_STATUS_READY ||
			old->status == CONTENT_STATUS_DONE) {
		if (amiga_icon_convert(&ai->base) == false) {
			content_destroy(&ai->base);
			return NSERROR_CLONE_FAILED;
		}
	}

	*newc = (struct content *) ai;

	return NSERROR_OK;
}

content_type amiga_icon_content_type(void)
{
	return CONTENT_IMAGE;
}

#endif /* WITH_AMIGA_ICON */

static ULONG *amiga_icon_convertcolouricon32(UBYTE *icondata, ULONG width, ULONG height,
		ULONG trans, ULONG pals1, struct ColorRegister *pal1, int alpha)
{
	ULONG *argbicon;
	struct ColorRegister *colour;
	struct ColorMap *cmap;
	ULONG i;
	ULONG a,r,g,b;

	if (alpha==0) alpha=0xff;

	argbicon = (ULONG *)malloc(width * height * 4);
	if (!argbicon) return(NULL);

	cmap=GetColorMap(pals1);
	if(!cmap) {
		free(argbicon);
		return(NULL);
	}

	for(i=0;i<(width*height);i++)
	{
		colour = &pal1[icondata[i]];

		if(icondata[i] == trans)
		{
			a=0x00;
		}
		else
		{
			a=alpha;
		}

		r = colour->red;
		g = colour->green;
		b = colour->blue;

		argbicon[i] = (a << 24) + 
		              (r << 16) +
		              (g << 8) +
		              (b);
	}

	return(argbicon);

}

void amiga_icon_superimpose_favicon_internal(struct hlcache_handle *icon, struct DiskObject *dobj)
{
	struct BitMap *bm = NULL;
	ULONG *icondata1, *icondata2;
	ULONG width, height;
	long format = 0;

	if(dobj == NULL) return;

	IconControl(dobj,
                  ICONCTRLA_GetImageDataFormat,&format,
                  ICONCTRLA_GetImageData1,&icondata1,
                  ICONCTRLA_GetImageData2,&icondata2,
                  ICONCTRLA_GetWidth,&width,
                  ICONCTRLA_GetHeight,&height,
                  TAG_DONE);

	if(format != IDFMT_DIRECTMAPPED) return;
#ifdef __amigaos4__
	if ((icon != NULL) && (content_get_bitmap(icon) != NULL)) {
		bm = ami_bitmap_get_native(content_get_bitmap(icon), 16, 16, false, NULL);
	}

	if(bm) {
		BltBitMapTags(BLITA_SrcX, 0,
					BLITA_SrcY, 0,
					BLITA_DestX, width - 16,
					BLITA_DestY, height - 16,
					BLITA_Width, 16,
					BLITA_Height, 16,
					BLITA_Source, bm,
					BLITA_Dest, icondata1,
					BLITA_SrcType, BLITT_BITMAP,
					BLITA_DestType, BLITT_ARGB32,
					BLITA_DestBytesPerRow, width * 4,
					BLITA_UseSrcAlpha, TRUE,
					TAG_DONE);

		BltBitMapTags(BLITA_SrcX, 0,
					BLITA_SrcY, 0,
					BLITA_DestX, width - 16,
					BLITA_DestY, height - 16,
					BLITA_Width, 16,
					BLITA_Height, 16,
					BLITA_Source, bm,
					BLITA_Dest, icondata2,
					BLITA_SrcType, BLITT_BITMAP,
					BLITA_DestType, BLITT_ARGB32,
					BLITA_DestBytesPerRow, width * 4,
					BLITA_UseSrcAlpha, TRUE,
					TAG_DONE);
	}
#endif
}

void amiga_icon_superimpose_favicon(char *path, struct hlcache_handle *icon, char *type)
{
	struct DiskObject *dobj = NULL;
	ULONG *icondata1, *icondata2;
	ULONG width, height;
	long format = 0;
	ULONG trans1, pals1;
	ULONG trans2, pals2;
	struct ColorRegister *pal1;
	struct ColorRegister *pal2;

	if(icon == NULL) return;

	if(!type)
	{
		dobj = GetIconTags(NULL,
						ICONGETA_GetDefaultType, WBDRAWER,
					    TAG_DONE);
	}
	else
	{
		dobj = GetIconTags(NULL, ICONGETA_GetDefaultName, type,
					    ICONGETA_GetDefaultType, WBPROJECT,
					    TAG_DONE);
	}

	if(dobj == NULL) return;

	IconControl(dobj,
                  ICONCTRLA_GetImageDataFormat,&format,
                  ICONCTRLA_GetImageData1,&icondata1,
                  ICONCTRLA_GetImageData2,&icondata2,
                  ICONCTRLA_GetWidth,&width,
                  ICONCTRLA_GetHeight,&height,
                  TAG_DONE);

	/* If we have a palette-mapped icon, convert it to a 32-bit one */
	if(format == IDFMT_PALETTEMAPPED)
	{
		IconControl(dobj, ICONCTRLA_GetTransparentColor1, &trans1,
		            ICONCTRLA_GetPalette1, &pal1,
	    	        ICONCTRLA_GetPaletteSize1, &pals1,
					ICONCTRLA_GetTransparentColor2, &trans2,
		            ICONCTRLA_GetPalette2, &pal2,
	    	        ICONCTRLA_GetPaletteSize2, &pals2,
    	    	    TAG_DONE);

		icondata1 = amiga_icon_convertcolouricon32((UBYTE *)icondata1,
						width, height, trans1, pals1, pal1, 0xff);

		icondata2 = amiga_icon_convertcolouricon32((UBYTE *)icondata2,
						width, height, trans2, pals2, pal2, 0xff);

		IconControl(dobj,
                  ICONCTRLA_SetImageDataFormat, IDFMT_DIRECTMAPPED,
                  ICONCTRLA_SetImageData1, icondata1,
                  ICONCTRLA_SetImageData2, icondata2,
                  TAG_DONE);
	}

	if((format == IDFMT_DIRECTMAPPED) || (format == IDFMT_PALETTEMAPPED))
		amiga_icon_superimpose_favicon_internal(icon, dobj);

	PutIconTags(path, dobj,
			ICONPUTA_NotifyWorkbench, TRUE, TAG_DONE);

	FreeDiskObject(dobj);

	if(format == IDFMT_PALETTEMAPPED)
	{
		/* Free the 32-bit data we created */
		free(icondata1);
		free(icondata2);
	}
}

struct DiskObject *amiga_icon_from_bitmap(struct bitmap *bm)
{
	struct DiskObject *dobj;
	struct BitMap *bitmap;
	ULONG *icondata;

#ifdef __amigaos4__
	if(bm)
	{
		bitmap = ami_bitmap_get_native(bm, THUMBNAIL_WIDTH,
									THUMBNAIL_HEIGHT, false, NULL);
		icondata = malloc(THUMBNAIL_WIDTH * 4 * THUMBNAIL_HEIGHT);
		ami_bitmap_set_icondata(bm, icondata);

		if(bitmap) {
			BltBitMapTags(BLITA_Width, THUMBNAIL_WIDTH,
						BLITA_Height, THUMBNAIL_HEIGHT,
						BLITA_SrcType, BLITT_BITMAP,
						BLITA_Source, bitmap,
						BLITA_DestType, BLITT_ARGB32,
						BLITA_DestBytesPerRow, THUMBNAIL_WIDTH * 4,
						BLITA_Dest, icondata,
					TAG_DONE);
		}
	}
#endif
	dobj = GetIconTags(NULL, ICONGETA_GetDefaultType, WBPROJECT,
						ICONGETA_GetDefaultName, "iconify",
						TAG_DONE);
#ifdef __amigaos4__
	if(bm)
	{
		IconControl(dobj,
			ICONCTRLA_SetImageDataFormat, IDFMT_DIRECTMAPPED,
			ICONCTRLA_SetWidth, THUMBNAIL_WIDTH,
			ICONCTRLA_SetHeight, THUMBNAIL_HEIGHT,
			ICONCTRLA_SetImageData1, icondata,
			ICONCTRLA_SetImageData2, NULL,
			TAG_DONE);
	}
#endif
	dobj->do_Gadget.UserData = bm;

	LayoutIconA(dobj, (struct Screen *)~0UL, NULL);

	return dobj;
}

void amiga_icon_free(struct DiskObject *dobj)
{
	struct bitmap *bm = dobj->do_Gadget.UserData;

	FreeDiskObject(dobj);
	if(bm) ami_bitmap_free_icondata(bm);
}

