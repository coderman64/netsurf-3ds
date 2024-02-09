/* intermediate representation of WebIDL and binding data
 *
 * This file is part of nsgenbind.
 * Published under the MIT License,
 *                http://www.opensource.org/licenses/mit-license.php
 * Copyright 2015 Vincent Sanders <vince@netsurf-browser.org>
 */

#include <stdbool.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "options.h"
#include "utils.h"
#include "nsgenbind-ast.h"
#include "webidl-ast.h"
#include "ir.h"

/** count the number of nodes of a given type on an interface */
static int
enumerate_interface_type(struct webidl_node *interface_node,
                         enum webidl_node_type node_type)
{
        int count = 0;
        struct webidl_node *members_node;

        members_node = webidl_node_find_type(
                webidl_node_getnode(interface_node),
                NULL,
                WEBIDL_NODE_TYPE_LIST);
        while (members_node != NULL) {
                count += webidl_node_enumerate_type(
                        webidl_node_getnode(members_node),
                        node_type);

                members_node = webidl_node_find_type(
                        webidl_node_getnode(interface_node),
                        members_node,
                        WEBIDL_NODE_TYPE_LIST);
        }

        return count;
}

/* find index of inherited node if it is one of those listed in the
 * binding also maintain refcounts
 */
static void
compute_inherit_refcount(struct ir_entry *entries, int entryc)
{
        int idx;
        int inf;

        for (idx = 0; idx < entryc; idx++ ) {
                entries[idx].inherit_idx = -1;
                for (inf = 0; inf < entryc; inf++ ) {
                        /* cannot inherit from self and name must match */
                        if ((inf != idx) &&
                            (entries[idx].inherit_name != NULL ) &&
                            (strcmp(entries[idx].inherit_name,
                                    entries[inf].name) == 0)) {
                                entries[idx].inherit_idx = inf;
                                entries[inf].refcount++;
                                break;
                        }
                }
        }
}

/** Topoligical sort based on the refcount
 *
 * do not need to consider loops as constructed graph is a acyclic
 *
 * alloc a second copy of the map
 * repeat until all entries copied:
 *   walk source mapping until first entry with zero refcount
 *   put the entry  at the end of the output map
 *   reduce refcount on inherit index if !=-1
 *   remove entry from source map
 */
static struct ir_entry *
entry_topoligical_sort(struct ir_entry *srcinf, int infc)
{
        struct ir_entry *dstinf;
        int idx;
        int inf;

        dstinf = calloc(infc, sizeof(struct ir_entry));
        if (dstinf == NULL) {
                return NULL;
        }

        for (idx = infc - 1; idx >= 0; idx--) {
                /* walk source map until first valid entry with zero refcount */
                inf = 0;
                while ((inf < infc) &&
                       ((srcinf[inf].name == NULL) ||
                        (srcinf[inf].refcount > 0))) {
                        inf++;
                }
                if (inf == infc) {
                        free(dstinf);
                        return NULL;
                }

                /* copy entry to the end of the output map */
                dstinf[idx].name = srcinf[inf].name;
                dstinf[idx].node = srcinf[inf].node;
                dstinf[idx].inherit_name = srcinf[inf].inherit_name;
                dstinf[idx].class = srcinf[inf].class;
                dstinf[idx].type = srcinf[inf].type;
                dstinf[idx].u = srcinf[inf].u;

                /* reduce refcount on inherit index if !=-1 */
                if (srcinf[inf].inherit_idx != -1) {
                        srcinf[srcinf[inf].inherit_idx].refcount--;
                }

                /* remove entry from source map */
                srcinf[inf].name = NULL;
        }

        return dstinf;
}

static struct ir_operation_entry *
find_operation_name(struct ir_operation_entry *operationv,
                     int operationc,
                     const char *name)
{
        struct ir_operation_entry *cure;
        int opc;

        for (opc = 0; opc < operationc; opc++) {
                cure = operationv + opc;

                if (cure->name == name) {
                        /* check pointers for equivalence */
                        return cure;
                } else {
                        if ((cure->name != NULL) &&
                            (name != NULL) &&
                            (strcmp(cure->name, name) == 0)) {
                                return cure;
                        }
                }
        }

        return NULL;
}

static int
argument_map_new(struct webidl_node *arg_list_node,
                 int *argumentc_out,
                 struct ir_operation_argument_entry **argumentv_out)
{
        int argumentc;
        struct webidl_node *argument;
        struct ir_operation_argument_entry *argumentv;
        struct ir_operation_argument_entry *cure;

