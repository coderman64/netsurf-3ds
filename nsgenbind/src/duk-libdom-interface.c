/* duktape binding generation implementation
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
        snprintf(proto_name, pnamelen + pfxlen,
                 "%sPROTOTYPE_%s", MAGICPFX, interface_name);
        for (pnamelen-- ; pnamelen >= 0; pnamelen--) {
                proto_name[pnamelen + pfxlen] = toupper(interface_name[pnamelen]);
        }
        return proto_name;
}


/**
 * Compare two nodes to check their c types match.
 */
static bool compare_ctypes(struct genbind_node *a, struct genbind_node *b)
{
        struct genbind_node *ta;
        struct genbind_node *tb;

        ta = genbind_node_find_type(genbind_node_getnode(a),
                                    NULL, GENBIND_NODE_TYPE_NAME);
        tb = genbind_node_find_type(genbind_node_getnode(b),
                                    NULL, GENBIND_NODE_TYPE_NAME);

        while ((ta != NULL) && (tb != NULL)) {
                char *txt_a;
                char *txt_b;

                txt_a = genbind_node_gettext(ta);
                txt_b = genbind_node_gettext(tb);

                if (strcmp(txt_a, txt_b) != 0) {
                        return false; /* missmatch */
                }

                ta = genbind_node_find_type(genbind_node_getnode(a),
                                            ta, GENBIND_NODE_TYPE_NAME);
                tb = genbind_node_find_type(genbind_node_getnode(b),
                                            tb, GENBIND_NODE_TYPE_NAME);
        }
        if (ta != tb) {
                return false;
        }

        return true;
}


/**
 * Generate code to create a private structure
 *
 * \param outc Output context
 * \param name of class private structure created for.
 */
static int
output_create_private(struct opctx *outc, char *class_name)
{
        outputf(outc,
                "\t/* create private data and attach to instance */\n");
        outputf(outc,
                "\t%s_private_t *priv = calloc(1, sizeof(*priv));\n",
                class_name);
        outputf(outc,
                "\tif (priv == NULL) return 0;\n");
        outputf(outc,
                "\tduk_push_pointer(ctx, priv);\n");
        outputf(outc,
                "\tduk_put_prop_string(ctx, 0, %s_magic_string_private);\n\n",
                DLPFX);

        return 0;
}


/**
 * generate code that gets a private pointer
 */
static int
output_safe_get_private(struct opctx *outc, char *class_name, int idx)
{
        outputf(outc,
                "\t%s_private_t *priv;\n", class_name);
        outputf(outc,
                "\tduk_get_prop_string(ctx, %d, %s_magic_string_private);\n",
                idx, DLPFX);
        outputf(outc,
                "\tpriv = duk_get_pointer(ctx, -1);\n");
        outputf(outc,
                "\tduk_pop(ctx);\n");
        outputf(outc,
                "\tif (priv == NULL) return 0;\n\n");

        return 0;
}


/**
 * generate code that gets a prototype by name
 */
static int output_get_prototype(struct opctx *outc, const char *interface_name)
{
        char *proto_name;

        proto_name = get_prototype_name(interface_name);

        outputf(outc,
                "\t/* get prototype */\n");
        outputf(outc,
                "\tduk_get_global_string(ctx, %s_magic_string_prototypes);\n",
                DLPFX);
        outputf(outc,
                "\tduk_get_prop_string(ctx, -1, \"%s\");\n",
                proto_name);
        outputf(outc,
                "\tduk_replace(ctx, -2);\n");

        free(proto_name);

        return 0;
}

/**
 * generate code that sets a destructor in a prototype
 */
static int output_set_destructor(struct opctx *outc, char *class_name, int idx)
{
        outputf(outc,
                "\t/* Set the destructor */\n");
        outputf(outc,
                "\tduk_dup(ctx, %d);\n", idx);
        outputf(outc,
                "\tduk_push_c_function(ctx, %s_%s___destructor, 1);\n",
                DLPFX, class_name);
        outputf(outc,
                "\tduk_set_finalizer(ctx, -2);\n");
        outputf(outc,
                "\tduk_pop(ctx);\n\n");

        return 0;
}

/**
 * generate code that sets a constructor in a prototype
 */
static int
output_set_constructor(struct opctx *outc, char *class_name, int idx, int argc)
{
        outputf(outc,
                "\t/* Set the constructor */\n");
        outputf(outc,
                "\tduk_dup(ctx, %d);\n", idx);
        outputf(outc,
                "\tduk_push_c_function(ctx, %s_%s___constructor, %d);\n",
                DLPFX, class_name, 1 + argc);
        outputf(outc,
                "\tduk_put_prop_string(ctx, -2, \"%sINIT\");\n",
                MAGICPFX);
        outputf(outc,
                "\tduk_pop(ctx);\n\n");

        return 0;
}


/**
 * generate code to dump javascript stack
 */
static int
output_dump_stack(struct opctx *outc)
{
        if (options->dbglog) {
                /* dump stack */
                outputf(outc,
                        "\tduk_push_context_dump(ctx);\n");
                outputf(outc,
                        "\tNSLOG(dukky, DEEPDEBUG, \"Stack: %%s\", duk_to_string(ctx, -1));\n");
                outputf(outc,
                        "\tduk_pop(ctx);\n");
        }
        return 0;
}


/**
 * generate code that adds a method in a prototype
 */
static int
output_add_method(struct opctx *outc,
                  const char *class_name,
                  const char *method)
{
        outputf(outc,
                "\t/* Add a method */\n");
        outputf(outc,
                "\tduk_dup(ctx, 0);\n");
        outputf(outc,
                "\tduk_push_string(ctx, \"%s\");\n", method);
        outputf(outc,
                "\tduk_push_c_function(ctx, %s_%s_%s, DUK_VARARGS);\n",
                DLPFX, class_name, method);
        output_dump_stack(outc);
        outputf(outc,
                "\tduk_def_prop(ctx, -3,\n");
        outputf(outc,
                "\t\t     DUK_DEFPROP_HAVE_VALUE |\n");
        outputf(outc,
                "\t\t     DUK_DEFPROP_HAVE_WRITABLE |\n");
        outputf(outc,
                "\t\t     DUK_DEFPROP_HAVE_ENUMERABLE |\n");
        outputf(outc,
                "\t\t     DUK_DEFPROP_ENUMERABLE |\n");
        outputf(outc,
                "\t\t     DUK_DEFPROP_HAVE_CONFIGURABLE);\n");
        outputf(outc,
                "\tduk_pop(ctx);\n\n");

        return 0;
}

