/**
 * \file
 * simple tool to split fat messages file without the capabilities of
 * the full tool but without the dependancy on perl.
 *
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <zlib.h>

#include "utils/errors.h"

enum out_fmt {
	      OUTPUTFMT_NONE = 0,
	      OUTPUTFMT_MESSAGES,
};

/**
 * parameters that control behaviour of tool
 */
struct param {
	/**
	 * compress output
	 */
	int compress;
	/**
	 * select language
	 */
	char *selected;
	/**
	 * fallback language for items unavailable in selecte dlanguage
	 */
	char *fallback;
	int warnings;
	char *platform;
	enum out_fmt format;
	char *infilename;
	char *outfilename;
};

struct trnsltn_entry {
	struct trnsltn_entry *next;
	char *lang;
	char *key;
	char *value;
};

static nserror usage(int argc, char **argv)
{
	fprintf(stderr,
		"Usage: %s -l lang [-z] [-d lang] [-W warning] [-o <file>] [-i <file>] [-p platform] [-f format] [<file> [<file>]]\n"
		"Options:\n"
		"  -z           Gzip output\n"
		"  -l lang      Language to select for\n"
		"  -d lang      Fallback language [default: en]\n"
		"  -W warning   Warnings generated none, all [default: none]\n"
		"  -p platform  Platform to select for any, gtk, ami [default: any]\n"
		"  -f format    Output format [default: messages]\n"
		"  -i filename  Input file\n"
		"  -o filename  Output file\n",
		argv[0]);
	return NSERROR_OK;
}

/**
 * process command line arguments
 *
 *
 */
static nserror process_cmdline(int argc, char **argv, struct param *param)
{
	int opt;

	memset(param, 0, sizeof(*param));

	while ((opt = getopt(argc, argv, "zl:d:W:o:i:p:f:")) != -1) {
		switch (opt) {
		case 'z':
			param->compress = 1;
			break;

		case 'l':
			param->selected = strdup(optarg);
			break;

		case 'd':
			param->fallback = strdup(optarg);
			break;

		case 'W':
			param->warnings = 1;
			break;

		case 'o':
			param->outfilename = strdup(optarg);
			break;

		case 'i':
			param->infilename = strdup(optarg);
			break;

		case 'p':
			param->platform = strdup(optarg);
			break;

		case 'f':
			if (strcmp(optarg, "messages") == 0) {
				param->format = OUTPUTFMT_MESSAGES;
			} else {
				fprintf(stderr,
					"output format %s not supported",
					optarg);
				usage(argc, argv);
				return NSERROR_NOT_IMPLEMENTED;
			}
			break;

		default:
			usage(argc, argv);
			return NSERROR_BAD_PARAMETER;
		}
	}

	/* trailing filename arguments */
	if (optind < argc) {
		param->infilename = strdup(argv[optind]);
		optind++;
	}

	if (optind < argc) {
		param->outfilename = strdup(argv[optind]);
		optind++;
	}

	/* parameter checks */
	if (param->selected == NULL) {
		fprintf(stderr, "A language to select must be specified\n");
		usage(argc, argv);
		return NSERROR_BAD_PARAMETER;
	}

	if (param->infilename == NULL) {
		fprintf(stderr, "Input file required\n");
		usage(argc, argv);
		return NSERROR_BAD_PARAMETER;
	}

	if (param->outfilename == NULL) {
		fprintf(stderr, "Output file required\n");
		usage(argc, argv);
		return NSERROR_BAD_PARAMETER;
	}

	if ((param->platform != NULL) &&
	    (strcmp(param->platform, "any") ==0)) {
		free(param->platform);
		param->platform = NULL;
	}

	/* defaults */
	if (param->fallback == NULL) {
		param->fallback = strdup("en");
	}

	if (param->format == OUTPUTFMT_NONE) {
		param->format = OUTPUTFMT_MESSAGES;
	}

	return NSERROR_OK;
}


/**
 * extract key/value from a line of input
 *
 * \retun NSERROR_OK and key_out and value_out updated
 *        NSERROR_NOT_FOUND if not a key/value input line
 *        NSERROR_INVALID if the line is and invalid format (missing colon)
 */
