# Content fetchers sources

S_FETCHERS_YES := data.c resource.c
S_FETCHERS_NO :=
S_FETCHERS_$(NETSURF_USE_CURL) += curl.c

S_FETCHERS := $(addprefix fetchers/,$(S_FETCHERS_YES))

# about fetcher
include content/fetchers/about/Makefile
S_FETCHERS += $(addprefix fetchers/about/,$(S_FETCHER_ABOUT))

# file fetcher
include content/fetchers/file/Makefile
S_FETCHERS += $(addprefix fetchers/file/,$(S_FETCHER_FILE))
