# GNU Makefile


ARCHIVE = glv-0.2

DIST_FILES = \
	LICENSE \
	README \
	ChangeLog \
	unix/glv.c \
	unix/glv.h \
	unix/glv_keys.h \
	unix/Makefile \
	win32/glv.c \
	win32/glv.h \
	win32/glv_keys.h \
	win32/Makefile \
	win32/GNUmakefile \
	examples/project.r \
	examples/Makefile \
	examples/Makefile.vc \
	examples/Makefile.cygwin \
	examples/keystr.h \
	examples/complete.c \
	examples/doc.c \
	examples/window.c \
	doc/html


all:
	@echo "Targets: linux, mac, dist"


linux:
	make -C x11


mac:
	make -C mac


.PHONY: dist
dist:
	doxygen
	tar -cf $(ARCHIVE).tar $(DIST_FILES)
	mkdir /tmp/$(ARCHIVE)
	tar -C /tmp/$(ARCHIVE) -xf $(ARCHIVE).tar
	tar -C /tmp -cf $(ARCHIVE).tar $(ARCHIVE)
	rm -rf /tmp/$(ARCHIVE)
	gzip -9f $(ARCHIVE).tar


#EOF