static nserror
get_key_value(char *line, ssize_t linelen, char **key_out, char **value_out)
{
	char *key;
	char *value;

	/* skip leading whitespace for start of key */
	for (key = line; *key != 0; key++) {
		if ((*key != ' ') && (*key != '\t') && (*key != '\n')) {
			break;
		}
	}

	/* empty line or only whitespace */
	if (*key == 0) {
		return NSERROR_NOT_FOUND;
	}

	/* comment */
	if (*key == '#') {
		return NSERROR_NOT_FOUND;
	}

	/* get start of value */
	for (value = key; *value != 0; value++) {
		if (*value == ':') {
			*value = 0;
			value++;
			break;
		}
	}

	/* missing colon separator */
	if (*value == 0) {
		return NSERROR_INVALID;
	}

	/* remove delimiter from value */
	if (line[linelen - 1] == '\n') {
		linelen--;
		line[linelen] = 0;
	}

	*key_out = key;
	*value_out = value;
	return NSERROR_OK;
}


/**
 * extract language, platform and token elements from a string
 */
static nserror
get_lang_plat_tok(char *str, char **lang_out, char **plat_out, char **tok_out)
{
	char *plat;
	char *tok;

	for (plat = str; *plat != 0; plat++) {
		if (*plat == '.') {
			*plat = 0;
			plat++;
			break;
		}
	}
	if (*plat == 0) {
		return NSERROR_INVALID;
	}

	for (tok = plat; *tok != 0; tok++) {
		if (*tok == '.') {
			*tok = 0;
			tok++;
			break;
		}
	}
	if (*tok == 0) {
		return NSERROR_INVALID;
	}

	*lang_out = str;
	*plat_out = plat;
	*tok_out = tok;

	return NSERROR_OK;
}


/**
 * reverse order of entries in a translation list
 */
static nserror
translation_list_reverse(struct trnsltn_entry **tlist)
{
	struct trnsltn_entry *prev;
	struct trnsltn_entry *next;
	struct trnsltn_entry *curr;

	prev = NULL;
	next = NULL;
	curr = *tlist;

	while (curr != NULL) {
		next = curr->next;
		curr->next = prev;
		prev = curr;
		curr = next;
	}

	*tlist = prev;
	return NSERROR_OK;
}


/**
 * find a translation entry from a key
 *
 * \todo This implementation is imcomplete! it only considers the very
 * first entry on the list. this introduces the odd ordering
 * requirement for keys in the fatmessages file. This is done to avoid
 * an O(n^2) list search for every line of input.
 *
 * \param tlist translation list head
 * \param key The key of the translation to search for
 * \param trans_out The sucessful result
 * \return NSERROR_OK and trans_out updated on success else NSERROR_NOT_FOUND;
 */
static nserror
translation_from_key(struct trnsltn_entry *tlist,
		     char *key,
		     struct trnsltn_entry **trans_out)
{
	if (tlist == NULL) {
		return NSERROR_NOT_FOUND;
	}

	if (strcmp(tlist->key, key) != 0) {
		return NSERROR_NOT_FOUND;
	}

	*trans_out = tlist;
	return NSERROR_OK;
}


/**
 * create and link an entry into translation list
 */
static nserror
translation_add(struct trnsltn_entry **tlist,
		const char *lang,
		const char *key,
		const char *value)
{
	struct trnsltn_entry *tnew;

	tnew = malloc(sizeof(*tnew));
	if (tnew == NULL) {
		return NSERROR_NOMEM;
	}
	tnew->next = *tlist;
	tnew->lang = strdup(lang);
	tnew->key = strdup(key);
	tnew->value = strdup(value);

	*tlist = tnew;
	return NSERROR_OK;
}


/**
 * replace key and value on a translation entry
 */
static nserror
translation_replace(struct trnsltn_entry *tran,
		const char *lang,
		const char *key,
		const char *value)
{
	free(tran->lang);
	tran->lang = strdup(lang);
	free(tran->key);
	tran->key = strdup(key);
	free(tran->value);
	tran->value = strdup(value);

	return NSERROR_OK;
}


/**
 * process a line of the input file
 *
 */
