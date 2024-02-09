# ----------------------------------------------------------------------------
# BeOS target setup
# ----------------------------------------------------------------------------

# Beos build tools
BEOS_BERES := beres
BEOS_RC := rc
BEOS_XRES := xres
BEOS_SETVER := setversion
BEOS_MIMESET := mimeset

# Linker flags
LDFLAGS += -L/boot/home/config/lib
LDFLAGS += -L/boot/common/lib
LDFLAGS += -lbe -ltranslation -ltracker -lcolumnlistview -lnetwork
ifeq ($(CC_MAJOR),2)
    LDFLAGS += -lstdc++.r4
else
    LDFLAGS += -lstdc++ -lsupc++
endif
LDFLAGS += -L$(PREFIX)/lib

COMMON_WARNFLAGS += -Wno-multichar

# compiler flags
CFLAGS += -std=c99 -Dnsbeos -D_BSD_SOURCE -D_POSIX_C_SOURCE -Drestrict="" -g
CFLAGS += -I$(PREFIX)/include

CXXFLAGS += -Dnsbeos -D_BSD_SOURCE -D_POSIX_C_SOURCE -Drestrict="" -g
CXXFLAGS += -I$(PREFIX)/include

VERSION_FULL := $(shell sed -n '/_version.*=.*"/{s/.*"\(.*\)".*/\1/;p;}' desktop/version.c))
VERSION_MAJ := $(shell sed -n '/_major/{s/.* = \([0-9]*\).*/\1/;p;}' desktop/version.c)
VERSION_MIN := $(shell sed -n '/_minor/{s/.* = \([0-9]*\).*/\1/;p;}' desktop/version.c)


HARCH := $(shell getarch)
HPARCH := $(shell getarch -p)
ifneq ($(HARCH),$(HPARCH))
PKGSARCH := _$(HARCH)
endif

PKGNAME := netsurf$(PKGSARCH)-$(VERSION_MAJ).$(VERSION_MIN)-1-$(HPARCH).hpkg
HAIKU_TARGET_DIR := netsurf_pkg

# Identity of the packager
PACKAGER := $(shell git config --get user.name) <$(shell git config --get user.email)>

# package manifest file
# TODO: remove requires hardcoding
PKGINFO := \
name	netsurf$(PKGSARCH)\n\
version	$(VERSION_MAJ).$(VERSION_MIN)-1\n\
architecture	$(HPARCH)\n\
summary	"A multi-platform web browser"\n\
description	"Small as a mouse, fast as a cheetah and available for free. NetSurf is a multi-platform web browser for RISC OS, UNIX-like platforms (including Linux), Mac OS X, and more.Whether you want to check your webmail, read the news or post to discussion forums, NetSurf is your lightweight gateway to the world wide web. Actively developed, NetSurf is continually evolving and improving."\n\
vendor	"NetSurf"\n\
packager	"$(PACKAGER)"\n\
copyrights	{\n\t"2003-2018 The NetSurf Browser project"\n}\n\
licenses	{\n\t"GNU GPL v2"\n}\n\
urls	{\n\t"http://www.netsurf-browser.org/"\n}\n\
source-urls	{\n\t"http://download.netsurf-browser.org/netsurf/releases/source/netsurf-$(VERSION_MAJ).$(VERSION_MIN)-src.tar.gz"\n}\n\
provides	{\n\
\tnetsurf$(PKGSARCH) = $(VERSION_MAJ).$(VERSION_MIN)\n\
\tapp:netsurf = $(VERSION_MAJ).$(VERSION_MIN)\n\
}\n\
requires	{\n\
\thaiku_x86>=r1~alpha4_pm_hrev51655-1\n\
\tlib:libcurl_x86>=4.4.0\n\
\tlib:libexpat_x86>=1.6.7\n\
\tlib:libiconv_x86>=2.5.0\n\
\tlib:libpng16_x86>=16.34.0\n\
\tlib:libjpeg_x86>=9.2.0\n\
\tlib:libssl_x86>=1.0.0\n\
\tlib:libutf8proc_x86>=1.3.1\n\
\tlib:libz_x86>=1.2.11\n\
}\n


# ----------------------------------------------------------------------------
# Source file setup
# ----------------------------------------------------------------------------

# sources purely for the BeOS build
S_FRONTEND := about.cpp bitmap.cpp cookies.cpp download.cpp		\
	fetch_rsrc.cpp filetype.cpp font.cpp gui.cpp login.cpp		\
	gui_options.cpp plotters.cpp scaffolding.cpp search.cpp		\
	schedule.cpp throbber.cpp window.cpp

# This is the final source build list
# Note this is deliberately *not* expanded here as common and image
#   are not yet available
SOURCES = $(S_COMMON) $(S_IMAGE) $(S_BROWSER) $(S_FRONTEND)
EXETARGET := NetSurf

# The filter and target for split messages
MESSAGES_FILTER=beos
MESSAGES_TARGET=$(FRONTEND_RESOURCES_DIR)

# ----------------------------------------------------------------------------
# Resources
# ----------------------------------------------------------------------------

RDEF_BEOS := res.rdef
RDEF_BEOS := $(addprefix $(FRONTEND_SOURCE_DIR)/,$(RDEF_BEOS))

