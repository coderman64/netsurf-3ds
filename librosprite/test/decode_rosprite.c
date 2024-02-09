/*
 * Copyright 2018 Vincent Sanders <vince@netsurf-browser.org>
 *
 * This file is part of NetSurf's librosprite, http://www.netsurf-browser.org/
 * Licenced under the MIT License,
 *                http://www.opensource.org/licenses/mit-license.php
 */

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include "../include/librosprite.h"


static void write_ppm(FILE* fh, const char *srcname, struct rosprite* sprite)
{
        fprintf(fh, "P3\n");
        fprintf(fh, "# %s\n", srcname);

        fprintf(fh, "# name %s\n", sprite->name);
        fprintf(fh, "# color_model %s\n",
                sprite->mode.color_model == ROSPRITE_RGB ? "RGB" : "CMYK");
        fprintf(fh, "# colorbpp %u\n", sprite->mode.colorbpp);
        fprintf(fh, "# xdpi %u\n", sprite->mode.xdpi);
        fprintf(fh, "# ydpi %u\n", sprite->mode.ydpi);
        fprintf(fh, "# width %u px\n", sprite->width);
        fprintf(fh, "# height %u px\n", sprite->height);
	
        fprintf(fh, "# hasPalette %s\n", sprite->has_palette ? "YES" : "NO");
        if (sprite->has_palette) {
                fprintf(fh, "# paletteSize %u\n", sprite->palettesize);
        }
        fprintf(fh, "# hasMask %s\n", sprite->has_mask ? "YES" : "NO");
        if (sprite->has_mask) {
                fprintf(fh, "# mask_width %u\n", sprite->mode.mask_width);
        }
        if (sprite->has_mask) {
                fprintf(fh, "# maskbpp %u\n", sprite->mode.maskbpp);
        }

        fprintf(fh, "%u %u 256\n", sprite->width, sprite->height);


        for (uint32_t y = 0; y < sprite->height; y++) {
                for (uint32_t x = 0; x < sprite->width; x++) {
                        uint32_t color; /* color is 0xrrggbbaa */

                        color = sprite->image[y*sprite->width + x];
                        fprintf(fh, "%u %u %u ",
                                (color & 0xff000000) >> 24,
                                (color & 0x00ff0000) >> 16,
                                (color & 0x0000ff00) >> 8);

                }
                fprintf(fh, "\n");
        }
        
}


int main(int argc, char *argv[])
{

        int res = 0;
        FILE *inf;
        FILE *outf = stdout;
        struct rosprite_file_context *ctx;
        struct rosprite_area *sprite_area;
        unsigned int sprite_number = 0;/* number of sprite in sprite area to convert */

        if (argc < 2) {
                fprintf(stderr, "Usage: %s image.spr [out]\n", argv[0]);
                return 1;
        }

        inf = fopen(argv[1], "rb");
        if (inf == NULL) {
                fprintf(stderr, "Unable to open %s for reading\n", argv[1]);
                return 3;
        }

        if (argc > 2) {
                outf = fopen(argv[2], "w+");
                if (outf == NULL) {
                        fprintf(stderr,
                                "Unable to open %s for writing\n", argv[2]);
                        fclose(inf);
                        return 2;
                }
        }

        if (rosprite_create_file_context(inf, &ctx) != ROSPRITE_OK) {
                fprintf(stderr, "Unable to create file context\n");
                res = 4;
                goto cleanup;
        }

        /* load sprites into sprite area */
        if (rosprite_load(rosprite_file_reader,
                          ctx,
                          &sprite_area) != ROSPRITE_OK) {
                fprintf(stderr, "Error loading spritefile\n");
                res = 5;
                goto cleanup;
        };

        if (sprite_number >= sprite_area->sprite_count) {
                fprintf(stderr,
                       "Sprite %d of %d is not present in sprite pool\n",
                       sprite_number, sprite_area->sprite_count);
                res = 6;
                goto cleanup;
        }

        /* write out sprite */
        write_ppm(outf, argv[1], sprite_area->sprites[sprite_number]);

        rosprite_destroy_file_context(ctx);
        rosprite_destroy_sprite_area(sprite_area);

cleanup:

        fclose(inf);

        if (argc > 2) {
                fclose(outf);
        }

        return res;
}
