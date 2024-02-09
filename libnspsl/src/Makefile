DIR_SOURCES := nspsl.c

src/nspsl.c:src/psl.inc

src/psl.inc:public_suffix_list.dat src/genpubsuffix.pl
	perl src/genpubsuffix.pl $< > $@

include $(NSBUILD)/Makefile.subdir