RDEF_IMP_BEOS := res_import.rdef
RDEF_IMP_BEOS := $(addprefix $(OBJROOT)/,$(subst /,_,$(RDEF_IMP_BEOS)))

RDEP_BEOS := \
	adblock.css beosdefault.css default.css internal.css quirks.css	\
	netsurf.png favicon.png ca-bundle.txt \
	credits.html licence.html welcome.html SearchEngines

RDEP_BEOS := $(addprefix $(FRONTEND_RESOURCES_DIR)/,$(RDEP_BEOS))	\
	$(wildcard $(FRONTEND_RESOURCES_DIR)/icons/*.png)		\
	$(wildcard $(FRONTEND_RESOURCES_DIR)/??/*)			\
	$(wildcard $(FRONTEND_RESOURCES_DIR)/throbber/throbber*.png)

RSRC_BEOS = $(addprefix $(OBJROOT)/,$(subst /,_,$(patsubst %.rdef,%.rsrc,$(RDEF_BEOS))))
RESOURCES = $(RSRC_BEOS)

$(RDEF_IMP_BEOS): $(RDEP_BEOS)
	$(VQ)echo "     GEN: $@"
	$(Q)n=5000; for f in $^; do echo "resource($$n,\"$${f#$(FRONTEND_RESOURCES_DIR)/}\") #'data' import \"$${f#$(FRONTEND_SOURCE_DIR)/}\";"; n=$$(($$n+1)); done > $@

$(RSRC_BEOS): $(RDEF_BEOS) $(RDEF_IMP_BEOS)
	$(VQ)echo "      RC: $<"
	$(Q)$(BEOS_RC) -I $(FRONTEND_SOURCE_DIR) -o $@ $^


# ----------------------------------------------------------------------------
# Install target
# ----------------------------------------------------------------------------

install-beos:
	$(Q)$(MKDIR) -p $(DESTDIR)$(NETSURF_BEOS_BIN)
	$(Q)$(MKDIR) -p $(DESTDIR)$(NETSURF_BEOS_RESOURCES)
	$(Q)copyattr -d $(EXETARGET) $(DESTDIR)$(NETSURF_BEOS_BIN)NetSurf
	$(Q)cp -vRL $(FRONTEND_RESOURCES_DIR)/adblock.css $(DESTDIR)$(NETSURF_BEOS_RESOURCES)
	$(Q)cp -vRL $(FRONTEND_RESOURCES_DIR)/ca-bundle.txt $(DESTDIR)$(NETSURF_BEOS_RESOURCES)
	$(Q)cp -vRL $(FRONTEND_RESOURCES_DIR)/default.css $(DESTDIR)$(NETSURF_BEOS_RESOURCES)
	$(Q)cp -vRL $(FRONTEND_RESOURCES_DIR)/beosdefault.css $(DESTDIR)$(NETSURF_BEOS_RESOURCES)
	$(Q)cp -vRL $(FRONTEND_RESOURCES_DIR)/license $(DESTDIR)$(NETSURF_BEOS_RESOURCES)
	$(Q)cp -vRL $(FRONTEND_RESOURCES_DIR)/SearchEngines $(DESTDIR)$(NETSURF_BEOS_RESOURCES)

# ----------------------------------------------------------------------------
# Package target
# ----------------------------------------------------------------------------

package-beos: $(PKGNAME)
	$(VQ)echo Creating $(PKGNAME)

$(PKGNAME): $(EXETARGET) $(POSTEXES)
	$(Q)rm -rf $(HAIKU_TARGET_DIR)
	$(Q)rm -rf $(PKGNAME)
	$(Q)$(MKDIR) $(HAIKU_TARGET_DIR)
	$(Q)echo -e '$(PKGINFO)' > $(HAIKU_TARGET_DIR)/.PackageInfo
	$(Q)$(MKDIR) -p $(HAIKU_TARGET_DIR)/apps/netsurf/res
	$(Q)$(MKDIR) -p $(HAIKU_TARGET_DIR)/data/deskbar/menu/Applications
	$(Q)copyattr -d $(EXETARGET) $(HAIKU_TARGET_DIR)/apps/NetSurf
	$(Q)cp -vRL $(FRONTEND_RESOURCES_DIR)/adblock.css $(HAIKU_TARGET_DIR)/apps/netsurf/res
	$(Q)cp -vRL $(FRONTEND_RESOURCES_DIR)/ca-bundle.txt $(HAIKU_TARGET_DIR)/apps/netsurf/res
	$(Q)cp -vRL $(FRONTEND_RESOURCES_DIR)/default.css $(HAIKU_TARGET_DIR)/apps/netsurf/res
	$(Q)cp -vRL $(FRONTEND_RESOURCES_DIR)/beosdefault.css $(HAIKU_TARGET_DIR)/apps/netsurf/res
	$(Q)cp -vRL $(FRONTEND_RESOURCES_DIR)/license $(HAIKU_TARGET_DIR)/apps/netsurf/res
	$(Q)cp -vRL $(FRONTEND_RESOURCES_DIR)/SearchEngines $(HAIKU_TARGET_DIR)/apps/netsurf/res
	$(Q)ln -s ../../../../apps/NetSurf $(HAIKU_TARGET_DIR)/data/deskbar/menu/Applications/
	package create -v -C $(HAIKU_TARGET_DIR) $(PKGNAME)