        argumentc = webidl_node_enumerate_type(
                            webidl_node_getnode(arg_list_node),
                            WEBIDL_NODE_TYPE_ARGUMENT);
        if (argumentc == 0) {
                *argumentc_out = 0;
                *argumentv_out = NULL;
                return 0;
        }

        argumentv = calloc(argumentc, sizeof(*argumentv));
        cure = argumentv;

        /* iterate each argument node within the list */
        argument = webidl_node_find_type(webidl_node_getnode(arg_list_node),
                                         NULL,
                                         WEBIDL_NODE_TYPE_ARGUMENT);

        while (argument != NULL) {

                cure->name = webidl_node_gettext(
                                webidl_node_find_type(
                                        webidl_node_getnode(argument),
                                        NULL,
                                        WEBIDL_NODE_TYPE_IDENT));

                cure->node = argument;

                cure->optionalc = webidl_node_enumerate_type(
                        webidl_node_getnode(argument),
                        WEBIDL_NODE_TYPE_OPTIONAL);

                cure->elipsisc = webidl_node_enumerate_type(
                        webidl_node_getnode(argument),
                        WEBIDL_NODE_TYPE_ELLIPSIS);

                cure++;

                argument = webidl_node_find_type(
                        webidl_node_getnode(arg_list_node),
                        argument,
                        WEBIDL_NODE_TYPE_ARGUMENT);
        }

        *argumentc_out = argumentc;
        *argumentv_out = argumentv;

        return 0;
}

/**
 * create a new overloaded parameter set on an operation
 *
 * each operation can be overloaded with multiple function signatures. By
 * adding them to the operation as overloads duplicate operation enrtries is
 * avoided.
 */
static int
overload_map_new(struct webidl_node *op_node,
                 int *overloadc_out,
                 struct ir_operation_overload_entry **overloadv_out)
{
        int overloadc = *overloadc_out;
        struct ir_operation_overload_entry *overloadv;
        struct ir_operation_overload_entry *cure;
        struct webidl_node *arg_list_node;
        int argc;

        /* update allocation */
        overloadc++;
        overloadv = realloc(*overloadv_out, overloadc * sizeof(*overloadv));
        if (overloadv == NULL) {
                return -1;
        }

        /* get added entry */
        cure = overloadv + (overloadc - 1);

        /* clear entry */
        cure = memset(cure, 0, sizeof(*cure));

        /* return type */
        cure->type = webidl_node_find_type(webidl_node_getnode(op_node),
                                           NULL,
                                           WEBIDL_NODE_TYPE_TYPE);

        arg_list_node = webidl_node_find_type(webidl_node_getnode(op_node),
                                              NULL,
                                              WEBIDL_NODE_TYPE_LIST);
        if (arg_list_node != NULL) {
                argument_map_new(arg_list_node,
                                 &cure->argumentc,
                                 &cure->argumentv);
        }

        for (argc = 0; argc < cure->argumentc; argc++) {
                struct ir_operation_argument_entry *arge;
                arge = cure->argumentv + argc;
                cure->optionalc += arge->optionalc;
                cure->elipsisc += arge->elipsisc;
        }

        /* return entry list */
        *overloadc_out = overloadc;
        *overloadv_out = overloadv;

        return 0;
}

static int
operation_map_new(struct webidl_node *interface,
                  struct genbind_node *class,
                  int *operationc_out,
                  struct ir_operation_entry **operationv_out)
{
        struct webidl_node *list_node;
        struct webidl_node *op_node; /* attribute node */
        struct ir_operation_entry *cure; /* current entry */
        struct ir_operation_entry *operationv;
        int operationc;

        /* enumerate operationss including overloaded members */
        operationc = enumerate_interface_type(interface,
                                              WEBIDL_NODE_TYPE_OPERATION);

        if (operationc < 1) {
                /* no operations so empty map */
                *operationc_out = 0;
                *operationv_out = NULL;
                return 0;
        }

        operationv = calloc(operationc,
                            sizeof(struct ir_operation_entry));
        if (operationv == NULL) {
                return -1;
        };
        cure = operationv;

        /* iterate each list node within the interface */
        list_node = webidl_node_find_type(
                webidl_node_getnode(interface),
                NULL,
                WEBIDL_NODE_TYPE_LIST);

