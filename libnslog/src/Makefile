DIR_SOURCES := core.c filter.c

CFLAGS := $(CFLAGS) -I$(BUILDDIR) -Isrc/

SOURCES := $(SOURCES) $(BUILDDIR)/filter-parser.c $(BUILDDIR)/filter-lexer.c

$(BUILDDIR)/%-lexer.inc $(BUILDDIR)/%-lexer.h: src/%-lexer.l
	$(VQ)$(ECHO) "	  FLEX: $<"
	$(Q)$(FLEX) --outfile=$(BUILDDIR)/$(*F)-lexer.inc --header-file=$(BUILDDIR)/$(*F)-lexer.h $<
.PRECIOUS: $(BUILDDIR)/%-lexer.inc

$(BUILDDIR)/%-lexer.inc: $(BUILDDIR)/%-parser.h

$(BUILDDIR)/%-lexer.c: $(BUILDDIR)/%-lexer.inc
	$(Q)echo "#ifndef __clang_analyzer__" > $@
	$(Q)echo "#include \"$(notdir $<)\"" >> $@
	$(Q)echo "#endif" >> $@

# Bison 3.0 and later require api.prefix in curly braces
# Bison 2.6 and later require api.prefix
# Bison 2.5 and earlier require name-prefix switch
bisonvsn := $(word 4,$(shell $(BISON) --version))
bisonmaj := $(word 1,$(subst ., ,$(bisonvsn)))
bisonmin := $(word 2,$(subst ., ,$(bisonvsn)))
ifeq ($(bisonmaj),1)
  BISON_DEFINES = --name-prefix=$(*F)_
else
  ifeq ($(bisonmaj),2)
    ifneq ($(findstring $(bisonmin),"0 1 2 3 4 5"),)
      BISON_DEFINES = --name-prefix=$(*F)_
    else
      BISON_DEFINES = --define=api.prefix=$(*F)_
    endif
  else
    BISON_DEFINES = --define=api.prefix={$(*F)_}
  endif
endif

$(BUILDDIR)/%-parser.c $(BUILDDIR)/%-parser.h: src/%-parser.y
	$(VQ)$(ECHO) "	 BISON: $<"
	$(Q)$(BISON) -d -t $(BISON_DEFINES) --report=all --output=$(BUILDDIR)/$(*F)-parser.c --defines=$(BUILDDIR)/$(*F)-parser.h $<

include $(NSBUILD)/Makefile.subdir
