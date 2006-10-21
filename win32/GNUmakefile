#----------------------------------------------------------------------------
# Makefile for Cygwin
# Project: glv
#----------------------------------------------------------------------------


#------ Compiler and tools

CC       = gcc
LINK	 = ld


#------ Target settings

GLV_CFLAGS   = -Wall -W -g -mwindows -mno-cygwin
GLV_INCPATH  = -I.
GLV_LFLAGS   =
GLV_OBJECTS  = 	glv.o


#------ Build rules

all: libglv.a
 
libglv.a: $(GLV_OBJECTS)
	ar rc $@ $(GLV_OBJECTS)
	ranlib $@

#	$(LINK) -o $@ $(GLV_OBJECTS)


.PHONY: clean
clean:
	@rm libglv.a $(GLV_OBJECTS)


#------ Compile rules

glv.o: glv.c
	$(CC) -c $(GLV_CFLAGS) -o glv.o $(GLV_INCPATH) glv.c


#EOF
