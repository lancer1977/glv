/*===========================================================================/

  GLV Library for X11
  Copyright (C) 2003-2006,2011,2012,2014,2022  Karl Robillard

/===========================================================================*/


/**
  \mainpage

  Start with the glv.h documentation.


  \file glv.h
  \brief The GLV library provides a small, cross-platform interface
  for creating a window or fullscreen display with an OpenGL context.

  Here is a short example of how to use GLV:
  \include doc.c


  \example window.c
  This shows how to use the window display modes.


  \struct GLView glv.h
  \brief The GLView struct defines a single window with an OpenGL context.

  All undocumented members are private and should not be accessed.


  \var void* GLView::user
  Unused by the GLV library.
  This pointer can be used to attach data to a view (e.g. to use in event
  callback functions).


  \var int GLView::width
  Pixel width of the view.
  This is a read-only value that should never be directly set by the user.


  \var int GLView::height
  Pixel height of the view.
  This is a read-only value that should never be directly set by the user.


  \struct GLViewEvent glv.h
  \brief The GLViewEvent struct is passed to the event handler callback.

  \sa GLViewEvent_f
  \sa glv_setEventHandler()


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


  \var int GLViewEvent::code
  Key code for GLV_EVENT_KEY_* events.


  \var int GLViewEvent::state
  Bit mask of key & button modifiers for GLV_EVENT_KEY_* events.


  \var int GLViewEvent::x
  Mouse pointer X position for GLV_EVENT_BUTTON_* & GLV_EVENT_MOTION events.
  Width for GLV_EVENT_RESIZE events.


  \var int GLViewEvent::y
  Mouse pointer Y position for GLV_EVENT_BUTTON_* & GLV_EVENT_MOTION events.
  Height for GLV_EVENT_RESIZE events.
*/

/**
  \struct GLViewMode glv.h
  \brief The GLViewMode struct holds information about a video mode.

  \def GLV_MODEID_WINDOW
  A GLViewMode::id of GLV_MODEID_WINDOW means the GLView is a window on
  the desktop rather than a fullscreen mode.

  \def GLV_MODEID_FULL_WINDOW
  A GLViewMode::id of GLV_MODEID_FULL_WINDOW will open a borderless window
  that covers the entire desktop at its current resolution.
  The mode width & height must still be set as a normal window fallback.

  \def GLV_MODEID_FIXED_WINDOW
  A GLViewMode::id of GLV_MODEID_FIXED_WINDOW opens a window on the desktop
  with a fixed size.  The window cannot be resized by the user but is
  otherwise the same as #GLV_MODEID_WINDOW.

  \var int GLViewMode::id
  Unique identifier for this mode.
  An id of #GLV_MODEID_WINDOW, #GLV_MODEID_FULL_WINDOW, or
  #GLV_MODEID_FIXED_WINDOW means the GLView is a window on the desktop rather
  than a fullscreen video mode.

  \var int GLViewMode::width
  Pixel width of screen or window.

  \var int GLViewMode::height
  Pixel height of screen or window.

  \var int GLViewMode::refreshRate
  Vertical refresh rate in Hz (60, 85, etc.).

  \var int GLViewMode::depth
  Bits per pixel (8, 16, 32, etc.).
*/

/**
  \typedef GLViewMode_f
  Callback for glv_queryModes().


  \typedef GLViewEvent_f
  Event callback for glv_handleEvents().
*/


#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <glv.h>
#include <GL/glxext.h>
#include <X11/Xatom.h>
#include <X11/XKBlib.h>

#ifdef USE_XF86VMODE
#include <X11/extensions/xf86vmode.h>
#endif


#define FLAG_ATTRIB                 0x000f
#define FLAG_FULLSCREEN_MODE        0x0010
#define FLAG_FULLWINDOW_MODE        0x0020
#define FLAG_FILTER_REPEAT          0x0040

#define DEFAULT_INPUT   (KeyPressMask | KeyReleaseMask | \
                         ButtonPressMask | ButtonReleaseMask | \
                         PointerMotionMask | \
                         ExposureMask | StructureNotifyMask | \
                         PropertyChangeMask)

