#ifndef rufl_test_harness_priv_h_
#define rufl_test_harness_priv_h_

#include <stdbool.h>
#include <stddef.h>
#include <inttypes.h>

#include "harness.h"

typedef struct {
	unsigned int refcnt;
	size_t name; /* Index of name in names array */
#define FONT_ENCODING_SYMBOL ((size_t) -1) /* Symbol, not language, font */
	size_t encoding; /* Index of encoding in encodings array */
	int xsize; /* XSize of this font */
	int ysize; /* YSize if this font */
	int xres; /* XResolution of this font */
	int yres; /* YResolution of this font */
} rufl_test_harness_sized_font;

typedef struct {
	int fm_version;
	bool fm_ucs;
	bool fm_broken_fec;

	const char **font_names;
	size_t n_font_names;

	const char **encodings;
	size_t n_encodings;

	/* n_font_names * (n_encodings + 1) entries */
	char **encoding_filenames;

	/* At most 256 active font handles */
	rufl_test_harness_sized_font fonts[256];
	int current_font;

	char *buffer;
	int buffer_flags;
} rufl_test_harness_t;

extern rufl_test_harness_t *h;

#endif
