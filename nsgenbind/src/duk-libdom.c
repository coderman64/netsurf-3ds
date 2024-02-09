/* duktape binding generation implementation
 *
 * This file is part of nsgenbind.
 * Licensed under the MIT License,
 *                http://www.opensource.org/licenses/mit-license.php
 * Copyright 2015 Vincent Sanders <vince@netsurf-browser.org>
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <errno.h>
#include <ctype.h>

#include "options.h"
#include "utils.h"
#include "nsgenbind-ast.h"
#include "webidl-ast.h"
#include "ir.h"
#include "output.h"
#include "duk-libdom.h"

/** prefix for all generated functions */
#define DLPFX "dukky"

#define MAGICPFX "\\xFF\\xFFNETSURF_DUKTAPE_"

/**
 * \todo Constructors
 * \todo dukky_inject_not_ctr as binding.c function
 * \todo instanceof needs cleaning/generalising
 * \todo large text sections
 * \todo operations with elipsis need parameter checking love
 * \todo parameter type checking probably should do instanceof on user objects
 * \todo tool should generate warning for interfaces with empty private structs for the entire inheritance chain
 */

/**
 * generate a duktape prototype name
 */
static char *get_prototype_name(const char *interface_name)
{
        char *proto_name;
        int pnamelen;
        int pfxlen;

        /* duplicate the interface name in upper case */
        pfxlen = SLEN(MAGICPFX) + SLEN("PROTOTYPE_");
        pnamelen = strlen(interface_name) + 1;

        proto_name = malloc(pnamelen + pfxlen);
        snprintf(proto_name, pnamelen + pfxlen, "%sPROTOTYPE_%s", MAGICPFX, interface_name);
        for (pnamelen-- ; pnamelen >= 0; pnamelen--) {
                proto_name[pnamelen + pfxlen] = toupper(interface_name[pnamelen]);
        }
        return proto_name;
}





static struct opctx *open_header(struct ir *ir, const char *name)
{
        char *fname;
        int fnamel;
        struct opctx *hdrc;
        int res;

        fnamel = strlen(name) + 4;
        fname = malloc(fnamel);
        snprintf(fname, fnamel, "%s.h", name);

        /* open the output header file */
        res = output_open(fname, &hdrc);
        free(fname);
        if (res != 0 ) {
                return NULL;
        }

        /* tool preface */
        output_tool_preface(hdrc);

        /* binding preface */
        output_method_cdata(hdrc,
                            ir->binding_node,
                            GENBIND_METHOD_TYPE_PREFACE);

        /* header guard */
        outputf(hdrc,
                "\n#ifndef %s_%s_h\n", DLPFX, name);
        outputf(hdrc,
                "#define %s_%s_h\n\n", DLPFX, name);

        return hdrc;
}

static int close_header(struct ir *ir, struct opctx *hdrc)
{
        /* close header guard */
        outputf(hdrc,
                "\n#endif\n");

        /* binding postface */
        output_method_cdata(hdrc,
                            ir->binding_node,
                            GENBIND_METHOD_TYPE_POSTFACE);

        output_close(hdrc);

        return 0;
}


/**
 * generate private header
 */
static int
output_private_header(struct ir *ir)
{
        int idx;
        struct opctx *privc;

        /* open header */
        privc = open_header(ir, "private");

        for (idx = 0; idx < ir->entryc; idx++) {
                struct ir_entry *interfacee;
                struct ir_entry *inherite;
                struct genbind_node *priv_node;

                interfacee = ir->entries + idx;

                /* do not generate private structs for interfaces marked no
                 * output
                 */
                if ((interfacee->type == IR_ENTRY_TYPE_INTERFACE) &&
                    (interfacee->u.interface.noobject)) {
                        continue;
                }

                switch (interfacee->type) {
                case IR_ENTRY_TYPE_INTERFACE:
                        outputf(privc,
                                "/* Private data for %s interface */\n",
                                interfacee->name);
                        break;

                case IR_ENTRY_TYPE_DICTIONARY:
                        outputf(privc,
                                "/* Private data for %s dictionary */\n",
                                interfacee->name);
                        break;
                }

                outputf(privc,
                        "typedef struct {\n");

                /* find parent entry and include in private */
                inherite = ir_inherit_entry(ir, interfacee);
                if (inherite != NULL) {
                        outputf(privc,
                                "\t%s_private_t parent;\n",
                                inherite->class_name);
                }

                /* for each private variable on the class output it here. */
                priv_node = genbind_node_find_type(
                        genbind_node_getnode(interfacee->class),
                        NULL,
                        GENBIND_NODE_TYPE_PRIVATE);
                while (priv_node != NULL) {
                        outputc(privc, '\t');

                        output_ctype(privc, priv_node, true);

                        outputf(privc, ";\n");

                        priv_node = genbind_node_find_type(
                                genbind_node_getnode(interfacee->class),
                                priv_node,
                                GENBIND_NODE_TYPE_PRIVATE);
                }

                outputf(privc,
                        "} __attribute__((aligned)) %s_private_t;\n\n",
                        interfacee->class_name);
        }

        close_header(ir, privc);

        return 0;
}