/**
 * Generate source to populate a read/write property on a prototype
 */
static int
output_populate_rw_property(struct opctx *outc,
                            const char *class_name,
                            const char *property)
{
        outputf(outc,
                "\t/* Add read/write property */\n");
        outputf(outc,
                "\tduk_dup(ctx, 0);\n");
        outputf(outc,
                "\tduk_push_string(ctx, \"%s\");\n", property);
        outputf(outc,
                "\tduk_push_c_function(ctx, %s_%s_%s_getter, 0);\n",
                DLPFX, class_name, property);
        outputf(outc,
                "\tduk_push_c_function(ctx, %s_%s_%s_setter, 1);\n",
                DLPFX, class_name, property);
        output_dump_stack(outc);
        outputf(outc,
                "\tduk_def_prop(ctx, -4, DUK_DEFPROP_HAVE_GETTER |\n");
        outputf(outc,
                "\t\tDUK_DEFPROP_HAVE_SETTER |\n");
        outputf(outc,
                "\t\tDUK_DEFPROP_HAVE_ENUMERABLE | DUK_DEFPROP_ENUMERABLE |\n");
        outputf(outc,
                "\t\tDUK_DEFPROP_HAVE_CONFIGURABLE);\n");
        outputf(outc,
                "\tduk_pop(ctx);\n\n");

        return 0;
}


/**
 * Generate source to populate a readonly property on a prototype
 */
static int
output_populate_ro_property(struct opctx *outc,
                            const char *class_name,
                            const char *property)
{
        outputf(outc,
                "\t/* Add readonly property */\n");
        outputf(outc,
                "\tduk_dup(ctx, 0);\n");
        outputf(outc,
                "\tduk_push_string(ctx, \"%s\");\n", property);
        outputf(outc,
                "\tduk_push_c_function(ctx, %s_%s_%s_getter, 0);\n",
                DLPFX, class_name, property);
        output_dump_stack(outc);
        outputf(outc,
                "\tduk_def_prop(ctx, -3, DUK_DEFPROP_HAVE_GETTER |\n");
        outputf(outc,
                "\t\tDUK_DEFPROP_HAVE_ENUMERABLE | DUK_DEFPROP_ENUMERABLE |\n");
        outputf(outc,
                "\t\tDUK_DEFPROP_HAVE_CONFIGURABLE);\n");
        outputf(outc,
                "\tduk_pop(ctx);\n\n");

        return 0;
}


/**
 * Generate source to add a constant int value on a prototype
 */
static int
output_prototype_constant_int(struct opctx *outc,
                              const char *constant_name,
                              int value)
{
        outputf(outc,
                "\tduk_dup(ctx, 0);\n");
        outputf(outc,
                "\tduk_push_string(ctx, \"%s\");\n", constant_name);
        outputf(outc,
                "\tduk_push_int(ctx, %d);\n", value);
        outputf(outc,
                "\tduk_def_prop(ctx, -3, DUK_DEFPROP_HAVE_VALUE |\n");
        outputf(outc,
                "\t\t     DUK_DEFPROP_HAVE_WRITABLE |\n");
        outputf(outc,
                "\t\t     DUK_DEFPROP_HAVE_ENUMERABLE |\n");
        outputf(outc,
                "\t\t     DUK_DEFPROP_ENUMERABLE |\n");
        outputf(outc,
                "\t\t     DUK_DEFPROP_HAVE_CONFIGURABLE);\n");
        outputf(outc,
                "\tduk_pop(ctx);\n\n");
        return 0;
}


/**
 * generate code that gets a private pointer for a method
 */
static int
output_get_method_private(struct opctx *outc, char *class_name, bool is_global)
{
        outputf(outc,
                "\t/* Get private data for method */\n");
        outputf(outc,
                "\t%s_private_t *priv = NULL;\n", class_name);
        if (is_global) {
                outputf(outc,
                        "\tduk_push_global_object(ctx);\n");
        } else {
                outputf(outc,
                        "\tduk_push_this(ctx);\n");
        }
        outputf(outc,
                "\tduk_get_prop_string(ctx, -1, %s_magic_string_private);\n",
                DLPFX);
        outputf(outc,
                "\tpriv = duk_get_pointer(ctx, -1);\n");
        outputf(outc,
                "\tduk_pop_2(ctx);\n");
        outputf(outc,
                "\tif (priv == NULL) {\n");
        if (options->dbglog) {
                outputf(outc,
                        "\t\tNSLOG(dukky, INFO, \"priv failed\");\n");
        }
        outputf(outc,
                "\t\treturn 0; /* can do? No can do. */\n");
        outputf(outc,
                "\t}\n\n");

        return 0;
}


/**
 * generate the interface constructor
 */
static int
output_interface_constructor(struct opctx *outc, struct ir_entry *interfacee)
{
        int init_argc;

        /* constructor definition */
        outputf(outc,
                "static duk_ret_t %s_%s___constructor(duk_context *ctx)\n",
                DLPFX, interfacee->class_name);
        outputf(outc,
                "{\n");

        output_create_private(outc, interfacee->class_name);

        /* generate call to initialisor */
        outputf(outc,
                "\t%s_%s___init(ctx, priv",
                DLPFX, interfacee->class_name);

        for (init_argc = 1;
             init_argc <= interfacee->class_init_argc;
             init_argc++) {
		switch (interfacee->class_init_argt[init_argc-1]) {
		case IR_INIT_ARG_BOOL:
			outputf(outc,
				", duk_get_bool(ctx, %d)",
				init_argc);
			break;
		case IR_INIT_ARG_UNSIGNED:
		case IR_INIT_ARG_INT:
			outputf(outc,
				", duk_get_int(ctx, %d)",
				init_argc);
			break;
		case IR_INIT_ARG_POINTER:
			outputf(outc,
				", duk_get_pointer(ctx, %d)",
				init_argc);
			break;
		}
        }
        outputf(outc,
                ");\n");

        outputf(outc,
                "\tduk_set_top(ctx, 1);\n");
        outputf(outc,
                "\treturn 1;\n");
        outputf(outc,
                "}\n\n");

        return 0;
}


