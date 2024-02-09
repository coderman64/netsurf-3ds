# ----------------------------------------------------------------------------
# Atari target setup
# ----------------------------------------------------------------------------

# !!!IMPORTANT!!!
# On regular m68k-atari-mint installation, these must be set
# manually via env. variables, because the m68k-atari-mint
# toolchain is located in /usr/bin and the wildcard matching
# fails then. So this wildcard matches only works for
# the netsurf environment!
STRIP := $(wildcard $(GCCSDK_INSTALL_CROSSBIN)/*-strip)
STACK := $(wildcard $(GCCSDK_INSTALL_CROSSBIN)/*stack)
FT2CF := $(GCCSDK_INSTALL_ENV)/bin/freetype-config

ifeq ($(ATARIARCH),68000)
PRGSUFFIX := 000.app
PKGNAME := ns000.zip
endif

ifeq ($(ATARIARCH),68020-60)
CFLAGS += -m68020-60
LDFLAGS += -m68020-60
PRGSUFFIX := 020.app
PKGNAME := ns020.zip
endif

ifeq ($(ATARIARCH),v4e)
CFLAGS += -mcpu=5475
LDFLAGS += -mcpu=5475
PRGSUFFIX := v4e.app
PKGNAME := nsv4e.zip
endif

# non-pkgconfig components

SPIDERMONKEY_CFLAGS := -DXP_UNIX -DJS_HAS_FILE_OBJECT=0 -DJSOPTION_JIT=0 -DPOSIX_SOURCE -D_BSD_SOURCE

# freetype is optional but older versions do not use pkg-config
ifeq ($(NETSURF_USE_ATARI_FREETYPE_FONT),YES)
  NETSURF_USE_FREETYPE2 := AUTO
  NETSURF_FEATURE_FREETYPE2_CFLAGS := -DWITH_FREETYPE_FONT_DRIVER

  $(eval $(call pkg_config_find_and_add_enabled,FREETYPE2,freetype2,freetype2))

  # try and use non pkg-config method
  ifeq ($(NETSURF_USE_FREETYPE2),NO)
    FREETYPE_CONFIG_EXISTS := $(shell $(FT2CF) --cflags >/dev/null && echo yes)
    ifeq ($(FREETYPE_CONFIG_EXISTS),yes)
      NETSURF_USE_FREETYPE2 := YES
      CFLAGS += $(shell $(FT2CF) --cflags) $(NETSURF_FEATURE_FREETYPE2_CFLAGS)
      LDFLAGS += $(shell $(FT2CF) --libs)
      $(info FT2.CNFG: freetype2 (freetype2)	enabled)
    else
      $(info FT2.CNFG: freetype2 (freetype2)	failed)
      $(error Unable to find library for: freetype2)
    endif
  endif
endif


$(eval $(call feature_enabled,ATARI_NETSURF_FONT,-DWITH_INTERNAL_FONT_DRIVER,,(Internal Font)))
$(eval $(call feature_enabled,ATARI_VDI_FONT,-DWITH_VDI_FONT_DRIVER,,(VDI Font)))
$(eval $(call feature_enabled,ATARI_8BPP_SUPPORT,-DWITH_8BPP_SUPPORT,,(Indexed screen format support)))

$(eval $(call pkg_config_find_and_add,libcares,Cares))

CFLAGS += -U__STRICT_ANSI__ -std=c99 -Dsmall -Dnsatari \
		-D_BSD_SOURCE \
		-D_XOPEN_SOURCE=600 \
		-D_POSIX_C_SOURCE=200112L \
		-DNSLOG_LEVEL_0x0010=NSLOG_LEVEL_INFO \
		$(shell $(PKG_CONFIG) --cflags openssl ) \
		$(shell $(PKG_CONFIG) --cflags libcurl )

LDFLAGS += -lcflib -lcurl
LDFLAGS += -lssl -lcrypto
LDFLAGS += -lz -lHermes -lgem -lm
LDFLAGS += -L$(GCCSDK_INSTALL_ENV)/lib


# sources purely for the Atari FreeMiNT build
S_FRONTEND := \
	about.c \
	bitmap.c \
	clipboard.c \
	ctxmenu.c \
	cookies.c \
	deskmenu.c \
	download.c \
	encoding.c \
	file.c \
	findfile.c \
	filetype.c \
	font.c \
	gui.c \
	hotlist.c \
	history.c \
	misc.c \
	osspec.c \
	redrawslots.c \
	rootwin.c \
	schedule.c \
	search.c \
	statusbar.c \
	settings.c \
	toolbar.c \
	treeview.c \
	plot/plot.c \
	plot/fontplot.c \
	plot/eddi.s \
	plot/font_freetype.c \
	plot/font_internal.c \
	plot/font_vdi.c \
	gemtk/aestabs.c \
	gemtk/dragdrop.c \
	gemtk/guiwin.c \
	gemtk/msgbox.c \
	gemtk/utils.c \
	gemtk/objc.c


# This is the final source build list
# Note this is deliberately *not* expanded here as common and image
#   are not yet available
SOURCES = $(S_COMMON) $(S_IMAGE) $(S_BROWSER) $(S_FRONTEND)

# ---------------------------------------------------------------------------
# Target setup
# ---------------------------------------------------------------------------

EXETARGET := ns$(SUBTARGET)$(PRGSUFFIX)

# The filter and target for split messages
MESSAGES_FILTER=atari

# ----------------------------------------------------------------------------
# Install target
# ----------------------------------------------------------------------------

ATARI_TARGET_DIR := netsurf/
ATARI_RES_DIR := $(FRONTEND_RESOURCES_DIR)/
ATARI_DOC_DIR := $(FRONTEND_SOURCE_DIR)/doc/
ATARI_FONT_NAME := ttf-bitstream-vera-1.10
ATARI_FONT_SOURCE_URL := http://ftp.gnome.org/pub/GNOME/sources/ttf-bitstream-vera/1.10/$(ATARI_FONT_NAME).tar.gz
#ATARI_FONT_SOURCE_URL := http://localhost/$(ATARI_FONT_NAME).tar.gz
ATARI_FONT_TMP_DIR := $(DEPROOT)/../
ATARI_FONT_SOURCE_DIR := $(ATARI_FONT_TMP_DIR)$(ATARI_FONT_NAME)/
ATARI_GENERIC_RESOURCES := de en it ja nl
ATARI_RESOURCES := $(addprefix resources/,$(ATARI_GENERIC_RESOURCES))

install-atari:

# ----------------------------------------------------------------------------
# Package target
# ----------------------------------------------------------------------------

$(ATARI_FONT_TMP_DIR)$(ATARI_FONT_NAME):
	$(Q)wget $(ATARI_FONT_SOURCE_URL) -O $(ATARI_FONT_TMP_DIR)$(ATARI_FONT_NAME).tar.gz
	tar xfz $(ATARI_FONT_TMP_DIR)/$(ATARI_FONT_NAME).tar.gz -C $(ATARI_FONT_TMP_DIR)

package-atari: $(ATARI_FONT_TMP_DIR)$(ATARI_FONT_NAME) $(PKGNAME)
	$(VQ)echo Creating $(PKGNAME)

$(PKGNAME): $(EXETARGET) $(POSTEXES)
ifneq ($(strip $(STRIP)),)
	$(Q)echo Stripping symbols from $(EXETARGET) with $(STRIP)
	$(Q)$(STRIP) $(EXETARGET)
endif
ifneq ($(strip $(STACK)),)
	$(Q)$(STACK) -S 256k $(EXETARGET)
endif
	$(Q)rm -rf $(ATARI_TARGET_DIR)
	$(Q)rm -rf $(PKGNAME)
	$(Q)$(MKDIR) $(ATARI_TARGET_DIR)
	$(Q)$(MKDIR) $(ATARI_TARGET_DIR)doc
	$(Q)$(MKDIR) $(ATARI_TARGET_DIR)downloads
	$(Q)$(MKDIR) $(ATARI_TARGET_DIR)res
	$(Q)$(MKDIR) $(ATARI_TARGET_DIR)res/fonts
	$(Q)$(MKDIR) $(ATARI_TARGET_DIR)res/icons
	$(Q)$(MKDIR) $(ATARI_TARGET_DIR)res/cache
	$(Q)touch $(ATARI_TARGET_DIR)cookies
	$(Q)touch $(ATARI_TARGET_DIR)url.db

	$(Q)cp $(ATARI_DOC_DIR)bugs $(ATARI_TARGET_DIR)doc/
	$(Q)cp $(ATARI_DOC_DIR)faq.txt $(ATARI_TARGET_DIR)doc/
	$(Q)cp $(ATARI_DOC_DIR)readme.txt $(ATARI_TARGET_DIR)doc/
	$(Q)cp $(ATARI_DOC_DIR)todo.txt $(ATARI_TARGET_DIR)doc/

# GUI resources:
	$(Q)cp -r $(ATARI_RESOURCES)  $(ATARI_TARGET_DIR)res/
	$(Q)cp $(ATARI_RES_DIR)/netsurf.rsc  $(ATARI_TARGET_DIR)res/netsurf.rsc
	$(Q)cp $(ATARI_RES_DIR)/languages  $(ATARI_TARGET_DIR)res/languages
	$(Q)cp $(ATARI_RES_DIR)/icons/toolbar -R $(ATARI_TARGET_DIR)res/icons
	$(Q)cp resources/icons/* -R $(ATARI_TARGET_DIR)res/icons/

	$(Q)cp resources/netsurf.png $(ATARI_TARGET_DIR)res/netsurf.png
	$(Q)cp resources/adblock.css $(ATARI_TARGET_DIR)res/adblock.css
	$(Q)cp resources/default.css $(ATARI_TARGET_DIR)res/default.css
	$(Q)cp resources/quirks.css $(ATARI_TARGET_DIR)res/quirks.css
	$(Q)cp resources/internal.css $(ATARI_TARGET_DIR)res/internal.css
	$(Q)cp resources/SearchEngines $(ATARI_TARGET_DIR)res/search
	$(Q)cp resources/ca-bundle $(ATARI_TARGET_DIR)res/cabundle
	$(Q)$(INSTALL) -m 644 -T $(MESSAGES_TARGET)/en/Messages $(ATARI_TARGET_DIR)res/messages
	$(Q)cp resources/en/welcome.html $(ATARI_TARGET_DIR)res/welcome.html
	$(Q)cp resources/en/licence.html $(ATARI_TARGET_DIR)res/licence.html
	$(Q)cp resources/en/credits.html $(ATARI_TARGET_DIR)res/credits.html

# copy "Bitstream Vera" font:
	$(Q)cp $(ATARI_FONT_SOURCE_DIR)RELEASENOTES.TXT $(ATARI_TARGET_DIR)res/fonts/
	$(Q)cp $(ATARI_FONT_SOURCE_DIR)README.TXT $(ATARI_TARGET_DIR)res/fonts/
	$(Q)cp $(ATARI_FONT_SOURCE_DIR)COPYRIGHT.TXT $(ATARI_TARGET_DIR)res/fonts/

	$(Q)cp $(ATARI_FONT_SOURCE_DIR)Vera.ttf $(ATARI_TARGET_DIR)res/fonts/ss.ttf
	$(Q)cp $(ATARI_FONT_SOURCE_DIR)VeraBd.ttf $(ATARI_TARGET_DIR)res/fonts/ssb.ttf
	$(Q)cp $(ATARI_FONT_SOURCE_DIR)VeraIt.ttf $(ATARI_TARGET_DIR)res/fonts/ssi.ttf
	$(Q)cp $(ATARI_FONT_SOURCE_DIR)VeraBI.ttf $(ATARI_TARGET_DIR)res/fonts/ssib.ttf
	$(Q)cp $(ATARI_FONT_SOURCE_DIR)VeraSe.ttf $(ATARI_TARGET_DIR)res/fonts/s.ttf
	$(Q)cp $(ATARI_FONT_SOURCE_DIR)VeraSeBd.ttf $(ATARI_TARGET_DIR)res/fonts/sb.ttf
	$(Q)cp $(ATARI_FONT_SOURCE_DIR)VeraMono.ttf $(ATARI_TARGET_DIR)res/fonts/mono.ttf
	$(Q)cp $(ATARI_FONT_SOURCE_DIR)VeraMoBd.ttf $(ATARI_TARGET_DIR)res/fonts/monob.ttf
	$(Q)cp $(ATARI_FONT_SOURCE_DIR)VeraMoIt.ttf $(ATARI_TARGET_DIR)res/fonts/cursive.ttf
	$(Q)cp $(ATARI_FONT_SOURCE_DIR)VeraMoBI.ttf $(ATARI_TARGET_DIR)res/fonts/fantasy.ttf

	$(Q)cp $(EXETARGET) $(ATARI_TARGET_DIR)$(EXETARGET)

# zip files
	$(Q)zip $(PKGNAME) -9 -r ./$(ATARI_TARGET_DIR)

# delete temporary folders
	$(Q)rm -rf $(ATARI_TARGET_DIR)