/**
 * generate prototype header
 */
static int
output_prototype_header(struct ir *ir)
{
        int idx;
        struct opctx *protoc;

        /* open header */
        protoc = open_header(ir, "prototype");

        for (idx = 0; idx < ir->entryc; idx++) {
                struct ir_entry *entry;

                entry = ir->entries + idx;

                switch (entry->type) {
                case IR_ENTRY_TYPE_INTERFACE:
                        output_interface_declaration(protoc, entry);
                        break;

                case IR_ENTRY_TYPE_DICTIONARY:
                        output_dictionary_declaration(protoc, entry);
                        break;
                }
        }

        close_header(ir, protoc);

        return 0;
}

/**
 * generate makefile fragment
 */
static int
output_makefile(struct ir *ir)
{
        int idx;
        FILE *makef;

        /* open output file */
        makef = genb_fopen_tmp("Makefile");
        if (makef == NULL) {
                return -1;
        }

        fprintf(makef, "# duk libdom makefile fragment\n\n");

        fprintf(makef, "NSGENBIND_SOURCES:=binding.c ");
        for (idx = 0; idx < ir->entryc; idx++) {
                struct ir_entry *interfacee;

                interfacee = ir->entries + idx;

                /* no source for interfaces marked no output */
                if ((interfacee->type == IR_ENTRY_TYPE_INTERFACE) &&
                    (interfacee->u.interface.noobject)) {
                        continue;
                }

                fprintf(makef, "%s ", interfacee->filename);
        }
        fprintf(makef, "\nNSGENBIND_PREFIX:=%s\n", options->outdirname);

        genb_fclose_tmp(makef, "Makefile");

        return 0;
}


/**
 * generate binding header
 *
 * The binding header contains all the duk-libdom specific binding interface
 * macros and definitions.
 *
 * the create prototypes interface is used to cause all the prototype creation
 * functions for all generated classes to be called in the correct order with
 * the primary global (if any) generated last.
 */
static int
output_binding_header(struct ir *ir)
{
        struct opctx *bindc;

        /* open header */
        bindc = open_header(ir, "binding");

        outputf(bindc,
                "#define _MAGIC(S) (\"%s\" S)\n"
                "#define MAGIC(S) _MAGIC(#S)\n"
                "#define PROTO_MAGIC MAGIC(PROTOTYPES)\n"
                "#define PRIVATE_MAGIC MAGIC(PRIVATE)\n"
                "#define INIT_MAGIC MAGIC(INIT)\n"
                "#define NODE_MAGIC MAGIC(NODE_MAP)\n"
                "#define _PROTO_NAME(K) _MAGIC(\"PROTOTYPE_\" K)\n"
                "#define PROTO_NAME(K) _PROTO_NAME(#K)\n"
                "#define _PROP_NAME(K,V) _MAGIC(K \"_PROPERTY_\" V)\n"
                "#define PROP_NAME(K,V) _PROP_NAME(#K,#V)\n"
                "\n",
                MAGICPFX);

        /* declaration of constant string values */
        outputf(bindc,
                "/* Constant strings */\n"
                "extern const char *%s_error_fmt_argument;\n"
                "extern const char *%s_error_fmt_bool_type;\n"
                "extern const char *%s_error_fmt_number_type;\n"
                "extern const char *%s_magic_string_private;\n"
                "extern const char *%s_magic_string_prototypes;\n"
                "\n",
                DLPFX, DLPFX, DLPFX, DLPFX, DLPFX);

        outputf(bindc,
                "duk_bool_t %s_instanceof(duk_context *ctx, duk_idx_t index, const char *klass);\n",
                DLPFX);

        outputf(bindc,
                "duk_ret_t %s_create_prototypes(duk_context *ctx);\n", DLPFX);

        close_header(ir, bindc);

        return 0;
}


/**
 * generate binding source
 *
 * The binding header contains all the duk-libdom specific binding
 * implementations.
 */
