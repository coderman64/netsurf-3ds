#
# Makefile for NetSurf Windows target
#
# This file is part of NetSurf 


VERSION_MAJ := $(shell sed -n '/_major/{s/.* = \([0-9]*\).*/\1/;p;}' desktop/version.c)
VERSION_MIN := $(shell sed -n '/_minor/{s/.* = \([0-9]*\).*/\1/;p;}' desktop/version.c)


LDFLAGS += -L${GCCSDK_INSTALL_ENV}/lib 
CFLAGS += -I${GCCSDK_INSTALL_ENV}/include/

$(eval $(call pkg_config_find_and_add,libcares,Cares))
$(eval $(call pkg_config_find_and_add,zlib,ZLib))

# libraries for windows API
LDFLAGS += -lgnurx -lgdi32 -lcomctl32 -lws2_32 -lmsimg32 -lshlwapi -lcrypt32 -mwindows

CFLAGS += -U__STRICT_ANSI__ -mwin32
# only windows versions after XP are supported
# https://msdn.microsoft.com/en-gb/library/windows/desktop/aa383745
CFLAGS += '-DWINVER=0x0501'
CFLAGS += '-D_WIN32_WINNT=0x0501'
CFLAGS += '-D_WIN32_WINDOWS=0x0501'
CFLAGS += '-D_WIN32_IE=0x0501'

#installed resource path
CFLAGS += '-DNETSURF_WINDOWS_RESPATH="$(NETSURF_WINDOWS_RESPATH)"'

WSCFLAGS := -std=c99 -Dnswin32 -DCURL_STATICLIB -DCARES_STATICLIB -g

CFLAGS += $(WSCFLAGS)
LDFLAGS += $(WSCFLAGS)

# The filter and target for split messages
MESSAGES_FILTER=win
MESSAGES_TARGET=$(FRONTEND_RESOURCES_DIR)

# ----------------------------------------------------------------------------
# built-in resource setup
# ----------------------------------------------------------------------------

$(OBJROOT)/windows_resource.o: $(FRONTEND_RESOURCES_DIR)/resource.rc $(addsuffix /Messages,$(addprefix $(MESSAGES_TARGET)/,$(MESSAGES_LANGUAGES)))
	$(VQ)echo " WINDRES: compiling windows resources"
	${Q}$(WINDRES) $< -O coff -o $@

S_RESOURCES := windows_resource.o

# ----------------------------------------------------------------------------
# Source file setup
# ----------------------------------------------------------------------------

# sources purely for the windows build
S_FRONTEND := main.c window.c gui.c clipboard.c drawable.c plot.c findfile.c \
	  font.c bitmap.c about.c prefs.c download.c fetch.c file.c	\
	  local_history.c schedule.c windbg.c pointers.c \
	  corewindow.c hotlist.c cookies.c global_history.c

# This is the final source build list
# Note this is deliberately *not* expanded here as common and image
#   are not yet available
SOURCES = $(S_COMMON) $(S_IMAGE) $(S_BROWSER) $(S_FRONTEND) $(S_RESOURCES)
EXETARGET := NetSurf.exe

# ----------------------------------------------------------------------------
# Install target
# ----------------------------------------------------------------------------

install-windows:

# ----------------------------------------------------------------------------
# Package target
# ----------------------------------------------------------------------------

package-windows: netsurf-installer.exe

WIN_RES_OBJ := installer.nsi NetSurf.ico netsurf.png welcome.html default.css
WIN_RES_INS_OBJ := $(addprefix $(FRONTEND_RESOURCES_DIR)/,$(WIN_RES_OBJ)) $(OBJROOT)/messages-en

# deal with making installer generation verbose
ifeq ($(Q),)
NSIS_VERBOSE := 4
else
NSIS_VERBOSE := 0
endif

# installer messages generation
$(OBJROOT)/messages-en: $(MESSAGES_TARGET)/en/Messages
	$(VQ)echo "   MSGCP: $< $@"
	$(Q)$(INSTALL) -m 644 -T $< $@

netsurf-installer.exe: $(EXETARGET) $(POSTEXES) $(WIN_RES_INS_OBJ)
	$(VQ)echo "MAKENSIS: $@"
	$(Q)makensis -V$(NSIS_VERBOSE) -NOCD -DOBJROOT=$(OBJROOT) -DRESDIR=$(FRONTEND_RESOURCES_DIR) -DVERSIONMAJOR=$(VERSION_MAJ) -DVERSIONMINOR=$(VERSION_MIN) -DOUTFNAME=$@ $(FRONTEND_RESOURCES_DIR)/installer.nsi

clean-installer:
	$(VQ)echo "   CLEAN: netsurf-installer.exe"
	$(Q)$(RM) netsurf-installer.exe
CLEANS += clean-installer
