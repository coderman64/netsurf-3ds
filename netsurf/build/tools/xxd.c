/*
 * xxd utility
 *
 * Copyright 2020 Lars Wirzenius 
 * Copyright 2020 Vincent Sanders <vince@netsurf-browser.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 *  * The above copyright notice and this permission notice shall be included in
 *    all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

static char *get_array_name(const char *fname)
{
	int fnamelen;
	char *aryname;
	int idx;

	fnamelen = strlen(fname);
	aryname = malloc(fnamelen + 1);

	if (aryname != NULL) {
		for (idx = 0; idx < fnamelen; idx++) {
			int c = fname[idx];
			if ((c >= '0' && c <= '9') ||
			    (c >= 'A' && c <= 'Z') ||
			    (c >= 'a' && c <= 'z')) {
				aryname[idx] = fname[idx];
			} else {
				aryname[idx] = '_';
			}
		}
		aryname[idx] = 0;
	}
	return aryname;
}

int main(int argc, char **argv)
{
	int inc = 0;
	int opt;
	int c, n;
	FILE *inf;
	FILE *outf;
	char *aryname = NULL;
	int outlen;

	while ((opt = getopt(argc, argv, "i")) != -1) {
		switch (opt) {
		case 'i':
			inc = 1;
			break;

		default: /* '?' */
			fprintf(stderr, "Usage: %s [-i] [infile [outfile]]]\n",
				argv[0]);
			exit(EXIT_FAILURE);
		}
	}

	if (optind < argc) {
		inf = fopen(argv[optind], "r");
		if (inf == NULL) {
			perror("Opening for read");
			exit(EXIT_FAILURE);			
		}
		aryname = get_array_name(argv[optind]);
		optind++;
	} else {
		inf = stdin;

	}

	if (optind < argc) {
		outf = fopen(argv[optind], "w");
		if (outf == NULL) {
			perror("Opening for write");
			exit(EXIT_FAILURE);			
		}
	} else {
		outf = stdout;
	}

	if ((inc != 0) && (aryname != NULL)) {
		fprintf(outf, "unsigned char %s[] = {\n", aryname);
	}

	outlen = 0;
	n = 0;
	while ((c = getc(inf)) != EOF) {
		if (n == 0) {
			fprintf(outf, " ");
		}
		fprintf(outf, " 0x%02x,", c);
		n += 1;
		outlen++;
		if (n >= 12) {
			fprintf(outf, "\n");
			n = 0;
		}
	}
	if (n > 0) {
		fprintf(outf, "\n");
	}

	if ((inc != 0) && (aryname != NULL)) {
		fprintf(outf, "};\nunsigned int %s_len = %d;\n",
			aryname, outlen);
	}


	
	fclose(outf);
	fclose(inf);
	
	return 0;
}