        while (list_node != NULL) {
                /* iterate through operations on list */
                op_node = webidl_node_find_type(
                        webidl_node_getnode(list_node),
                        NULL,
                        WEBIDL_NODE_TYPE_OPERATION);

                while (op_node != NULL) {
                        const char *operation_name;
                        struct ir_operation_entry *finde;

                        /* get operation name */
                        operation_name = webidl_node_gettext(
                                webidl_node_find_type(
                                        webidl_node_getnode(op_node),
                                        NULL,
                                        WEBIDL_NODE_TYPE_IDENT));
                        /* if this operation is already an entry in the list
                         * augment that entry else create a new one
                         */
                        finde = find_operation_name(operationv,
                                                    operationc,
                                                    operation_name);
                        if (finde == NULL) {
                                /* operation does not already exist in list */

                                cure->name = operation_name;

                                cure->node = op_node;

                                cure->method = genbind_node_find_method_ident(
                                               class,
                                               NULL,
                                               GENBIND_METHOD_TYPE_METHOD,
                                               cure->name);

                                overload_map_new(op_node,
                                                 &cure->overloadc,
                                                 &cure->overloadv);

                                cure++; /* advance to next entry */
                        } else {
                                overload_map_new(op_node,
                                                 &finde->overloadc,
                                                 &finde->overloadv);
                                /* Overloaded entry does not advance the
                                 * current entry but does reduce list
                                 * length. Do not bother shortening allocation.
                                 */
                                operationc--;
                        }

                        /* move to next operation */
                        op_node = webidl_node_find_type(
                                webidl_node_getnode(list_node),
                                op_node,
                                WEBIDL_NODE_TYPE_OPERATION);
                }

                list_node = webidl_node_find_type(
                        webidl_node_getnode(interface),
                        list_node,
                        WEBIDL_NODE_TYPE_LIST);
        }

        *operationc_out = operationc;
        *operationv_out = operationv; /* resulting operations map */

        return 0;
}

/**
 * get the value of an extended attribute key/value item
 */
static char *
get_extended_value(struct webidl_node *node, const char *key)
{
        char *ident;
        struct webidl_node *ext_attr;
        struct webidl_node *elem;

        /* walk each extended attribute */
        ext_attr = webidl_node_find_type(
                webidl_node_getnode(node),
                NULL,
                WEBIDL_NODE_TYPE_EXTENDED_ATTRIBUTE);
        while (ext_attr != NULL) {

                elem = webidl_node_find_type(
                        webidl_node_getnode(ext_attr),
                        NULL,
                        WEBIDL_NODE_TYPE_IDENT);
                ident = webidl_node_gettext(elem);

                if ((ident != NULL) && (strcmp(ident, key) == 0)) {
                        /* first identifier matches */

                        elem = webidl_node_find_type(
                                webidl_node_getnode(ext_attr),
                                elem,
                                WEBIDL_NODE_TYPE_IDENT);
                        ident = webidl_node_gettext(elem);

                        if ((ident != NULL) && (*ident == '=')) {
                                return webidl_node_gettext(
                                        webidl_node_find_type(
                                                webidl_node_getnode(ext_attr),
                                                elem,
                                                WEBIDL_NODE_TYPE_IDENT));
                        }
                }

                ext_attr = webidl_node_find_type(
                        webidl_node_getnode(node),
                        ext_attr,
                        WEBIDL_NODE_TYPE_EXTENDED_ATTRIBUTE);
        }

        return NULL;
}


/**
 * Create IR entry for a type
 */
static int
type_map_new(struct webidl_node *node,
             int *typec_out,
             struct ir_type_entry **typev_out)
{
        int typec;
        struct webidl_node *type_node;
        struct ir_type_entry *typev;
        struct ir_type_entry *cure;

        typec = webidl_node_enumerate_type(
                            webidl_node_getnode(node),
                            WEBIDL_NODE_TYPE_TYPE);
        if (typec == 0) {
                *typec_out = 0;
                *typev_out = NULL;
                return 0;
        }

        typev = calloc(typec, sizeof(*typev));
        cure = typev;

        type_node = webidl_node_find_type(webidl_node_getnode(node),
                                          NULL,
                                          WEBIDL_NODE_TYPE_TYPE);

