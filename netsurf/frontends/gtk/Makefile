#
# Makefile for NetSurf GTK target
#
# This file is part of NetSurf 
#
# ----------------------------------------------------------------------------
# GTK flag setup (using pkg-config)
# ----------------------------------------------------------------------------

# define additional CFLAGS and LDFLAGS requirements for pkg-configed libs here
NETSURF_FEATURE_RSVG_CFLAGS := -DWITH_RSVG
NETSURF_FEATURE_VIDEO_CFLAGS := -DWITH_VIDEO

# determine if the rsvg library API version
RSVG_API := $(shell $(PKG_CONFIG) --atleast-version=2.46 librsvg-2.0 && echo 246)
$(eval $(call pkg_config_find_and_add_enabled,RSVG,librsvg-2.0,SVG))
$(eval $(call pkg_config_find_and_add_enabled,VIDEO,gstreamer-0.10,Video))

# GTK and GLIB flags to disable depricated usage
GTKDEPFLAGS := -DG_DISABLE_SINGLE_INCLUDES \
		-DG_DISABLE_DEPRECATED \
	       	-DGTK_DISABLE_SINGLE_INCLUDES \
		-DGTK_MULTIHEAD_SAFE \
		-DPANGO_DISABLE_DEPRECATED

# later editions of gtk 2 deprecate interfaces we rely upon for cursors
# -DGDK_PIXBUF_DISABLE_DEPRECATED

# libsexy currently means we cannot enable this
#		-DGDK_DISABLE_DEPRECATED 

# gtk3 is depricating interfaces we use a lot
ifeq ($(NETSURF_GTK_MAJOR),2)
GTKDEPFLAGS += -DGTK_DISABLE_DEPRECATED
endif


GTKCFLAGS := -std=c99 -Dgtk -Dnsgtk -g \
		$(GTKDEPFLAGS) \
		-D_BSD_SOURCE \
		-D_DEFAULT_SOURCE \
		-D_XOPEN_SOURCE=700 \
		-D_POSIX_C_SOURCE=200809L \
		-D_NETBSD_SOURCE \
		-DGTK_RESPATH=\"$(NETSURF_GTK_RES_PATH)\"

# non optional pkg-configed libs
$(eval $(call pkg_config_find_and_add,gtk+-$(NETSURF_GTK_MAJOR).0,GTK-$(NETSURF_GTK_MAJOR)))
$(eval $(call pkg_config_find_and_add,gthread-2.0,GThread2))
$(eval $(call pkg_config_find_and_add,gmodule-2.0,GModule2))


CFLAGS += $(GTKCFLAGS)
LDFLAGS += -lm

# ---------------------------------------------------------------------------
# Target setup
# ---------------------------------------------------------------------------

# Path to GTK resources
NSGTK_RESOURCES_DIR := $(FRONTEND_RESOURCES_DIR)

# The gtk binary target.
EXETARGET := nsgtk$(NETSURF_GTK_MAJOR)

# The filter and target for split messages
MESSAGES_FILTER=gtk
MESSAGES_TARGET=$(NSGTK_RESOURCES_DIR)

# ---------------------------------------------------------------------------
# Windows flag setup
# ---------------------------------------------------------------------------

ifeq ($(HOST),Windows_NT)
    CFLAGS += -U__STRICT_ANSI__
endif

# ----------------------------------------------------------------------------
# Builtin resource handling
# ----------------------------------------------------------------------------

# builtin resource sources
S_RESOURCE :=

# Glib prior to 2.32 does not have GResource handling.
#
# This uses pkg-config to check for the minimum required version for
#  this feature in a way similar to the pkg_config_find_and_add_enabled
#  macro. Note we check for gmodule-2.0 which is a specific part of
#  glib we require.
#
# It would be nice if we could check for this functionality rather
#  than "knowing" the version but there does not appear to be a simple
#  way to implement that.
#
NETSURF_FEATURE_GRESOURCE_AVAILABLE := $(shell $(PKG_CONFIG) --atleast-version=2.32 gmodule-2.0 && echo yes)
ifneq (,$(filter $(NETSURF_USE_GRESOURCE),AUTO YES))
ifeq ($(NETSURF_FEATURE_GRESOURCE_AVAILABLE),yes)

# Gresource use has been enabled
NETSURF_FEATURE_GRESOURCE_ENABLED := yes

#resource compiler tool
GLIB_COMPILE_RESOURCES := glib-compile-resources
CFLAGS += -DWITH_GRESOURCE

NETSURF_GRESOURCE_XML := $(NSGTK_RESOURCES_DIR)/netsurf.gresource.xml
UI_GRESOURCE_XML := $(NSGTK_RESOURCES_DIR)/ui.gresource.xml
MESSAGES_GRESOURCE_XML := $(NSGTK_RESOURCES_DIR)/messages.gresource.xml

# generate the netsurf gresource source files
$(OBJROOT)/netsurf_gresource.c: $(NETSURF_GRESOURCE_XML) $(shell $(GLIB_COMPILE_RESOURCES) --sourcedir $(NSGTK_RESOURCES_DIR) --generate-dependencies $(NETSURF_GRESOURCE_XML))
	$(VQ)echo "GRESORCE: $<"
	$(Q)$(GLIB_COMPILE_RESOURCES) --generate-source --sourcedir $(NSGTK_RESOURCES_DIR) --target=$@ $<

S_RESOURCE += $(OBJROOT)/netsurf_gresource.c

