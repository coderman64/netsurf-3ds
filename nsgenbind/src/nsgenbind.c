/* binding generator main and command line parsing
 *
 * This file is part of nsgenbind.
 * Licensed under the MIT License,
 *                http://www.opensource.org/licenses/mit-license.php
 * Copyright 2012 Vincent Sanders <vince@netsurf-browser.org>
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <errno.h>

#include "options.h"
#include "nsgenbind-ast.h"
#include "webidl-ast.h"
#include "ir.h"
#include "output.h"
#include "jsapi-libdom.h"
#include "duk-libdom.h"

struct options *options;

enum bindingtype_e {
    BINDINGTYPE_UNKNOWN,
    BINDINGTYPE_JSAPI_LIBDOM,
    BINDINGTYPE_DUK_LIBDOM,
};

static struct options* process_cmdline(int argc, char **argv)
{
        int opt;

        options = calloc(1,sizeof(struct options));
        if (options == NULL) {
                fprintf(stderr, "Allocation error\n");
                return NULL;
        }

        while ((opt = getopt(argc, argv, "vngDW::I:")) != -1) {
                switch (opt) {
                case 'I':
                        options->idlpath = strdup(optarg);
                        break;

                case 'v':
                        options->verbose = true;
                        break;

                case 'n':
                        options->dryrun = true;
                        break;

                case 'D':
                        options->debug = true;
                        break;

                case 'g':
                        options->dbglog = true;
                        break;

                case 'W':
                        if ((optarg == NULL) ||
                            (strcmp(optarg, "all") == 0)) {
                                /* all warnings */
                                options->warnings |= WARNING_ALL;
                        } else if (strcmp(optarg, "unimplemented") == 0) {
                                options->warnings |= WARNING_UNIMPLEMENTED;
                        } else if (strcmp(optarg, "duplicated") == 0) {
                                options->warnings |= WARNING_DUPLICATED;
                        } else if (strcmp(optarg, "generated") == 0) {
                                options->warnings |= WARNING_GENERATED;
                        } else {
                                fprintf(stderr,
                                        "Unknown warning option \"%s\" valid options are: all, unimplemented,\n"
                                        "                                                 duplicated, generated\n",
                                        optarg);
                                free(options);
                                return NULL;

                        }
                        break;

                default: /* '?' */
                        fprintf(stderr,
                             "Usage: %s [-v] [-g] [-D] [-W] [-I idlpath] inputfile outputdir\n",
                                argv[0]);
                        free(options);
                        return NULL;
                }
        }

        if (optind > (argc - 2)) {
                fprintf(stderr,
                       "Error: expected input filename and output directory\n");
                free(options);
                return NULL;
        }

        options->infilename = strdup(argv[optind]);

        options->outdirname = strdup(argv[optind + 1]);

        return options;

}

static int webidl_file_cb(struct genbind_node *node, void *ctx)
{
	struct webidl_node **webidl_ast = ctx;
	char *filename;

	filename = genbind_node_gettext(node);

        if (options->verbose) {
                printf("Opening IDL file \"%s\"\n", filename);
        }

	return webidl_parsefile(filename, webidl_ast);
}

static int genbind_load_idl(struct genbind_node *genbind,
                            struct webidl_node **webidl_out)
{
        int res;
        struct genbind_node *binding_node;

        binding_node = genbind_node_find_type(genbind, NULL,
                                              GENBIND_NODE_TYPE_BINDING);

	/* walk AST and load any web IDL files required */
	res = genbind_node_foreach_type(
                genbind_node_getnode(binding_node),
                GENBIND_NODE_TYPE_WEBIDL,
                webidl_file_cb,
                webidl_out);
	if (res != 0) {
		fprintf(stderr, "Error: failed reading Web IDL\n");
		return -1;
	}

        /* implements are implemented as mixins so intercalate them */
        res = webidl_intercalate_implements(*webidl_out);
	if (res != 0) {
		fprintf(stderr, "Error: Failed to intercalate implements\n");
		return -1;
	}

        return 0;
}

/**
 * get the type of binding
 */
static enum bindingtype_e genbind_get_type(struct genbind_node *node)
{
        struct genbind_node *binding_node;
        const char *binding_type;

        binding_node = genbind_node_find_type(node,
                                              NULL,
                                              GENBIND_NODE_TYPE_BINDING);
        if (binding_node == NULL) {
            /* binding entry is missing which is invalid */
            return BINDINGTYPE_UNKNOWN;
        }

        binding_type = genbind_node_gettext(
                genbind_node_find_type(
                        genbind_node_getnode(binding_node),
                        NULL,
                        GENBIND_NODE_TYPE_NAME));
        if (binding_type == NULL) {
                fprintf(stderr, "Error: missing binding type\n");
                return BINDINGTYPE_UNKNOWN;
        }

        if (strcmp(binding_type, "jsapi_libdom") == 0) {
                return BINDINGTYPE_JSAPI_LIBDOM;
        }

        if (strcmp(binding_type, "duk_libdom") == 0) {
                return BINDINGTYPE_DUK_LIBDOM;
        }

        fprintf(stderr, "Error: unsupported binding type \"%s\"\n", binding_type);

        return BINDINGTYPE_UNKNOWN;
}

int main(int argc, char **argv)
{
        int res;
        struct genbind_node *genbind_root = NULL;
        struct webidl_node *webidl_root = NULL;
        struct ir *ir = NULL;
        enum bindingtype_e bindingtype;

        options = process_cmdline(argc, argv);
        if (options == NULL) {
                return 1; /* bad commandline */
        }

        /* parse binding */
        res = genbind_parsefile(options->infilename, &genbind_root);
        if (res != 0) {
                fprintf(stderr, "Error: parse failed with code %d\n", res);
                return res;
        }

        /* dump the binding AST */
        genbind_dump_ast(genbind_root);

        /* get type of binding */
        bindingtype = genbind_get_type(genbind_root);
        if (bindingtype == BINDINGTYPE_UNKNOWN) {
                return 3;
        }

        /* load the IDL files specified in the binding */
        res = genbind_load_idl(genbind_root, &webidl_root);
        if (res != 0) {
                return 4;
        }

	/* debug dump of web idl AST */
        webidl_dump_ast(webidl_root);

        /* generate intermediate representation */
        res = ir_new(genbind_root, webidl_root, &ir);
        if (res != 0) {
                return 5;
        }

        /* dump the intermediate representation */
        ir_dump(ir);
        ir_dumpdot(ir);

        /* generate binding */
        switch (bindingtype) {
        case BINDINGTYPE_DUK_LIBDOM:
                res = duk_libdom_output(ir);
                break;

        default:
                fprintf(stderr, "Unable to generate binding of this type\n");
                res = 7;
        }

        return res;
}
