/*
 * This file is part of Pencil
 * Licensed under the MIT License,
 *                http://www.opensource.org/licenses/mit-license
 * Copyright 2005 James Bursa <james@semichrome.net>
 */

#include <assert.h>
#include <stdio.h>
#include <oslib/osfile.h>
#include <oslib/osspriteop.h>
#include <oslib/squash.h>
#include <rufl.h>
#include "pencil.h"


#define SPRITE "Resources:$.Resources.Desktop.Sprites"


static void test_pencil(void);


int main(int argc, char *argv[])
{
	rufl_code code;

	(void) argc;
	(void) argv;

	code = rufl_init();
	if (code != rufl_OK) {
		printf("rufl_init failed: %i\n", code);
		return 1;
	}

	test_pencil();

	rufl_quit();

	return 0;
}


void test_pencil(void)
{
	struct pencil_diagram *diagram;
	pencil_code code;
	int path[] = {2, 100, 40, 8, 100, 400, 8, 300, 300, 0};
	char utf8_test[] = "Hello,	world! ὕαλον "
			"Uherské Hradiště. 𐀀\xf0\xa0\x80\xa1";
	char *drawfile_buffer;
	size_t drawfile_size;
	os_error *error;
	fileswitch_object_type obj_type;
	int size;
	bits load;
	osspriteop_area *area;

	diagram = pencil_create();
	if (!diagram) {
		printf("pencil_create failed\n");
		return;
	}

	code = pencil_text(diagram,
			100, 40,
			"Homerton", rufl_WEIGHT_400,
			320,
			"Hello, world!", 13,
			0x000000);
	if (code != pencil_OK) {
		printf("pencil_text failed: %i\n", code);
		return;
	}

	code = pencil_path(diagram,
			path, sizeof path / sizeof path[0],
			0x00ff00, 0x0000ff,
			5, pencil_JOIN_ROUND,
			pencil_CAP_BUTT, pencil_CAP_TRIANGLE,
			15, 20,
			false, pencil_SOLID);
	if (code != pencil_OK) {
		printf("pencil_path failed: %i\n", code);
		return;
	}

	code = pencil_text(diagram,
			100, 400,
			"NewHall", rufl_WEIGHT_400,
			320,
			utf8_test, sizeof utf8_test,
			0xff0000);
	if (code != pencil_OK) {
		printf("pencil_text failed: %i\n", code);
		return;
	}

	error = xosfile_read_no_path(SPRITE, &obj_type, &load, 0, &size, 0);
	if (error) {
		printf("xosfile_read_no_path failed: 0x%x: %s\n",
				error->errnum, error->errmess);
		return;
	}
	if (obj_type != fileswitch_IS_FILE) {
		printf("File " SPRITE " does not exist\n");
		return;
	}
	if ((load & 0xfff00000) != 0xfff00000 ||
			((load & 0xfff00) != 0xff900 &&
			 (load & 0xfff00) != 0xfca00)) {
		printf("File " SPRITE " is not a sprite file\n");
		return;
	}

	if ((load & 0xfff00) == 0xfca00) {
		/* File is squashed */
		squash_output_status status;
		struct squash_file_base sf;
		char *buf, *ws;
		int wslen;
		FILE *fp;

		fp = fopen(SPRITE, "r");
		if (!fp) {
			printf("Failed opening " SPRITE "\n");
			return;
		}
		if (fread(&sf, 1, sizeof(sf), fp) != sizeof(sf)) {
			printf("Failed loading squash header\n");
			fclose(fp);
			return;
		}
		if ((sf.load_addr & 0xffffff00) != 0xfffff900) {
			printf("File " SPRITE " is not a sprite file\n");
			fclose(fp);
			return;
		}

		buf = malloc(size - sizeof(sf));
		if (!buf) {
			printf("Out of memory\n");
			fclose(fp);
			return;
		}
		if (fread(buf, 1, size - sizeof(sf), fp) != size - sizeof(sf)) {
			printf("Failed reading squashed data\n");
			free(buf);
			fclose(fp);
			return;
		}

		error = xsquash_decompress_return_sizes(size - sizeof(sf),
				&wslen, NULL);
		if (error) {
			printf("xsquash_decompress_return_sizes failed: 0x%x: %s\n",
					error->errnum, error->errmess);
			free(buf);
			fclose(fp);
			return;
		}

		ws = malloc(wslen);
		if (!ws) {
			printf("Out of memory\n");
			free(buf);
			fclose(fp);
			return;
		}

		area = malloc(sf.size + 4);
		if (!area) {
			printf("Out of memory\n");
			free(ws);
			free(buf);
			fclose(fp);
			return;
		}
		area->size = sf.size + 4;

		error = xsquash_decompress(squash_INPUT_ALL_PRESENT, ws,
				(byte *) buf, size - sizeof(sf),
				(byte *) &area->sprite_count, sf.size,
				&status, NULL, NULL, NULL, NULL);
		if (error) {
			printf("xsquash_decompress failed: 0x%x: %s\n",
					error->errnum, error->errmess);
			free(area);
			free(ws);
			free(buf);
			fclose(fp);
			return;
		}
		if (status != 0) {
			printf("xsquash_decompress did not complete: %x\n", status);
			free(area);
			free(ws);
			free(buf);
			fclose(fp);
			return;
		}

		free(ws);
		free(buf);
		fclose(fp);
	} else {
		area = malloc(size + 4);
		if (!area) {
			printf("Out of memory\n");
			return;
		}
		area->size = size + 4;
		area->sprite_count = 0;
		area->first = 0;
		area->used = 16;

		error = xosspriteop_load_sprite_file(osspriteop_USER_AREA,
				area, SPRITE);
		if (error) {
			printf("xosspriteop_load_sprite_file failed: 0x%x: %s\n",
					error->errnum, error->errmess);
			return;
		}
	}

	code = pencil_sprite(diagram, 400, 200, 200, 100,
			((char *) area) + area->first);
	if (code != pencil_OK) {
		printf("pencil_sprite failed: %i\n", code);
		return;
	}

	pencil_dump(diagram);

	code = pencil_save_drawfile(diagram, "Pencil-Test",
			&drawfile_buffer, &drawfile_size);
	if (code != pencil_OK) {
		printf("pencil_save_drawfile failed: %i\n", code);
		return;
	}
	assert(drawfile_buffer);

	error = xosfile_save_stamped("DrawFile", osfile_TYPE_DRAW,
			(byte *) drawfile_buffer, 
			(byte *) drawfile_buffer + drawfile_size);
	if (error) {
		printf("xosfile_save_stamped failed: 0x%x: %s\n",
				error->errnum, error->errmess);
		return;
	}

	pencil_free(diagram);
}
