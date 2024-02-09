#
# Makefile for NetSurf RISC OS target
#
# This file is part of NetSurf 

# ----------------------------------------------------------------------------
# RISC OS target setup
# ----------------------------------------------------------------------------

$(eval $(call pkg_config_find_and_add,libcares,Cares))

$(eval $(call feature_enabled,DRAW,-DWITH_DRAW,,Drawfile rendering))
$(eval $(call feature_enabled,SPRITE,-DWITH_SPRITE,,Sprite rendering))
$(eval $(call feature_enabled,ARTWORKS,-DWITH_ARTWORKS,,ArtWorks rendering))
$(eval $(call feature_enabled,DRAW_EXPORT,-DWITH_DRAW_EXPORT,-lpencil,Drawfile export))


TPD_RISCOS = $(foreach TPL,$(notdir $(TPL_RISCOS)), \
		$(FRONTEND_SOURCE_DIR)/appdir/Resources/$(TPL)/Templates$(TPLEXT))

RESOURCES = $(TPD_RISCOS)

CFLAGS += -Driscos -std=c99 -D_BSD_SOURCE -D_POSIX_C_SOURCE \
	    -mpoke-function-name -fno-strict-aliasing
ifeq ($(findstring -elfeabi,$(SUBTARGET)),-elfeabi)
  CFLAGS += -funwind-tables
endif

CFLAGS += -I$(GCCSDK_INSTALL_ENV)/include
ifeq ($(HOST),riscos)
  CFLAGS += -I<OSLib$$Dir> -mthrowback
endif
ASFLAGS += -xassembler-with-cpp -I. -I$(GCCSDK_INSTALL_ENV)/include
LDFLAGS += -L$(GCCSDK_INSTALL_ENV)/lib -lrufl
ifeq ($(HOST),riscos)
  LDFLAGS += -LOSLib: -lOSLib32
else
  LDFLAGS += -lOSLib32
  ifeq ($(findstring -elf,$(SUBTARGET)),-elf)
    # Go for static builds & AIF binary at the moment:
    CFLAGS += -static
    LDFLAGS += -static
    EXEEXT := ,ff8
  endif
endif

# ----------------------------------------------------------------------------
# Source file setup
# ----------------------------------------------------------------------------

# S_RISCOS are sources purely for the RISC OS build
S_FRONTEND :=			\
	assert.c		\
	bitmap.c 		\
	buffer.c		\
	configure.c		\
	cookies.c		\
	corewindow.c		\
	dialog.c		\
	download.c		\
	filetype.c		\
	font.c			\
	global_history.c	\
	gui.c			\
	help.c			\
	hotlist.c		\
	image.c			\
	iconbar.c		\
	local_history.c		\
	menus.c			\
	message.c		\
	mouse.c			\
	pageinfo.c		\
	palettes.c		\
	plotters.c		\
	print.c			\
	query.c			\
	save.c			\
	save_draw.c		\
	save_pdf.c		\
	schedule.c		\
	search.c		\
	searchweb.c		\
	textarea.c		\
	textselection.c		\
	theme.c			\
	theme_install.c		\
	toolbar.c		\
	url_suggest.c		\
	ucstables.c		\
	uri.c			\
	url_complete.c		\
	url_protocol.c		\
	window.c		\
	wimp.c			\
	wimp_event.c		\
	$(addprefix content-handlers/,artworks.c awrender.s draw.c sprite.c) \
	$(addprefix gui/,button_bar.c progress_bar.c status_bar.c	\
	throbber.c url_bar.c)						\
	$(addprefix configure/,con_cache.c con_connect.c con_content.c	\
	con_fonts.c con_home.c con_image.c con_inter.c con_language.c	\
	con_secure.c con_theme.c)

# This is the final source build list
# Note this is deliberately *not* expanded here as common and image
#   are not yet available
SOURCES = $(S_COMMON) $(S_IMAGE) $(S_BROWSER) $(S_FRONTEND)

EXETARGET := $(FRONTEND_SOURCE_DIR)/appdir/!RunImage$(EXEEXT)

# The filter and target for split messages
MESSAGES_FILTER=ro

$(FRONTEND_SOURCE_DIR)/appdir/!Run$(RUNEXT): $(FRONTEND_SOURCE_DIR)/scripts/Run $(EXETARGET)
	$(VQ)echo " MAKERUN: $@"
	$(Q)$(MAKERUN) $(EXETARGET) $< $@