        while (type_node != NULL) {
                enum webidl_type *base;
                enum webidl_type_modifier *modifier;

                /* type base */
                base = (enum webidl_type *)webidl_node_getint(
                        webidl_node_find_type(
                                webidl_node_getnode(type_node),
                                NULL,
                                WEBIDL_NODE_TYPE_TYPE_BASE));
                if (base != NULL) {
                        cure->base = *base;
                }

                /* type modifier */
                modifier = (enum webidl_type_modifier *)webidl_node_getint(
                        webidl_node_find_type(
                                webidl_node_getnode(type_node),
                                NULL,
                                WEBIDL_NODE_TYPE_MODIFIER));
                if (modifier != NULL) {
                        cure->modifier = *modifier;
                } else {
                        cure->modifier = WEBIDL_TYPE_MODIFIER_NONE;
                }

                /* type nullability */
                cure->nullable = (webidl_node_find_type(
                        webidl_node_getnode(type_node),
                        NULL,
                        WEBIDL_NODE_TYPE_TYPE_NULLABLE) != NULL);

                /* type name */
                cure->name = webidl_node_gettext(
                        webidl_node_find_type(
                                webidl_node_getnode(type_node),
                                NULL,
                                WEBIDL_NODE_TYPE_IDENT));

                /* next entry */
                cure++;

                type_node = webidl_node_find_type(
                        webidl_node_getnode(node),
                        type_node,
                        WEBIDL_NODE_TYPE_TYPE);
        }

        *typec_out = typec;
        *typev_out = typev;

        return 0;
}

/**
 * Create a new ir entry for an attribute
 */
static int
attribute_map_new(struct webidl_node *interface,
                  struct genbind_node *class,
                  int *attributec_out,
                  struct ir_attribute_entry **attributev_out)
{
        struct webidl_node *list_node;
        struct webidl_node *at_node; /* attribute node */
        struct ir_attribute_entry *cure; /* current entry */
        struct ir_attribute_entry *attributev;
        int attributec;

        /* enumerate attributes */
        attributec = enumerate_interface_type(interface,
                                              WEBIDL_NODE_TYPE_ATTRIBUTE);
        *attributec_out = attributec;

        if (attributec < 1) {
                *attributev_out = NULL; /* no attributes so empty map */
                return 0;
        }

        attributev = calloc(attributec, sizeof(struct ir_attribute_entry));
        if (attributev == NULL) {
                return -1;
        };
        cure = attributev;

        /* iterate each list node within the interface */
        list_node = webidl_node_find_type(
                webidl_node_getnode(interface),
                NULL,
                WEBIDL_NODE_TYPE_LIST);

        while (list_node != NULL) {
                /* iterate through attributes on list */
                at_node = webidl_node_find_type(
                        webidl_node_getnode(list_node),
                        NULL,
                        WEBIDL_NODE_TYPE_ATTRIBUTE);

                while (at_node != NULL) {
                        /* process attribute node into an entry */

                        enum webidl_type_modifier *modifier;

                        cure->node = at_node;

                        cure->name = webidl_node_gettext(
                                webidl_node_find_type(
                                        webidl_node_getnode(at_node),
                                        NULL,
                                        WEBIDL_NODE_TYPE_IDENT));

                        cure->getter = genbind_node_find_method_ident(
                                               class,
                                               NULL,
                                               GENBIND_METHOD_TYPE_GETTER,
                                               cure->name);

                        /* create attribute type vector */
                        type_map_new(at_node, &cure->typec, &cure->typev);


                        /* get binding node for read/write attributes */
                        modifier = (enum webidl_type_modifier *)webidl_node_getint(
                                webidl_node_find_type(
                                        webidl_node_getnode(at_node),
                                        NULL,
                                        WEBIDL_NODE_TYPE_MODIFIER));
                        if ((modifier != NULL) &&
                            (*modifier == WEBIDL_TYPE_MODIFIER_READONLY)) {
                                cure->modifier = WEBIDL_TYPE_MODIFIER_READONLY;
                        } else {
                                cure->modifier = WEBIDL_TYPE_MODIFIER_NONE;
                                cure->setter = genbind_node_find_method_ident(
                                                     class,
                                                     NULL,
                                                     GENBIND_METHOD_TYPE_SETTER,
                                                     cure->name);
                        }

                        /* check for putforwards extended attribute */
                        cure->putforwards = get_extended_value(at_node,
                                                               "PutForwards");

                        if ((cure->putforwards != NULL) &&
                            (cure->modifier != WEBIDL_TYPE_MODIFIER_READONLY)) {
                                WARN(WARNING_WEBIDL,
                                     "putforwards on a writable attribute (%s) is prohibited",
                                     cure->name);
                        }

                        /* check for treatnullas extended attribute */
                        cure->treatnullas = get_extended_value(at_node,
                                                               "TreatNullAs");

                        /* move to next attribute */
                        cure++;

                        at_node = webidl_node_find_type(
                                webidl_node_getnode(list_node),
                                at_node,
                                WEBIDL_NODE_TYPE_ATTRIBUTE);
                }

                list_node = webidl_node_find_type(
                        webidl_node_getnode(interface),
                        list_node,
                        WEBIDL_NODE_TYPE_LIST);
        }

