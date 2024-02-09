# Tools

# testament rules

clean-testament:
	$(VQ)echo "   CLEAN: testament.h"
	$(Q)$(RM) $(OBJROOT)/testament.h
CLEANS += clean-testament

.PHONY: testament

testament $(OBJROOT)/testament.h:
	$(Q)$(PERL) tools/git-testament.pl $(CURDIR) $(OBJROOT)/testament.h


# lib png build compiler flags
ifeq ($(HOST),OpenBSD)
    BUILD_LIBPNG_CFLAGS += $(shell $(PKG_CONFIG) --cflags libpng)
    BUILD_LIBPNG_LDFLAGS += $(shell $(PKG_CONFIG) --libs libpng)
else
  ifeq ($(HOST),FreeBSD)
      BUILD_LIBPNG_CFLAGS += $(shell $(PKG_CONFIG) --cflags libpng)
      BUILD_LIBPNG_LDFLAGS += $(shell $(PKG_CONFIG) --libs libpng)
  else
      BUILD_LIBPNG_CFLAGS +=
      BUILD_LIBPNG_LDFLAGS += -lpng
  endif
endif


# Build tool to convert file to comiled data
#
$(TOOLROOT)/xxd: tools/xxd.c $(TOOLROOT)/created
	$(VQ)echo "BUILD CC: $@"
	$(Q)cp ../tools/xxd $(TOOLROOT)/xxd
# $(Q)$(BUILD_CC) $(BUILD_CFLAGS) -o $@ $<
# $(BUILD_LDFLAGS)


# Build tool to filter messages
#
$(TOOLROOT)/split-messages: tools/split-messages.c $(TOOLROOT)/created
	$(VQ)echo "BUILD CC: $@"
	$(Q)cp ../tools/split-messages $(TOOLROOT)/split-messages
#$(BUILD_CC) $(BUILD_CFLAGS) -I. -o $@ $<  -L$(DEVKITPRO)/libctru/lib -L$(DEVKITPRO)/portlibs/3ds/lib -lz
# $(BUILD_LDFLAGS)


# Build tool to convert image bitmaps to source code.
#
$(TOOLROOT)/convert_image: tools/convert_image.c $(TOOLROOT)/created
	$(VQ)echo "BUILD CC: $@"
	$(Q)cp ../tools/convert_image $(TOOLROOT)/convert_image
#$(BUILD_CC) $(BUILD_CFLAGS) $(BUILD_LIBPNG_CFLAGS) -o $@ $<  -L$(DEVKITPRO)/libctru/lib -L$(DEVKITPRO)/portlibs/3ds/lib $(BUILD_LIBPNG_LDFLAGS)
# $(BUILD_LDFLAGS)

# Build too to perform font conversion
$(TOOLROOT)/convert_font: tools/convert_font.c $(TOOLROOT)/created
	$(VQ)echo "BUILD CC: $@"
	$(Q)cp ../tools/convert_font $(TOOLROOT)/convert_font
#$(BUILD_CC) $(BUILD_CFLAGS) -o $@ $< -L$(DEVKITPRO)/libctru/lib -L$(DEVKITPRO)/portlibs/3ds/lib

# idna
#
IDNA_UNICODE_MAJOR=11

tools/DerivedJoiningType.txt:
	curl -o $@ "https://www.unicode.org/Public/$(IDNA_UNICODE_MAJOR).0.0/ucd/extracted/DerivedJoiningType.txt"

tools/IdnaMappingTable.txt:
	curl -o $@ "https://www.unicode.org/Public/idna/$(IDNA_UNICODE_MAJOR).0.0/IdnaMappingTable.txt"

tools/idna-tables-properties.csv:
	curl -o $@ "https://www.iana.org/assignments/idna-tables-$(IDNA_UNICODE_MAJOR).0.0/idna-tables-properties.csv"

# the idna props header must be explicitly rebuilt
ifneq ($(filter $(MAKECMDGOALS),utils/idna_props.h),)
utils/idna_props.h: tools/DerivedJoiningType.txt tools/idna-tables-properties.csv
	$(VQ)echo "    IDNA: $@"
	$(Q)$(PERL) tools/idna-derived-props-gen.pl -o $@ -p tools/idna-tables-properties.csv -j tools/DerivedJoiningType.txt
endif