ifeq ($(findstring -elfeabi,$(SUBTARGET)),-elfeabi)
	$(Q)$(SED) -i -e 's/^|\(RMEnsure ARMEABISupport\)/\1/' -e 's/SharedUnixLibrary 1\.07/SharedUnixLibrary 1.16/g' $@
endif

$(FRONTEND_SOURCE_DIR)/appdir/!Help$(RUNEXT): $(FRONTEND_SOURCE_DIR)/scripts/Help
	$(VQ)echo "      CP: $@"
	$(Q)cp $< $@

$(DEPROOT)/squeeze.d: $(EXETARGET)
	$(VQ)echo " SQUEEZE: $<"
	$(Q)$(SQUEEZE) -f -v $(EXETARGET)
	$(Q)$(TOUCH) $@

POSTEXES += $(FRONTEND_SOURCE_DIR)/appdir/!Run$(RUNEXT) $(FRONTEND_SOURCE_DIR)/appdir/!Help$(RUNEXT) $(DEPROOT)/squeeze.d


clean-run:
	$(VQ)echo "   CLEAN: !Run$(RUNEXT)"
	$(Q) $(RM) $(FRONTEND_SOURCE_DIR)/appdir/!Run$(RUNEXT)

clean-help:
	$(VQ)echo "   CLEAN: !Help$(RUNEXT)"
	$(Q) $(RM) $(FRONTEND_SOURCE_DIR)/appdir/!Help$(RUNEXT)

CLEANS += clean-run clean-help

# ----------------------------------------------------------------------------
# Template targets
# ----------------------------------------------------------------------------

# Template objects
TPL_RISCOS := de en fr nl # TODO: It'd be nice to auto-detect these
TPL_RISCOS := $(addprefix $(FRONTEND_SOURCE_DIR)/templates/,$(TPL_RISCOS))

# Template target creation macro
define compile_template
$(FRONTEND_SOURCE_DIR)/appdir/Resources/$(1)/Templates$$(TPLEXT): $(2)
	$$(VQ)echo "TEMPLATE: $(2)"
	$$(Q)$$(MKDIR) -p $(FRONTEND_SOURCE_DIR)/appdir/Resources/$(1)
	$$(Q)$$(CC) -x c -E -P $$(CFLAGS) $(2) | $$(CCRES) - $$@

CLEAN_TEMPLATES += $(FRONTEND_SOURCE_DIR)/appdir/Resources/$(1)/Templates$$(TPLEXT)

endef

$(eval $(foreach TPL,$(TPL_RISCOS), \
	$(call compile_template,$(notdir $(TPL)),$(TPL))))

clean-templates:
	$(VQ)echo "   CLEAN: $(CLEAN_TEMPLATES)"
	$(Q)$(RM) $(CLEAN_TEMPLATES)
CLEANS += clean-templates


# ----------------------------------------------------------------------------
# Install target
# ----------------------------------------------------------------------------

install-riscos:

# ----------------------------------------------------------------------------
# Package target
# ----------------------------------------------------------------------------

package-riscos: netsurf.zip

netsurf.zip: $(EXETARGET) $(POSTEXES)
	$(eval $@_TMPDIR := $(shell mktemp -d))
	$(Q) $(RM) $@
	$(Q) cp -rLvp $(FRONTEND_SOURCE_DIR)/appdir $($@_TMPDIR)/!NetSurf
	$(Q) $(CURDIR)/tools/git-date.sh $(FRONTEND_SOURCE_DIR)/distribution
	$(Q) rsync --archive --verbose $(FRONTEND_SOURCE_DIR)/distribution/!Boot $($@_TMPDIR)
	$(Q) rsync --archive --verbose $(FRONTEND_SOURCE_DIR)/distribution/!System $($@_TMPDIR)
	$(Q) rsync --archive --verbose $(FRONTEND_SOURCE_DIR)/distribution/3rdParty $($@_TMPDIR)
	$(Q) cp $(FRONTEND_SOURCE_DIR)/distribution/ReadMe $($@_TMPDIR)
	$(Q) cp $(FRONTEND_SOURCE_DIR)/distribution/LeesMij $($@_TMPDIR)
	$(Q) cd $($@_TMPDIR) && $(ZIP) -9vr\, $(CURDIR)/$@ *
	$(Q) $(RM) -rf $($@_TMPDIR)
