/*===========================================================================/

  GLV Library for X11
  Copyright (C) 2003-2006  Karl Robillard

/===========================================================================*/


/**
  \mainpage

  Start with the glv.h documentation.
*/


/**
  \struct GLView glv.h
  \brief The GLView struct...

  All undocumented members are private and should not be accessed.
*/

/**
  \var void* GLView::user
  Unused by the GLV library.
  This pointer can be used to attach data to a view (e.g. for use in event
  callback functions).
*/

/**
  \var int GLView::width
  Pixel width of the view.
*/

/**
  \var int GLView::height
  Pixel height of the view.
*/


/*--------------------------------------------------------------------------*/


/**
  \struct GLViewEvent glv.h
  \brief The GLViewEvent struct is passed to the event handler callback.

  \sa GLViewEvent_f
  \sa glv_setEventHandler()
*/

/**
  \var int GLViewEvent::type
  Type of event.
  \code
    GLV_EVENT_RESIZE
    GLV_EVENT_CLOSE
    GLV_EVENT_BUTTON_DOWN
    GLV_EVENT_BUTTON_UP
    GLV_EVENT_MOTION
    GLV_EVENT_WHEEL
    GLV_EVENT_KEY_DOWN
    GLV_EVENT_KEY_UP
    GLV_EVENT_FOCUS_IN
    GLV_EVENT_FOCUS_OUT
    GLV_EVENT_EXPOSE
    GLV_EVENT_USER
  \endcode
*/

/**
  \var int GLViewEvent::code
  Key code for GLV_EVENT_KEY_DOWN/GLV_EVENT_KEY_UP events.
*/

/**
  \var int GLViewEvent::state
  Bit mask of key & button modifiers.
*/

/**
  \var int GLViewEvent::x
  Mouse pointer X position. Width for GLV_EVENT_RESIZE events.
*/

/**
  \var int GLViewEvent::y
  Mouse pointer Y position. Height for GLV_EVENT_RESIZE events.
*/


/*--------------------------------------------------------------------------*/


/**
  \struct GLViewMode glv.h
  \brief The GLViewMode struct holds information about a video mode.
*/

/**
  \def GLV_MODEID_WINDOW
  A GLViewMode::id of GL_MODEID_WINDOW means the GLView is a window on
  the desktop rather than a fullscreen mode.
*/

/**
  \var int GLViewMode::id
  Unique identifier for this mode.
  An id of GLV_MODEID_WINDOW means the GLView is a window on
  the desktop rather than a fullscreen mode.
*/

/**
  \var int GLViewMode::width
  Pixel width of screen or window.
*/

/**
  \var int GLViewMode::height
  Pixel height of screen or window.
*/

/**
  \var int GLViewMode::refreshRate
  Vertical refresh rate in Hz (60, 85, etc.).
*/

/**
  \var int GLViewMode::depth
  Bits per pixel (8, 16, 32, etc.).
*/


/*--------------------------------------------------------------------------*/


/**
  \typedef void (*GLViewMode_f)( const GLViewMode*, void* )
  Callback for glv_queryModes().
*/

/**
  \typedef void (*GLViewEvent_f)( GLView*, GLViewEvent* )
  Event callback for glv_handleEvents().
*/


/*--------------------------------------------------------------------------*/


/**
  \file glv.h
  \brief The GLV library provides a small, cross-platform interface 
  for creating a window or fullscreen display with an OpenGL context.

  Here is a short example of how to use GLV:
  \include doc.c
*/


/**
  \struct GLViewMode glv.h
  \brief The GLViewMode struct...
*/


#include <stdio.h>
#include <stdlib.h>
#include <glv.h>
#include <X11/Xatom.h>

#ifdef USE_XF86VMODE
#include <X11/extensions/xf86vmode.h>
#endif

#ifdef USE_XME
#include <X11/extensions/xme.h>
#endif


#define FLAG_ATTRIB                 0x0007
#define FLAG_FULLSCREEN_MODE        0x0010
#define FLAG_FILTER_REPEAT          0x0020

#define DEFAULT_INPUT   (KeyPressMask | KeyReleaseMask | \
                         ButtonPressMask | ButtonReleaseMask | \
                         PointerMotionMask | \
                         ExposureMask | StructureNotifyMask | \
                         PropertyChangeMask)


