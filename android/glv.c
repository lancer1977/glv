/*===========================================================================/

  GLV Library for Android
  Copyright (C) 2012,2019  Karl Robillard
  SPDX-License-Identifier: MIT

/===========================================================================*/


#include <stdio.h>
#include <stdlib.h>
#include "glv.h"
#include "glv_activity.h"
#include <EGL/eglext.h>         /* Defines EGL_OPENGL_ES3_BIT_KHR */
#include <android/keycodes.h>


#define FLAG_ATTRIB                 0x00ff
#define FLAG_FILTER_REPEAT          0x0100


extern struct android_app* gGlvApp;


void glv_nullHandler( void* v, GLViewEvent* e )
{
    (void) v;
    (void) e;
}


void glv_initEGL( GLView* view, ANativeWindow* window )
{
    const EGLint fbAttr[] = {
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT_KHR,
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_BLUE_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_RED_SIZE, 8,
        EGL_DEPTH_SIZE, 24,
        EGL_NONE
    };
    EGLint ctxAttr[7];
    EGLConfig config;
    EGLDisplay disp;
    EGLint numConfigs;
    EGLint format;
    EGLint w, h;
    EGLint* ap = ctxAttr;

    if( ! view->glVersion )
        view->glVersion = 0x301;

    *ap++ = EGL_CONTEXT_MAJOR_VERSION_KHR;
    *ap++ = view->glVersion >> 8;
    *ap++ = EGL_CONTEXT_MINOR_VERSION_KHR;
    *ap++ = view->glVersion & 0xff;
    if( view->flags & GLV_ATTRIB_DEBUG )
    {
        *ap++ = EGL_CONTEXT_FLAGS_KHR;
        *ap++ = EGL_CONTEXT_OPENGL_DEBUG_BIT_KHR;
    }
    *ap = EGL_NONE;

    view->display = disp = eglGetDisplay( EGL_DEFAULT_DISPLAY );

    eglInitialize( disp, 0, 0 );
    eglChooseConfig( disp, fbAttr, &config, 1, &numConfigs );
    eglGetConfigAttrib( disp, config, EGL_NATIVE_VISUAL_ID, &format );
    ANativeWindow_setBuffersGeometry( window, 0, 0, format );
    view->surface = eglCreateWindowSurface( disp, config, window, NULL );
    view->ctx     = eglCreateContext( disp, config, NULL, ctxAttr );

    glv_makeCurrent( view );

    eglQuerySurface( disp, view->surface, EGL_WIDTH, &w );
    eglQuerySurface( disp, view->surface, EGL_HEIGHT, &h );
    view->width  = w;
    view->height = h;
}


void glv_freeEGL( GLView* view )
{
    if( view->display != EGL_NO_DISPLAY )
    {
        eglMakeCurrent( view->display, EGL_NO_SURFACE, EGL_NO_SURFACE,
                        EGL_NO_CONTEXT );
        if( view->ctx != EGL_NO_CONTEXT )
        {
            eglDestroyContext( view->display, view->ctx );
            view->ctx = EGL_NO_CONTEXT;
        }
        if( view->surface != EGL_NO_SURFACE )
        {
            eglDestroySurface( view->display, view->surface );
            view->surface = EGL_NO_SURFACE;
        }
        eglTerminate( view->display );
        view->display = EGL_NO_DISPLAY;
    }
}


/**
  Creates a view.
  Returns a GLView pointer or zero if the view could not be created.

  If glv_create fails then no other GLV function should be called
  (though it is safe to call glv_destroy()).

  The possible attributes are GLV_ATTRIB_DOUBLEBUFFER, GLV_ATTRIB_STENCIL,
  and GLV_ATTRIB_MULTISAMPLE.  Only RGBA visuals will be created.

  A valid view may be returned even if all attributes could not be set.
  Use glv_attributes() to check which are set.
*/
GLView* glv_create( int attributes, int glVersion )
{
    GLView* view;

    /* On Android the GLView is actually part of android_app, so gGlvApp is
       used to get a handle to that struct. */
    if( ! gGlvApp )
        return NULL;
    view = &gGlvApp->view;
    if( view->appRef )
        return NULL;

    // Initialize non-zero members.
    view->appRef = 1;
    view->flags = attributes & FLAG_ATTRIB;
    view->glVersion = glVersion;
    view->eventHandler = glv_nullHandler;

    if( ! android_app_wait_window( gGlvApp ) )
    {
        view->appRef = 0;
        return NULL;
    }

    return view;
}


