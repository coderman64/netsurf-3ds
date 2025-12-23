#
# Makefile for NetSurf
#
# Copyright 2007 Daniel Silverstone <dsilvers@netsurf-browser.org>
# Copyright 2008 Rob Kendrick <rjek@netsurf-browser.org>
#
# Trivially, invoke as:
#   make
# to build native, or:
#   make TARGET=riscos
# to cross-build for RO.
#
# Look at Makefile.config for configuration options.
#
# Best results obtained building on unix platforms cross compiling for others
#
# To clean, invoke as above, with the 'clean' target
#
# To build developer Doxygen generated documentation, invoke as above,
# with the 'docs' target:
#   make docs
#

.PHONY: all

all: all-program

# default values for base variables

# Resources executable target depends upon
RESOURCES=
# Messages executable target depends on
MESSAGES:=

# The filter applied to the fat (full) messages to generate split messages
MESSAGES_FILTER=any
# The languages in the fat messages to convert
MESSAGES_LANGUAGES=de en fr it nl zh_CN
# The target directory for the split messages
MESSAGES_TARGET=resources

# Defaults for tools
PERL=perl
MKDIR=mkdir
TOUCH=touch
STRIP?=strip
INSTALL?=install

# build verbosity
ifeq ($(V),1)
  Q:=
else
  Q=@
endif
VQ=@

# Override this only if the host compiler is called something different
BUILD_CC := $(DEVKITARM)/bin/arm-none-eabi-gcc 

MAKEROM := makerom
#cc
BUILD_CFLAGS = -g -W -Wall -Wundef -Wpointer-arith -Wcast-align \
	-Wwrite-strings -Wmissing-declarations -Wuninitialized \
	-Wno-unused-parameter  \
	-I$(DEVKITPRO)/libctru/include \
	-I$(DEVKITPRO)/portlibs/3ds/include \
	-I$(DEVKITARM)/arm-none-eabi/include \
	-I$(DEVKITARM)/lib/gcc/arm-none-eabi/13.2.0/include \
	-DARM11 -D_3DS \
	-D__3DS__ -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft \
	-g -Wall -O2 -mword-relocations
# -nostdinc -nostdinc++
# compute HOST, TARGET and SUBTARGET
include frontends/Makefile.hts

# target specific tool overrides
include frontends/$(TARGET)/Makefile.tools

# compiler versioning to adjust warning flags
CC_VERSION := $(shell $(CC) -dumpfullversion -dumpversion)
CC_MAJOR := $(word 1,$(subst ., ,$(CC_VERSION)))
CC_MINOR := $(word 2,$(subst ., ,$(CC_VERSION)))
define cc_ver_ge
$(shell expr $(CC_MAJOR) \> $(1) \| \( $(CC_MAJOR) = $(1) \& $(CC_MINOR) \>= $(2) \) )
endef

# CCACHE
# ifeq ($(origin CCACHE),undefined)
#   CCACHE=$(word 1,$(shell ccache -V 2>/dev/null))
# endif
# CC := $(CCACHE) $(CC)

# Target paths
OBJROOT = build/$(HOST)-$(TARGET)$(SUBTARGET)
DEPROOT := $(OBJROOT)/deps
TOOLROOT := $(OBJROOT)/tools

# keep C flags from environment
CFLAGS_ENV := $(CFLAGS)
CXXFLAGS_ENV := $(CXXFLAGS)

# library and feature building macros
include Makefile.macros

# ----------------------------------------------------------------------------
# General flag setup
# ----------------------------------------------------------------------------

# Set up the warning flags here so that they can be overridden in the
#   Makefile.config
COMMON_WARNFLAGS = -W -Wall -Wundef -Wpointer-arith -Wcast-align \
	-Wwrite-strings -Wmissing-declarations -Wuninitialized

ifneq ($(CC_MAJOR),2)
  COMMON_WARNFLAGS += -Wno-unused-parameter
endif

# deal with lots of unwanted warnings from javascript
ifeq ($(call cc_ver_ge,4,6),1)
  COMMON_WARNFLAGS += -Wno-unused-but-set-variable
endif

# Implicit fallthrough warnings suppressed by comment
ifeq ($(call cc_ver_ge,7,1),1)
  COMMON_WARNFLAGS += -Wimplicit-fallthrough=3
endif

# deal with chaging warning flags for different platforms
ifeq ($(HOST),OpenBSD)
  # OpenBSD headers are not compatible with redundant declaration warning
  COMMON_WARNFLAGS += -Wno-redundant-decls
else
  COMMON_WARNFLAGS += -Wredundant-decls
endif