static XVisualInfo* chooseVisual( Display* disp, int screen, int flags )
{
    int* end;
    int attrib[] = {
        GLX_RGBA,
        GLX_RED_SIZE,   1,
        GLX_GREEN_SIZE, 1,
        GLX_BLUE_SIZE,  1,
        GLX_DEPTH_SIZE, 1,
        GLX_DOUBLEBUFFER,
        GLX_STENCIL_SIZE, 1,
        0, 1, 0, 16,
        None
    };


    end = attrib + 9;

    if( flags & GLV_ATTRIB_DOUBLEBUFFER )
    {
        *end++ = GLX_DOUBLEBUFFER;
    }

    if( flags & GLV_ATTRIB_STENCIL )
    {
        *end++ = GLX_STENCIL_SIZE;
        *end++ = 1;
    }

    if( flags &  GLV_ATTRIB_MULTISAMPLE )
    {
#ifdef GL_ARB_multisample
        XVisualInfo* vis;
        int samples;


        *end++ = GLX_SAMPLE_BUFFERS_ARB;
        *end++ = 1;
        *end++ = GLX_SAMPLES_ARB;
        *end++ = 16;
        *end   = None;

        --end;

        for( samples = 16; samples > 1; --samples )
        {
            *end = samples;

            vis = glXChooseVisual( disp, screen, attrib );
            if( vis )
                return vis;
        }
#endif
        return 0;
    }

    *end = None;
    return glXChooseVisual( disp, screen, attrib );
}


/*
  Sets flags to valid attributes.
*/
static XVisualInfo* bestVisual( Display* disp, int screen, int* flags )
{
    XVisualInfo* vi = chooseVisual( disp, screen, *flags );
    if( ! vi )
    {
        int valid = 0;
        int bit = GLV_ATTRIB_MULTISAMPLE;

        // Test each requested attribute separately.
        while( bit )
        {
            if( *flags & bit )
            {
                vi = chooseVisual( disp, screen, bit );
                if( vi )
                {
                    XFree( vi );
                    valid |= bit;
                }
            }
            bit >>= 1;
        }

        *flags = valid;
        vi = chooseVisual( disp, screen, valid );
    }
    return vi;
}


static void glv_nullHandler( void* v, GLViewEvent* e )
{
    (void) v;
    (void) e;
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
    Display* disp;
    XVisualInfo* vinfo;
    int screen;


    disp = XOpenDisplay( 0 );
    if( ! disp )
    {
        fprintf( stderr, "XOpenDisplay failed!\n" );
        return( 0 );
    }

    if( glXQueryExtension( disp, 0, 0 ) == 0 )
    {
        fprintf( stderr, "GLX Extension not available!\n" );
        XCloseDisplay( disp );
        return( 0 );
    }

    screen = DefaultScreen( disp );

    attributes &= FLAG_ATTRIB;
    vinfo = bestVisual( disp, screen, &attributes );
    if( ! vinfo )
    {
        fprintf( stderr, "glXChooseVisual failed!\n" );
        XCloseDisplay( disp );
        return( 0 );
    }

    /* We have a valid visual so create view. */
    view = (GLView*) calloc( 1, sizeof(GLView) );
    if( ! view )
    {
        XCloseDisplay( disp );
        return( 0 );
    }

    // Initialize non-zero members.
    view->display    = disp;
    view->screen     = screen;
    view->vinfo      = vinfo;
    view->flags      = attributes;
    view->nullCursor = -1;
    view->eventHandler = glv_nullHandler;


    view->ctx = glXCreateContext( disp, view->vinfo, NULL, True );


    {
        XSetWindowAttributes attr;
        XVisualInfo* vi = view->vinfo;

        /* GLX requires a colormap (see the glXIntro man page). */

        attr.event_mask   = DEFAULT_INPUT;
        attr.border_pixel = BlackPixel( disp, vi->screen );
        attr.colormap = XCreateColormap( disp,
                                         RootWindow( disp, vi->screen ),
                                         vi->visual, AllocNone );

        view->window = XCreateWindow( disp, RootWindow( disp, vi->screen ),
                                      0, 0, 640, 480,
                                      0, vi->depth, InputOutput, vi->visual,
                                      CWEventMask | CWBorderPixel | CWColormap,
                                      &attr );
    }

    /* Enable the delete window protocol. */
    view->deleteAtom = XInternAtom( disp, "WM_DELETE_WINDOW", False );
    XSetWMProtocols( disp, view->window, &view->deleteAtom, 1 );

    glv_makeCurrent( view );

    return( view );
}