#define FB_ATTR_SIZE    20

/*
  Fill glXChooseFBConfig attribute array.

  \param attr   Must be at least FB_ATTR_SIZE elements.
*/
static void _setFBAttr( int* attr, int glvFlags )
{
    // GLX_RENDER_TYPE   defaults to GLX_RGBA_BIT
    // GLX_DRAWABLE_TYPE defaults to GLX_WINDOW_BIT

    *attr++ = GLX_RED_SIZE;
    *attr++ = 4;
    *attr++ = GLX_GREEN_SIZE;
    *attr++ = 4;
    *attr++ = GLX_BLUE_SIZE;
    *attr++ = 4;
    *attr++ = GLX_ALPHA_SIZE;
    *attr++ = 4;
    *attr++ = GLX_DEPTH_SIZE;
    *attr++ = 4;

    if( glvFlags & GLV_ATTRIB_DOUBLEBUFFER )
    {
        *attr++ = GLX_DOUBLEBUFFER;
        *attr++ = True;
    }

    if( glvFlags & GLV_ATTRIB_STENCIL )
    {
        *attr++ = GLX_STENCIL_SIZE;
        *attr++ = 1;
    }

    /*
    We don't want to fail if multi-sampling is requested and it's not available.
#ifdef GLX_ARB_multisample
    if( glvFlags & GLV_ATTRIB_MULTISAMPLE )
    {
        *attr++ = GLX_SAMPLE_BUFFERS_ARB;
        *attr++ = 1;
        *attr++ = GLX_SAMPLES_ARB;
        *attr++ = 2;
    }
#endif
    */

    *attr = None;
}


typedef struct
{
    Atom type;
    int  format;
    unsigned long count;
    unsigned long remain;
    unsigned char* data;
}
Property;

