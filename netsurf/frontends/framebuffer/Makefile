#
# Makefile for NetSurf Framebuffer frontend
#
# This file is part of NetSurf 
#
# ----------------------------------------------------------------------------
# Framebuffer flag setup (using pkg-config)
# ----------------------------------------------------------------------------

CFLAGS += -std=c99 -g \
	  -Dnsframebuffer -Dsmall

#resource path
CFLAGS += '-DNETSURF_FB_RESPATH="$(NETSURF_FB_RESPATH)"'

# compile time font locations
CFLAGS += '-DNETSURF_FB_FONTPATH="$(NETSURF_FB_FONTPATH)"'
CFLAGS += '-DNETSURF_FB_FONT_SANS_SERIF="$(NETSURF_FB_FONT_SANS_SERIF)"'
CFLAGS += '-DNETSURF_FB_FONT_SANS_SERIF_BOLD="$(NETSURF_FB_FONT_SANS_SERIF_BOLD)"'
CFLAGS += '-DNETSURF_FB_FONT_SANS_SERIF_ITALIC="$(NETSURF_FB_FONT_SANS_SERIF_ITALIC)"'
CFLAGS += '-DNETSURF_FB_FONT_SANS_SERIF_ITALIC_BOLD="$(NETSURF_FB_FONT_SANS_SERIF_ITALIC_BOLD)"'
CFLAGS += '-DNETSURF_FB_FONT_SERIF="$(NETSURF_FB_FONT_SERIF)"'
CFLAGS += '-DNETSURF_FB_FONT_SERIF_BOLD="$(NETSURF_FB_FONT_SERIF_BOLD)"'
CFLAGS += '-DNETSURF_FB_FONT_MONOSPACE="$(NETSURF_FB_FONT_MONOSPACE)"'
CFLAGS += '-DNETSURF_FB_FONT_MONOSPACE_BOLD="$(NETSURF_FB_FONT_MONOSPACE_BOLD)"'
CFLAGS += '-DNETSURF_FB_FONT_CURSIVE="$(NETSURF_FB_FONT_CURSIVE)"'
CFLAGS += '-DNETSURF_FB_FONT_FANTASY="$(NETSURF_FB_FONT_FANTASY)"'

LDFLAGS += -lm

# freetype is optional but older versions do not use pkg-config
ifeq ($(NETSURF_FB_FONTLIB),freetype)
  NETSURF_USE_FREETYPE2 := AUTO
  NETSURF_FEATURE_FREETYPE2_CFLAGS := -DFB_USE_FREETYPE

  $(eval $(call pkg_config_find_and_add_enabled,FREETYPE2,freetype2,freetype2))

  # try and use non pkg-config method
  ifeq ($(NETSURF_USE_FREETYPE2),NO)
    FREETYPE_CONFIG_EXISTS := $(shell freetype-config --cflags >/dev/null && echo yes)
    ifeq ($(FREETYPE_CONFIG_EXISTS),yes)
      NETSURF_USE_FREETYPE2 := YES
      CFLAGS += $(shell freetype-config --cflags) $(NETSURF_FEATURE_FREETYPE2_CFLAGS)
      LDFLAGS += $(shell freetype-config --libs)
      $(info FT2.CNFG: freetype2 (freetype2)	enabled)
    else
      $(info FT2.CNFG: freetype2 (freetype2)	failed)
      $(error Unable to find library for: freetype2)
    endif
  endif
endif

# non optional pkg-configed libs
LDFLAGS += -Wl,--whole-archive
$(eval $(call pkg_config_find_and_add,libnsfb,libnsfb))
LDFLAGS += -Wl,--no-whole-archive

# ---------------------------------------------------------------------------
# Target setup
# ---------------------------------------------------------------------------

# The filter and target for split messages
MESSAGES_FILTER=fb
MESSAGES_TARGET=$(FRONTEND_RESOURCES_DIR)

# ---------------------------------------------------------------------------
# HOST specific feature flags
# ---------------------------------------------------------------------------

# enable POSIX and XSI feature flasg except:
#   - the default set on freebsd already has them enabled
#   - openbsd does not require the default source flags
ifneq ($(HOST),FreeBSD)
  ifneq ($(HOST),OpenBSD)
    CFLAGS += -D_POSIX_C_SOURCE=200809L \
	      -D_XOPEN_SOURCE=700 \
	      -D_BSD_SOURCE \
	      -D_DEFAULT_SOURCE \
	      -D_NETBSD_SOURCE
  else
    CFLAGS += -D_POSIX_C_SOURCE=200809L
  endif
endif

# ----------------------------------------------------------------------------
# built-in resource setup
# ----------------------------------------------------------------------------

FB_IMAGE_left_arrow := icons/back.png
FB_IMAGE_right_arrow := icons/forward.png
FB_IMAGE_reload := icons/reload.png
FB_IMAGE_stop_image := icons/stop.png
FB_IMAGE_history_image := icons/history.png

FB_IMAGE_left_arrow_g := icons/back_g.png
FB_IMAGE_right_arrow_g := icons/forward_g.png
FB_IMAGE_reload_g := icons/reload_g.png
FB_IMAGE_stop_image_g := icons/stop_g.png
FB_IMAGE_history_image_g := icons/history_g.png

FB_IMAGE_scrolll := icons/scrolll.png
FB_IMAGE_scrollr := icons/scrollr.png
FB_IMAGE_scrollu := icons/scrollu.png
FB_IMAGE_scrolld := icons/scrolld.png