/**
  Closes the GLView and frees any used resources.
*/
void glv_destroy( GLView* view )
{
    if( view && view->appRef )
    {
        view->appRef = 0;
        view->eventHandler = glv_nullHandler;
        glv_freeEGL( view );
    }
}


/**
  Returns a mask of GL_ATTRIB_* bits which apply to the view.
*/
int glv_attributes( GLView* view )
{
    return view->flags & FLAG_ATTRIB;
}


/* Approximate vertical refresh rate (Hz) */
#define VRATE(mi) \
    ((int) (((mi)->dotclock * 1000.0) / ((mi)->htotal * (mi)->vtotal) + 0.5))


/**
  Calls func for each mode and returns the number of available fullscreen
  modes.

  \sa glv_changeMode()
*/
int glv_queryModes( GLViewMode_f func, void* data )
{
    (void) func;
    (void) data;
#if 0
    GLViewMode vmode;

    vmode.id          = 0;
    vmode.width       = vmi->hdisplay;
    vmode.height      = vmi->vdisplay;
    vmode.refreshRate = 60;
    vmode.depth       = 16;

    (*func)( &vmode, data );
#endif
    return 1;
}


/**
  Returns non-zero if successful.
  It must not be called from within an input handler function.

  To make a window for the view use the following code:
  \code
GLViewMode mode;

mode.id     = GLV_MODEID_WINDOW;
mode.width  = 640;
mode.height = 480;

glv_changeMode( &view, &mode );
  \endcode

  \sa glv_queryModes()
*/
int glv_changeMode( GLView* view, const GLViewMode* mode )
{
    (void) view;
    (void) mode;
    return 0;
}


/**
  Swaps the GL buffers of the view.
*/
void glv_swapBuffers( GLView* view )
{
    eglSwapBuffers( view->display, view->surface );
}


/**
  Makes the GL context of the view current.
*/
void glv_makeCurrent( GLView* view )
{
    eglMakeCurrent( view->display, view->surface, view->surface, view->ctx );
}


/**
  Makes the window visible.
*/
void glv_show( GLView* view )
{
    (void) view;
}


/**
  Hides the window.
*/
void glv_hide( GLView* view )
{
    (void) view;
}


/**
  Sets window title and icon name.
*/
void glv_setTitle( GLView* view, const char* title )
{
    (void) view;
    (void) title;
}


/**
  Positions window on screen.
  Should only be called when in windowed mode.
*/
void glv_move( GLView* view, int x, int y )
{
}


/**
  Sets window dimensions.
  Should only be called when in windowed mode.
*/
void glv_resize( GLView* view, int w, int h )
{
}


/**
  Show window on top of all other windows.
*/
void glv_raise( GLView* view )
{
}


/**
  Minimizes the window.
*/
void glv_iconify( GLView* view )
{
}


/**
  Show or hide the native mouse pointer.
  There are no provisions to set the native pointer image.  It is assumed
  that a GL primitive will be used for custom pointers.
*/
void glv_showCursor( GLView* view, int on )
{
}


typedef void (*GLViewEvent_vf)( void*, GLViewEvent* );

/**
  Sets the function called during glv_handleEvents()
*/
void glv_setEventHandler( GLView* view, GLViewEvent_f func )
{
    view->eventHandler = func ? (GLViewEvent_vf) func : glv_nullHandler;
}


#include "rqueue.c"
RQUEUE_IMP(GLViewEvent);