static int
output_binding_src(struct ir *ir)
{
        int idx;
        struct ir_entry *pglobale = NULL;
        char *proto_name;
        struct opctx *bindc;
        int res;

        /* open the output binding file */
        res = output_open("binding.c", &bindc);
        if (res != 0 ) {
                return -1;
        }

        /* tool preface */
        output_tool_preface(bindc);

        /* binding preface */
        output_method_cdata(bindc,
                            ir->binding_node,
                            GENBIND_METHOD_TYPE_PREFACE);

        /* tool prologue */
        output_tool_prologue(bindc);

        /* binding prologue */
        output_method_cdata(bindc,
                            ir->binding_node,
                            GENBIND_METHOD_TYPE_PROLOGUE);

        outputc(bindc, '\n');

        outputf(bindc,
                "/* Error format strings */\n"
                "const char *%s_error_fmt_argument =\"%%d argument required, but ony %%d present.\";\n"
                "const char *%s_error_fmt_bool_type =\"argument %%d (%%s) requires a bool\";\n"
                "const char *%s_error_fmt_number_type =\"argument %%d (%%s) requires a number\";\n",
                DLPFX, DLPFX, DLPFX);

        outputf(bindc, "\n");

        outputf(bindc,
                "/* Magic identifiers */\n"
                "const char *%s_magic_string_private =\"%sPRIVATE\";\n"
                "const char *%s_magic_string_prototypes =\"%sPROTOTYPES\";\n",
                DLPFX, MAGICPFX, DLPFX, MAGICPFX);

        outputf(bindc, "\n");


        /* instanceof helper */
        outputf(bindc,
                "duk_bool_t\n"
                "%s_instanceof(duk_context *ctx, duk_idx_t _idx, const char *klass)\n"
                "{\n"
                "\tduk_idx_t idx = duk_normalize_index(ctx, _idx);\n"
                "\t/* ... ??? ... */\n"
                "\tif (!duk_check_type(ctx, idx, DUK_TYPE_OBJECT)) {\n"
                "\t\treturn false;\n"
                "\t}\n"
                "\t/* ... obj ... */\n"
                "\tduk_get_global_string(ctx, %s_magic_string_prototypes);\n"
                "\t/* ... obj ... protos */\n"
                "\tduk_get_prop_string(ctx, -1, klass);\n"
                "\t/* ... obj ... protos goalproto */\n"
                "\tduk_get_prototype(ctx, idx);\n"
                "\t/* ... obj ... protos goalproto proto? */\n"
                "\twhile (!duk_is_undefined(ctx, -1)) {\n"
                "\t\tif (duk_strict_equals(ctx, -1, -2)) {\n"
                "\t\t\tduk_pop_3(ctx);\n"
                "\t\t\t/* ... obj ... */\n"
                "\t\t\treturn true;\n"
                "\t\t}\n"
                "\t\tduk_get_prototype(ctx, -1);\n"
                "\t\t/* ... obj ... protos goalproto proto proto? */\n"
                "\t\tduk_replace(ctx, -2);\n"
                "\t\t/* ... obj ... protos goalproto proto? */\n"
                "\t}\n"
                "\tduk_pop_3(ctx);\n"
                "\t/* ... obj ... */\n"
                "\treturn false;\n"
                "}\n"
                "\n",
                DLPFX, DLPFX);

        /* prototype creation helper function */
        outputf(bindc,
                "static duk_ret_t\n"
                "%s_to_string(duk_context *ctx)\n"
                "{\n"
                "\t/* */\n"
                "\tduk_push_this(ctx);\n"
                "\t/* this */\n"
                "\tduk_get_prototype(ctx, -1);\n"
                "\t/* this proto */\n"
                "\tduk_get_prop_string(ctx, -1, \"%sklass_name\");\n"
                "\t/* this proto classname */\n"
                "\tduk_push_string(ctx, \"[object \");\n"
                "\t/* this proto classname str */\n"
                "\tduk_insert(ctx, -2);\n"
                "\t/* this proto str classname */\n"
                "\tduk_push_string(ctx, \"]\");\n"
                "\t/* this proto str classname str */\n"
                "\tduk_concat(ctx, 3);\n"
                "\t/* this proto str */\n"
                "\treturn 1;\n"
                "}\n"
                "\n",
                DLPFX,
                MAGICPFX);

        outputf(bindc,
                "static duk_ret_t %s_create_prototype(duk_context *ctx,\n",
                DLPFX);
        outputf(bindc,
                "\t\t\t\t\tduk_safe_call_function genproto,\n"
                "\t\t\t\t\tconst char *proto_name,\n"
                "\t\t\t\t\tconst char *klass_name)\n"
                "{\n"
                "\tduk_int_t ret;\n"
                "\tduk_push_object(ctx);\n"
                "\tif ((ret = duk_safe_call(ctx, genproto, NULL, 1, 1)) != DUK_EXEC_SUCCESS) {\n"
                "\t\tduk_pop(ctx);\n"
                "\t\tNSLOG(dukky, WARNING, \"Failed to register prototype for %%s\", proto_name + 2);\n"
                "\t\treturn ret;\n"
                "\t}\n"
                "\t/* top of stack is the ready prototype, inject it */\n"
                "\tduk_push_string(ctx, klass_name);\n"
                "\tduk_put_prop_string(ctx, -2, \"%sklass_name\");\n"
                "\tduk_push_c_function(ctx, %s_to_string, 0);\n"
                "\tduk_put_prop_string(ctx, -2, \"toString\");\n"
                "\tduk_push_string(ctx, \"toString\");\n"
                "\tduk_def_prop(ctx, -2, DUK_DEFPROP_HAVE_ENUMERABLE);\n"
                "\tduk_put_global_string(ctx, proto_name);\n"
                "\treturn DUK_ERR_NONE;\n"
                "}\n\n",
                MAGICPFX,
                DLPFX);

        /* generate prototype creation */
        outputf(bindc,
                "duk_ret_t %s_create_prototypes(duk_context *ctx)\n", DLPFX);

        outputf(bindc, "{\n");

        for (idx = 0; idx < ir->entryc; idx++) {
                struct ir_entry *interfacee;

                interfacee = ir->entries + idx;

                if (interfacee->type == IR_ENTRY_TYPE_DICTIONARY) {
                        continue;
                }

                /* do not generate prototype calls for interfaces marked
                 * no output
                 */
                if (interfacee->type == IR_ENTRY_TYPE_INTERFACE) {
                        if (interfacee->u.interface.noobject) {
                                continue;
                        }

                        if (interfacee->u.interface.primary_global) {
                                pglobale = interfacee;
                                continue;
                        }
                }
                proto_name = get_prototype_name(interfacee->name);

                outputf(bindc,
                        "\t%s_create_prototype(ctx, %s_%s___proto, \"%s\", \"%s\");\n",
                        DLPFX,
                        DLPFX,
                        interfacee->class_name,
                        proto_name,
                        interfacee->name);

                free(proto_name);
        }

        if (pglobale != NULL) {
                outputf(bindc, "\n\t/* Global object prototype is last */\n");

                proto_name = get_prototype_name(pglobale->name);
                outputf(bindc,
                        "\t%s_create_prototype(ctx, %s_%s___proto, \"%s\", \"%s\");\n",
                        DLPFX,
                        DLPFX,
                        pglobale->class_name,
                        proto_name,
                        pglobale->name);
                free(proto_name);
        }

        outputf(bindc, "\n\treturn DUK_ERR_NONE;\n");

        outputf(bindc, "}\n");

        /* binding postface */
        output_method_cdata(bindc,
                            ir->binding_node,
                            GENBIND_METHOD_TYPE_POSTFACE);

        output_close(bindc);

        return 0;
}

