#----------------------------------------------------------------------------
# Makefile for NMAKE
# Project: glv
#----------------------------------------------------------------------------


#------ Compiler and tools

CC       = cl.exe
LINK	 = link.exe


#------ Target settings

GLV_CFLAGS   = /nologo -W3 -Zi 
GLV_INCPATH  = /I.
GLV_LFLAGS   = /nologo /incremental:no /subsystem:windows /DEBUG
GLV_OBJECTS  = 	glv.o


#------ Build rules

all: glv.lib
 
glv.lib: $(GLV_OBJECTS)
	lib /nologo /out:$@ $(GLV_OBJECTS)


.PHONY: clean
clean:
	-@del glv.lib $(GLV_OBJECTS)


#------ Compile rules

glv.o: glv.c
	$(CC) /c $(GLV_CFLAGS) /Foglv.o $(GLV_INCPATH) glv.c


#EOF