        *attributev_out = attributev; /* resulting attributes map */

        return 0;
}

static int
constant_map_new(struct webidl_node *interface,
                 int *constantc_out,
                 struct ir_constant_entry **constantv_out)
{
        struct webidl_node *list_node;
        struct webidl_node *constant_node; /* constant node */
        struct ir_constant_entry *cure; /* current entry */
        struct ir_constant_entry *constantv;
        int constantc;

        /* enumerate constants */
        constantc = enumerate_interface_type(interface,
                                              WEBIDL_NODE_TYPE_CONST);

        if (constantc < 1) {
                *constantc_out = 0;
                *constantv_out = NULL; /* no constants so empty map */
                return 0;
        }

        *constantc_out = constantc;

        constantv = calloc(constantc,
                           sizeof(struct ir_constant_entry));
        if (constantv == NULL) {
                return -1;
        };
        cure = constantv;

        /* iterate each list node within the interface */
        list_node = webidl_node_find_type(
                webidl_node_getnode(interface),
                NULL,
                WEBIDL_NODE_TYPE_LIST);

        while (list_node != NULL) {
                /* iterate through constants on list */
                constant_node = webidl_node_find_type(
                        webidl_node_getnode(list_node),
                        NULL,
                        WEBIDL_NODE_TYPE_CONST);

                while (constant_node != NULL) {
                        cure->node = constant_node;

                        cure->name = webidl_node_gettext(
                                webidl_node_find_type(
                                        webidl_node_getnode(constant_node),
                                        NULL,
                                        WEBIDL_NODE_TYPE_IDENT));

                        cure++;

                        /* move to next constant */
                        constant_node = webidl_node_find_type(
                                webidl_node_getnode(list_node),
                                constant_node,
                                WEBIDL_NODE_TYPE_CONST);
                }

                list_node = webidl_node_find_type(
                        webidl_node_getnode(interface),
                        list_node,
                        WEBIDL_NODE_TYPE_LIST);
        }

        *constantv_out = constantv; /* resulting constants map */

        return 0;
}


static int
member_map_new(struct webidl_node *dictionary,
               int *memberc_out,
               struct ir_operation_argument_entry **memberv_out)
{
        struct webidl_node *list_node;
        struct webidl_node *member_node; /* member node */
        struct ir_operation_argument_entry *cure; /* current entry */
        struct ir_operation_argument_entry *memberv;
        int memberc;

        /* enumerate members */
        memberc = enumerate_interface_type(dictionary,
                                           WEBIDL_NODE_TYPE_ARGUMENT);

        if (memberc < 1) {
                *memberc_out = 0;
                *memberv_out = NULL; /* no members so empty map */
                return 0;
        }

        memberv = calloc(memberc, sizeof(struct ir_operation_argument_entry));
        if (memberv == NULL) {
                return -1;
        };
        cure = memberv;

        /* iterate each list node within the dictionary */
        list_node = webidl_node_find_type(
                webidl_node_getnode(dictionary),
                NULL,
                WEBIDL_NODE_TYPE_LIST);

        while (list_node != NULL) {
                /* iterate through members on list */
                member_node = webidl_node_find_type(
                        webidl_node_getnode(list_node),
                        NULL,
                        WEBIDL_NODE_TYPE_ARGUMENT);

                while (member_node != NULL) {
                        cure->node = member_node;

                        cure->name = webidl_node_gettext(
                                webidl_node_find_type(
                                        webidl_node_getnode(member_node),
                                        NULL,
                                        WEBIDL_NODE_TYPE_IDENT));

                        cure->optionalc = webidl_node_enumerate_type(
                                webidl_node_getnode(member_node),
                                WEBIDL_NODE_TYPE_OPTIONAL);

                        cure->elipsisc = webidl_node_enumerate_type(
                                webidl_node_getnode(member_node),
                                WEBIDL_NODE_TYPE_ELLIPSIS);

                        cure++;

                        /* move to next member */
                        member_node = webidl_node_find_type(
                                webidl_node_getnode(list_node),
                                member_node,
                                WEBIDL_NODE_TYPE_ARGUMENT);
                }

                list_node = webidl_node_find_type(
                        webidl_node_getnode(dictionary),
                        list_node,
                        WEBIDL_NODE_TYPE_LIST);
        }

