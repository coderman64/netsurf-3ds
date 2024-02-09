#
# NetSurf javascript source file inclusion
#
# Included by javascript/Makefile
#

content/handlers/javascript/duktape/dukky.c: \
	$(OBJROOT)/duktape/binding.h \
	$(OBJROOT)/duktape/generics.js.inc \
	$(OBJROOT)/duktape/polyfill.js.inc

BINDINGS := $(wildcard content/handlers/javascript/duktape/*.bnd)

# Generator for the C include representing the generics.js
$(OBJROOT)/duktape/generics.js.inc: content/handlers/javascript/duktape/generics.js $(TOOLROOT)/xxd
	$(Q)$(MKDIR) -p $(OBJROOT)/duktape
	$(VQ)echo "     XXD: $<"
	$(Q)$(TOOLROOT)/xxd -i $< $@.tmp
	$(Q)sed -e 's/content_handlers_javascript_duktape_generics_js/generics_js/' $@.tmp > $@

# Generator for the C include representing the polyfill.js
$(OBJROOT)/duktape/polyfill.js.inc: content/handlers/javascript/duktape/polyfill.js $(TOOLROOT)/xxd
	$(Q)$(MKDIR) -p $(OBJROOT)/duktape
	$(VQ)echo "     XXD: $<"
	$(Q)$(TOOLROOT)/xxd -i $< $@.tmp
	$(Q)sed -e 's/content_handlers_javascript_duktape_polyfill_js/polyfill_js/' $@.tmp > $@

# ensure genbind generates debugging files
GBFLAGS+=-D

$(OBJROOT)/duktape/binding.h $(OBJROOT)/duktape/Makefile: content/handlers/javascript/duktape/netsurf.bnd $(BINDINGS)
	$(Q)$(MKDIR) -p $(OBJROOT)/duktape
	$(VQ)echo " GENBIND: $<"
	$(Q)nsgenbind $(GBFLAGS) -I content/handlers/javascript/WebIDL $< $(OBJROOT)/duktape

# create unimplemented report for doxygen
docs/UnimplementedJavascript.md: content/handlers/javascript/duktape/netsurf.bnd $(BINDINGS)
	$(Q)$(MKDIR) -p $(OBJROOT)/duktape
	$(VQ)echo "Unimplemented javascript bindings {#unimplemented}" > $@
	$(VQ)echo "=================================" >> $@
	$(VQ)echo "" >> $@
	$(VQ)echo "This is a list of all the binding methods, getters and setters without an implementation in a binding." >> $@
	$(VQ)echo "" >> $@
	$(VQ)echo "Type | Unimplemented binding" >> $@
	$(VQ)echo "---- | ---------------------" >> $@
	$(VQ)echo " GENBIND: $<"
	$(Q)nsgenbind $(GBFLAGS) -Wunimplemented -I content/handlers/javascript/WebIDL $< $(OBJROOT)/duktape 2>&1 >/dev/null | awk 'BEGIN{count=0} /.*Unimplemented.*/{count++; out=$$4" |"; for(i=5;i<=NF;i++){out=out" "$$i}; print out} END{print "\n",count,"unimplemented bindings"}' >> $@
	$(VQ)echo "" >> $@

ifeq ($(filter $(MAKECMDGOALS),clean test coverage),)
-include $(OBJROOT)/duktape/Makefile
endif

S_JAVASCRIPT_BINDING:=$(addprefix $(OBJROOT)/duktape/,$(NSGENBIND_SOURCES))

$(S_JAVASCRIPT_BINDING): $(BINDINGS)

S_JAVASCRIPT += content.c duktape/dukky.c duktape/duktape.c

CFLAGS += -DDUK_OPT_HAVE_CUSTOM_H