# c++ default warning flags
CXXWARNFLAGS :=

# C default warning flags
CWARNFLAGS := -Wstrict-prototypes -Wmissing-prototypes -Wnested-externs

# Pull in the default configuration
include Makefile.defaults

# Pull in the user configuration
-include Makefile.config

# libraries enabled by feature switch without pkgconfig file 
$(eval $(call feature_switch,JPEG,JPEG (libjpeg),-DWITH_JPEG,-ljpeg,-UWITH_JPEG,))
$(eval $(call feature_switch,HARU_PDF,PDF export (haru),-DWITH_PDF_EXPORT,-lhpdf -lpng,-UWITH_PDF_EXPORT,))
$(eval $(call feature_switch,LIBICONV_PLUG,glibc internal iconv,-DLIBICONV_PLUG,,-ULIBICONV_PLUG,-liconv))
$(eval $(call feature_switch,DUKTAPE,Javascript (Duktape),,,,,))

# Common libraries with pkgconfig
$(eval $(call pkg_config_find_and_add,libcss,CSS))
$(eval $(call pkg_config_find_and_add,libdom,DOM))
$(eval $(call pkg_config_find_and_add,libnsutils,nsutils))

# Common libraries without pkg-config support
LDFLAGS += -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft \
	-L$(DEVKITPRO)/libctru/lib \
	-L$(DEVKITPRO)/portlibs/3ds \
	-L$(DEVKITPRO)/portlibs/3ds/lib \
	-lz -specs=3dsx.specs -lpthread -D_3DS -D__3DS__ -lcurl -lmbedtls -lmbedx509 -lz -lmbedcrypto
# -lSDL -lcitro3d -lctru -lm

# Optional libraries with pkgconfig

# define additional CFLAGS and LDFLAGS requirements for pkg-configed libs
# We only need to define the ones where the feature name doesn't exactly
# match the WITH_FEATURE flag
NETSURF_FEATURE_NSSVG_CFLAGS := -DWITH_NS_SVG
NETSURF_FEATURE_ROSPRITE_CFLAGS := -DWITH_NSSPRITE

# libcurl and openssl ordering matters as if libcurl requires ssl it
#  needs to come first in link order to ensure its symbols can be
#  resolved by the subsequent openssl

# freemint does not support pkg-config for libcurl
# ifeq ($(HOST),mint)
# CFLAGS += $(shell $(DEVKITPRO)/portlibs/3ds/bin/curl-config --cflags)
# LDFLAGS += $(shell $(DEVKITPRO)/portlibs/3ds/bin/curl-config --libs)

# $(eval $(call pkg_config_find_and_add,sdl,sdl))

# $(DEVKITPRO)/portlibs/3ds/bin/arm-none-eabi-pkg-config
#else
# $(eval $(call pkg_config_find_and_add_enabled,CURL,libcurl,Curl))
# endif
$(eval $(call pkg_config_find_and_add_enabled,OPENSSL,openssl,OpenSSL))

$(eval $(call pkg_config_find_and_add_enabled,UTF8PROC,libutf8proc,utf8))
$(eval $(call pkg_config_find_and_add_enabled,JPEGXL,libjxl,JPEGXL))
$(eval $(call pkg_config_find_and_add_enabled,WEBP,libwebp,WEBP))
$(eval $(call pkg_config_find_and_add_enabled,PNG,libpng,PNG))
$(eval $(call pkg_config_find_and_add_enabled,BMP,libnsbmp,BMP))
$(eval $(call pkg_config_find_and_add_enabled,GIF,libnsgif,GIF))
$(eval $(call pkg_config_find_and_add_enabled,NSSVG,libsvgtiny,SVG))
$(eval $(call pkg_config_find_and_add_enabled,ROSPRITE,librosprite,Sprite))
$(eval $(call pkg_config_find_and_add_enabled,NSPSL,libnspsl,PSL))
$(eval $(call pkg_config_find_and_add_enabled,NSLOG,libnslog,LOG))



# List of directories in which headers are searched for
INCLUDE_DIRS := $(DEVKITPRO)/libctru/include $(DEVKITPRO)/devkitARM/arm-none-eabi/include $(DEVKITPRO)/portlibs/3ds/include . include $(OBJROOT)
# export the user agent format
CFLAGS += -DNETSURF_UA_FORMAT_STRING=\"$(NETSURF_UA_FORMAT_STRING)\"
CXXFLAGS += -DNETSURF_UA_FORMAT_STRING=\"$(NETSURF_UA_FORMAT_STRING)\"