        *memberc_out = memberc;
        *memberv_out = memberv; /* resulting members map */

        return 0;

}

static int
entry_map_new(struct genbind_node *genbind,
                  struct webidl_node *interface,
                  int *interfacec_out,
                  struct ir_entry **interfacev_out)
{
        int interfacec;
        int dictionaryc;
        int entryc;
        struct ir_entry *entries;
        struct ir_entry *sorted_entries;
        struct ir_entry *cure;
        struct webidl_node *node;

        interfacec = webidl_node_enumerate_type(interface,
                                            WEBIDL_NODE_TYPE_INTERFACE);
        dictionaryc = webidl_node_enumerate_type(interface,
                                            WEBIDL_NODE_TYPE_DICTIONARY);
        if (options->verbose) {
                printf("Mapping %d interfaces\n", interfacec);
                printf("Mapping %d dictionaries\n", dictionaryc);
        }

        entryc = interfacec + dictionaryc;

        entries = calloc(entryc, sizeof(struct ir_entry));
        if (entries == NULL) {
                return -1;
        }

        /* for each interface populate an entry in the map */
        cure = entries;
        node = webidl_node_find_type(interface,
                                     NULL,
                                     WEBIDL_NODE_TYPE_INTERFACE);
        while (node != NULL) {

                /* fill map entry */
                cure->node = node;

                /* name of interface */
                cure->name = webidl_node_gettext(
                        webidl_node_find_type(
                                webidl_node_getnode(node),
                                NULL,
                                WEBIDL_NODE_TYPE_IDENT));

                /* name of the inherited interface (if any) */
                cure->inherit_name = webidl_node_gettext(
                        webidl_node_find_type(
                                webidl_node_getnode(node),
                                NULL,
                                WEBIDL_NODE_TYPE_INHERITANCE));

                /* matching class from binding  */
                cure->class = genbind_node_find_type_ident(
                        genbind,
                        NULL,
                        GENBIND_NODE_TYPE_CLASS,
                        cure->name);

                /* identify this is an interface entry */
                cure->type = IR_ENTRY_TYPE_INTERFACE;

                /* is the interface marked as not generating an object */
                if (webidl_node_find_type_ident(
                            webidl_node_getnode(node),
                            WEBIDL_NODE_TYPE_EXTENDED_ATTRIBUTE,
                            "NoInterfaceObject") != NULL) {
                        /** \todo we should ensure inherit is unset as this
                         * cannot form part of an inheritance chain if it is
                         * not generating an output class
                         */
                        cure->u.interface.noobject = true;
                }

                /* is the interface marked as the primary global */
                if (webidl_node_find_type_ident(
                            webidl_node_getnode(node),
                            WEBIDL_NODE_TYPE_EXTENDED_ATTRIBUTE,
                            "PrimaryGlobal") != NULL) {
                        /** \todo we should ensure nothing inherits *from* this
                         * class or all hell will break loose having two
                         * primary globals.
                         */
                        cure->u.interface.primary_global = true;
                }

                /* enumerate and map the interface operations */
                operation_map_new(node,
                                  cure->class,
                                  &cure->u.interface.operationc,
                                  &cure->u.interface.operationv);

                /* enumerate and map the interface attributes */
                attribute_map_new(node,
                                  cure->class,
                                  &cure->u.interface.attributec,
                                  &cure->u.interface.attributev);

                /* enumerate and map the interface constants */
                constant_map_new(node,
                                 &cure->u.interface.constantc,
                                 &cure->u.interface.constantv);

                /* move to next interface */
                node = webidl_node_find_type(interface,
                                             node,
                                             WEBIDL_NODE_TYPE_INTERFACE);
                cure++;
        }

