#----------------------------------------------------------------------------
# Mac OS X Makefile for GNU make
# $Id: Makefile,v 1.3 2004/11/13 22:43:38 karl Exp $
#----------------------------------------------------------------------------


#------ Compiler and tools

CC       = gcc
#LINK	 = ld
#LINK	 = cc
LINK	 = libtool
LDCONFIG = /sbin/ldconfig
INSTALL  = install


#------ Target settings

FW_LIB       = /System/Library/Frameworks
GLV_TARGET   = libglv.dylib
GLV_CFLAGS   = -fPIC -Wall -W -Wno-unused-parameter -g
GLV_INCPATH  = -I.
GLV_LFLAGS   =
#GLV_LIBS     = $(FW_LIB)/OpenGL.framework/Libraries/libGL.dylib
#GLV_LIBS     = -bundle -framework OpenGL
GLV_LIBS     = -framework OpenGL
GLV_LIBS    += -framework AGL
GLV_LIBS    += -framework Carbon
GLV_LIBS    += -lSystem
GLV_OBJECTS  = 	glv.o
INSTALL_DIR  = /usr/local/lib
HEADER_DIR   = /usr/local/include/GL


#------ Build rules

all: $(GLV_TARGET)
 
$(GLV_TARGET): $(GLV_OBJECTS)
	$(LINK) -dynamic -o $@ $(GLV_LFLAGS) $(GLV_OBJECTS) $(GLV_LIBS)

.PHONY: clean
clean:
	-rm -f $(GLV_TARGET) $(GLV_OBJECTS)

.PHONY: install
install:
	mkdir -p $(HEADER_DIR)
	$(INSTALL) glv.h glv_keys.h $(HEADER_DIR)
	$(INSTALL) $(GLV_TARGET) $(INSTALL_DIR)

.PHONY: uninstall
uninstall:
	-rm -f $(INSTALL_DIR)/$(GLV_TARGET)
	-rm -f $(HEADER_DIR)/glv.h $(HEADER_DIR)/glv_keys.h


#------ Compile rules

glv.o: glv.c glv.h
	$(CC) -c $(GLV_CFLAGS) -o glv.o $(GLV_INCPATH) glv.c


#EOF