/* Restore original video mode. */
static void _restoreVideo( GLView* view )
{
    if( view->flags & FLAG_FULLSCREEN_MODE )
    {
        if( view->omode )
        {
#ifdef USE_XF86VMODE
            XF86VidModeModeInfo* vm = (XF86VidModeModeInfo*) view->omode;
            XF86VidModeLockModeSwitch( view->display, view->screen, False );
            XF86VidModeSwitchToMode( view->display, view->screen, vm );
#endif

#ifdef USE_XME
            XiGMiscResolutionInfo* vm = (XiGMiscResolutionInfo*) view->omode;
            XiGMiscChangeResolution( view->display, view->screen, 0,
                                     vm->width, vm->height, 0 );
#endif
        }

        XUngrabKeyboard( view->display, CurrentTime );
        XUngrabPointer( view->display, CurrentTime );

        view->flags &= ~FLAG_FULLSCREEN_MODE;
    }
}


/**
  Closes the GLView and frees any used resources.
*/
void glv_destroy( GLView* view )
{
    if( ! view )
        return;

    if( view->display )
    {
        Display* disp = view->display;

        _restoreVideo( view );

        if( view->omode )
        {
            free( view->omode );
            view->omode = 0;
        }

        if( view->window )
        {
            if( view->nullCursor != (Cursor) -1 )
            {
                XFreeCursor( disp, view->nullCursor );
                view->nullCursor = -1;
            }

            XDestroyWindow( disp, view->window );
            view->window = 0;
        }

        if( view->ctx )
        {
            glXDestroyContext( disp, view->ctx );
            view->ctx = 0;
        }

        if( view->vinfo )
        {
            XFree( view->vinfo );
            view->vinfo = 0;
        }

        XCloseDisplay( view->display );
        view->display = 0;
    }

    free( view );
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
    GLViewMode vmode;
    Display* disp;
    int screen;
    int modeCount = 0;


    disp = XOpenDisplay( 0 );
    if( ! disp )
        return( 0 );

    screen = DefaultScreen( disp );

    /*int XDefaultDepth( display, screen );*/
    /*int XDisplayPlanes( display, screen );*/

    /* X11 cannot change depth on the fly(?) */
    vmode.depth = XDisplayPlanes( disp, screen );


#ifdef USE_XF86VMODE
    {
    XF86VidModeModeInfo** modelist;
    int eventBase;
    int errorBase;
    int i;

    if( XF86VidModeQueryExtension( disp, &eventBase, &errorBase ) )
    {
        XF86VidModeGetAllModeLines( disp, screen, &modeCount, &modelist );

        for( i = 0; i < modeCount; ++i )
        {
            XF86VidModeModeInfo* vmi = modelist[i];

            /* Since the first mode is always the current one vmode.id is
               not particularly meaningful with XF86VidMode. */

            vmode.id          = i;
            vmode.width       = vmi->hdisplay;
            vmode.height      = vmi->vdisplay;
            vmode.refreshRate = VRATE(vmi);

            (*func)( &vmode, data );
#if 0
            printf( "%d: %dx%d  dot: %d ht: %d vt: %d\n", i,
                    vmi->hdisplay, vmi->vdisplay, vmi->dotclock,
                    vmi->htotal, vmi->vtotal );
#endif
        }

        XFree( modelist );
    }
    }
#endif


#ifdef USE_XME
    {
    int major, minor;

    if( XiGMiscQueryVersion( disp, &major, &minor ) && (major >= 2) )
    {
        XiGMiscResolutionInfo* modelist;
        int active;
        int i;

        modeCount = XiGMiscQueryResolutions( disp, screen,
                                             0, &active, &modelist );

        for( i = 0; i < modeCount; ++i )
        {
            vmode.id          = i;
            vmode.width       = modelist[i].width;
            vmode.height      = modelist[i].height;
            vmode.refreshRate = modelist[i].refresh;

            (*func)( &vmode, data );
        }

        XFree( modelist );
    }
    }
#endif

    XCloseDisplay( disp );

    return( modeCount );
}