static int output_interfaces_dictionaries(struct ir *ir)
{
        int res;
        int idx;

        /* generate interfaces */
        for (idx = 0; idx < ir->entryc; idx++) {
                struct ir_entry *irentry;

                irentry = ir->entries + idx;

                switch (irentry->type) {
                case IR_ENTRY_TYPE_INTERFACE:
                        /* do not generate class for interfaces marked no
                         * output
                         */
                        if (!irentry->u.interface.noobject) {
                                res = output_interface(ir, irentry);
                                if (res != 0) {
                                        return res;
                                }
                        }
                        break;

                case IR_ENTRY_TYPE_DICTIONARY:
                        res = output_dictionary(ir, irentry);
                        if (res != 0) {
                                return res;
                        }

                default:
                        break;
                }
        }

        return 0;
}

int duk_libdom_output(struct ir *ir)
{
        int idx;
        int res = 0;

        /* process ir entries for output */
        for (idx = 0; idx < ir->entryc; idx++) {
                struct ir_entry *irentry;

                irentry = ir->entries + idx;

                /* compute class name */
                irentry->class_name = gen_idl2c_name(irentry->name);

                if (irentry->class_name != NULL) {
                        int ifacenamelen;

                        /* generate source filename */
                        ifacenamelen = strlen(irentry->class_name) + 4;
                        irentry->filename = malloc(ifacenamelen);
                        snprintf(irentry->filename,
                                 ifacenamelen,
                                 "%s.c",
                                 irentry->class_name);
                }
        }

        res = output_interfaces_dictionaries(ir);
        if (res != 0) {
                goto output_err;
        }

        /* generate private header */
        res = output_private_header(ir);
        if (res != 0) {
                goto output_err;
        }

        /* generate prototype header */
        res = output_prototype_header(ir);
        if (res != 0) {
                goto output_err;
        }

        /* generate binding header */
        res = output_binding_header(ir);
        if (res != 0) {
                goto output_err;
        }

        /* generate binding source */
        res = output_binding_src(ir);
        if (res != 0) {
                goto output_err;
        }

        /* generate makefile fragment */
        res = output_makefile(ir);

output_err:

        return res;
}