FB_IMAGE_osk_image := icons/osk.png

FB_IMAGE_pointer_image := pointers/default.png
FB_IMAGE_hand_image := pointers/point.png
FB_IMAGE_caret_image := pointers/caret.png
FB_IMAGE_menu_image := pointers/menu.png
FB_IMAGE_progress_image := pointers/progress.png
FB_IMAGE_move_image := pointers/move.png

FB_IMAGE_throbber0 := throbber/throbber0.png
FB_IMAGE_throbber1 := throbber/throbber1.png
FB_IMAGE_throbber2 := throbber/throbber2.png
FB_IMAGE_throbber3 := throbber/throbber3.png
FB_IMAGE_throbber4 := throbber/throbber4.png
FB_IMAGE_throbber5 := throbber/throbber5.png
FB_IMAGE_throbber6 := throbber/throbber6.png
FB_IMAGE_throbber7 := throbber/throbber7.png
FB_IMAGE_throbber8 := throbber/throbber8.png

# make convert_image depend on fbtk.h so that if we change that
# header, we get new images built.
$(TOOLROOT)/convert_image: $(FRONTEND_SOURCE_DIR)/fbtk.h


# 1: input file
# 2: output file
# 3: bitmap name
define convert_image

S_IMAGES += $(2)

$(2): $(1) $(TOOLROOT)/convert_image
	$(Q)$(TOOLROOT)/convert_image $(1) $(2) $(3)

endef

S_IMAGES :=

$(eval $(foreach V,$(filter FB_IMAGE_%,$(.VARIABLES)),$(call convert_image,$(FRONTEND_RESOURCES_DIR)/$($(V)),$(OBJROOT)/image-$(patsubst FB_IMAGE_%,%,$(V)).c,$(patsubst FB_IMAGE_%,%,$(V)))))


# Internal fonts to generate
FB_FONT_internal_ns-sans := fonts/glyph_data

# 1: input file
# 2: output source code file
# 3: output header file
# 4: font name
define convert_font

S_FONTS += $(2)

$(2) $(3): $(1) $(TOOLROOT)/convert_font
	$(VQ)echo "    FONT: $(1) ($(4))"
	$(Q)$(TOOLROOT)/convert_font -H $(3) $(1) $(2)

frontends/framebuffer/font_internal.c: $(2)

endef

S_FONTS :=

$(eval $(foreach V,$(filter FB_FONT_$(NETSURF_FB_FONTLIB)_%,$(.VARIABLES)),$(call convert_font,$(FRONTEND_RESOURCES_DIR)/$($(V)),$(OBJROOT)/font-$(patsubst FB_FONT_$(NETSURF_FB_FONTLIB)_%,%,$(V)).c,$(OBJROOT)/font-$(patsubst FB_FONT_$(NETSURF_FB_FONTLIB)_%,%,$(V)).h,$(patsubst FB_FONT_$(NETSURF_FB_FONTLIB)_%,%,$(V)))))

# ----------------------------------------------------------------------------
# Source file setup
# ----------------------------------------------------------------------------

# S_FRONTEND are sources purely for the framebuffer build
S_FRONTEND := gui.c framebuffer.c schedule.c bitmap.c fetch.c	\
	findfile.c corewindow.c local_history.c clipboard.c

# toolkit sources
S_FRAMEBUFFER_FBTK := fbtk.c event.c fill.c bitmap.c user.c window.c 	\
	text.c scroll.c osk.c 

S_FRONTEND += font_$(NETSURF_FB_FONTLIB).c

S_FRONTEND += $(addprefix fbtk/,$(S_FRAMEBUFFER_FBTK))

# This is the final source build list
# Note this is deliberately *not* expanded here as common and image
#   are not yet available
SOURCES = $(S_COMMON) $(S_IMAGE) $(S_BROWSER) $(S_FRONTEND) $(S_IMAGES) $(S_FONTS)
EXETARGET := nsfb

# ----------------------------------------------------------------------------
# Install target
# ----------------------------------------------------------------------------

NETSURF_FRAMEBUFFER_RESOURCE_LIST := adblock.css credits.html	\
	default.css internal.css licence.html			\
	netsurf.png quirks.css welcome.html

install-framebuffer:
	$(VQ)echo " INSTALL: $(DESTDIR)/$(PREFIX)"
	$(Q)$(INSTALL) -d $(DESTDIR)/$(NETSURF_FRAMEBUFFER_BIN)
	$(Q)$(INSTALL) -T $(EXETARGET) $(DESTDIR)/$(NETSURF_FRAMEBUFFER_BIN)/netsurf-fb
	$(Q)$(INSTALL) -d $(DESTDIR)/$(NETSURF_FRAMEBUFFER_RESOURCES)
	$(Q)for F in $(NETSURF_FRAMEBUFFER_RESOURCE_LIST); do $(INSTALL) -m 644 $(FRONTEND_RESOURCES_DIR)/$$F $(DESTDIR)/$(NETSURF_FRAMEBUFFER_RESOURCES); done
	$(Q)$(INSTALL) -m 644 -T $(MESSAGES_TARGET)/en/Messages $(DESTDIR)/$(NETSURF_FRAMEBUFFER_RESOURCES)/Messages

# ----------------------------------------------------------------------------
# Package target
# ----------------------------------------------------------------------------

package-framebuffer:
