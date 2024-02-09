#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "harness-priv.h"

rufl_test_harness_t *h = NULL;

static void rufl_test_harness_free(void)
{
	size_t ni, ei;

	free(h->font_names);
	free(h->encodings);
	if (h->encoding_filenames != NULL) {
		for (ni = 0; ni != h->n_font_names; ni++) {
			for (ei = 0; ei != h->n_encodings + 1; ei++) {
				free(h->encoding_filenames[
						(ni * (h->n_encodings + 1)) + ei]);
			}
		}
	}
	free(h->encoding_filenames);
	free(h);
}

void rufl_test_harness_init(int fm_version, bool fm_ucs, bool preload)
{
	h = calloc(1, sizeof(*h));
	assert(h != NULL);

	h->fm_version = fm_version;
	h->fm_ucs = fm_ucs;
	h->fm_broken_fec = fm_version < 364;

	if (preload) {
		/* Register ROM fonts as a convenience */
		rufl_test_harness_register_font("Corpus.Bold");
		rufl_test_harness_register_font("Corpus.Bold.Oblique");
		rufl_test_harness_register_font("Corpus.Medium");
		rufl_test_harness_register_font("Corpus.Medium.Oblique");
		rufl_test_harness_register_font("Homerton.Bold");
		rufl_test_harness_register_font("Homerton.Bold.Oblique");
		rufl_test_harness_register_font("Homerton.Medium");
		rufl_test_harness_register_font("Homerton.Medium.Oblique");
		rufl_test_harness_register_font("Trinity.Bold");
		rufl_test_harness_register_font("Trinity.Bold.Italic");
		rufl_test_harness_register_font("Trinity.Medium");
		rufl_test_harness_register_font("Trinity.Medium.Italic");

		/* Register encodings as a convenience */
		rufl_test_harness_register_encoding("Cyrillic");
		rufl_test_harness_register_encoding("Greek");
		rufl_test_harness_register_encoding("Hebrew");
		rufl_test_harness_register_encoding("Latin1");
		rufl_test_harness_register_encoding("Latin2");
		rufl_test_harness_register_encoding("Latin3");
		rufl_test_harness_register_encoding("Latin4");
		rufl_test_harness_register_encoding("Latin5");
		rufl_test_harness_register_encoding("Latin6");
		rufl_test_harness_register_encoding("Latin7");
		rufl_test_harness_register_encoding("Latin8");
		rufl_test_harness_register_encoding("Latin9");
		rufl_test_harness_register_encoding("Latin10");
		if (fm_ucs)
			rufl_test_harness_register_encoding("UTF8");
		rufl_test_harness_register_encoding("Welsh");
	}

	atexit(rufl_test_harness_free);
}

void rufl_test_harness_register_font(const char *name)
{
	const char **names;

	/* Encoding paths must be registered last */
	assert(h->encoding_filenames == NULL);

	names = realloc(h->font_names,
			(h->n_font_names + 1) * sizeof(*names));
	assert(names != NULL);

	h->font_names = names;

	h->font_names[h->n_font_names++] = name;
}

void rufl_test_harness_register_encoding(const char *encoding)
{
	const char **encodings;

	/* Encoding paths must be registered last */
	assert(h->encoding_filenames == NULL);

	encodings = realloc(h->encodings,
			(h->n_encodings + 1) * sizeof(*encodings));
	assert(encodings != NULL);

	h->encodings = encodings;

	h->encodings[h->n_encodings++] = encoding;
}

void rufl_test_harness_set_font_encoding(const char *fontname,
		const char *encoding, const char *path)
{
	size_t ni, ei;

	if (h->encoding_filenames == NULL) {
		h->encoding_filenames = calloc(
				h->n_font_names * (h->n_encodings + 1),
				sizeof(*h->encoding_filenames));
		assert(h->encoding_filenames != NULL);
	}

	/* Find font index */
	for (ni = 0; ni < h->n_font_names; ni++) {
		if (strcmp(h->font_names[ni], fontname) == 0)
			break;
	}
	assert(ni != h->n_font_names);

	/* Find encoding index */
	if (strcmp("Symbol", encoding) == 0) {
		ei = h->n_encodings;
	} else {
		for (ei = 0; ei < h->n_encodings; ei++) {
			if (strcmp(h->encodings[ei], encoding) == 0)
				break;
		}
		assert(ei != h->n_encodings);
	}

	if (h->encoding_filenames[(ni * (h->n_encodings + 1)) + ei] != NULL)
		free(h->encoding_filenames[(ni * (h->n_encodings + 1)) + ei]);
	h->encoding_filenames[(ni * (h->n_encodings + 1)) + ei] = strdup(path);
	assert(h->encoding_filenames[(ni * (h->n_encodings + 1)) + ei] != NULL);
}
