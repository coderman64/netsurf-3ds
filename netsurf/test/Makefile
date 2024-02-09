#
# NetSurf unit tests

TESTS := \
	nsurl \
	urldbtest \
	nsoption \
	bloom \
	hashtable \
	hashmap \
	urlescape \
	utils \
	messages \
	time \
	mimesniff \
	corestrings #llcache

# sources necessary to use nsurl functionality
NSURL_SOURCES := utils/nsurl/nsurl.c utils/nsurl/parse.c utils/idna.c \
	utils/punycode.c

# nsurl test sources
nsurl_SRCS := $(NSURL_SOURCES) utils/corestrings.c test/log.c test/nsurl.c

# url database test sources
urldbtest_SRCS := $(NSURL_SOURCES) \
	utils/bloom.c utils/nsoption.c utils/corestrings.c utils/time.c	\
	utils/hashtable.c utils/messages.c utils/utils.c \
	utils/http/primitives.c utils/http/generics.c \
	utils/http/strict-transport-security.c \
	content/urldb.c \
	test/log.c test/urldbtest.c

# low level cache test sources
llcache_SRCS := content/fetch.c content/fetchers/curl.c \
	content/fetchers/about.c content/fetchers/data.c \
	content/fetchers/resource.c content/llcache.c \
	content/urldb.c \
	image/image_cache.c \
	$(NSURL_SOURCES) utils/base64.c utils/corestrings.c utils/hashtable.c \
	utils/messages.c utils/url.c utils/useragent.c utils/utils.c \
	test/log.c test/llcache.c

# messages test sources
messages_SRCS := utils/messages.c utils/hashtable.c test/log.c test/messages.c

# nsoption test sources
nsoption_SRCS := utils/nsoption.c test/log.c test/nsoption.c

# Bloom filter test sources
bloom_SRCS := utils/bloom.c test/bloom.c

# hash table test sources
hashtable_SRCS := utils/hashtable.c test/log.c test/hashtable.c

# hashmap test sources
hashmap_SRCS := $(NSURL_SOURCES) utils/hashmap.c utils/corestrings.c test/log.c test/hashmap.c
hashmap_LD := -lmalloc_fig

# url escape test sources
urlescape_SRCS := utils/url.c test/log.c test/urlescape.c

# utility test sources
utils_SRCS := $(NSURL_SOURCES) utils/utils.c utils/messages.c \
	utils/hashtable.c utils/corestrings.c  \
	test/log.c test/utils.c

# time test sources
time_SRCS := utils/time.c test/log.c test/time.c

# mimesniff test sources
mimesniff_SRCS := $(NSURL_SOURCES) utils/hashtable.c utils/corestrings.c \
	utils/http/generics.c utils/http/content-type.c \
	utils/http/primitives.c utils/messages.c utils/http/parameter.c \
	content/mimesniff.c \
	test/log.c test/mimesniff.c

# corestrings test sources
corestrings_SRCS := $(NSURL_SOURCES) utils/corestrings.c \
	test/log.c test/corestrings.c
corestrings_LD := -lmalloc_fig


# Coverage builds need additional flags
COV_ROOT := build/$(HOST)-coverage
ifeq ($(MAKECMDGOALS),coverage)
  COV_CFLAGS ?= -fprofile-arcs -ftest-coverage -O0
  COV_CXXFLAGS ?= -fprofile-arcs -ftest-coverage -O0
  COV_LDFLAGS ?= -lgcov -fprofile-arcs
  TESTROOT := $(COV_ROOT)
  ifeq ($(NOASSERTCOVERAGE),yes)
    NOCOV_TESTSOURCES ?=
    COV_CPPFLAGS ?=
  else
    NOCOV_TESTSOURCES ?= test/assert.c
    COV_CPPFLAGS ?= -D__assert_fail=__ns_assert_fail
  endif
else
  COV_CFLAGS ?= -O0
  COV_CXXFLAGS ?= -O0
  COV_CPPFLAGS ?=
  TESTROOT := build/$(HOST)-test
  NOCOV_TESTSOURCES ?=
endif


# Extend flags with appropriate values from pkg-config for enabled features
#
# 1: pkg-config required modules for feature
# 2: Human-readable name for the feature
define pkg_cfg_detect_lib
  ifeq ($$(PKG_CONFIG),)
    $$(error pkg-config is required to auto-detect feature availability)
  endif

  PKG_CONFIG_$(1)_EXISTS := $$(shell $$(PKG_CONFIG) --exists $(1) && echo yes)

  ifeq ($$(PKG_CONFIG_$(1)_EXISTS),yes)
      LIB_CFLAGS += $$(shell $$(PKG_CONFIG) --cflags $(1))
      LIB_CXXFLAGS += $$(shell $$(PKG_CONFIG) --cflags $(1))
      LIB_LDFLAGS += $$(shell $$(PKG_CONFIG) --libs $(1))
      ifneq ($(MAKECMDGOALS),clean)
        $$(info PKG.CNFG: $(2) ($(1))	enabled)
      endif
  else
    ifneq ($(MAKECMDGOALS),clean)
      $$(info PKG.CNFG: $(2) ($(1))	disabled)
    endif
  endif
endef


$(eval $(call pkg_cfg_detect_lib,check,Check))

