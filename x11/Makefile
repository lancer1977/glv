#----------------------------------------------------------------------------
# Makefile for GNU make
#----------------------------------------------------------------------------


#------ Install settings

PREFIX ?= /usr/local

# DESTDIR used for staged installs.
ifeq ($(shell file -b /usr/lib64), directory)
LIB_DIR      = $(DESTDIR)$(PREFIX)/lib64
else
LIB_DIR      = $(DESTDIR)$(PREFIX)/lib
endif
HEADER_DIR   = $(DESTDIR)$(PREFIX)/include/GL


#------ Compiler and tools

CC       = gcc
# Using gcc instead of ld because it's broken on Ubuntu.
LINK     = gcc
LDCONFIG = /sbin/ldconfig
INSTALL  = install


#------ Target settings

ifdef STATIC_LIB
GLV_TARGET   = libglv.a
else
VER_M        = 0
VER_MM       = 0.5
GLV_TARGET   = libglv.so.$(VER_MM)
endif
GLV_CFLAGS   = -fPIC -Wall -Wextra
GLV_INCPATH  = -I.
GLV_LFLAGS   = -shared
GLV_LIBS     =
GLV_OBJECTS  = glv.o

GLV_CFLAGS  += -O3 -DNDEBUG
#GLV_CFLAGS  += -g -DDEBUG

GLV_CFLAGS  += -DUSE_CURSORS
GLV_LIBS    += -lXcursor

#GLV_CFLAGS  += -DUSE_XF86VMODE
#GLV_LIBS    += -lXxf86vm
# GLV_LIBS    += -L/usr/X11R6/lib
# GLV_LIBS    += -L/usr/X11R6/lib64

# GLV_CFLAGS += -DUSE_XME
# GLV_LIBS   += -L/usr/X11R6/lib -lXxme

# GLV_LIBS    += -lXext -lX11


#------ Build rules

.PHONY: all clean install uninstall

all: $(GLV_TARGET) es_profile window
 
$(GLV_TARGET): $(GLV_OBJECTS)
ifdef STATIC_LIB
	ar rc $@ $^
	ranlib $@
	#strip -d $@
else
	$(LINK) -o $@ -Wl,-soname,libglv.so.$(VER_M) $(GLV_LFLAGS) $(GLV_OBJECTS) $(GLV_LIBS)
	ln -sf libglv.so.$(VER_MM) libglv.so.$(VER_M)
	ln -sf libglv.so.$(VER_MM) libglv.so
endif
 
es_profile: ../examples/es_profile.c $(GLV_TARGET)
	$(CC) $< -o $@ -I. -L. -lglv -lGL -lXcursor -lX11

window: ../examples/window.c $(GLV_TARGET)
	$(CC) $< -o $@ -I. -L. -lglv -lGL -lXcursor -lX11

clean:
	-rm -f $(GLV_TARGET) $(GLV_OBJECTS)
	-rm -f es_profile window
ifndef STATIC_LIB
	-rm -f libglv.so.$(VER_M) libglv.so
endif

install:
	mkdir -p $(HEADER_DIR) $(LIB_DIR)
	$(INSTALL) -m 644 glv.h glv_keys.h $(HEADER_DIR)
ifdef STATIC_LIB
	$(INSTALL) -m 644 $(GLV_TARGET) $(LIB_DIR)
else
	$(INSTALL) -m 755 -s $(GLV_TARGET) $(LIB_DIR)
	ln -sf libglv.so.$(VER_MM) $(LIB_DIR)/libglv.so.$(VER_M)
	ln -sf libglv.so.$(VER_MM) $(LIB_DIR)/libglv.so
	$(LDCONFIG) -v -n $(LIB_DIR)
endif

uninstall:
	-rm -f $(LIB_DIR)/$(GLV_TARGET)
ifndef STATIC_LIB
	-rm -f $(LIB_DIR)/libglv.so
	-rm -f $(LIB_DIR)/libglv.so.$(VER_M)
endif
	-rm -f $(HEADER_DIR)/glv.h $(HEADER_DIR)/glv_keys.h


#------ Compile rules

glv.o: glv.c glv.h
	$(CC) -c $(GLV_CFLAGS) -o glv.o $(GLV_INCPATH) glv.c


#EOF
