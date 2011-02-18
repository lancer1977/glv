# GNU Makefile


ARCHIVE = libglv-0.3.1

DIST_FILES = \
	LICENSE \
	README \
	ChangeLog \
	glv.spec \
	x11/glv.c \
	x11/glv.h \
	x11/glv_keys.h \
	x11/Makefile \
	mac/glv.c \
	mac/glv.h \
	mac/glv_keys.h \
	mac/Makefile \
	win32/glv.c \
	win32/glv.h \
	win32/glv_keys.h \
	win32/Makefile \
	win32/GNUmakefile \
	examples/project.r \
	examples/Makefile \
	examples/keystr.h \
	examples/complete.c \
	examples/doc.c \
	examples/window.c \
	doc/html

#	examples/Makefile.vc \
#	examples/Makefile.cygwin \


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
	mv $(ARCHIVE).tar.gz $(ARCHIVE).tgz


#EOF
