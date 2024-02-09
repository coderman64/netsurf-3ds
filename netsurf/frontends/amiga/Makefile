# ----------------------------------------------------------------------------
# Amiga target setup
# ----------------------------------------------------------------------------

CFLAGS += -std=c99 -Dnsamiga

ifneq ($(SUBTARGET),os3)
  CFLAGS += -O2 -mstrict-align -finline-functions -U__STRICT_ANSI__ -D__USE_INLINE__ -D__USE_BASETYPE__
else
  CFLAGS += -O2 -DPATH_MAX=1024 -D__m68k__ -m68020
endif

$(eval $(call feature_enabled,AMIGA_ICON,-DWITH_AMIGA_ICON,,Amiga icon))
$(eval $(call feature_enabled,AMIGA_DATATYPES,-DWITH_AMIGA_DATATYPES,,DataTypes))
$(eval $(call feature_enabled,AMISSL,-DWITH_AMISSL -D__NO_NET_API -D__NO_NETINCLUDE_ERRNO -I$(GCCSDK_INSTALL_ENV)/netinclude,-lamisslauto,AmiSSL))
#-DWITH_SSL

CFLAGS += -I$(GCCSDK_INSTALL_ENV)/include
CFLAGS += $(shell $(PKG_CONFIG) --cflags tre)

LDFLAGS += $(shell $(PKG_CONFIG) --static --libs libcurl)
LDFLAGS += $(shell $(PKG_CONFIG) --libs tre)
LDFLAGS += -L$(GCCSDK_INSTALL_ENV)/lib

LDFLAGS += -lpbl -liconv

ifeq ($(NETSURF_USE_OPENSSL),YES)
  LDFLAGS += $(shell $(PKG_CONFIG) --static --libs openssl)
endif

ifeq ($(SUBTARGET),os3)
  LDFLAGS += -lamiga -lm
endif

COMMON_WARNFLAGS += -Wno-cast-align

EXETARGET := NetSurf

# The filter and target for split messages
MESSAGES_FILTER=ami

# ----------------------------------------------------------------------------
# Source file setup
# ----------------------------------------------------------------------------

# sources purely for the Amiga build
S_FRONTEND := gui.c history.c hotlist.c schedule.c file.c	\
	misc.c bitmap.c font.c filetype.c utf8.c memory.c	\
	plotters.c object.c menu.c save_pdf.c arexx.c version.c		\
	cookies.c ctxmenu.c clipboard.c help.c font_scan.c		\
	launch.c search.c history_local.c download.c iff_dr2d.c		\
	gui_options.c print.c	theme.c	drag.c icon.c libs.c	\
	datatypes.c dt_picture.c dt_anim.c dt_sound.c plugin_hack.c	\
	stringview/stringview.c stringview/urlhistory.c rtg.c		\
	agclass/amigaguide_class.c os3support.c font_diskfont.c		\
	selectmenu.c hash/xxhash.c font_cache.c font_bullet.c		\
	nsoption.c corewindow.c gui_menu.c pageinfo.c

# This is the final source build list
# Note this is deliberately *not* expanded here as common and image
#   are not yet available
SOURCES = $(S_COMMON) $(S_IMAGE) $(S_BROWSER) $(S_FRONTEND)

# ----------------------------------------------------------------------------
# Install target
# ----------------------------------------------------------------------------

install-amiga:

# ----------------------------------------------------------------------------
# Package target
# ----------------------------------------------------------------------------

package-amiga: netsurf.lha

AMIGA_LANGUAGES := de en it ja nl
AMIGA_PLATFORM_RESOURCES := Pointers Themes default.css default.css.info favicon.png LangNames mimetypes Resource.map splash.png
AMIGA_GENERIC_RESOURCES := $(AMIGA_LANGUAGES) ca-bundle icons
AMIGA_RESOURCES := $(addprefix $(FRONTEND_SOURCE_DIR)/resources/,$(AMIGA_PLATFORM_RESOURCES)) $(addprefix resources/,$(AMIGA_GENERIC_RESOURCES))
AMIGA_DISTRIBUTION_FILES := $(FRONTEND_SOURCE_DIR)/dist/*
AMIGA_PKG_DIR := $(FRONTEND_SOURCE_DIR)/pkg
AMIGA_INSTALL_TARGET_DIR := NetSurf_Amiga

netsurf.lha: $(EXETARGET) $(POSTEXES)
	$(VQ)echo Creating netsurf.lha
	$(Q)rm -rf $(AMIGA_INSTALL_TARGET_DIR)
	$(Q)$(MKDIR) -p $(AMIGA_INSTALL_TARGET_DIR)/NetSurf
	$(Q)cp -p $(EXETARGET) $(AMIGA_INSTALL_TARGET_DIR)/NetSurf
	$(Q)$(MKDIR) $(AMIGA_INSTALL_TARGET_DIR)/NetSurf/Resources
	$(Q)cp -rp $(AMIGA_RESOURCES) $(AMIGA_INSTALL_TARGET_DIR)/NetSurf/Resources
	$(Q)mv $(AMIGA_INSTALL_TARGET_DIR)/NetSurf/Resources/icons $(AMIGA_INSTALL_TARGET_DIR)/NetSurf/Resources/Icons
	$(Q)cp -rp $(AMIGA_DISTRIBUTION_FILES) $(AMIGA_INSTALL_TARGET_DIR)/NetSurf
	$(Q)cat resources/SearchEngines $(AMIGA_PKG_DIR)/SearchEngines >$(AMIGA_INSTALL_TARGET_DIR)/NetSurf/Resources/SearchEngines
	$(Q)cp -p resources/adblock.css $(AMIGA_INSTALL_TARGET_DIR)/NetSurf/Resources/adblock.css
	$(Q)cp -p resources/default.css $(AMIGA_INSTALL_TARGET_DIR)/NetSurf/Resources/nsdefault.css
	$(Q)cp -p resources/internal.css $(AMIGA_INSTALL_TARGET_DIR)/NetSurf/Resources/internal.css
	$(Q)cp -p resources/quirks.css $(AMIGA_INSTALL_TARGET_DIR)/NetSurf/Resources/quirks.css
	$(Q)cp -p resources/netsurf.png $(AMIGA_INSTALL_TARGET_DIR)/NetSurf/Resources/netsurf.png
	$(Q)cp -p $(AMIGA_PKG_DIR)/drawer.info $(AMIGA_INSTALL_TARGET_DIR)/NetSurf.info
	$(Q)cp -p $(AMIGA_PKG_DIR)/AutoInstall $(AMIGA_INSTALL_TARGET_DIR)
  ifeq ($(SUBTARGET),os3)
	$(Q)cp -p $(AMIGA_PKG_DIR)/netsurf_os3.readme $(AMIGA_INSTALL_TARGET_DIR)/NetSurf
	$(Q)cp -p $(AMIGA_PKG_DIR)/netsurf_os3.readme.info $(AMIGA_INSTALL_TARGET_DIR)/NetSurf
  else
	$(Q)cp -p $(AMIGA_PKG_DIR)/netsurf.readme $(AMIGA_INSTALL_TARGET_DIR)/NetSurf
	$(Q)cp -p $(AMIGA_PKG_DIR)/netsurf.readme.info $(AMIGA_INSTALL_TARGET_DIR)/NetSurf
  endif
	$(Q)cd $(AMIGA_INSTALL_TARGET_DIR); \
	  lha ao6 netsurf.lha NetSurf NetSurf.info AutoInstall

