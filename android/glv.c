/*===========================================================================/

  GLV Library for Android
  Copyright (C) 2012,2019  Karl Robillard

/===========================================================================*/


#include <stdio.h>
#include <stdlib.h>
#include <glv.h>
#include <glv_activity.h>
#include <EGL/eglext.h>         /* Defines EGL_OPENGL_ES3_BIT_KHR */


#define FLAG_ATTRIB                 0x000f
#define FLAG_FULLSCREEN_MODE        0x0010
#define FLAG_FILTER_REPEAT          0x0020


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
        EGL_NONE
    };
    const EGLint ctxAttr[] = {
        EGL_CONTEXT_CLIENT_VERSION, 3,
        EGL_NONE
    };
    EGLConfig config;
    EGLDisplay disp;
    EGLint numConfigs;
    EGLint format;
    EGLint w, h;

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
GLView* glv_create( int attributes )
{
    GLView* view;

    if( ! gGlvApp )
        return NULL;
    view = &gGlvApp->view;
    if( view->appRef )
        return NULL;

    // Initialize non-zero members.
    view->appRef = 1;
    view->flags = attributes & FLAG_ATTRIB;
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
#if 0
    int oldModeFS = (view->flags & FLAG_FULLSCREEN_MODE) ? 1 : 0;
    int newModeFS = (mode->id != GLV_MODEID_WINDOW) ? 1 : 0;


    /* Return if mode is current */

    if( (oldModeFS == newModeFS) &&
        (mode->width == view->width) &&
        (mode->height == view->height) )
    {
        return 1;
    }

    /*
    XSync( disp, True );
    glXMakeCurrent( disp, window, view->ctx );
    */

    view->width  = attr.width;
    view->height = attr.height;

    ve.type = GLV_EVENT_RESIZE;
    ve.x    = attr.width;
    ve.y    = attr.height;
    view->eventHandler( view, &ve );
#endif
    return 1;
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


/**
  Waits until an event is recieved.

  \sa glv_handleEvents()
*/
void glv_waitEvent( GLView* view )
{
}


//static XKeyEvent* glv_keyEvent;


/*
  Note that the KeySym returned from XLookupString takes into account the
  Shift key whereas XKeycodeToKeysym does not.
*/
#define KEYSYM(e)   XKeycodeToKeysym( view->display, e.xkey.keycode, 0 )


#define COPY_KEY(ve,xe,t) \
        glv_keyEvent = &xe.xkey; \
        ve.type  = t; \
        ve.code  = KEYSYM(xe); \
        ve.state = xe.xkey.state; \
        ve.x     = xe.xkey.x; \
        ve.y     = xe.xkey.y;


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
    GLViewEvent ve;
    struct android_poll_source* source;
    int ident;
    int events;
    int haveKeyUp = 0;

    /*
       glv_filterRepeatKeys may be called from event handler.
       This avoids a filter state change until the next glv_handleEvents.
    */
    int filter = view->flags & FLAG_FILTER_REPEAT;

    while( (ident = ALooper_pollAll(0, NULL, &events, (void**)&source)) >= 0 )
    {
        if( source != NULL )
            source->process( gGlvApp, source );
#if 0
        switch( event.type )
        {
            case ClientMessage:
                if( (event.xclient.format == 32) &&
                    (event.xclient.data.l[ 0 ] == (int) view->deleteAtom) )
                {
                    ve.type = GLV_EVENT_CLOSE;
                    view->eventHandler( view, &ve );
                }
                break;

            case ConfigureNotify:
                if( event.xconfigure.window == view->window )
                {
                    if( (view->width != event.xconfigure.width) ||
                        (view->height != event.xconfigure.height) )
                    {
                        view->width  = event.xconfigure.width;
                        view->height = event.xconfigure.height;

                        ve.type = GLV_EVENT_RESIZE;
                        ve.x    = view->width;
                        ve.y    = view->height;
                        view->eventHandler( view, &ve );
                    }
                }
                break;

            case ButtonPress:
                if( event.xbutton.button == Button4 )
                {
                    ve.type  = GLV_EVENT_WHEEL;
                    ve.code  = 0;
                    ve.state = event.xbutton.state;
                    ve.x     = 0;
                    ve.y     = GLV_WHEEL_DELTA;
                    view->eventHandler( view, &ve );
                }
                else if( event.xbutton.button == Button5 )
                {
                    ve.type  = GLV_EVENT_WHEEL;
                    ve.code  = 0;
                    ve.state = event.xbutton.state;
                    ve.x     = 0;
                    ve.y     = -GLV_WHEEL_DELTA;
                    view->eventHandler( view, &ve );
                }
                else
                {
                    ve.type  = GLV_EVENT_BUTTON_DOWN;
                    ve.code  = event.xbutton.button;
                    ve.state = event.xbutton.state;
                    ve.x     = event.xbutton.x;
                    ve.y     = event.xbutton.y;
                    view->eventHandler( view, &ve );
                }
                break;

            case ButtonRelease:
                if( event.xbutton.button != Button4 &&
                    event.xbutton.button != Button5 )
                {
                    ve.type  = GLV_EVENT_BUTTON_UP;
                    ve.code  = event.xbutton.button;
                    ve.state = event.xbutton.state;
                    ve.x     = event.xbutton.x;
                    ve.y     = event.xbutton.y;
                    view->eventHandler( view, &ve );
                }
                break;

            case MotionNotify:
                ve.type  = GLV_EVENT_MOTION;
                ve.code  = 0;
                ve.state = event.xmotion.state;
                ve.x     = event.xmotion.x;
                ve.y     = event.xmotion.y;
                view->eventHandler( view, &ve );
                break;

            case KeyPress:
                if( filter && haveKeyUp )
                {
                    if( (event.xkey.keycode != prevKeyUp.xkey.keycode) ||
                        (event.xkey.time != prevKeyUp.xkey.time) )
                    {
                        COPY_KEY( ve, prevKeyUp, GLV_EVENT_KEY_UP )
                        view->eventHandler( view, &ve );

                        COPY_KEY( ve, event, GLV_EVENT_KEY_DOWN )
                        view->eventHandler( view, &ve );
                    }
                    haveKeyUp = 0;
                }
                else
                {
                    COPY_KEY( ve, event, GLV_EVENT_KEY_DOWN )
                    view->eventHandler( view, &ve );
                }
                break;

            case KeyRelease:
                if( filter )
                {
                    if( haveKeyUp )
                    {
                        COPY_KEY( ve, prevKeyUp, GLV_EVENT_KEY_UP )
                        view->eventHandler( view, &ve );
                    }
                    prevKeyUp = event;
                    haveKeyUp = 1;
                }
                else
                {
                    COPY_KEY( ve, event, GLV_EVENT_KEY_UP )
                    view->eventHandler( view, &ve );
                }
                break;

            case FocusIn:
                /* event.xfocus */
                ve.type  = GLV_EVENT_FOCUS_IN;
                view->eventHandler( view, &ve );
                break;

            case FocusOut:
                /* event.xfocus */
                ve.type  = GLV_EVENT_FOCUS_OUT;
                view->eventHandler( view, &ve );
                break;

            case Expose:
                /* event.xexpose */
                ve.type  = GLV_EVENT_EXPOSE;
                view->eventHandler( view, &ve );
                break;

            default:
                /*unknownEvent( &event );*/
                break;
        }
#endif
    }

#if 0
    if( haveKeyUp )
    {
        COPY_KEY( ve, prevKeyUp, GLV_EVENT_KEY_UP )
        view->eventHandler( view, &ve );
    }
#endif
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


/*
  This function is private and may not exist on all platforms.
  Users should use the KEY_ASCII macro.
*/
int glv_ascii()
{
    /*
    char buf[ 4 ];
    if( XLookupString( glv_keyEvent, buf, 4, NULL, NULL ) == 1 )
        return *buf;
    */
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