# set the default homepage to use
CFLAGS += -DNETSURF_HOMEPAGE=\"$(NETSURF_HOMEPAGE)\"
CXXFLAGS += -DNETSURF_HOMEPAGE=\"$(NETSURF_HOMEPAGE)\"

# set the logging level
CFLAGS += -DNETSURF_LOG_LEVEL=$(NETSURF_LOG_LEVEL)
CXXFLAGS += -DNETSURF_LOG_LEVEL=$(NETSURF_LOG_LEVEL)

# If we're building the sanitize goal, override things
ifneq ($(filter-out sanitize,$(MAKECMDGOALS)),$(MAKECMDGOALS))
override NETSURF_USE_SANITIZER := YES
override NETSURF_RECOVER_SANITIZERS := NO
endif

# If we're going to use the sanitizer set it up
ifeq ($(NETSURF_USE_SANITIZER),YES)
SAN_FLAGS := -fsanitize=address -fsanitize=undefined
ifeq ($(NETSURF_RECOVER_SANITIZERS),NO)
SAN_FLAGS += -fno-sanitize-recover
endif
else
SAN_FLAGS :=
endif
CFLAGS += $(SAN_FLAGS)
CXXFLAGS += $(SAN_FLAGS)
LDFLAGS += $(SAN_FLAGS)

# and the logging filter
CFLAGS += -DNETSURF_BUILTIN_LOG_FILTER=\"$(NETSURF_BUILTIN_LOG_FILTER)\"
CXXFLAGS += -DNETSURF_BUILTIN_LOG_FILTER=\"$(NETSURF_BUILTIN_LOG_FILTER)\"
# and the verbose logging filter
CFLAGS += -DNETSURF_BUILTIN_VERBOSE_FILTER=\"$(NETSURF_BUILTIN_VERBOSE_FILTER)\"
CXXFLAGS += -DNETSURF_BUILTIN_VERBOSE_FILTER=\"$(NETSURF_BUILTIN_VERBOSE_FILTER)\"

# Determine if the C compiler supports statement expressions
# This is needed to permit certain optimisations in our library headers
ifneq ($(shell $(BUILD_CC) -dM -E - < /dev/null | grep __GNUC__),)
CFLAGS += -DSTMTEXPR=1
CXXFLAGS += -DSTMTEXPR=1
endif

# We trace during link so that we can determine if a libary changes under us in
# order to re-link.  This *may* be gcc specific, so may need tweaks in future.
LDFLAGS += -Wl,--trace

# ----------------------------------------------------------------------------
# General make rules
# ----------------------------------------------------------------------------

$(OBJROOT)/created:
	$(VQ)echo "   MKDIR: $(OBJROOT)"
	$(Q)$(MKDIR) -p $(OBJROOT)
	$(Q)$(TOUCH) $(OBJROOT)/created

$(DEPROOT)/created: $(OBJROOT)/created
	$(VQ)echo "   MKDIR: $(DEPROOT)"
	$(Q)$(MKDIR) -p $(DEPROOT)
	$(Q)$(TOUCH) $(DEPROOT)/created

$(TOOLROOT)/created: $(OBJROOT)/created
	$(VQ)echo "   MKDIR: $(TOOLROOT)"
	$(Q)$(MKDIR) -p $(TOOLROOT)
	$(Q)$(TOUCH) $(TOOLROOT)/created

CLEANS :=
POSTEXES :=

# ----------------------------------------------------------------------------
# Target specific setup
# ----------------------------------------------------------------------------

include frontends/Makefile

# ----------------------------------------------------------------------------
# Build tools setup
# ----------------------------------------------------------------------------

include tools/Makefile

# ----------------------------------------------------------------------------
# General source file setup
# ----------------------------------------------------------------------------

# Content sources
include content/Makefile

# utility sources
include utils/Makefile

# http utility sources
include utils/http/Makefile

# nsurl utility sources
include utils/nsurl/Makefile

# Desktop sources
include desktop/Makefile

# S_COMMON are sources common to all builds
S_COMMON := \
	$(S_CONTENT) \
	$(S_FETCHERS) \
	$(S_UTILS) \
	$(S_HTTP) \
	$(S_NSURL) \
	$(S_DESKTOP) \
	$(S_JAVASCRIPT_BINDING)


# ----------------------------------------------------------------------------
# Message targets
# ----------------------------------------------------------------------------


# generate the message file rules
$(eval $(foreach LANG,$(MESSAGES_LANGUAGES), \
	$(call split_messages,$(LANG))))

clean-messages:
	$(VQ)echo "   CLEAN: $(CLEAN_MESSAGES)"
	$(Q)$(RM) $(CLEAN_MESSAGES)