#if 0
static void _map( Display* disp, Window win )
{
    XWindowAttributes attr;
    XEvent event;

    /* Ensure the window is mapped to prevent hang in loop below. */
    XGetWindowAttributes( disp, win, &attr );
    if( attr.map_state == IsViewable )
        return;

    XMapRaised( disp, win );

    /* Wait to be mapped. This loop removes ConfigureNotify events as well. */
    while( 1 )
    {
        XMaskEvent( disp, StructureNotifyMask, &event );
        //printf( "StructureNotifyMask %d\n", event.type );
        if( (event.type == MapNotify) && (event.xmap.event == win) )
            break;
    }
}
#endif


/*
  Properly unmap
*/
static void _withdraw( Display* disp, Window win, int screen )
{
    XWindowAttributes wattr;
    Atom ATOM_WM_STATE;
    XEvent event;

    XGetWindowAttributes( disp, win, &wattr );
    if( wattr.map_state == IsUnmapped )
        return;

    ATOM_WM_STATE = XInternAtom( disp, "WM_STATE", False );

    XWithdrawWindow( disp, win, screen );

    /* Wait till window manager tells us the window is unmapped. */
    while( 1 )
    {
        XNextEvent( disp, &event );
        //XWindowEvent( disp, win,
        //XMaskEvent( disp,
        //            StructureNotifyMask | PropertyChangeMask, &event );

        if( event.type == PropertyNotify )
        {
            if( event.xproperty.atom == ATOM_WM_STATE )
            {
                if( event.xproperty.state == PropertyDelete )
                {
                    // Some window managers (KWin) delete the property.
                    break;
                }
                else
                {
                    // Other window managers (WindowMaker, Metacity, twm)
                    // just change the state to WithdrawnState.
                    int err, format;
                    Atom ret;
                    unsigned char* data;
                    unsigned long nitems, after;

                    err = XGetWindowProperty( disp, win, ATOM_WM_STATE,
                                              0, 2, False, ATOM_WM_STATE,
                                              &ret, &format, &nitems,
                                              &after, &data );

                    if( (err == Success) &&
                        (ret == ATOM_WM_STATE) &&
                        (format == 32) &&
                        (nitems > 0) )
                    {
                        long state = *((long*) data);
                        if( state == WithdrawnState )
                        {
                            XDeleteProperty( disp, win, ATOM_WM_STATE );
                            break;
                        }
                    }
                }
            }
        }
#if 0
        else if( (event.type == UnmapNotify) && (event.xunmap.event == win) )
        {
            printf( "UnmapNotify\n" );
        }
        else
        {
            printf( "Event %d\n", event.type );
        }
#endif
    }

    /* Clear event queue to eliminate bastard ConfigureNotify events. */
    //XSync( disp, True );
}


#if 0
static void _report( Display* disp, Window win )
{
    XWindowAttributes attr;
    XGetWindowAttributes( disp, win, &attr );

    printf( "  x: %d\n", attr.x );
    printf( "  y: %d\n", attr.y );
    printf( "  width: %d\n", attr.width );
    printf( "  height: %d\n", attr.height );
    //printf( "  root: %ld\n", attr.root );
    printf( "  map_state: %d\n", attr.map_state );
    printf( "  override_redirect: %d\n", attr.override_redirect );


    {
    Window root;
    Window parent;
    Window* children;
    unsigned int childCount;

    XQueryTree( disp, win, &root, &parent, &children, &childCount );

    printf( "  root: %ld\n", root );
    printf( "  parent: %ld\n", parent );
    printf( "  children: %d\n", childCount );
    }
}
#endif