# generate the ui gresource source files
$(OBJROOT)/ui_gresource.c: $(UI_GRESOURCE_XML) $(shell $(GLIB_COMPILE_RESOURCES) --sourcedir $(NSGTK_RESOURCES_DIR)/gtk$(NETSURF_GTK_MAJOR) --generate-dependencies $(UI_GRESOURCE_XML))
	$(VQ)echo "GRESORCE: $<"
	$(Q)$(GLIB_COMPILE_RESOURCES) --generate-source --sourcedir $(NSGTK_RESOURCES_DIR)/gtk$(NETSURF_GTK_MAJOR) --target=$@ $<

S_RESOURCE += $(OBJROOT)/ui_gresource.c

# generate the messages gresource source file
$(OBJROOT)/messages_gresource.c: $(MESSAGES_GRESOURCE_XML) $(addsuffix /Messages,$(addprefix $(MESSAGES_TARGET)/,$(MESSAGES_LANGUAGES)))
	$(VQ)echo "GRESORCE: $<"
	$(Q)$(GLIB_COMPILE_RESOURCES) --generate-source --sourcedir $(NSGTK_RESOURCES_DIR) --target=$@ $<

S_RESOURCE += $(OBJROOT)/messages_gresource.c


endif
endif

# Build pixbufs as inlines if enabled
ifneq (,$(filter $(NETSURF_USE_INLINE_PIXBUF),AUTO YES))
ifneq ($(NETSURF_FEATURE_GRESOURCE_ENABLED),yes)

CFLAGS += -DWITH_BUILTIN_PIXBUF

GTK_IMAGE_favicon := favicon.png
GTK_IMAGE_netsurf := netsurf.xpm
GTK_IMAGE_menu_cursor := menu_cursor.png

# 1: input file
# 2: output file
# 3: bitmap name
define convert_image

# add converted pixbuf to builtin resource sources
S_RESOURCE += $(2)

$(2): $(1)
	$(VQ)echo "  INLINE: ${3}"
	$(Q)echo "#include <gdk-pixbuf/gdk-pixdata.h>" > $(2)
	$(Q)gdk-pixbuf-csource --extern --raw --name=$(3) $(1) >> $(2) || \
	( rm -f $(2) && false )

endef

$(eval $(foreach V,$(filter GTK_IMAGE_%,$(.VARIABLES)),$(call convert_image,$(addprefix $(NSGTK_RESOURCES_DIR)/,$($(V))),$(OBJROOT)/$(patsubst GTK_IMAGE_%,%,$(V)).c,$(patsubst GTK_IMAGE_%,%,$(V))_pixdata)))
endif
endif

# ----------------------------------------------------------------------------
# Source file setup
# ----------------------------------------------------------------------------

# S_FRONTEND are sources purely for the GTK frontend
S_FRONTEND := gui.c misc.c schedule.c layout_pango.c bitmap.c plotters.c \
	scaffolding.c gdk.c completion.c throbber.c accelerator.c        \
	selection.c window.c fetch.c download.c menu.c print.c		 \
	search.c tabs.c toolbar.c gettext.c compat.c viewdata.c		 \
	viewsource.c preferences.c about.c resources.c corewindow.c	 \
	local_history.c global_history.c cookies.c hotlist.c page_info.c

# This is the final source build list
# Note this is deliberately *not* expanded here as common and image
#   are not yet available
SOURCES = $(S_COMMON) $(S_IMAGE) $(S_BROWSER) $(S_RESOURCE) $(S_FRONTEND)

# ----------------------------------------------------------------------------
# Install target
# ----------------------------------------------------------------------------

GTK_RESOURCES_LIST := \
	languages SearchEngines ca-bundle.txt \
	default.css adblock.css quirks.css internal.css \
	credits.html licence.html welcome.html Messages \
	default.ico favicon.png netsurf.png netsurf.xpm netsurf-16x16.xpm

GTK_RESOURCES_LIST := \
	$(addprefix $(NSGTK_RESOURCES_DIR)/, $(GTK_RESOURCES_LIST)) \
	$(wildcard $(NSGTK_RESOURCES_DIR)/gtk$(NETSURF_GTK_MAJOR)/*.ui)

# translations with more than just Messages files
GTK_TRANSLATIONS_HTML := de en fr it ja nl zh_CN

# destination for installed resources is the first entry in the gtk resource path
NSGTK_RESOURCES_DESTDIR := $(DESTDIR)$(word 1,$(subst :, ,$(NETSURF_GTK_RES_PATH)))

install-gtk:
	$(Q)$(MKDIR) -p $(DESTDIR)$(NETSURF_GTK_BIN)
	$(Q)install $(EXETARGET) $(DESTDIR)$(NETSURF_GTK_BIN)netsurf-gtk$(SUBTARGET)
	$(Q)$(MKDIR) -p $(NSGTK_RESOURCES_DESTDIR)/icons
	$(Q)install -m 0644 $(NSGTK_RESOURCES_DIR)/icons/*.png $(NSGTK_RESOURCES_DESTDIR)/icons
	$(Q)$(MKDIR) -p $(NSGTK_RESOURCES_DESTDIR)/throbber
	$(Q)install -m 0644 $(NSGTK_RESOURCES_DIR)/throbber/*.png $(NSGTK_RESOURCES_DESTDIR)/throbber
	$(Q)tar -c -h -C $(NSGTK_RESOURCES_DIR) -f - $(GTK_TRANSLATIONS_HTML) | tar -xv -C $(NSGTK_RESOURCES_DESTDIR) -f -
	$(Q)install -m 0644 $(GTK_RESOURCES_LIST) $(NSGTK_RESOURCES_DESTDIR)

# ----------------------------------------------------------------------------
# Package target
# ----------------------------------------------------------------------------

package-gtk:
