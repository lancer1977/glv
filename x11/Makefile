#----------------------------------------------------------------------------
# Makefile for GNU make
#----------------------------------------------------------------------------


#------ Compiler and tools

CC       = gcc
LINK	 = ld
LDCONFIG = /sbin/ldconfig
INSTALL  = install


#------ Target settings

VER_M        = 0
VER_MM       = 0.3
GLV_TARGET   = libglv.so.$(VER_MM)
GLV_CFLAGS   = -fPIC -Wall -W -Wno-unused-parameter -g
GLV_INCPATH  = -I.
GLV_LFLAGS   = -shared
GLV_LIBS     =
GLV_OBJECTS  = 	glv.o
INSTALL_DIR  = /usr/local/lib
HEADER_DIR   = /usr/local/include/GL


GLV_CFLAGS  += -DUSE_XF86VMODE
GLV_LIBS    += -L/usr/X11R6/lib -lXxf86vm
#GLV_LIBS    += -L/usr/X11R6/lib64 -lXxf86vm

#GLV_CFLAGS += -DUSE_XME
#GLV_LIBS   += -L/usr/X11R6/lib -lXxme

# GLV_LIBS    += -lXext -lX11


#------ Build rules

all: $(GLV_TARGET)
 
$(GLV_TARGET): $(GLV_OBJECTS)
	$(LINK) -o $@ -soname libglv.so $(GLV_LFLAGS) $(GLV_OBJECTS) $(GLV_LIBS)
	ln -sf $(GLV_TARGET) libglv.so
 
.PHONY: clean
clean:
	-rm -f $(GLV_TARGET) $(GLV_OBJECTS)

.PHONY: install
install:
	mkdir -p $(HEADER_DIR)
	$(INSTALL) glv.h glv_keys.h $(HEADER_DIR)
	$(INSTALL) $(GLV_TARGET) $(INSTALL_DIR)
	ln -sf libglv.so.$(VER_MM) $(INSTALL_DIR)/libglv.so
	$(LDCONFIG) -v -n $(INSTALL_DIR)

.PHONY: uninstall
uninstall:
	-rm -f $(INSTALL_DIR)/libglv.so
	-rm -f $(INSTALL_DIR)/libglv.so.$(VER_M)
	-rm -f $(INSTALL_DIR)/libglv.so.$(VER_MM)
	-rm -f $(HEADER_DIR)/glv.h $(HEADER_DIR)/glv_keys.h


#------ Compile rules

glv.o: glv.c glv.h
	$(CC) -c $(GLV_CFLAGS) -o glv.o $(GLV_INCPATH) glv.c


#EOF
