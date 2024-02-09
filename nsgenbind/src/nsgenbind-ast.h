/* binding file AST interface
 *
 * This file is part of nsnsgenbind.
 * Licensed under the MIT License,
 *                http://www.opensource.org/licenses/mit-license.php
 * Copyright 2012 Vincent Sanders <vince@netsurf-browser.org>
 */

#ifndef nsgenbind_nsgenbind_ast_h
#define nsgenbind_nsgenbind_ast_h

enum genbind_node_type {
        GENBIND_NODE_TYPE_ROOT = 0,
        GENBIND_NODE_TYPE_IDENT, /**< generic identifier string */
        GENBIND_NODE_TYPE_NAME, /**< generic type string */
        GENBIND_NODE_TYPE_MODIFIER, /**< node modifier */
        GENBIND_NODE_TYPE_CDATA, /**< verbatim block of character data */
        GENBIND_NODE_TYPE_STRING, /**< text string */
        GENBIND_NODE_TYPE_LINE, /**< linenumber */
        GENBIND_NODE_TYPE_FILE, /**< file name */

        GENBIND_NODE_TYPE_BINDING, /**< Binding */
        GENBIND_NODE_TYPE_WEBIDL,

        GENBIND_NODE_TYPE_CLASS, /**< class definition */
        GENBIND_NODE_TYPE_PRIVATE,
        GENBIND_NODE_TYPE_INTERNAL,
        GENBIND_NODE_TYPE_PROPERTY,
        GENBIND_NODE_TYPE_FLAGS,

        GENBIND_NODE_TYPE_METHOD, /**< binding method */
        GENBIND_NODE_TYPE_METHOD_TYPE, /**< binding method type */

        GENBIND_NODE_TYPE_PARAMETER, /**< method parameter */
};

/* modifier flags */
enum genbind_type_modifier {
        GENBIND_TYPE_NONE = 0,
        GENBIND_TYPE_TYPE = 1, /**< identifies a type handler */
        GENBIND_TYPE_UNSHARED = 2, /**< unshared item */
        GENBIND_TYPE_TYPE_UNSHARED = 3, /**< identifies a unshared type handler */
};

/* binding method types */
enum genbind_method_type {
        GENBIND_METHOD_TYPE_INIT = 0, /**< method is initialiser */
        GENBIND_METHOD_TYPE_FINI, /**< method is finalizer */
        GENBIND_METHOD_TYPE_METHOD, /**< method is a method */
        GENBIND_METHOD_TYPE_GETTER, /**< method is a getter */
        GENBIND_METHOD_TYPE_SETTER, /**< method is a setter */
        GENBIND_METHOD_TYPE_PROTOTYPE, /**< method is a prototype */
        GENBIND_METHOD_TYPE_PREFACE, /**< method is a preface */
        GENBIND_METHOD_TYPE_PROLOGUE, /**< method is a prologue */
        GENBIND_METHOD_TYPE_EPILOGUE, /**< method is a epilogue */
        GENBIND_METHOD_TYPE_POSTFACE, /**< method is a postface */
};

struct genbind_node;

/** callback for search and iteration routines */
typedef int (genbind_callback_t)(struct genbind_node *node, void *ctx);

int genbind_fprintf(FILE *stream, const char *format, ...);

int genbind_cmp_node_type(struct genbind_node *node, void *ctx);

FILE *genbindopen(const char *filename);

int genbind_parsefile(char *infilename, struct genbind_node **ast);

char *genbind_strapp(char *a, char *b);

/**
 * create a new node with value from pointer
 */
struct genbind_node *genbind_new_node(enum genbind_node_type type, struct genbind_node *l, void *r);

/**
 * create a new number node
 *
 * Create a node with of number type
 */
struct genbind_node *genbind_new_number_node(enum genbind_node_type type, struct genbind_node *l, int number);

struct genbind_node *genbind_node_link(struct genbind_node *tgt, struct genbind_node *src);

struct genbind_node *genbind_node_prepend(struct genbind_node *list, struct genbind_node *inst);

struct genbind_node *genbind_node_add(struct genbind_node *node, struct genbind_node *list);

/**
 * Dump the binding AST to file
 *
 * If the debug flag has been set this causes the binding AST to be written to
 * a binding-ast output file
 *
 * \param node Node of the tree to start dumping from (usually tree root)
 * \return 0 on sucess or non zero on faliure and error message printed.
 */