/**
 * generate the interface destructor
 */
static int
output_interface_destructor(struct opctx *outc, struct ir_entry *interfacee)
{
        /* destructor definition */
        outputf(outc,
                "static duk_ret_t %s_%s___destructor(duk_context *ctx)\n",
                DLPFX, interfacee->class_name);
        outputf(outc,
                "{\n");

        output_safe_get_private(outc, interfacee->class_name, 0);

        /* generate call to finaliser */
        outputf(outc,
                "\t%s_%s___fini(ctx, priv);\n",
                DLPFX, interfacee->class_name);

        outputf(outc,
                "\tfree(priv);\n");
        outputf(outc,
                "\treturn 0;\n");

        outputf(outc,
                "}\n\n");

        return 0;
}


/**
 * generate an initialisor call to parent interface
 */
static int
output_interface_inherit_init(struct opctx *outc,
                              struct ir_entry *interfacee,
                              struct ir_entry *inherite)
{
        struct genbind_node *init_node;
        struct genbind_node *inh_init_node;
        struct genbind_node *param_node;
        struct genbind_node *inh_param_node;

        /* only need to call parent initialisor if there is one */
        if (inherite == NULL) {
                return 0;
        }

        /* find the initialisor method on the class (if any) */
        init_node = genbind_node_find_method(interfacee->class,
                                             NULL,
                                             GENBIND_METHOD_TYPE_INIT);


        inh_init_node = genbind_node_find_method(inherite->class,
                                                 NULL,
                                                 GENBIND_METHOD_TYPE_INIT);


        outputf(outc,
                "\t%s_%s___init(ctx, &priv->parent",
                DLPFX, inherite->class_name);

        /* for each parameter in the parent find a matching named
         * parameter to pass and cast if necessary
         */

        inh_param_node = genbind_node_find_type(
                genbind_node_getnode(inh_init_node),
                NULL, GENBIND_NODE_TYPE_PARAMETER);
        while (inh_param_node != NULL) {
                char *param_name;
                param_name = genbind_node_gettext(
                        genbind_node_find_type(
                                genbind_node_getnode(inh_param_node),
                                NULL,
                                GENBIND_NODE_TYPE_IDENT));

                param_node = genbind_node_find_type_ident(
                        genbind_node_getnode(init_node),
                        NULL,
                        GENBIND_NODE_TYPE_PARAMETER,
                        param_name);
                if (param_node == NULL) {
                        fprintf(stderr,
                                "class \"%s\" (interface %s) parent class \"%s\" (interface %s) initialisor requires a parameter \"%s\" with compatible identifier\n",
                                interfacee->class_name,
                                interfacee->name,
                                inherite->class_name,
                                inherite->name,
                                param_name);
                        return -1;
                } else {
                        outputf(outc, ", ");

                        /* cast the parameter if required */
                        if (compare_ctypes(param_node,
                                           inh_param_node) == false) {
                                outputc(outc, '(');
                                output_ctype(outc, inh_param_node, false);
                                outputc(outc, ')');
                        }

                        /* output the parameter identifier */
                        output_cdata(outc, param_node, GENBIND_NODE_TYPE_IDENT);
                }

                inh_param_node = genbind_node_find_type(
                        genbind_node_getnode(inh_init_node),
                        inh_param_node, GENBIND_NODE_TYPE_METHOD);
        }

        outputf(outc, ");\n");

        return 0;
}

static enum ir_init_argtype
guess_argtype_from(struct genbind_node *param_node)
{
        const char *type_cdata = NULL;
        struct genbind_node *typename_node;
	bool unsigned_ = false;
	bool int_ = false;
	bool bool_ = false;

        typename_node = genbind_node_find_type(genbind_node_getnode(param_node),
                                               NULL,
                                               GENBIND_NODE_TYPE_NAME);
        while (typename_node != NULL) {
                type_cdata = genbind_node_gettext(typename_node);
		if (strcmp(type_cdata, "unsigned") == 0) {
			unsigned_ = true;
		} else if (strcmp(type_cdata, "int") == 0) {
			int_ = true;
		} else if (strcmp(type_cdata, "bool") == 0) {
			bool_ = true;
		}
                typename_node = genbind_node_find_type(
                        genbind_node_getnode(param_node),
                        typename_node,
                        GENBIND_NODE_TYPE_NAME);
	}

	if (type_cdata[0] == '*') {
		return IR_INIT_ARG_POINTER;
	} else if (unsigned_) {
		return IR_INIT_ARG_UNSIGNED;
	} else if (int_) {
		return IR_INIT_ARG_INT;
	} else if (bool_) {
		return IR_INIT_ARG_BOOL;
	}

	/* If we have no better idea do this */
	return IR_INIT_ARG_POINTER;
}

static int
output_interface_init_declaration(struct opctx *outc,
                                  struct ir_entry *interfacee,
                                  struct genbind_node *init_node)
{
        struct genbind_node *param_node;

        if  (interfacee->refcount == 0) {
                outputf(outc, "static ");
        }

        outputf(outc,
                "void %s_%s___init(duk_context *ctx, %s_private_t *priv",
                DLPFX, interfacee->class_name, interfacee->class_name);

        /* count the number of arguments on the initializer */
        interfacee->class_init_argc = 0;
	interfacee->class_init_argt = NULL;

