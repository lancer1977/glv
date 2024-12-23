# GNU Makefile


ARCHIVE = libglv-0.5.0

DIST_FILES = \
	LICENSE \
	README.md \
	ChangeLog \
	dist/glv.spec \
	android/gesture.c \
	android/gesture.h \
	android/glv_activity.c \
	android/glv_activity.h \
	android/glv_asset.c \
	android/glv_asset.h \
	android/glv.c \
	android/glv.h \
	android/glv_keys.h \
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
	examples/project.b \
	examples/Makefile.linux \
	examples/keystr.h \
	examples/complete.c \
	examples/doc.c \
	examples/es_profile.c \
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


#EOF