CLEANS += clean-messages


# ----------------------------------------------------------------------------
# Source file setup
# ----------------------------------------------------------------------------

# Force exapnsion of source file list
SOURCES := $(SOURCES)

ifeq ($(SOURCES),)
$(error Unable to build NetSurf, could not determine set of sources to build)
endif

OBJECTS := $(sort $(addprefix $(OBJROOT)/,$(subst /,_,$(patsubst %.c,%.o,$(patsubst %.cpp,%.o,$(patsubst %.m,%.o,$(patsubst %.s,%.o,$(SOURCES))))))))

# Include directory flags
IFLAGS = $(addprefix -I,$(INCLUDE_DIRS))

$(EXETARGET): $(OBJECTS) $(RESOURCES) $(MESSAGES) tools/linktrace-to-depfile.pl
	$(VQ)echo "    LINK: $(EXETARGET)"
ifneq ($(TARGET),riscos)
	$(BUILD_CC) -o $(EXETARGET) $(OBJECTS) $(LDFLAGS) -D__3DS__ -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft > $(DEPROOT)/link-raw.d
else
	@# RISC OS targets are a bit special: we need to convert ELF -> AIF
  ifeq ($(SUBTARGET),-aof)
	$(Q)$(BUILD_CC) -o $(EXETARGET) $(OBJECTS) $(LDFLAGS) -D__3DS__ -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft > $(DEPROOT)/link-raw.d
  else
	$(Q)$(CXX) -o $(EXETARGET:,ff8=,e1f) $(OBJECTS) $(LDFLAGS) -D__3DS__ -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft > $(DEPROOT)/link-raw.d
	$(Q)$(ELF2AIF) $(EXETARGET:,ff8=,e1f) $(EXETARGET)
	$(Q)$(RM) $(EXETARGET:,ff8=,e1f)
  endif
endif
	$(VQ)echo "LINKDEPS: $(EXETARGET)"
	$(Q)echo -n "$(EXETARGET) $(DEPROOT)/link.d: " > $(DEPROOT)/link.d
	$(Q)$(PERL) tools/linktrace-to-depfile.pl < $(DEPROOT)/link-raw.d >> $(DEPROOT)/link.d
ifeq ($(NETSURF_STRIP_BINARY),YES)
	$(VQ)echo "   STRIP: $(EXETARGET)"
	$(Q)$(STRIP) $(EXETARGET)
endif
ifeq ($(TARGET),beos)
	$(VQ)echo "    XRES: $(EXETARGET)"
	$(Q)$(BEOS_XRES) -o $(EXETARGET) $(RSRC_BEOS)
	$(VQ)echo "  SETVER: $(EXETARGET)"
	$(Q)$(BEOS_SETVER) $(EXETARGET) \
                -app $(VERSION_MAJ) $(VERSION_MIN) 0 d 0 \
                -short "NetSurf $(VERSION_FULL)" \
                -long "NetSurf $(VERSION_FULL) © 2003 - 2021 The NetSurf Developers"
	$(VQ)echo " MIMESET: $(EXETARGET)"
	$(Q)$(BEOS_MIMESET) $(EXETARGET)
endif
	$(VQ)echo "  MAKE 3DSX: $(EXETARGET).3dsx"
	$(Q)3dsxtool $(EXETARGET) $(EXETARGET).3dsx --smdh=nsfb.smdh --romfs=resources
ifndef NOCIA
	$(VQ)echo "  MAKE CIA: $(EXETARGET).cia"
	$(Q)$(MAKEROM) -f cia -elf $(EXETARGET) -o $(EXETARGET).cia -rsf ./nsfb.rsf -icon nsfb.smdh -banner nsfb.bnr
	$(VQ)echo "  MAKE HIMEM CIA: $(EXETARGET)_himem.cia"
	$(Q)$(MAKEROM) -f cia -elf $(EXETARGET) -o $(EXETARGET)_himem.cia -rsf ./nsfb_himem.rsf -icon nsfb_himem.smdh -banner nsfb_himem.bnr
else
	$(VQ)echo "  SKIPPING CIA BUILD"
endif

clean-target:
	$(VQ)echo "   CLEAN: $(EXETARGET)"
	$(Q)$(RM) $(EXETARGET)
	$(Q)$(RM) $(EXETARGET).3dsx
	$(Q)$(RM) $(EXETARGET).cia
	$(Q)$(RM) $(EXETARGET)_himem.cia
CLEANS += clean-target


clean-builddir:
	$(VQ)echo "   CLEAN: $(OBJROOT)"
	$(Q)$(RM) -r $(OBJROOT)