        /* output the paramters on the method (if any) */
        param_node = genbind_node_find_type(
                genbind_node_getnode(init_node),
                NULL, GENBIND_NODE_TYPE_PARAMETER);
        while (param_node != NULL) {
                interfacee->class_init_argc++;
		interfacee->class_init_argt = realloc(interfacee->class_init_argt,
						      interfacee->class_init_argc * sizeof(enum ir_init_argtype));
		interfacee->class_init_argt[interfacee->class_init_argc - 1] =
			guess_argtype_from(param_node);
                outputf(outc, ", ");

                output_ctype(outc, param_node, true);

                param_node = genbind_node_find_type(
                        genbind_node_getnode(init_node),
                        param_node, GENBIND_NODE_TYPE_PARAMETER);
        }

        outputc(outc, ')');

        return 0;
}


/**
 * generate code for interface (class) initialisor
 */
static int
output_interface_init(struct opctx *outc,
                      struct ir_entry *interfacee,
                      struct ir_entry *inherite)
{
        struct genbind_node *init_node;
        int res;

        /* find the initialisor method on the class (if any) */
        init_node = genbind_node_find_method(interfacee->class,
                                             NULL,
                                             GENBIND_METHOD_TYPE_INIT);

        /* initialisor definition */
        output_interface_init_declaration(outc, interfacee, init_node);

        outputf(outc, "\n{\n");

        /* if this interface inherits ensure we call its initialisor */
        res = output_interface_inherit_init(outc, interfacee, inherite);
        if (res != 0) {
                return res;
        }

        /* generate log statement */
        if (options->dbglog) {
                outputf(outc,
                        "\tNSLOG(dukky, INFO, \"Initialise %%p (priv=%%p)\", duk_get_heapptr(ctx, 0), priv);\n" );
        }

        /* output the initaliser code from the binding */
        output_ccode(outc, init_node);

        outputf(outc, "}\n\n");

        return 0;

}


/**
 * generate code for interface (class) finaliser
 */
static int
output_interface_fini(struct opctx *outc,
                      struct ir_entry *interfacee,
                      struct ir_entry *inherite)
{
        struct genbind_node *fini_node;

        /* find the finaliser method on the class (if any) */
        fini_node = genbind_node_find_method(interfacee->class,
                                             NULL,
                                             GENBIND_METHOD_TYPE_FINI);

        /* finaliser definition */
        if  (interfacee->refcount == 0) {
                outputf(outc, "static ");
        }
        outputf(outc,
                "void %s_%s___fini(duk_context *ctx, %s_private_t *priv)\n",
                DLPFX, interfacee->class_name, interfacee->class_name);
        outputf(outc, "{\n");

        /* generate log statement */
        if (options->dbglog) {
                outputf(outc,
                        "\tNSLOG(dukky, INFO, \"Finalise %%p\", duk_get_heapptr(ctx, 0));\n" );
        }

        /* output the finialisor code from the binding */
        output_cdata(outc, fini_node, GENBIND_NODE_TYPE_CDATA);

        /* if this interface inherits ensure we call its finaliser */
        if (inherite != NULL) {
                outputf(outc,
                        "\t%s_%s___fini(ctx, &priv->parent);\n",
                        DLPFX, inherite->class_name);
        }
        outputf(outc, "}\n\n");

        return 0;
}


/**
 * generate a prototype add for a single class method
 */
static int
output_prototype_method(struct opctx *outc,
                        struct ir_entry *interfacee,
                        struct ir_operation_entry *operatione)
{

        if (operatione->name != NULL) {
                /* normal method on prototype */
                output_add_method(outc,
                                  interfacee->class_name,
                                  operatione->name);
        } else {
                /* special method on prototype */
                outputf(outc,
                     "\t/* Special method on prototype - UNIMPLEMENTED */\n\n");
        }

        return 0;
}


/**
 * generate prototype method definitions
 */
static int
output_prototype_methods(struct opctx *outc, struct ir_entry *entry)
{
        int opc;
        int res = 0;

        for (opc = 0; opc < entry->u.interface.operationc; opc++) {
                res = output_prototype_method(
                        outc,
                        entry,
                        entry->u.interface.operationv + opc);
                if (res != 0) {
                        break;
                }
        }

        return res;
}


static int
output_prototype_attribute(struct opctx *outc,
                           struct ir_entry *interfacee,
                           struct ir_attribute_entry *attributee)
{
    if ((attributee->putforwards == NULL) &&
        (attributee->modifier == WEBIDL_TYPE_MODIFIER_READONLY)) {
                return output_populate_ro_property(outc,
                                                   interfacee->class_name,
                                                   attributee->name);
        }
        return output_populate_rw_property(outc,
                                           interfacee->class_name,
                                           attributee->name);
}


/**
 * generate prototype attribute definitions
 */
static int
output_prototype_attributes(struct opctx *outc, struct ir_entry *entry)
{
        int attrc;
        int res = 0;

        for (attrc = 0; attrc < entry->u.interface.attributec; attrc++) {
                res = output_prototype_attribute(
                        outc,
                        entry,
                        entry->u.interface.attributev + attrc);
                if (res != 0) {
                        break;
                }
        }

        return res;
}


/**
 * output constants on the prototype
 *
 * \todo This implementation assumes the constant is a literal int and should
 * check the type node base value.
 */
static int
output_prototype_constant(struct opctx *outc,
                          struct ir_constant_entry *constante)
{
        int *value;

        value = webidl_node_getint(
                webidl_node_find_type(
                        webidl_node_getnode(constante->node),
                        NULL,
                        WEBIDL_NODE_TYPE_LITERAL_INT));

        output_prototype_constant_int(outc, constante->name, *value);

        return 0;
}


/**
 * generate prototype constant definitions
 */
static int
output_prototype_constants(struct opctx *outc, struct ir_entry *entry)
{
        int attrc;
        int res = 0;

        for (attrc = 0; attrc < entry->u.interface.constantc; attrc++) {
                res = output_prototype_constant(
                        outc,
                        entry->u.interface.constantv + attrc);
                if (res != 0) {
                        break;
                }
        }

        return res;
}


static int
output_global_create_prototype(struct opctx *outc,
                               struct ir *ir,
                               struct ir_entry *interfacee)
{
        int idx;