static nserror
messageline(struct param *param,
	    struct trnsltn_entry **tlist,
	    char *line, ssize_t linelen)
{
	nserror res;
	char *key;
	char *value;
	char *lang;
	char *plat;
	char *tok;
	struct trnsltn_entry *tran;

	res = get_key_value(line, linelen, &key, &value);
	if (res != NSERROR_OK) {
		/* skip line as no valid key value pair found */
		return res;
	}

	res = get_lang_plat_tok(key, &lang, &plat, &tok);
	if (res != NSERROR_OK) {
		/* malformed key */
		return res;
	}

	if ((param->platform != NULL) &&
	    (strcmp(plat, "all") != 0) &&
	    (strcmp(plat, param->platform) != 0)) {
		/* this translation is not for the selected platform */
		return NSERROR_OK;
	}

	res = translation_from_key(*tlist, tok, &tran);
	if (res == NSERROR_OK) {
		if (strcmp(tran->lang, param->selected) != 0) {
			/* current entry is not the selected language */
			if (strcmp(lang, param->selected) == 0) {
				/*
				 * new entry is in selected language and
				 * current entry is not
				 */
				res = translation_replace(tran, lang, tok, value);
			} else if ((strcmp(lang, param->fallback) != 0) &&
				   (strcmp(tran->lang, param->fallback) != 0)) {
				/*
				 * new entry is in fallback language and
				 *  current entry is not.
				 */
				res = translation_replace(tran, lang, tok, value);
			}
		} else {
			if (strcmp(tran->lang, lang) == 0) {
				/* second entry with matching language */
				res = translation_replace(tran, lang, tok, value);
			}
		}
	} else if (res == NSERROR_NOT_FOUND) {
		res = translation_add(tlist, lang, tok, value);
	}

	return res;
}


/**
 * read fatmessages file and create a translation entry list
 */
static nserror
fatmessages_read(struct param *param, struct trnsltn_entry **tlist)
{
	nserror res;
	FILE *infile;
	char *line = NULL;
	size_t linealloc = 0;
	ssize_t linelen;
	int linenum = 0;

	infile = fopen(param->infilename, "r");
	if (infile == NULL) {
		perror("Unable to open input file");
		return NSERROR_NOT_FOUND;
	}

	while (1) {
		linelen = getline(&line, &linealloc, infile);
		if (linelen == -1) {
			break;
		}
		linenum++;

		res = messageline(param, tlist, line, linelen);
		if ((res == NSERROR_INVALID) && (param->warnings > 0)) {
			fprintf(stderr, "line %d Malformed: \"%s\"\n",
				linenum, line);
		}
	}

	fclose(infile);

	res = translation_list_reverse(tlist);

	return res;
}


/**
 * write output in NetSurf messages format
 */
static nserror
message_write(struct param *param, struct trnsltn_entry *tlist)
{
	gzFile outf;
	const char *mode;

	if (param->compress == 0) {
		mode = "wbT";
	} else {
		mode = "wb9";
	}

	outf = gzopen(param->outfilename, mode);
	if (outf == NULL) {
		perror("Unable to open output file");
		return NSERROR_PERMISSION;
	}

	if (gzprintf(outf,
		"# This messages file is automatically generated from %s\n"
		"# at build-time.  Please go and edit that instead of this.\n\n",
		param->infilename) < 1) {
		gzclose(outf);
		unlink(param->outfilename);
		return NSERROR_NOSPACE;
	};

	while (tlist != NULL) {
		if (gzprintf(outf, "%s:%s\n", tlist->key, tlist->value) < 1) {
			gzclose(outf);
			unlink(param->outfilename);
			return NSERROR_NOSPACE;
		}
		tlist = tlist->next;
	}

	gzclose(outf);

	return NSERROR_OK;
}

int main(int argc, char **argv)
{
	nserror res;
	struct param param; /* control paramters */
	struct trnsltn_entry *translations = NULL;

	res = process_cmdline(argc, argv, &param);
	if (res != NSERROR_OK) {
		return EXIT_FAILURE;
	}

	res = fatmessages_read(&param, &translations);
	if (res != NSERROR_OK) {
		return EXIT_FAILURE;
	}

	switch (param.format) {
	case OUTPUTFMT_NONE:
		res = NSERROR_OK;
		break;

	case OUTPUTFMT_MESSAGES:
		res = message_write(&param, translations);
		break;
	}

	if (res != NSERROR_OK) {
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