/**
  Waits until an event is recieved.

  \sa glv_handleEvents()
*/
void glv_waitEvent( GLView* view )
{
    struct android_poll_source* source;
    int ident;
    int events;

    // NOTE: This implementation dispatches one event after waiting, so it
    // does not exactly match the documented behavior.

    if( (ident = ALooper_pollOnce(-1, NULL, &events, (void**)&source)) >= 0 )
    {
        if( source != NULL )
            source->process( source->app );
    }

    {
    GLViewEvent* ve;
    RQueue* queue = &((struct android_app*) view)->eventQueue;
    while ((ve = rqueue_removeHead_GLViewEvent(queue)))
        view->eventHandler( view, ve );
    }
}


/**
  Calls the event handler for all pending events.

  This should be called periodically (e.g. from your main loop).
  \code
    // Example main loop.
    running = 1;
    while( running )
    {
        glv_handleEvents( &view );

        // Update simulation...
        // Draw frame using GL calls...

        glv_swapBuffers( &view );
    }
  \endcode

  \sa glv_waitEvent()
*/
void glv_handleEvents( GLView* view )
{
    struct android_poll_source* source;
    int ident;
    int events;

    while( (ident = ALooper_pollAll(0, NULL, &events, (void**)&source)) >= 0 )
    {
        if( source != NULL )
            source->process( source->app );
    }

    {
    GLViewEvent* ve;
    RQueue* queue = &((struct android_app*) view)->eventQueue;
    while ((ve = rqueue_removeHead_GLViewEvent(queue)))
        view->eventHandler( view, ve );
    }
}


/**
  Enables or disables key repeat for the view.
  Repeat is on by default.
*/
void glv_filterRepeatKeys( GLView* view, int on )
{
    if( on )
        view->flags |= FLAG_FILTER_REPEAT;
    else
        view->flags &= ~FLAG_FILTER_REPEAT;
}


static const char glv_asciiKeyMap[82] =
{
    0, 0, 0, 0,        0,  0,  0,'0',
    '1','2','3','4', '5','6','7','8',
    '9','*','#',  0,   0,  0,  0,  0,
      0,  0,  0,  0,   0,'a','b','c',
    'd','e','f','g', 'h','i','j','k',
    'l','m','n','o', 'p','q','r','s',
    't','u','v','w', 'x','y','z',',',
    '.',  0,  0,  0,  0,'\t',' ',  0,
      0, 0,'\n',127, '`','-','=','[',
    ']','\\',';','\'', '/','@',  0,  0,
      0,'+'
};


static const char glv_asciiKeyMapShift[82] =
{
    0, 0, 0, 0,        0,  0,  0,')',
    '!','@','#','$', '%','^','&','*',
    '(','*','#',  0,   0,  0,  0,  0,
      0,  0,  0,  0,   0,'A','B','C',
    'D','E','F','G', 'H','I','J','K',
    'L','M','N','O', 'P','Q','R','S',
    'T','U','V','W', 'X','Y','Z',',',
    '.',  0,  0,  0,  0,'\t',' ',  0,
      0, 0,'\n',127, '`','_','+','{',
    '}','|',':','"', '?','@',  0,  0,
      0,'+'
};


/*
  This function is private and may not exist on all platforms.
  Users should use the KEY_ASCII macro.
*/
int glv_ascii( const GLViewEvent* ev )
{
    int code = ev->code;

    // Basic implementation which avoids having to call Java code.

    if( code < 82 )
    {
        if( ev->state & AMETA_SHIFT_ON )
            return glv_asciiKeyMapShift[ code ];
        return glv_asciiKeyMap[ code ];
    }
    if( code == AKEYCODE_ESCAPE )
        return 27;
    return 0;
}


/**
  Calls func with the current system clipboard text.

  \return Non-zero if data is present and func is called.
*/
int glv_clipboardText( GLView* view,
                       void (*func)(const char* data, int len, void* user),
                       void* user )
{
    return 0;
}


/*EOF*/