        outputf(outc,
                "\t/* Create interface objects */\n");
        for (idx = 0; idx < ir->entryc; idx++) {
                struct ir_entry *entry;

                entry = ir->entries + idx;

                if (entry->type == IR_ENTRY_TYPE_INTERFACE) {

                        if (entry->u.interface.noobject) {
                                continue;
                        }

                        if (entry == interfacee) {
                                outputf(outc,
                                        "\tduk_dup(ctx, 0);\n");
                        } else {
                                output_get_prototype(outc, entry->name);
                        }

                        outputf(outc,
                                "\tdukky_inject_not_ctr(ctx, 0, \"%s\");\n",
                                entry->name);
                }
        }
        return 0;
}


/**
 * generate the interface prototype creator
 */
static int
output_interface_prototype(struct opctx *outc,
                           struct ir *ir,
                           struct ir_entry *interfacee,
                           struct ir_entry *inherite)
{
        struct genbind_node *proto_node;

        /* find the prototype method on the class */
        proto_node = genbind_node_find_method(interfacee->class,
                                              NULL,
                                              GENBIND_METHOD_TYPE_PROTOTYPE);

        /* prototype definition */
        outputf(outc,
                "duk_ret_t %s_%s___proto(duk_context *ctx, void *udata)\n",
                DLPFX, interfacee->class_name);
        outputf(outc, "{\n");

        /* Output any binding data first */
        if (output_cdata(outc, proto_node, GENBIND_NODE_TYPE_CDATA) != 0) {
                outputf(outc,
                        "\n");
        }

        /* generate prototype chaining if interface has a parent */
        if (inherite != NULL) {
                outputf(outc,
                      "\t/* Set this prototype's prototype (left-parent) */\n");
                output_get_prototype(outc, inherite->name);
                outputf(outc,
                        "\tduk_set_prototype(ctx, 0);\n\n");
        }

        /* generate setting of methods */
        output_prototype_methods(outc, interfacee);

        /* generate setting of attributes */
        output_prototype_attributes(outc, interfacee);

        /* generate setting of constants */
        output_prototype_constants(outc, interfacee);

        /* if this is the global object, output all interfaces which do not
         * prevent us from doing so
         */
        if (interfacee->u.interface.primary_global) {
                output_global_create_prototype(outc, ir, interfacee);
        }

        /* generate setting of destructor */
        output_set_destructor(outc, interfacee->class_name, 0);

        /* generate setting of constructor */
        output_set_constructor(outc,
                               interfacee->class_name,
                               0,
                               interfacee->class_init_argc);

        outputf(outc,
                "\treturn 1; /* The prototype object */\n");

        outputf(outc,
                "}\n\n");

        return 0;
}


/**
 * generate a single class method for an interface operation with elipsis
 */
static int
output_interface_elipsis_operation(struct opctx *outc,
                                   struct ir_entry *interfacee,
                                   struct ir_operation_entry *operatione)
{
        int cdatac; /* cdata blocks output */

        /* overloaded method definition */
        outputf(outc,
                "static duk_ret_t %s_%s_%s(duk_context *ctx)\n",
                DLPFX, interfacee->class_name, operatione->name);
        outputf(outc,
                "{\n");

        /**
         * \todo This is where the checking of the parameters to the
         * operation with elipsis should go
         */
        WARN(WARNING_UNIMPLEMENTED,
             "Elipsis parameters not checked: method %s::%s();",
                     interfacee->name, operatione->name);

        output_get_method_private(outc, interfacee->class_name,
                                  interfacee->u.interface.primary_global);

        cdatac = output_ccode(outc, operatione->method);
        if (cdatac == 0) {
                /* no implementation so generate default */
                WARN(WARNING_UNIMPLEMENTED,
                     "Unimplemented: method %s::%s();",
                     interfacee->name, operatione->name);
                outputf(outc,
                        "\treturn 0;\n");
        }

        outputf(outc,
                "}\n\n");

        return 0;
}


/**
 * generate a single class method for an interface overloaded operation
 */
static int
output_interface_overloaded_operation(struct opctx *outc,
                                      struct ir_entry *interfacee,
                                      struct ir_operation_entry *operatione)
{
        int cdatac; /* cdata blocks output */

        /* overloaded method definition */
        outputf(outc,
                "static duk_ret_t %s_%s_%s(duk_context *ctx)\n",
                DLPFX, interfacee->class_name, operatione->name);
        outputf(outc,
                "{\n");

        /** \todo This is where the checking of the parameters to the
         * overloaded operation should go
         */

        output_get_method_private(outc, interfacee->class_name,
                                  interfacee->u.interface.primary_global);

        cdatac = output_ccode(outc,
                              operatione->method);

        if (cdatac == 0) {
                /* no implementation so generate default */
                WARN(WARNING_UNIMPLEMENTED,
                     "Unimplemented: method %s::%s();",
                     interfacee->name, operatione->name);
                outputf(outc,
                        "\treturn 0;\n");
        }

        outputf(outc,
                "}\n\n");

        return 0;
}


/**
 * generate a single class method for an interface special operation
 */
static int
output_interface_special_operation(struct opctx *outc,
                                   struct ir_entry *interfacee,
                                   struct ir_operation_entry *operatione)
{
        /* special method definition */
        outputf(outc,
                "/* Special method definition - UNIMPLEMENTED */\n\n");

        WARN(WARNING_UNIMPLEMENTED,
             "Special operation on interface %s (operation entry %p)",
             interfacee->name,
             operatione);

        return 0;
}


/**
 * generate default values on the duk stack
 */