        /* for each dictionary populate an entry in the map */
        node = webidl_node_find_type(interface,
                                     NULL,
                                     WEBIDL_NODE_TYPE_DICTIONARY);
        while (node != NULL) {

                /* fill map entry */
                cure->node = node;

                /* name of interface */
                cure->name = webidl_node_gettext(
                        webidl_node_find_type(
                                webidl_node_getnode(node),
                                NULL,
                                WEBIDL_NODE_TYPE_IDENT));

                /* name of the inherited interface (if any) */
                cure->inherit_name = webidl_node_gettext(
                        webidl_node_find_type(
                                webidl_node_getnode(node),
                                NULL,
                                WEBIDL_NODE_TYPE_INHERITANCE));

                /* matching class from binding  */
                cure->class = genbind_node_find_type_ident(
                        genbind,
                        NULL,
                        GENBIND_NODE_TYPE_CLASS,
                        cure->name);

                /* identify this is an interface entry */
                cure->type = IR_ENTRY_TYPE_DICTIONARY;

                /* enumerate and map the dictionary members */
                member_map_new(node,
                               &cure->u.dictionary.memberc,
                               &cure->u.dictionary.memberv);

                /* move to next interface */
                node = webidl_node_find_type(interface,
                                             node,
                                             WEBIDL_NODE_TYPE_DICTIONARY);
                cure++;
        }

        /* compute inheritance and refcounts on map */
        compute_inherit_refcount(entries, entryc);

        /* sort entries to ensure correct ordering */
        sorted_entries = entry_topoligical_sort(entries, entryc);
        free(entries);
        if (sorted_entries == NULL) {
                return -1;
        }

        /* compute inheritance and refcounts on sorted map */
        compute_inherit_refcount(sorted_entries, entryc);

        *interfacec_out = entryc;
        *interfacev_out = sorted_entries;

        return 0;
}


int ir_new(struct genbind_node *genbind,
           struct webidl_node *webidl,
           struct ir **map_out)
{
        struct ir *map;
        int ret;

        map = malloc(sizeof(struct ir));
        if (map == NULL) {
            return -1;
        }

        map->webidl = webidl;
        map->binding_node = genbind_node_find_type(genbind, NULL,
                                                   GENBIND_NODE_TYPE_BINDING);

        /* interfaces */
        ret = entry_map_new(genbind,
                            webidl,
                            &map->entryc,
                            &map->entries);
        if (ret != 0) {
                free(map);
                return ret;
        }

        *map_out = map;

        return 0;
}

static int ir_dump_dictionary(FILE *dumpf, struct ir_entry *ecur)
{
        if (ecur->u.dictionary.memberc > 0) {
                int argc;

                fprintf(dumpf, "\t%d members\n", ecur->u.dictionary.memberc);

                for (argc = 0; argc < ecur->u.dictionary.memberc; argc++) {
                        struct ir_operation_argument_entry *arge;

                        arge = ecur->u.dictionary.memberv + argc;

                        fprintf(dumpf, "\t\t%s\n", arge->name);

                        if (arge->optionalc != 0) {
                                fprintf(dumpf,
                                        "\t\t\toptional:%d\n",
                                        arge->optionalc);
                        }

                        if (arge->elipsisc != 0) {
                                fprintf(dumpf,
                                        "\t\t\telipsis:%d\n",
                                        arge->elipsisc);
                        }
                }
        }
        return 0;
}

static int ir_dump_interface(FILE *dumpf, struct ir_entry *ecur)
{
        if (ecur->u.interface.operationc > 0) {
                int opc;

                fprintf(dumpf, "\t%d operations\n",
                        ecur->u.interface.operationc);

                for (opc = 0; opc < ecur->u.interface.operationc; opc++) {
                        int ovlc;
                        struct ir_operation_entry *ope;

                        ope = ecur->u.interface.operationv + opc;

                        fprintf(dumpf,
                                "\t\t%s\n",
                                ope->name);
                        fprintf(dumpf,
                                "\t\t\tmethod:%p\n",
                                ope->method);
                        for(ovlc = 0; ovlc < ope->overloadc;ovlc++) {
                                int argc;
                                struct ir_operation_overload_entry *ovle;
                                ovle = ope->overloadv + ovlc;

                                fprintf(dumpf,
                                        "\t\t\toverload:%d\n", ovlc);

                                fprintf(dumpf,
                                        "\t\t\t\treturn type:%p\n",
                                        ovle->type);

                                fprintf(dumpf,
                                        "\t\t\t\targuments:%d\n",
                                        ovle->argumentc);

                                fprintf(dumpf,
                                        "\t\t\t\toptionals:%d\n",
                                        ovle->optionalc);

                                fprintf(dumpf,
                                        "\t\t\t\telipsis:%d\n",
                                        ovle->elipsisc);

                                for (argc = 0; argc < ovle->argumentc; argc++) {
                                        struct ir_operation_argument_entry *arge;
                                        arge = ovle->argumentv + argc;

                                        fprintf(dumpf,
                                                "\t\t\t\t\t%s\n",
                                                arge->name);

                                        if (arge->optionalc != 0) {
                                                fprintf(dumpf,
                                                        "\t\t\t\t\t\toptional:%d\n",
                                                        arge->optionalc);
                                        }

                                        if (arge->elipsisc != 0) {
                                                fprintf(dumpf,
                                                        "\t\t\t\t\t\telipsis:%d\n",
                                                        arge->elipsisc);
                                        }

                                }
                        }
                }
        }

        if (ecur->u.interface.attributec > 0) {
                int attrc = ecur->u.interface.attributec;
                struct ir_attribute_entry *attre;

                fprintf(dumpf, "\t%d attributes\n", attrc);

                attre = ecur->u.interface.attributev;
                while (attre != NULL) {
                        fprintf(dumpf, "\t\t%s %p",
                                attre->name,
                                attre->getter);
                        if (attre->modifier == WEBIDL_TYPE_MODIFIER_NONE) {
                                fprintf(dumpf, " %p\n", attre->setter);
                        } else {
                                fprintf(dumpf, "\n");
                        }
                        attre++;
                        attrc--;
                        if (attrc == 0) {
                                break;
                        }
                }
        }
        if (ecur->u.interface.constantc > 0) {
                int idx;

                fprintf(dumpf, "\t%d constants\n",
                        ecur->u.interface.constantc);

                for (idx = 0; idx < ecur->u.interface.constantc; idx++) {
                        struct ir_constant_entry *cone;
                        cone = ecur->u.interface.constantv + idx;
                        fprintf(dumpf, "\t\t%s\n", cone->name);
                }
        }
        return 0;
}