// Caller must call XFree(pr->data).
static int getProperty(const GLView* view, Property* pr,
                       Atom name, long long len, Atom reqType)
{
    pr->count = 0;
    pr->data = NULL;
    return XGetWindowProperty(view->display, view->window,
                              name, 0, len, False, reqType,
                              &pr->type, &pr->format, &pr->count, &pr->remain,
                              &pr->data);
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

  A valid view may be returned even if all attributes could not be set.
  Use glv_attributes() to check which are set.

  \param attributes The possible attributes are GLV_ATTRIB_DOUBLEBUFFER,
                    GLV_ATTRIB_STENCIL, GLV_ATTRIB_MULTISAMPLE, GLV_ATTRIB_ES,
                    and GLV_ATTRIB_DEBUG.  Only RGBA visuals will be created.

  \param glVersion  This contains the OpenGL major version in bits 8-15 and
                    the minor in bits 0-7, so version 3.2 is 0x302.
                    If zero, no specific version is requested.
*/
GLView* glv_create( int attributes, int glVersion )
{
    GLView* view;
    Display* disp;
    GLXFBConfig* fbCfg;
    int ci = 0;
    int fbCount;
    int fbAttr[ FB_ATTR_SIZE ];


    disp = XOpenDisplay( 0 );
    if( ! disp )
    {
        fprintf( stderr, "XOpenDisplay failed!\n" );
        return 0;
    }

    if( glXQueryExtension( disp, 0, 0 ) == 0 )
    {
        fprintf( stderr, "GLX Extension not available!\n" );
        goto fail_disp;
    }

    view = (GLView*) calloc( 1, sizeof(GLView) );
    if( ! view )
        goto fail_disp;

    // Initialize non-zero members.
    view->display      = disp;
    view->screen       = DefaultScreen( disp );
    view->flags        = attributes & FLAG_ATTRIB;
    view->nullCursor   = -1;
    view->eventHandler = glv_nullHandler;


    _setFBAttr( fbAttr, attributes );

    fbCfg = glXChooseFBConfig( disp, view->screen, fbAttr, &fbCount );
    if( ! fbCfg )
    {
        fprintf( stderr, "glXChooseFBConfig failed!\n" );
        goto fail_view;
    }

#ifdef GLX_ARB_multisample
    if( attributes & GLV_ATTRIB_MULTISAMPLE )
    {
        // The config array should be sorted with the highest capability modes
        // at the end, so we're looking for the first one with the largest
        // GLX_SAMPLES_ARB.
        int i;
        int val;
        int high = 0;
        for( i = 0; i < fbCount; ++i )
        {
            glXGetFBConfigAttrib( disp, fbCfg[ i ], GLX_SAMPLES_ARB, &val );
            if( high < val )
            {
                high = val;
                ci = i;
            }
        }
        //printf( "KR Selected config %d\n", ci );
    }
#endif

#if 0
    {
    int i;
    int r, g, b, a, d, s, samp;
    printf( "%d FBConfigs\n"
            "   R  G  B  A  Dep Sten Samp\n"
            "  ----------------------------\n", fbCount );
    for( i = 0; i < fbCount; ++i )
    {
        glXGetFBConfigAttrib( disp, fbCfg[ i ], GLX_RED_SIZE,     &r );
        glXGetFBConfigAttrib( disp, fbCfg[ i ], GLX_GREEN_SIZE,   &g );
        glXGetFBConfigAttrib( disp, fbCfg[ i ], GLX_BLUE_SIZE,    &b );
        glXGetFBConfigAttrib( disp, fbCfg[ i ], GLX_ALPHA_SIZE,   &a );
        glXGetFBConfigAttrib( disp, fbCfg[ i ], GLX_DEPTH_SIZE,   &d );
        glXGetFBConfigAttrib( disp, fbCfg[ i ], GLX_STENCIL_SIZE, &s );
        glXGetFBConfigAttrib( disp, fbCfg[ i ], GLX_SAMPLES_ARB,  &samp );
        printf( "  %2d,%2d,%2d,%2d  %2d  %2d  %2d\n", r, g, b, a, d, s, samp );
    }
    }
#endif

#if defined(GLX_VERSION_1_4)
    {
        int ctxAttr[ 10 ];
        int* cp = ctxAttr;

        if( attributes & GLV_ATTRIB_ES )
        {
            /* Requires "GLX_EXT_create_context_es_profile" */
#if defined(GLX_CONTEXT_ES_PROFILE_BIT_EXT)
            if( ! glVersion )
                glVersion = 0x302;
            *cp++ = GLX_CONTEXT_MAJOR_VERSION_ARB;
            *cp++ = glVersion >> 8;
            *cp++ = GLX_CONTEXT_MINOR_VERSION_ARB;
            *cp++ = glVersion & 0xff;
            *cp++ = GLX_CONTEXT_PROFILE_MASK_ARB;
            *cp++ = GLX_CONTEXT_ES_PROFILE_BIT_EXT;
#else
            fprintf(stderr, "libglv not compiled with GLV_ATTRIB_ES support\n");
            goto fail_fb;
#endif
        }
        else if( glVersion )
        {
            *cp++ = GLX_CONTEXT_MAJOR_VERSION_ARB;
            *cp++ = glVersion >> 8;
            *cp++ = GLX_CONTEXT_MINOR_VERSION_ARB;
            *cp++ = glVersion & 0xff;
        }

        if( attributes & GLV_ATTRIB_DEBUG )
        {
            *cp++ = GLX_CONTEXT_FLAGS_ARB;
            *cp++ = GLX_CONTEXT_DEBUG_BIT_ARB;
        }

        *cp = None;

        PFNGLXCREATECONTEXTATTRIBSARBPROC glXCreateContextAttribsARB =
            (PFNGLXCREATECONTEXTATTRIBSARBPROC)
            glXGetProcAddress( (const GLubyte*) "glXCreateContextAttribsARB" );
        if( ! glXCreateContextAttribsARB )
        {
            fprintf( stderr, "glXCreateContextAttribsARB is not present!\n" );
            goto fail_fb;
        }
        view->ctx = glXCreateContextAttribsARB( disp, fbCfg[ci], NULL, True,
                                                ctxAttr );
        if( ! view->ctx )
        {
            fprintf( stderr, "Could not create %sGLXContext\n",
                     (attributes & GLV_ATTRIB_ES) ? "ES 3.2 profile " : "" );
            goto fail_fb;
        }
    }
#else
    view->ctx = glXCreateNewContext( disp, fbCfg[ci], GLX_RGBA_TYPE,
                                     NULL, True );
    if( ! view->ctx )
    {
        fprintf( stderr, "Could not create GLXContext\n" );
        goto fail_fb;
    }
#endif

    {
        XSetWindowAttributes attr;
        XVisualInfo* vi = glXGetVisualFromFBConfig( disp, fbCfg[ci] );
        if( ! vi )
        {
            fprintf( stderr, "glXGetVisualFromFBConfig failed!\n" );
            glXDestroyContext( disp, view->ctx );
            goto fail_fb;
        }

        /* GLX requires a colormap (see the glXIntro man page). */

        attr.event_mask   = DEFAULT_INPUT;
        attr.border_pixel = BlackPixel( disp, vi->screen );
        attr.colormap = XCreateColormap( disp,
                                         RootWindow( disp, vi->screen ),
                                         vi->visual, AllocNone );

        view->window = XCreateWindow( disp, RootWindow( disp, vi->screen ),
                                      0, 0, 256, 256,
                                      0, vi->depth, InputOutput, vi->visual,
                                      CWEventMask | CWBorderPixel | CWColormap,
                                      &attr );
        XFree( vi );
    }

    XFree( fbCfg );

    /* Enable the delete window protocol. */
    view->deleteAtom = XInternAtom( disp, "WM_DELETE_WINDOW", False );
    XSetWMProtocols( disp, view->window, &view->deleteAtom, 1 );

    glv_makeCurrent( view );
    return view;

fail_fb:
    XFree( fbCfg );
fail_view:
    free( view );
fail_disp:
    XCloseDisplay( disp );
    return 0;
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
            glXMakeCurrent( disp, None, NULL );     // Release context.
            glXDestroyContext( disp, view->ctx );
            view->ctx = 0;
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


/**
  Returns vertical dots per inch of display.
*/
int glv_dpi( GLView* view )
{
    Display* d = view->display;
    int s = view->screen;
    double vr = ((double) DisplayHeight(d, s) * 25.4) /
                 (double) DisplayHeightMM(d, s);
    return (int) (vr + 0.5);
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
#ifdef USE_XF86VMODE
    GLViewMode vmode;
    Display* disp;
    int screen;
    int modeCount = 0;


    disp = XOpenDisplay( 0 );
    if( ! disp )
        return 0;

    screen = DefaultScreen( disp );

    /*int XDefaultDepth( display, screen );*/
    /*int XDisplayPlanes( display, screen );*/

    /* X11 cannot change depth on the fly(?) */
    vmode.depth = XDisplayPlanes( disp, screen );

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

    XCloseDisplay( disp );

    return modeCount;
#else
    return 0;
#endif
}


#if 0
static void _mapRaisedWait( Display* disp, Window win )
{
    XWindowAttributes attr;
    XEvent event;

    /* Ensure the window is unmapped to prevent hang in loop below. */
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


#ifdef USE_XF86VMODE
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
#endif


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


#ifdef USE_XF86VMODE
/*
  Determine current mode so it can be restored later.
  Caller must free() the returned pointer .
*/
static void _changeVideoMode( GLView* view, const GLViewMode* mode )
{
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
}
#endif


#if 0
static void _stateQuery(const GLView* view)
{
    Property prop;

    getProperty(view, &prop, view->wmAtom[0], 8, XA_ATOM);
    printf("WM_STATE type:%ld fmt:%d num:%ld rem:%ld data:%p\n",
            prop.type, prop.format, prop.count, prop.remain, prop.data);

    if (prop.count) {
        char* name;
        Atom* atoms = (Atom*) prop.data;
        long unsigned int i;
        for (i = 0; i < prop.count; ++i) {
            name = XGetAtomName(view->display, atoms[i]);
            printf("  atom %ld %s\n", atoms[i], name);
            XFree(name);
        }
    }

    XFree(prop.data);
}
#endif


static void _resetSizeHints(GLView* view)
{
    XSizeHints* xsh = XAllocSizeHints();
    if (xsh) {
        xsh->flags = 0;
        XSetWMNormalHints(view->display, view->window, xsh);
        XFree(xsh);
    }
}


/*
   \param action    0 = unset, 1 = set, 2 = toggle
*/
static int _stateFullscreen(GLView* view, int action, int setComposite)
{
    static const char* names[3] = {
        "_NET_WM_STATE",
        "_NET_WM_STATE_FULLSCREEN",
        "_NET_WM_BYPASS_COMPOSITOR"
    };
    XEvent xev;
    Atom wm_state;
    Display* disp = view->display;


    if (! view->wmAtom[0]) {
        if (! XInternAtoms(disp, (char**) names, 3, True, view->wmAtom)) {
            fprintf(stderr, "_NET_WM_STATE atoms do not exist\n");
            return 0;
        }
    }
    wm_state = view->wmAtom[0];


    // Ensure the _NET_WM_STATE window property exists, or else sending the
    // state change message will have no effect.  It won't exist on a new,
    // unmapped window.
    {
    Property prop;
    getProperty(view, &prop, wm_state, 4, XA_ATOM);
    XFree(prop.data);

    //printf("WM_STATE type:%ld fmt:%d num:%ld rem:%ld prop:%p\n",
    //        type, format, count, remain, prop);

    if (! prop.count) {
        XChangeProperty(disp, view->window, wm_state, XA_ATOM, 32,
                        PropModeReplace,
                        (unsigned char*) (view->wmAtom + 1), 1);
    }
    }


    xev.xclient.type    = ClientMessage;
    xev.xclient.serial  = 0;
    xev.xclient.send_event = True;
    xev.xclient.window  = view->window;
    xev.xclient.message_type = wm_state;
    xev.xclient.format  = 32;

    xev.xclient.data.l[0] = action;
    xev.xclient.data.l[1] = view->wmAtom[1];    // _NET_WM_STATE_FULLSCREEN
    xev.xclient.data.l[2] = 0;
    xev.xclient.data.l[3] = 1;      // Normal application source.
    xev.xclient.data.l[4] = 0;

    XSendEvent(disp, RootWindow(disp, view->screen), False,
               SubstructureRedirectMask | SubstructureNotifyMask,
               &xev);

    if (setComposite) {
        // Set _NET_WM_BYPASS_COMPOSITOR property.
        unsigned long bypass = (action == 1) ? 1 : 0;
        XChangeProperty(disp, view->window, view->wmAtom[2], XA_CARDINAL, 32,
                        PropModeReplace, (unsigned char*) &bypass, 1);
    }

    return 1;
}


/*
  Wait for a specific type of XEvent.  The event is discarded.

  \param timeout    Milliseconds to wait.

  \return Non-zero if the event occured before the timeout period.
*/
static int _waitXEvent(Display* disp, int etype, int timeout)
{
    XEvent event;
    struct pollfd pread;

    pread.fd = ConnectionNumber(disp);
    pread.events = POLLIN;

    while (1) {
        int n = poll(&pread, 1, timeout);
        if (n < 0) {
            fprintf(stderr, "X11 socket wait failed!\n");
            return 0;
        }
        if (n == 0)
            return 0;       // Timed out.

        while (XPending(disp)) {
            XNextEvent(disp, &event);
            //printf("KR waitXEvent %d\n", event.type);
            if (event.type == etype)
                return 1;
        }
    }
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
    int newModeFS = (mode->id > GLV_MODEID_WINDOW) ? 1 : 0;
    int fullWindowTransition = 0;


    /* Return if mode is current */

    if( oldModeFS == newModeFS )
    {
        if( mode->id == GLV_MODEID_FULL_WINDOW ) {
            if( view->flags & FLAG_FULLWINDOW_MODE )
                return 1;
        } else {
            if( mode->width == view->width && mode->height == view->height )
                return 1;
        }
    }


#ifdef USE_XF86VMODE
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

        if( mode->id == GLV_MODEID_FULL_WINDOW )
        {
            /* Ensure the window is resizable or some window managers may
               not transition to the fullscreen state. */
            _resetSizeHints(view);

            /* Fallback to mode size if fullscreen fails. */
            if (! view->width)
                XResizeWindow( disp, window, mode->width, mode->height );

            if (_stateFullscreen(view, 1, True)) {
                view->flags |= FLAG_FULLWINDOW_MODE;
                fullWindowTransition = 1;
            }
        }
        else
        {
            if (view->flags & FLAG_FULLWINDOW_MODE) {
                _stateFullscreen(view, 0, True);
                view->flags &= ~FLAG_FULLWINDOW_MODE;
                fullWindowTransition = 1;
            }

            if( mode->id == GLV_MODEID_FIXED_WINDOW ) {
                XSizeHints* xsh = XAllocSizeHints();
                if (xsh) {
                    xsh->flags = PMinSize | PMaxSize;
                    xsh->min_width  = xsh->max_width  = mode->width;
                    xsh->min_height = xsh->max_height = mode->height;
                    XSetWMNormalHints(disp, window, xsh);
                    XFree(xsh);
                }
            } else
                _resetSizeHints(view);

            XResizeWindow( disp, window, mode->width, mode->height );
        }

        XMapRaised( disp, window );
    }


    XSync( disp, True );
    glXMakeCurrent( disp, window, view->ctx );


    /* Generate GLV_EVENT_RESIZE
     * The geometry is queried (rather than assuming the requested size is
     * used) just to be safe.
     */
    {
    GLViewEvent ve;
    XWindowAttributes attr;

    XGetWindowAttributes( disp, window, &attr );
    if( attr.map_state != IsViewable )
    {
        /* Wait to be mapped or else the XCreateWindow size may be returned. */
        _waitXEvent(disp, MapNotify, 500);

        /* The transition to _NET_WM_STATE_FULLSCREEN may take a moment
           and a ConfigureNotify event with the previous size may occur
           before the fullscreen size is set. */
        if (fullWindowTransition)
            _waitXEvent(disp, ConfigureNotify, 330);

        XGetWindowAttributes(disp, window, &attr);
    }
    else if (fullWindowTransition)
    {
        _waitXEvent(disp, ConfigureNotify, 330);
        XGetWindowAttributes(disp, window, &attr);
    }

    view->width  = attr.width;
    view->height = attr.height;

    ve.type = GLV_EVENT_RESIZE;
    ve.x    = attr.width;
    ve.y    = attr.height;
    view->eventHandler( view, &ve );
    }

    //_report( disp, window );
    return 1;
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
  This should only be called when the view was created with #GLV_MODEID_WINDOW
  or #GLV_MODEID_FIXED_WINDOW.
*/
void glv_move( GLView* view, int x, int y )
{
    XMoveWindow( view->display, view->window, x, y );
}


/**
  Sets window dimensions.
  This should only be called when the view was created with #GLV_MODEID_WINDOW.
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

  Now using XkbKeycodeToKeysym since XKeycodeToKeysym is deprecated.
*/
#define KEYSYM(e)   XkbKeycodeToKeysym( view->display, e.xkey.keycode, 0, 0 )


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
                if( event.xexpose.count == 0 )
                {
                    ve.type = GLV_EVENT_EXPOSE;
                    view->eventHandler( view, &ve );
                }
                break;
#if 0
            case PropertyNotify:
                if (event.xproperty.atom == view->wmAtom[0] &&
                    event.xproperty.state == 0)
                    _stateQuery(view);
                break;
#endif
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
    XFree( data );
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