static int
output_operation_optional_defaults(
        struct opctx *outc,
        struct ir_operation_argument_entry *argumentv,
        int argumentc)
{
        int argc;
        for (argc = 0; argc < argumentc; argc++) {
                struct ir_operation_argument_entry *cure;
                struct webidl_node *lit_node; /* literal node */
                enum webidl_node_type lit_type;
                int *lit_int;
                char *lit_str;

                cure = argumentv + argc;

                lit_node = webidl_node_getnode(
                        webidl_node_find_type(
                                webidl_node_getnode(cure->node),
                                NULL,
                                WEBIDL_NODE_TYPE_OPTIONAL));

                if (lit_node != NULL) {

                        lit_type = webidl_node_gettype(lit_node);

                        switch (lit_type) {
                        case WEBIDL_NODE_TYPE_LITERAL_NULL:
                                outputf(outc,
                                        "\t\tduk_push_null(ctx);\n");
                                break;

                        case WEBIDL_NODE_TYPE_LITERAL_INT:
                                lit_int = webidl_node_getint(lit_node);
                                outputf(outc,
                                        "\t\tduk_push_int(ctx, %d);\n",
                                        *lit_int);
                                break;

                        case WEBIDL_NODE_TYPE_LITERAL_BOOL:
                                lit_int = webidl_node_getint(lit_node);
                                outputf(outc,
                                        "\t\tduk_push_boolean(ctx, %d);\n",
                                        *lit_int);
                                break;

                        case WEBIDL_NODE_TYPE_LITERAL_STRING:
                                lit_str = webidl_node_gettext(lit_node);
                                outputf(outc,
                                        "\t\tduk_push_string(ctx, \"%s\");\n",
                                        lit_str);
                                break;

                        case WEBIDL_NODE_TYPE_LITERAL_FLOAT:
                        default:
                                outputf(outc,
                                        "\t\tduk_push_undefined(ctx);\n");
                                break;
                        }
                } else {
                        outputf(outc,
                                "\t\tduk_push_undefined(ctx);\n");
                }
        }
        return 0;
}


static int
output_operation_argument_type_check(
        struct opctx *outc,
        struct ir_entry *interfacee,
        struct ir_operation_entry *operatione,
        struct ir_operation_overload_entry *overloade,
        int argidx)
{
        struct ir_operation_argument_entry *argumente;
        struct webidl_node *type_node;
        enum webidl_type *argument_type;

        argumente = overloade->argumentv + argidx;

        type_node = webidl_node_find_type(
                webidl_node_getnode(argumente->node),
                NULL,
                WEBIDL_NODE_TYPE_TYPE);

        if (type_node == NULL) {
                fprintf(stderr, "%s:%s %dth argument %s has no type\n",
                        interfacee->name,
                        operatione->name,
                        argidx,
                        argumente->name);
                return -1;
        }

        argument_type = (enum webidl_type *)webidl_node_getint(
                webidl_node_find_type(
                        webidl_node_getnode(type_node),
                        NULL,
                        WEBIDL_NODE_TYPE_TYPE_BASE));

        if (argument_type == NULL) {
                fprintf(stderr,
                        "%s:%s %dth argument %s has no type base\n",
                        interfacee->name,
                        operatione->name,
                        argidx,
                        argumente->name);
                return -1;
        }

        if (*argument_type == WEBIDL_TYPE_ANY) {
                /* allowing any type needs no check */
                return 0;
        }

        outputf(outc,
                "\tif (%s_argc > %d) {\n", DLPFX, argidx);

        switch (*argument_type) {
        case WEBIDL_TYPE_STRING:
                /* coerce values to string */
                outputf(outc,
                        "\t\tif (!duk_is_string(ctx, %d)) {\n"
                        "\t\t\tduk_to_string(ctx, %d);\n"
                        "\t\t}\n", argidx, argidx);
                break;

        case WEBIDL_TYPE_BOOL:
                outputf(outc,
			"\t\tif (!duk_is_boolean(ctx, %d)) {\n"
			"\t\t\tif (duk_is_number(ctx, %d)) {\n"
			"\t\t\t\tduk_to_boolean(ctx, %d);\n"
			"\t\t\t} else {\n"
			"\t\t\t\treturn duk_error(ctx, DUK_ERR_ERROR, %s_error_fmt_bool_type, %d, \"%s\");\n"
			"\t\t\t}\n"
			"\t\t}\n", argidx, argidx, argidx, DLPFX, argidx, argumente->name);
                break;

        case WEBIDL_TYPE_FLOAT:
        case WEBIDL_TYPE_DOUBLE:
        case WEBIDL_TYPE_SHORT:
        case WEBIDL_TYPE_LONG:
        case WEBIDL_TYPE_LONGLONG:
                outputf(outc,
                        "\t\tif (!duk_is_number(ctx, %d)) {\n"
                        "\t\t\treturn duk_error(ctx, DUK_ERR_ERROR, %s_error_fmt_number_type, %d, \"%s\");\n"
                        "\t\t}\n",
                        argidx, DLPFX, argidx, argumente->name);
                break;


        default:
                outputf(outc,
                        "\t\t/* unhandled type check */\n");
        }

        outputf(outc,
                "\t}\n");

        return 0;
}


/**
 * generate a single class method for an interface operation
 */
static int
output_interface_operation(struct opctx *outc,
                           struct ir_entry *interfacee,
                           struct ir_operation_entry *operatione)
{
        int cdatac; /* cdata blocks output */
        struct ir_operation_overload_entry *overloade;
        int fixedargc; /* number of non optional arguments */
        int argidx; /* loop counter for arguments */
        int optargc; /* loop counter for optional arguments */

        if (operatione->name == NULL) {
                return output_interface_special_operation(outc,
                                                          interfacee,
                                                          operatione);
        }

        if (operatione->overloadc != 1) {
                return output_interface_overloaded_operation(outc,
                                                             interfacee,
                                                             operatione);
        }

        if (operatione->overloadv->elipsisc != 0) {
                return output_interface_elipsis_operation(outc,
                                                          interfacee,
                                                          operatione);
        }

        /* normal method definition */
        overloade = operatione->overloadv;

        outputf(outc,
                "static duk_ret_t %s_%s_%s(duk_context *ctx)\n",
                DLPFX, interfacee->class_name, operatione->name);
        outputf(outc,
                "{\n");

        /* check arguments */

        /* generate check for minimum number of parameters */

        fixedargc = overloade->argumentc - overloade->optionalc;

        outputf(outc,
                "\t/* ensure the parameters are present */\n"
                "\tduk_idx_t %s_argc = duk_get_top(ctx);\n\t", DLPFX);