TEST_WARNFLAGS = -W -Wall -Wundef -Wpointer-arith -Wcast-align \
	-Wwrite-strings -Wmissing-declarations -Wuninitialized

ifneq ($(CC_MAJOR),2)
  TEST_WARNFLAGS += -Wno-unused-parameter
endif

BASE_TESTCFLAGS := -std=c99 -g \
	$(TEST_WARNFLAGS) \
	-D_DEFAULT_SOURCE \
	-D_POSIX_C_SOURCE=200809L \
	-D_XOPEN_SOURCE=600 \
	-Itest -Iinclude -Icontent/handlers -Ifrontends -I. -I.. \
	-Dnsgtk \
	-DNETSURF_BUILTIN_LOG_FILTER=\"level:WARNING\" \
	-DNETSURF_BUILTIN_VERBOSE_FILTER=\"level:DEBUG\" \
	-DTESTROOT=\"$(TESTROOT)\" \
	-DWITH_UTF8PROC \
	$(SAN_FLAGS) \
	$(shell pkg-config --cflags libcurl libparserutils libwapcaplet libdom libnsutils libutf8proc) \
	$(LIB_CFLAGS)
TESTCFLAGS := $(BASE_TESTCFLAGS) \
	$(COV_CFLAGS) \
	$(COV_CPPFLAGS)

TESTLDFLAGS := -L$(TESTROOT) \
	$(shell pkg-config --libs libcurl libparserutils libwapcaplet libdom libnsutils libutf8proc) -lz \
	$(SAN_FLAGS) \
	$(LIB_LDFLAGS)\
	$(COV_LDFLAGS)

# malloc faliure injection generator
$(TESTROOT)/libmalloc_fig.so:test/malloc_fig.c
	$(CC) -shared -fPIC -I. -std=c99 -D__3DS__ -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft $(TEST_WARNFLAGS) $^ -ldl -o $@

# Source files for all tests being compiled
TESTSOURCES :=

GCOV ?= gcov

define gen_test_target
$$(TESTROOT)/$(1): $$(sort $$(addprefix $$(TESTROOT)/,$$(subst /,_,$$(patsubst %.c,%.o,$$(patsubst %.cpp,%.o,$$(patsubst %.m,%.o,$$(patsubst %.s,%.o,$$($(1)_SRCS) $$(NOCOV_TESTSOURCES))))))))
	$$(VQ)echo "LINKTEST: $$@"
	$$(Q)$$(CC) $$(TESTCFLAGS) -D__3DS__ -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft $$^ -o $$@ $$($(1)_LD) $$(TESTLDFLAGS)

.PHONY:$(1)_test

$(1)_test:$$(TESTROOT)/$(1)
	$$(VQ)echo "RUN TEST: $(1)"
	$$(Q)LD_LIBRARY_PATH=$$(TESTROOT)/ $$(TESTROOT)/$(1)

TESTSOURCES += $$($(1)_SRCS)

endef

define compile_test_target_c
$$(TESTROOT)/$(2): $(1) $$(TESTROOT)/created
	$$(VQ)echo " COMPILE: $(1)"
	$$(Q)$$(RM) $$(TESTROOT)/$(2)
	$$(Q)$$(CC) $$(TESTCFLAGS) -D__3DS__ -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft -o $$(TESTROOT)/$(2) -c $(1)

endef

define compile_test_nocov_target_c
$$(TESTROOT)/$(2): $(1) $$(TESTROOT)/created
	$$(VQ)echo " COMPILE: $(1) (No coverage)"
	$$(Q)$$(RM) $$(TESTROOT)/$(2)
	$$(Q)$$(CC) $$(BASE_TESTCFLAGS) -D__3DS__ -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft -o $$(TESTROOT)/$(2) -c $(1)

endef

# Generate target for each test program and the list of objects it needs
$(eval $(foreach TST,$(TESTS), $(call gen_test_target,$(TST))))

# generate target rules for test objects
$(eval $(foreach SOURCE,$(sort $(filter %.c,$(TESTSOURCES))), \
	$(call compile_test_target_c,$(SOURCE),$(subst /,_,$(SOURCE:.c=.o)),$(subst /,_,$(SOURCE:.c=.d)))))
$(eval $(foreach SOURCE,$(sort $(filter %.c,$(NOCOV_TESTSOURCES))), \
	$(call compile_test_nocov_target_c,$(SOURCE),$(subst /,_,$(SOURCE:.c=.o)),$(subst /,_,$(SOURCE:.c=.d)))))


.PHONY:test coverage sanitize

test: $(TESTROOT)/created $(TESTROOT)/libmalloc_fig.so $(addsuffix _test,$(TESTS))

coverage: test
sanitize: test

$(TESTROOT)/created:
	$(VQ)echo "   MKDIR: $(TESTROOT)"
	$(Q)$(MKDIR) -p $(TESTROOT)
	$(Q)$(TOUCH) $@

.PHONY: test-clean

test-clean:
	$(VQ)echo "   CLEAN: $(TESTROOT)"
	$(VQ)echo "   CLEAN: $(COV_ROOT)"
	$(Q)$(RM) -r $(TESTROOT) $(COV_ROOT)
CLEANS += test-clean
