#!/bin/make
#
# Create the WebIDL for core DOM and HTML spec directly from
# downloaded specifications. The resulting IDL is pretty close but
# should be manually cleaned up and tagged with the retrival date and
# source URL
#
# The dom spec has an index on the end that contains all the IDL a
# second time! be aware htis needs removing or your IDL will have two
# copies of all interfaces which is bad.
#
# needs debian packages: curl w3m tidy html-xml-utils
#
# Copyright 2012 Vincent Sanders 
# MIT licenced

.PHONY:all clean

all: dom.idl html.idl uievents.idl cssom.idl

.INTERMEDIATE:dom-spec.html dom-spec.xml dom-idl.html
.INTERMEDIATE:html-spec.html html-spec.xml html-idl.html
.INTERMEDIATE:uievents-spec.html uievents-spec.xml uievents-idl.html
.INTERMEDIATE:urlutils-spec.html urlutils-spec.xml urlutils-idl.html
.INTERMEDIATE:cssom-spec.html cssom-spec.xml cssom-idl.html

dom-spec.html:
	curl -s https://dom.spec.whatwg.org/ -o $@

html-spec.html:
	curl -s https://html.spec.whatwg.org/ -o $@

uievents-spec.html:
	curl -s http://www.w3.org/TR/uievents/ -o $@

urlutils-spec.html:
	curl -s https://url.spec.whatwg.org/ -o $@

cssom-spec.html:
	curl -s https://drafts.csswg.org/cssom-1/ -o $@

%-spec.xml: %-spec.html
	-tidy -q -f $@.errors --new-blocklevel-tags header,hgroup,figure,time,main,nav,svg,rect,text,image,mark,figcaption,section,g,path,circle -o $@ -asxml $<

%-idl.html: %-spec.xml
	hxselect 'pre[class="idl"]' < $< > $@

%.idl: %-idl.html
	@echo "// Retrived from " >$@
	@echo "// $$(date)" >>$@
	@echo "" >>$@
	@echo "" >>$@
	cat $< | w3m -dump -T text/html >>$@


clean:
	${RM} dom.idl html.idl uievents.idl dom-spec.html dom-spec.xml dom-idl.html html-spec.html html-spec.xml html-idl.html uievents-spec.html uievents-spec.xml uievents-idl.html uievents-spec.xml.errors