        if (fixedargc > 0) {
                outputf(outc,
                        "if (%s_argc < %d) {\n",
                        DLPFX, fixedargc);
                outputf(outc,
                        "\t\t/* not enough arguments */\n");
                outputf(outc,
                        "\t\treturn duk_error(ctx, DUK_RET_TYPE_ERROR, %s_error_fmt_argument, %d, %s_argc);\n",
                        DLPFX, fixedargc, DLPFX);
                outputf(outc,
                        "\t} else ");
        }

        for (optargc = fixedargc;
             optargc < overloade->argumentc;
             optargc++) {
                outputf(outc,
                        "if (%s_argc == %d) {\n"
                        "\t\t/* %d optional arguments need adding */\n",
                        DLPFX,
                        optargc,
                        overloade->argumentc - optargc);
                output_operation_optional_defaults(outc,
                        overloade->argumentv + optargc,
                        overloade->argumentc - optargc);
                outputf(outc,
                        "\t} else ");
        }

        outputf(outc,
                "if (%s_argc > %d) {\n"
                "\t\t/* remove extraneous parameters */\n"
                "\t\tduk_set_top(ctx, %d);\n"
                "\t}\n",
                DLPFX,
                overloade->argumentc,
                overloade->argumentc);
        outputf(outc,
                "\n");

        /* generate argument type checks */

        outputf(outc,
                "\t/* check types of passed arguments are correct */\n");

        for (argidx = 0; argidx < overloade->argumentc; argidx++) {
                output_operation_argument_type_check(outc,
                                                     interfacee,
                                                     operatione,
                                                     overloade,
                                                     argidx);
       }

        output_get_method_private(outc, interfacee->class_name,
                                  interfacee->u.interface.primary_global);

        cdatac = output_ccode(outc, operatione->method);
        if (cdatac == 0) {
                /* no implementation so generate default */
                WARN(WARNING_UNIMPLEMENTED,
                     "Unimplemented: method %s::%s();",
                     interfacee->name, operatione->name);

                if (options->dbglog) {
                        outputf(outc,
                              "\tNSLOG(dukky, WARNING, \"Unimplemented\");\n");
                }

                outputf(outc,
                        "\treturn 0;\n");
        }

        outputf(outc,
                "}\n\n");

        return 0;
}

/**
 * generate class methods for each interface operation
 */
static int
output_interface_operations(struct opctx *outc, struct ir_entry *ife)
{
        int opc;
        int res = 0;

        for (opc = 0; opc < ife->u.interface.operationc; opc++) {
                res = output_interface_operation(
                        outc,
                        ife,
                        ife->u.interface.operationv + opc);
                if (res != 0) {
                        break;
                }
        }

        return res;
}



/**
 * Output class property getter for a single attribute
 */
static int
output_attribute_getter(struct opctx *outc,
                        struct ir_entry *interfacee,
                        struct ir_attribute_entry *atributee)
{
        /* getter definition */
        outputf(outc,
                "static duk_ret_t %s_%s_%s_getter(duk_context *ctx)\n",
                DLPFX, interfacee->class_name, atributee->name);
        outputf(outc,
                "{\n");

        output_get_method_private(outc,
                                  interfacee->class_name,
                                  interfacee->u.interface.primary_global);

        /* if binding available for this attribute getter process it */
        if (atributee->getter != NULL) {
                int res;
                res = output_ccode(outc, atributee->getter);
                if (res == 0) {
                        /* no code provided for this getter so generate */
                        res = output_generated_attribute_getter(outc,
                                                                interfacee,
                                                                atributee);
                }
                if (res >= 0) {
                        outputf(outc,
                                "}\n\n");
                        return res;
                }
        }

        /* no implementation so generate default and warnings if required */
        const char *type_str;
        if (atributee->typec == 0) {
                type_str = "";
        } else if (atributee->typec == 1) {
                type_str = webidl_type_to_str(atributee->typev[0].modifier,
                                              atributee->typev[0].base);
        } else {
                type_str = "multiple";
        }

        WARN(WARNING_UNIMPLEMENTED,
             "Unimplemented: getter %s::%s(%s);",
             interfacee->name,
             atributee->name,
             type_str);

        if (options->dbglog) {
                outputf(outc,
                        "\tNSLOG(dukky, WARNING, \"Unimplemented\");\n" );
        }

        outputf(outc,
                "\treturn 0;\n"
                "}\n\n");

        return 0;
}


/**
 * Generate class property setter for a putforwards attribute
 */
static int
output_putforwards_setter(struct opctx *outc,
                          struct ir_entry *interfacee,
                          struct ir_attribute_entry *atributee)
{
        /* generate autogenerated putforwards */

        outputf(outc,
                "\tduk_ret_t get_ret;\n\n");

        outputf(outc,
                "\tget_ret = %s_%s_%s_getter(ctx);\n",
                DLPFX, interfacee->class_name, atributee->name);

        outputf(outc,
                "\tif (get_ret != 1) {\n"
                "\t\treturn 0;\n"
                "\t}\n\n"
                "\t/* parameter ... attribute */\n\n"
                "\tduk_dup(ctx, 0);\n"
                "\t/* ... attribute parameter */\n\n"
                "\t/* call the putforward */\n");

        outputf(outc,
                "\tduk_put_prop_string(ctx, -2, \"%s\");\n\n",
                atributee->putforwards);

        outputf(outc,
                "\treturn 0;\n");

        return 0;
}


/**
 * Generate class property setter for a single attribute
 */
static int
output_attribute_setter(struct opctx *outc,
                        struct ir_entry *interfacee,
                        struct ir_attribute_entry *atributee)
{
        int res = -1;

       /* setter definition */
        outputf(outc,
                "static duk_ret_t %s_%s_%s_setter(duk_context *ctx)\n",
                DLPFX, interfacee->class_name, atributee->name);
        outputf(outc,
                "{\n");

        output_get_method_private(outc,
                                  interfacee->class_name,
                                  interfacee->u.interface.primary_global);