int ir_dump(struct ir *ir)
{
        FILE *dumpf;
        int eidx;
        struct ir_entry *ecur;

        /* only dump AST to file if required */
        if (!options->debug) {
                return 0;
        }

        dumpf = genb_fopen("ir-map", "w");
        if (dumpf == NULL) {
                return 2;
        }

        ecur = ir->entries;
        for (eidx = 0; eidx < ir->entryc; eidx++) {
                fprintf(dumpf, "%d %s\n", eidx, ecur->name);
                if (ecur->inherit_name != NULL) {
                        fprintf(dumpf, "\tinherit:%s\n", ecur->inherit_name);
                }
                if (ecur->class != NULL) {
                        fprintf(dumpf, "\tclass:%p\n", ecur->class);
                }

                switch (ecur->type) {
                case IR_ENTRY_TYPE_INTERFACE:
                        ir_dump_interface(dumpf, ecur);
                        break;

                case IR_ENTRY_TYPE_DICTIONARY:
                        ir_dump_dictionary(dumpf, ecur);
                        break;

                }
                ecur++;
        }

        fclose(dumpf);

        return 0;
}

int ir_dumpdot(struct ir *index)
{
        FILE *dumpf;
        int eidx;
        struct ir_entry *ecur;

        /* only dump AST to file if required */
        if (!options->debug) {
                return 0;
        }

        dumpf = genb_fopen("ir.dot", "w");
        if (dumpf == NULL) {
                return 2;
        }

        fprintf(dumpf, "digraph interfaces {\n");

        fprintf(dumpf, "node [shape=box]\n");

        ecur = index->entries;
        for (eidx = 0; eidx < index->entryc; eidx++) {
                fprintf(dumpf, "%04d [label=\"%s\"", eidx, ecur->name);
                if ((ecur == IR_ENTRY_TYPE_INTERFACE) &&
                    (ecur->u.interface.noobject == true)) {
                        /* noobject interfaces in red */
                        fprintf(dumpf, "fontcolor=\"red\"");
                } else if (ecur->class != NULL) {
                        /* interfaces bound to a class are shown in blue */
                        fprintf(dumpf, "fontcolor=\"blue\"");
                }

                fprintf(dumpf, "];\n");
                ecur++;
        }

        ecur = index->entries;
        for (eidx = 0; eidx < index->entryc; eidx++) {
            if (index->entries[eidx].inherit_idx != -1) {
                fprintf(dumpf, "%04d -> %04d;\n",
                        eidx, index->entries[eidx].inherit_idx);
            }
        }

        fprintf(dumpf, "}\n");

        fclose(dumpf);

        return 0;
}

struct ir_entry *
ir_inherit_entry(struct ir *map,
                 struct ir_entry *entry)
{
        struct ir_entry *res = NULL;

        if ((entry != NULL) &&
            (entry->inherit_idx != -1)) {
                res = &map->entries[entry->inherit_idx];
        }
        return res;
}