int genbind_dump_ast(struct genbind_node *node);

/**
 *Depth first left hand search using user provided comparison
 *
 * @param node The node to start the search from
 * @param prev The node at which to stop the search, either NULL to
 *             search the full tree depth (initial search) or the result
 *             of a previous search to continue.
 * @param cb Comparison callback
 * @param ctx Context for callback
 */
struct genbind_node *
genbind_node_find(struct genbind_node *node,
                  struct genbind_node *prev,
                  genbind_callback_t *cb,
                  void *ctx);

/**
 * Depth first left hand search returning nodes of the specified type
 *
 * @param node The node to start the search from
 * @param prev The node at which to stop the search, either NULL to
 *             search the full tree depth (initial search) or the result
 *             of a previous search to continue.
 * @param nodetype The type of node to seach for
 * @return The found node or NULL for no nodes.
 */
struct genbind_node *
genbind_node_find_type(struct genbind_node *node,
                       struct genbind_node *prev,
                       enum genbind_node_type nodetype);

/**
 * count how many nodes of a specified type.
 *
 * Enumerate how many nodes of the specified type there are by
 * performing a depth first search for nodes of the given type and
 * counting the number of results.
 *
 * @param node The node to start the search from
 * @param nodetype The type of node to count
 * @return The number of nodes found.
 */
int
genbind_node_enumerate_type(struct genbind_node *node,
                            enum genbind_node_type type);


/**
 * Depth first left hand search returning nodes of the specified type
 *    with an ident child node with matching text
 *
 * @param node The node to start the search from
 * @param prev The node at which to stop the search, either NULL to
 *             search the full tree depth (initial search) or the result
 *             of a previous search to continue.
 * @param nodetype The type of node to seach for
 * @param ident The text to match the ident child node to
 */
struct genbind_node *
genbind_node_find_type_ident(struct genbind_node *node,
                             struct genbind_node *prev,
                             enum genbind_node_type nodetype,
                             const char *ident);


/**
 * Find a method node of a given method type
 *
 * \param node A node of type GENBIND_NODE_TYPE_CLASS to search for methods.
 * \param prev The node at which to stop the search, either NULL to
 *             search the full tree depth (initial search) or the result
 *             of a previous search to continue.
 * \param methodtype The type of method to find.
 * \return A node of type GENBIND_NODE_TYPE_METHOD on success or NULL on faliure
 */
struct genbind_node *
genbind_node_find_method(struct genbind_node *node,
                         struct genbind_node *prev,
                         enum genbind_method_type methodtype);


/**
 * Find a method node of a given method type and identifier
 *
 * \param node A node of type GENBIND_NODE_TYPE_CLASS to search for methods.
 * \param prev The node at which to stop the search, either NULL to
 *             search the full tree depth (initial search) or the result
 *             of a previous search to continue.
 * \param methodtype The type of method to find.
 * \param ident The identifier to search for
 * \return A node of type GENBIND_NODE_TYPE_METHOD on success or NULL on faliure
 */
struct genbind_node *
genbind_node_find_method_ident(struct genbind_node *node,
                               struct genbind_node *prev,
                               enum genbind_method_type methodtype,
                               const char *ident);


/**
 * Iterate all nodes of a certian type from a node with a callback.
 *
 * Depth first search for nodes of the given type calling the callback
 * with context.
 *
 * @param node The node to start the search from.
 */
int genbind_node_foreach_type(struct genbind_node *node,
                              enum genbind_node_type type,
                              genbind_callback_t *cb,
                              void *ctx);

/** get a nodes node list content
 *
 * @param node The nodes to get node list from
 * @return pointer to the node list or NULL if the node does not contain a list
 */
struct genbind_node *genbind_node_getnode(struct genbind_node *node);

/** get a nodes text content
 *
 * @param node The nodes to get text from
 * @return pointer to the node text or NULL if the node is not of type
 *         text or is empty.
 */
char *genbind_node_gettext(struct genbind_node *node);

/** get a nodes integer value
 *
 * @param node The nodes to get integer from
 * @return pointer to the node value or NULL if the node is not of type
 *         int or is empty.
 */
int *genbind_node_getint(struct genbind_node *node);

#endif