/*
  Determine current mode so it can be restored later.
  Caller must free() the returned pointer .
*/
static void _changeVideoMode( GLView* view, const GLViewMode* mode )
{
#ifdef USE_XF86VMODE
    int eventBase;
    int errorBase;
    Display* disp = view->display;

    if( XF86VidModeQueryExtension( disp, &eventBase, &errorBase ) )
    {
        XF86VidModeModeInfo** modelist;
        XF86VidModeModeInfo* vmi;
        int count;
        int i;

        XF86VidModeGetAllModeLines( disp, view->screen, &count, &modelist );

        if( ! view->omode )
        {
            /* Save original desktop mode */
            vmi = (XF86VidModeModeInfo*) malloc( sizeof(XF86VidModeModeInfo) );
            if( vmi )
            {
                /* According to the man page the first element is the
                   current video mode.  It seems to always be the mode X was
                   started it.
                */
                *vmi = *modelist[0];
                view->omode = vmi;
            }
        }

        for( i = 0; i < count; ++i )
        {
            vmi = modelist[i];

            if( (vmi->hdisplay == mode->width) &&
                (vmi->vdisplay == mode->height) &&
                (VRATE(vmi) == mode->refreshRate) )
            {
                break;
            }
        }

#if 1
        if( i > 0 )
        {
            XF86VidModeSwitchToMode( disp, view->screen, vmi );
            XSync( disp, False );
            XF86VidModeSetViewPort( disp, view->screen, 0, 0 );
            XF86VidModeLockModeSwitch( disp, view->screen, True );
        }
#else
        printf( "KR cvm %d %d\n", count, i );
#endif

        XFree( modelist );
    }
#endif

#ifdef USE_XME
    int major, minor;

    if( XiGMiscQueryVersion( view->display, &major, &minor ) && (major >= 2) )
    {
        if( ! view->omode )
        {
            /* Save original desktop mode */

            XiGMiscResolutionInfo* info;

            info = (XiGMiscResolutionInfo*)
                   malloc( sizeof(XiGMiscResolutionInfo) );
            if( info )
            {
                XiGMiscResolutionInfo* modelist;
                int active;

                XiGMiscQueryResolutions( view->display, view->screen,
                                         0, &active, &modelist );

                *info = modelist[ active ];
                view->omode = info;

                XFree( modelist );
            }
        }

        XiGMiscChangeResolution( view->display, view->screen, 0,
                                 mode->width,
                                 mode->height, 0 );
    }
#endif
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
    Display* disp = view->display;
    Window window = view->window;
    int oldModeFS = (view->flags & FLAG_FULLSCREEN_MODE) ? 1 : 0;
    int newModeFS = (mode->id != GLV_MODEID_WINDOW) ? 1 : 0;


    /* Return if mode is current */

    if( (oldModeFS == newModeFS) &&
        (mode->width == view->width) &&
        (mode->height == view->height) )
    {
        return( 1 );
    }


#if defined(USE_XF86VMODE) || defined(USE_XME)
    if( newModeFS )
    {
        /* Change to fullscreen mode. */

        /* The order of calls is important in order to make the change into
           fullscreen mode successfully.  The following problems can occur:

           - Viewport is not at 0, 0.
           - Window is not at 0, 0.
           - Window does not appear at all!
        */

        /* override_redirect does not take affect unless we unmap.
           We must wait for the window manager to tell us the window is
           unmapped or sometimes it does not get remapped below!
        */
        if( ! oldModeFS )
        {
            XSetWindowAttributes attr;

            _withdraw( disp, window, view->screen );

            attr.override_redirect = True;
            XChangeWindowAttributes( disp, window, CWOverrideRedirect, &attr );

            view->flags |= FLAG_FULLSCREEN_MODE;
        }

        _changeVideoMode( view, mode );

        XMoveResizeWindow( disp, window, 0, 0, mode->width, mode->height );

        /* NOTE: Must map after override_redirect has been set. */
        XMapRaised( disp, window );
        //_map( disp, window );
        //XMapWindow( disp, window );
        //XSync( disp, True );

        XGrabKeyboard( disp, window, True,
                       GrabModeAsync, GrabModeAsync, CurrentTime );

        XWarpPointer( disp, None, window, 0, 0, 0, 0,
                      mode->width / 2, mode->height / 2 );

        /*
          NOTE: Must grab pointer and set the viewport after the window is
          mapped or viewport may be scrolled off of 0, 0.
         */
        XGrabPointer( disp, window, True, 0,
                      GrabModeAsync, GrabModeAsync,
                      window, None, CurrentTime );

        /* Calling XF86VidModeSetViewPort here causes pointer to jump to
           0, 0 until the mouse is moved (when it then pops to the warp
           position). */
    }
    else
#endif
    {
        /* Change to managed window. */

        if( oldModeFS )
        {
            XSetWindowAttributes attr;

            _restoreVideo( view );

            /*
             * override_redirect does not take affect unless we unmap.
             * Since override is currently active we can directly unmap.
             * (the window manager will not do any property changes anyway).
             */
            XUnmapWindow( disp, window );
            XSync( disp, False );

            attr.override_redirect = False;
            XChangeWindowAttributes( disp, window, CWOverrideRedirect, &attr );
        }

        XResizeWindow( disp, window, mode->width, mode->height );

        //_map( disp, window );
        XMapRaised( disp, window );
    }


    XSync( disp, True );
    glXMakeCurrent( disp, window, view->ctx );


    // Generate GLV_EVENT_RESIZE
    {
    GLViewEvent ve;
    Window root;
    int x, y;
    unsigned int w, h, u;

    XGetGeometry( disp, window, &root, &x, &y, &w, &h, &u, &u );
    view->width  = w;
    view->height = h;

    ve.type = GLV_EVENT_RESIZE;
    ve.x    = w;
    ve.y    = h;
    view->eventHandler( view, &ve );
    }

    //_report( disp, window );

    return( 1 );
}