        /* if binding available for this attribute getter process it */
        if (atributee->setter != NULL) {
                res = output_ccode(outc, atributee->setter);
                if (res == 0) {
                        /* no code provided for this setter so generate */
                        res = output_generated_attribute_setter(outc,
                                                                interfacee,
                                                                atributee);
                }
        } else if (atributee->putforwards != NULL) {
                res = output_putforwards_setter(outc,
                                                interfacee,
                                                atributee);
        }

        /* implementation not generated from any other source */
        if (res < 0) {
                const char *type_str;
                if (atributee->typec == 0) {
                        type_str = "";
                } else if (atributee->typec == 1) {
                        type_str = webidl_type_to_str(
                                atributee->typev[0].modifier,
                                atributee->typev[0].base);
                } else {
                        type_str = "multiple";
                }
                WARN(WARNING_UNIMPLEMENTED,
                     "Unimplemented: setter %s::%s(%s);",
                     interfacee->name,
                     atributee->name,
                     type_str);
                if (options->dbglog) {
                        outputf(outc,
                               "\tNSLOG(dukky, WARNING, \"Unimplemented\");\n");
                }

                /* no implementation so generate default */
                outputf(outc,
                        "\treturn 0;\n");
        }

        outputf(outc,
                "}\n\n");

        return res;
}


/**
 * Generate class property getter/setter for a single attribute
 */
static int
output_interface_attribute(struct opctx *outc,
                           struct ir_entry *interfacee,
                           struct ir_attribute_entry *atributee)
{
        int res;

        if (atributee->property_name == NULL) {
            atributee->property_name = gen_idl2c_name(atributee->name);
        }

        res = output_attribute_getter(outc, interfacee, atributee);

        /* only read/write and putforward attributes have a setter */
        if ((atributee->modifier != WEBIDL_TYPE_MODIFIER_READONLY) ||
            (atributee->putforwards != NULL)) {
                res = output_attribute_setter(outc, interfacee, atributee);
        }

        return res;
}


/**
 * generate class property getters and setters for each interface attribute
 */
static int
output_interface_attributes(struct opctx *outc, struct ir_entry *ife)
{
        int attrc;

        for (attrc = 0; attrc < ife->u.interface.attributec; attrc++) {
                output_interface_attribute(
                        outc,
                        ife,
                        ife->u.interface.attributev + attrc);
        }

        return 0;
}


/*
 * generate a source file to implement an interface using duk and libdom.
 *
 * exported interface documented in duk-libdom.h
 */
int output_interface(struct ir *ir, struct ir_entry *interfacee)
{
        struct opctx *ifop;
        struct ir_entry *inherite;
        int res = 0;

        /* open the output */
        res = output_open(interfacee->filename, &ifop);
        if (res !=0 ) {
                return res;
        }

        /* find parent interface entry */
        inherite = ir_inherit_entry(ir, interfacee);

        /* tool preface */
        output_tool_preface(ifop);

        /* binding preface */
        output_method_cdata(ifop,
                            ir->binding_node,
                            GENBIND_METHOD_TYPE_PREFACE);

        /* class preface */
        output_method_cdata(ifop,
                            interfacee->class,
                            GENBIND_METHOD_TYPE_PREFACE);

        /* tool prologue */
        output_tool_prologue(ifop);

        /* binding prologue */
        output_method_cdata(ifop,
                            ir->binding_node,
                            GENBIND_METHOD_TYPE_PROLOGUE);

        /* class prologue */
        output_method_cdata(ifop,
                            interfacee->class,
                            GENBIND_METHOD_TYPE_PROLOGUE);

        outputf(ifop,
                "\n");

        /* initialisor */
        res = output_interface_init(ifop, interfacee, inherite);
        if (res != 0) {
                goto op_error;
        }

        /* finaliser */
        output_interface_fini(ifop, interfacee, inherite);

        /* constructor */
        output_interface_constructor(ifop, interfacee);

        /* destructor */
        output_interface_destructor(ifop, interfacee);

        /* operations */
        output_interface_operations(ifop, interfacee);

        /* attributes */
        output_interface_attributes(ifop, interfacee);

        /* prototype */
        output_interface_prototype(ifop, ir, interfacee, inherite);

        outputf(ifop, "\n");

        /* class epilogue */
        output_method_cdata(ifop,
                            interfacee->class,
                            GENBIND_METHOD_TYPE_EPILOGUE);

        /* binding epilogue */
        output_method_cdata(ifop,
                            ir->binding_node,
                            GENBIND_METHOD_TYPE_EPILOGUE);

        /* class postface */
        output_method_cdata(ifop,
                            interfacee->class,
                            GENBIND_METHOD_TYPE_POSTFACE);

        /* binding postface */
        output_method_cdata(ifop,
                            ir->binding_node,
                            GENBIND_METHOD_TYPE_POSTFACE);

op_error:
        output_close(ifop);

        return res;
}


/* exported function documented in duk-libdom.h */
int
output_interface_declaration(struct opctx *outc, struct ir_entry *interfacee)
{
        struct genbind_node *init_node;

        /* do not generate prototype declarations for interfaces marked no
         * output
         */
        if (interfacee->u.interface.noobject) {
                return 0;
        }

        /* prototype declaration */
        outputf(outc,
                "duk_ret_t %s_%s___proto(duk_context *ctx, void *udata);\n",
                DLPFX, interfacee->class_name);

        /* if the interface has no references (no other interface inherits from
         * it) there is no reason to export the initalisor/finaliser as no
         * other class constructor/destructor should call them.
         */
        if (interfacee->refcount < 1) {
                outputf(outc,
                        "\n");
                return 0;
        }

        /* finaliser declaration */
        outputf(outc,
                "void %s_%s___fini(duk_context *ctx, %s_private_t *priv);\n",
                DLPFX, interfacee->class_name, interfacee->class_name);

        /* find the initialisor method on the class (if any) */
        init_node = genbind_node_find_method(interfacee->class,
                                             NULL,
                                             GENBIND_METHOD_TYPE_INIT);

        /* initialisor definition */
        output_interface_init_declaration(outc, interfacee, init_node);

        outputf(outc,
                ";\n\n");

        return 0;
}
