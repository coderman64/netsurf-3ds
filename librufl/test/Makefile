# Tests
ifeq ($(findstring -riscos,$(HOST)),-riscos)
  DIR_TEST_ITEMS := $(DIR_TEST_ITEMS) \
	  rufl_test:rufl_test.c \
	  rufl_chars:rufl_chars.c
  # We do not want to run tests if building for RISC OS
  TESTRUNNER := echo
else
  DIR_TEST_ITEMS := nofonts:nofonts.c;harness.c;mocks.c \
	oldfminit:oldfminit.c;harness.c;mocks.c \
	olducsinit:olducsinit.c;harness.c;mocks.c \
	ucsinit:ucsinit.c;harness.c;mocks.c \
	manyfonts:manyfonts.c;harness.c;mocks.c
endif

include $(NSBUILD)/Makefile.subdir