/**
  Swaps the GL buffers of the view.
*/
void glv_swapBuffers( GLView* view )
{
    glXSwapBuffers( view->display, view->window );
}


/**
  Makes the GL context of the view current.
*/
void glv_makeCurrent( GLView* view )
{
    glXMakeCurrent( view->display, view->window, view->ctx );
}


/**
  Makes the window visible.
*/
void glv_show( GLView* view )
{
    XMapWindow( view->display, view->window );
    XSync( view->display, False );
}


/**
  Hides the window.
*/
void glv_hide( GLView* view )
{
    XUnmapWindow( view->display, view->window );
    XSync( view->display, False );
}


/**
  Sets window title and icon name.
*/
void glv_setTitle( GLView* view, const char* title )
{
    XStoreName( view->display, view->window, title );
    XSetIconName( view->display, view->window, title );
}


/**
  Positions window on screen.
  Should only be called when in windowed mode.
*/
void glv_move( GLView* view, int x, int y )
{
    XMoveWindow( view->display, view->window, x, y );
}


/**
  Sets window dimensions.
  Should only be called when in windowed mode.
*/
void glv_resize( GLView* view, int w, int h )
{
    XResizeWindow( view->display, view->window, w, h );
}


/**
  Show window on top of all other windows.
*/
void glv_raise( GLView* view )
{
    XRaiseWindow( view->display, view->window );
}


/**
  Minimizes the window.
*/
void glv_iconify( GLView* view )
{
    XIconifyWindow( view->display, view->window, view->screen );
}


/**
  Show or hide the native mouse pointer.
  There are no provisions to set the native pointer image.  It is assumed
  that a GL primitive will be used for custom pointers.
*/
void glv_showCursor( GLView* view, int on )
{
    if( on )
    {
        XUndefineCursor( view->display, view->window );
    }
    else
    {
        Display* disp = view->display;

        if( view->nullCursor == (Cursor) -1 )
        {
            Pixmap cursormask;
            XGCValues xgc;
            GC gc;
            XColor dummycolour;

            cursormask = XCreatePixmap( disp, view->window, 1, 1, 1/*depth*/);
            xgc.function = GXclear;
            gc = XCreateGC( disp, cursormask, GCFunction, &xgc );
            XFillRectangle( disp, cursormask, gc, 0, 0, 1, 1 );
            dummycolour.pixel = 0;
            dummycolour.red   = 0;
            dummycolour.flags = 04;
            view->nullCursor = XCreatePixmapCursor( disp, cursormask,
                                cursormask, &dummycolour, &dummycolour, 0, 0 );
            XFreePixmap( disp, cursormask );
            XFreeGC( disp, gc );
        }

        XDefineCursor( disp, view->window, view->nullCursor );
    }
}


typedef void (*GLViewEvent_vf)( void*, GLViewEvent* );

