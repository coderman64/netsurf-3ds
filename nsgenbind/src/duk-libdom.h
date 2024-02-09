/* DukTape binding generation
 *
 * This file is part of nsgenbind.
 * Licensed under the MIT License,
 *                http://www.opensource.org/licenses/mit-license.php
 * Copyright 2015 Vincent Sanders <vince@netsurf-browser.org>
 */

#ifndef nsgenbind_duk_libdom_h
#define nsgenbind_duk_libdom_h

/**
 * Generate output for duktape and libdom bindings.
 */
int duk_libdom_output(struct ir *ir);

/**
 * generate a source file to implement an interface using duk and libdom.
 *
 * \param ir The intermediate representation of the binding tree.
 * \param interfacee The interface entry to output within the ir.
 */
int output_interface(struct ir *ir, struct ir_entry *interfacee);

/**
 * generate a source file to implement a dictionary using duk and libdom.
 */
int output_dictionary(struct ir *ir, struct ir_entry *dictionarye);

/**
 * generate a declaration to implement a dictionary using duk and libdom.
 */
int output_interface_declaration(struct opctx *outc, struct ir_entry *interfacee);

/**
 * generate a declaration to implement a dictionary using duk and libdom.
 */
int output_dictionary_declaration(struct opctx *outc, struct ir_entry *dictionarye);

/**
 * generate preface block for nsgenbind
 */
int output_tool_preface(struct opctx *outc);

/**
 * generate preface block for nsgenbind
 */
int output_tool_prologue(struct opctx *outc);

/**
 * output character data of node of given type.
 *
 * used for any cdata sections
 *
 * \param outf The file handle to write output.
 * \param node The node to search.
 * \param nodetype the type of child node to search for.
 * \return The number of nodes written or 0 for none.
 */
int output_cdata(struct opctx *outc, struct genbind_node *node, enum genbind_node_type nodetype);


/**
 * output c code with line directives if possible.
 *
 * used for any cdata sections
 *
 * \param outf The file handle to write output.
 * \param node The node to search.
 * \param nodetype the type of child node to search for.
 * \return The number of nodes written or 0 for none.
 */
int output_ccode(struct opctx *outc, struct genbind_node *node);

/**
 * output character data of method node of given type.
 *
 * used for any cdata including pre/pro/epi/post sections
 *
 * \param outf The file handle to write output.
 * \param node The node to search.
 * \param nodetype the type of child node to search for.
 * \return The number of nodes written or 0 for none.
 */
int output_method_cdata(struct opctx *outc, struct genbind_node *node, enum genbind_method_type sel_method_type);

/**
 * output a C variable type
 *
 * Used to output c type and optionlly identifier declarations for parameters
 * and structure entries.
 * If the optional identifier is output it is ensured the type is separated
 * from the identifier with either a * or space.
 *
 * \param outf The file handle to write output.
 * \param node The node to generate content for.
 * \param identifier If the indentifier should be output.
 * \return 0 on success.
 */
int output_ctype(struct opctx *outc, struct genbind_node *node, bool identifier);

/**
 * Generate a C name from an IDL name.
 *
 * The IDL interface names are camelcase and not similar to libdom naming so it
 *  is necessary to convert them to a libdom compatible class name. This
 *  implementation is simple ASCII capable only and cannot cope with multibyte
 *  codepoints.
 *
 * The algorithm is:
 *  - copy characters to output lowering their case
 *  - if the previous character in the input name was uppercase and the current
 *    one is lowercase insert an underscore before the *previous* character.
 */
char *gen_idl2c_name(const char *idlname);

/**
 * Generate class property setter for a single attribute.
 */
int output_generated_attribute_setter(struct opctx *outc, struct ir_entry *interfacee, struct ir_attribute_entry *atributee);

/**
 * Generate class property getter for a single attribute.
 */
int output_generated_attribute_getter(struct opctx *outc, struct ir_entry *interfacee, struct ir_attribute_entry *atributee);


#endif