CLEANS += clean-builddir


.PHONY: all-program

all-program: $(EXETARGET) $(POSTEXES)

.SUFFIXES:

DEPFILES :=

# Rules to construct dep lines for each object...
$(eval $(foreach SOURCE,$(filter %.c,$(SOURCES)), \
	$(call dependency_generate_c,$(SOURCE),$(subst /,_,$(SOURCE:.c=.d)),$(subst /,_,$(SOURCE:.c=.o)))))

$(eval $(foreach SOURCE,$(filter %.cpp,$(SOURCES)), \
	$(call dependency_generate_c,$(SOURCE),$(subst /,_,$(SOURCE:.cpp=.d)),$(subst /,_,$(SOURCE:.cpp=.o)))))

$(eval $(foreach SOURCE,$(filter %.m,$(SOURCES)), \
	$(call dependency_generate_c,$(SOURCE),$(subst /,_,$(SOURCE:.m=.d)),$(subst /,_,$(SOURCE:.m=.o)))))

# Cannot currently generate dep files for S files because they're objasm
# when we move to gas format, we will be able to.

#$(eval $(foreach SOURCE,$(filter %.s,$(SOURCES)), \
#	$(call dependency_generate_s,$(SOURCE),$(subst /,_,$(SOURCE:.s=.d)),$(subst /,_,$(SOURCE:.s=.o)))))

ifeq ($(filter $(MAKECMDGOALS),clean test coverage),)
-include $(sort $(addprefix $(DEPROOT)/,$(DEPFILES)))
-include $(DEPROOT)/link.d
endif

# And rules to build the objects themselves...

$(eval $(foreach SOURCE,$(filter %.c,$(SOURCES)), \
	$(call compile_target_c,$(SOURCE),$(subst /,_,$(SOURCE:.c=.o)),$(subst /,_,$(SOURCE:.c=.d)))))

$(eval $(foreach SOURCE,$(filter %.cpp,$(SOURCES)), \
	$(call compile_target_cpp,$(SOURCE),$(subst /,_,$(SOURCE:.cpp=.o)),$(subst /,_,$(SOURCE:.cpp=.d)))))

$(eval $(foreach SOURCE,$(filter %.m,$(SOURCES)), \
	$(call compile_target_c,$(SOURCE),$(subst /,_,$(SOURCE:.m=.o)),$(subst /,_,$(SOURCE:.m=.d)))))

$(eval $(foreach SOURCE,$(filter %.s,$(SOURCES)), \
	$(call compile_target_s,$(SOURCE),$(subst /,_,$(SOURCE:.s=.o)),$(subst /,_,$(SOURCE:.s=.d)))))

# ----------------------------------------------------------------------------
# Test setup
# ----------------------------------------------------------------------------

include test/Makefile


# ----------------------------------------------------------------------------
# Clean setup
# ----------------------------------------------------------------------------

.PHONY: clean

clean: $(CLEANS)


# ----------------------------------------------------------------------------
# build distribution package
# ----------------------------------------------------------------------------

.PHONY: package-$(TARGET) package

package: all-program package-$(TARGET)


# ----------------------------------------------------------------------------
# local install on the host system
# ----------------------------------------------------------------------------

.PHONY: install install-$(TARGET)

install: all-program install-$(TARGET)


# ----------------------------------------------------------------------------
# Documentation build
# ----------------------------------------------------------------------------

.PHONY: docs

docs: docs/Doxyfile
	doxygen $<


# ----------------------------------------------------------------------------
# Transifex message processing
# ----------------------------------------------------------------------------

.PHONY: messages-split-tfx messages-fetch-tfx messages-import-tfx

# split fat messages into properties files suitable for uploading to transifex
messages-split-tfx:
	for splitlang in $(FAT_LANGUAGES);do perl ./utils/split-messages.pl -l $${splitlang} -f transifex -p any -o Messages.any.$${splitlang}.tfx resources/FatMessages;done

# download property files from transifex
messages-fetch-tfx:
	for splitlang in $(FAT_LANGUAGES);do $(RM) Messages.any.$${splitlang}.tfx ; perl ./utils/fetch-transifex.pl -w insecure -l $${splitlang} -o Messages.any.$${splitlang}.tfx ;done

# merge property files into fat messages
messages-import-tfx: messages-fetch-tfx
	for tfxlang in $(FAT_LANGUAGES);do perl ./utils/import-messages.pl -l $${tfxlang} -p any -f transifex -o resources/FatMessages -i resources/FatMessages -I Messages.any.$${tfxlang}.tfx ; $(RM) Messages.any.$${tfxlang}.tfx; done