/**
  Sets the function called during glv_handleEvents()
*/
void glv_setEventHandler( GLView* view, GLViewEvent_f func )
{
    view->eventHandler = func ? (GLViewEvent_vf) func : glv_nullHandler;
}


static Bool _predicateWait( Display* disp, XEvent* event, XPointer arg )
{
    (void) disp;
    if( event->xany.window == ((GLView*) arg)->window )
        return True;
    else
        return False;
}


/**
  Waits until an event is recieved.

  \sa glv_handleEvents()
*/
void glv_waitEvent( GLView* view )
{
    XEvent event;
    XPeekIfEvent( view->display, &event, _predicateWait, (XPointer) view );
}


static XKeyEvent* glv_keyEvent;


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
    XEvent event;
    XEvent prevKeyUp;
    int haveKeyUp = 0;

    /*
       glv_filterRepeatKeys may be called from event handler.
       This avoids a filter state change until the next glv_handleEvents.
    */
    int filter = view->flags & FLAG_FILTER_REPEAT;

    /*
      Filters repeat keys without using the global XAutoRepeatOff().
      This works by dropping consecutive press/realease events - it is assumed
      that the press immediately follows the release and has the same time.
    */

    while( XPending( view->display ) )
        /*
    while( (XCheckWindowEvent( view->display, view->window,
                               DEFAULT_INPUT, &event ) == True) )
       */
    {
        XNextEvent( view->display, &event );

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
    }

    if( haveKeyUp )
    {
        COPY_KEY( ve, prevKeyUp, GLV_EVENT_KEY_UP )
        view->eventHandler( view, &ve );
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


/*
  This function is private and may not exist on all platforms.
  Users should use the KEY_ASCII macro.
*/
int glv_ascii()
{
    char buf[ 4 ];
    if( XLookupString( glv_keyEvent, buf, 4, NULL, NULL ) == 1 )
        return *buf;
    return 0;
}


static Bool _predicateSelection( Display* disp, XEvent* event, XPointer arg )
{
    (void) disp;
    (void) arg;

    if( event->type == SelectionNotify )
        return True;
    else
        return False;
}


/**
  Calls func with the current system clipboard text.

  \return Non-zero if data is present and func is called.
*/
int glv_clipboardText( GLView* view,
                       void (*func)(const char* data, int len, void* user),
                       void* user )
{
    Display* disp;
    Window owner;
    int format;
    Atom type;
    Atom atom_sel;
    unsigned long nitems;
    unsigned long bytesLeft;
    unsigned long dummy;
    unsigned char* data;

    disp = view->display;

    owner = XGetSelectionOwner( disp, XA_PRIMARY );
    if( owner == None )
        return 0;
    if( owner == view->window )
        return 0;

    atom_sel  = XInternAtom( disp, "APP_SELECTION", False );

    XConvertSelection( disp, XA_PRIMARY, XA_STRING, atom_sel,
                       view->window, CurrentTime /*view->lastInputTime*/ );

    // Wait for SelectionNotify event.
    {
    XEvent event;
    XIfEvent( disp, &event, _predicateSelection, NULL );
    if( event.xselection.property == None )
        return 0;
    }

    // Query size of data.
    data = NULL;
    XGetWindowProperty( disp, view->window, atom_sel,
                        0, 0, False,
                        AnyPropertyType, &type, &format,
                        &nitems, &bytesLeft, &data );
    if( type == None )
        return 0;
    if( data != NULL )
    {
        XFree( data );
        data = NULL;
    }
    if( bytesLeft < 1 )
        return 0;

    // Retrieve data.
    XGetWindowProperty( disp, view->window, atom_sel,
                        0, (bytesLeft+3)/4, False,
                        AnyPropertyType,&type, &format,
                        &nitems, &dummy, &data );
    if( (type == None) || (data == NULL) )
        return 0;

    func( (char*) data, nitems * format / 8, user );

    XFree( data );
    //XDeleteProperty( disp, view->window, atom_sel );
    return 1;

#if 0
    char* clip;
    int size;

    clip = XFetchBytes( disp, &size );
    if( clip )
    {
        func( clip, size, user );
        XFree( clip );
        return 1;
    }
    return 0;
#endif
}


/*EOF*/
