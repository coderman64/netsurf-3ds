# Content sources

S_CONTENT :=			\
	content.c		\
	content_factory.c	\
	fetch.c			\
	hlcache.c		\
	llcache.c		\
	mimesniff.c		\
	textsearch.c		\
	urldb.c			\
	no_backing_store.c

# Make filesystem backing store available
ifeq ($(NETSURF_FS_BACKING_STORE),YES)
	S_CONTENT += fs_backing_store.c
endif


# Content fetcher sources
include content/fetchers/Makefile

S_FETCHERS := $(addprefix content/,$(S_FETCHERS))

# Content handler sources
include content/handlers/Makefile

S_CONTENT := $(addprefix content/,$(S_CONTENT))
