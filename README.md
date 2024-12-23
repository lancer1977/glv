GLV Window Library
==================

The GLV library provides a small, cross-platform, C interface for creating a
window or fullscreen display with an OpenGL context.  It was written for use
in games and demos as an alternative to larger libraries such as GLUT, SDL,
GLFW, PLIB, etc.  There are no thread, joystick, or timer wrappers here.

The library is made available under the MIT License and the implementation
for Linux & Windows is a single source file to facilitate copying directly
into your own projects.

The currently supported systems are Android, Linux (X11), and Windows.
The Linux version is the most complete and gets the most testing.
The Mac version is obsolete as it has not been updated for Cocoa.

The small size of GLV relative to alternatives can be seen on the following
chart:

**Linux x86_64 Shared Libraries**

|  Bytes  | Library Version |
|--------:|:----------------|
|   32480 |  GLV 0.5        |
|  286752 |  GLFW 3.3       |
|  369088 |  GLUT 3.11.1    |
| 1503144 |  Allegro 5.2.10 |
| 1935080 |  SDL 2.26.3     |


Requirements
------------

OpenGL.

The X11 version is built with the XFree86 XF86VidMode extension by default,
but this can be disabled in the Makefile.

The Windows version has been built with MinGW and Visual C++ using NMAKE.


Installation
------------

### Linux

To use in source form copy these three files into your project:

    x11/glv.c x11/glv.h x11/glv_keys.h

To build a shared library run the following commands:

    make -C x11
    make -C examples -f Makefile.linux
    sudo make -C x11 install


### Windows

To use in source form copy these three files into your project:

    win32/glv.c win32/glv.h win32/glv_keys.h

To build a shared library run the following commands from the command prompt:

    cd win32
    nmake
    cd ..\examples
    nmake -f Makefile.vc


### Android

Add the android source and header files to your project.
