/*
 * This file is part of Libsvgtiny
 * Licensed under the MIT License,
 *                http://opensource.org/licenses/mit-license.php
 * Copyright 2008 James Bursa <james@semichrome.net>
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include "svgtiny.h"


static void write_mvg(FILE *fh, float scale, struct svgtiny_diagram *diagram)
{
        unsigned int i;

        fprintf(fh, "viewbox 0 0 %g %g\n",
               scale * diagram->width, scale * diagram->height);

        for (i = 0; i != diagram->shape_count; i++) {
                if (diagram->shape[i].fill == svgtiny_TRANSPARENT) {
                        fprintf(fh, "fill none ");
                } else {
                        fprintf(fh, "fill #%.6x ", diagram->shape[i].fill);
                }

                if (diagram->shape[i].stroke == svgtiny_TRANSPARENT) {
                        fprintf(fh, "stroke none ");
                } else {
                        fprintf(fh, "stroke #%.6x ", diagram->shape[i].stroke);
                }
                fprintf(fh, "stroke-width %g ",
                       scale * diagram->shape[i].stroke_width);

                if (diagram->shape[i].path) {
                        unsigned int j;
                        fprintf(fh, "path '");
                        for (j = 0; j != diagram->shape[i].path_length; ) {
                                switch ((int) diagram->shape[i].path[j]) {
                                case svgtiny_PATH_MOVE:
                                        fprintf(fh, "M %g %g ",
                                               scale * diagram->shape[i].path[j + 1],
                                               scale * diagram->shape[i].path[j + 2]);
                                        j += 3;
                                        break;

                                case svgtiny_PATH_CLOSE:
                                        fprintf(fh, "Z ");
                                        j += 1;
                                        break;

                                case svgtiny_PATH_LINE:
                                        fprintf(fh, "L %g %g ",
                                               scale * diagram->shape[i].path[j + 1],
                                               scale * diagram->shape[i].path[j + 2]);
                                        j += 3;
                                        break;

                                case svgtiny_PATH_BEZIER:
                                        fprintf(fh, "C %g %g %g %g %g %g ",
                                               scale * diagram->shape[i].path[j + 1],
                                               scale * diagram->shape[i].path[j + 2],
                                               scale * diagram->shape[i].path[j + 3],
                                               scale * diagram->shape[i].path[j + 4],
                                               scale * diagram->shape[i].path[j + 5],
                                               scale * diagram->shape[i].path[j + 6]);
                                        j += 7;
                                        break;

                                default:
                                        fprintf(fh, "error ");
                                        j += 1;
                                }
                        }
                        fprintf(fh, "' ");
                } else if (diagram->shape[i].text) {
                        fprintf(fh, "text %g %g '%s' ",
                               scale * diagram->shape[i].text_x,
                               scale * diagram->shape[i].text_y,
                               diagram->shape[i].text);
                }
                fprintf(fh, "\n");
        }
}

int main(int argc, char *argv[])
{
        FILE *fd;
        float scale = 1.0;
        struct stat sb;
        char *buffer;
        size_t size;
        size_t n;
        struct svgtiny_diagram *diagram;
        svgtiny_code code;
        FILE *outf = stdout;

        if (argc < 2 || argc > 4) {
                fprintf(stderr, "Usage: %s FILE [SCALE] [out]\n", argv[0]);
                return 1;
        }

        /* load file into memory buffer */
        fd = fopen(argv[1], "rb");
        if (!fd) {
                perror(argv[1]);
                return 1;
        }

        if (stat(argv[1], &sb)) {
                perror(argv[1]);
                return 1;
        }
        size = sb.st_size;

        buffer = malloc(size);
        if (!buffer) {
                fprintf(stderr, "Unable to allocate %lld bytes\n",
                                (long long) size);
                return 1;
        }

        n = fread(buffer, 1, size, fd);
        if (n != size) {
                perror(argv[1]);
                return 1;
        }

        fclose(fd);

        /* read scale argument */
        if (argc > 2) {
                scale = atof(argv[2]);
                if (scale == 0)
                        scale = 1.0;
        }

        /* output file */
        if (argc > 3) {
                outf = fopen(argv[3], "w+");
                if (outf == NULL) {
                        fprintf(stderr, "Unable to open %s for writing\n", argv[3]);
                        return 2;
                }
        }

        /* create svgtiny object */
        diagram = svgtiny_create();
        if (!diagram) {
                fprintf(stderr, "svgtiny_create failed\n");
                return 1;
        }

        /* parse */
        code = svgtiny_parse(diagram, buffer, size, argv[1], 1000, 1000);
        if (code != svgtiny_OK) {
                fprintf(stderr, "svgtiny_parse failed: ");
                switch (code) {
                case svgtiny_OUT_OF_MEMORY:
                        fprintf(stderr, "svgtiny_OUT_OF_MEMORY");
                        break;
                case svgtiny_LIBDOM_ERROR:
                        fprintf(stderr, "svgtiny_LIBDOM_ERROR");
                        break;
                case svgtiny_NOT_SVG:
                        fprintf(stderr, "svgtiny_NOT_SVG");
                        break;
                case svgtiny_SVG_ERROR:
                        fprintf(stderr, "svgtiny_SVG_ERROR: line %i: %s",
                                        diagram->error_line,
                                        diagram->error_message);
                        break;
                default:
                        fprintf(stderr, "unknown svgtiny_code %i", code);
                        break;
                }
                fprintf(stderr, "\n");
        }

        free(buffer);

        write_mvg(outf, scale, diagram);

        if (argc > 3) {
                fclose(outf);
        }

        svgtiny_free(diagram);

        return 0;
}
