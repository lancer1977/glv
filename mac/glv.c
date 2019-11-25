/*===========================================================================/

  GLV Library for Mac OS X
  Copyright (C) 2004-2007 Karl Robillard

  TODO:
    [ ] Fullscreen
    [ ] Test glv_queryModes on CRT (powerbook only reports one mode @ 0Hz)
    [ ] Enable Apple/Application menu.
    [ ] properly implement handleWindowDMEvent
    [?] Icon snapshot when minimized.

/===========================================================================*/


#include <stdio.h>
#include <stdlib.h>
#include <glv.h>
#include <glv_keys.h>


#define FLAG_ATTRIB                 0x0007
#define FLAG_FULLSCREEN_MODE        0x0040
#define FLAG_FILTER_REPEAT          0x0080


static UInt32 mouseButtonModifier[4] =
{
    0, GLV_MASK_LEFT, GLV_MASK_RIGHT, GLV_MASK_MIDDLE
};


static void glv_nullHandler( void* v, GLViewEvent* e ) {}


#define GET_KEY_MOD(e,m) \
     GetEventParameter( e, kEventParamKeyModifiers, \
                        typeUInt32, NULL, sizeof(UInt32), NULL, m )

#define CHECK_MOD(md,cd) \
    if( changed & (md) ) { \
        ve.type = (modifiers & (md)) ? GLV_EVENT_KEY_DOWN : GLV_EVENT_KEY_UP; \
        ve.code = cd; \
        view->eventHandler( view, &ve ); }

/*
  NOTE: The modifiers are not the key state!  E.G. The alphaLock will be set
  when the Caps Lock LED is on and clear when the LED is off.
*/
static void modifersChanged( GLView* view, UInt32 modifiers )
{
    GLViewEvent ve;
    UInt32 changed = modifiers ^ view->modifiers;

    ve.state = modifiers;
    ve.x     = 0;
    ve.y     = 0;

    //printf( "KR mod %04lx\n", modifiers );

    CHECK_MOD( shiftKey,        KEY_Shift_L )
    CHECK_MOD( rightShiftKey,   KEY_Shift_R )
    CHECK_MOD( controlKey,      KEY_Control_L )
    CHECK_MOD( rightControlKey, KEY_Control_R )
    CHECK_MOD( optionKey,       KEY_Alt_L )
    CHECK_MOD( rightOptionKey,  KEY_Alt_R )
    CHECK_MOD( cmdKey,          KEY_Meta_L )
    CHECK_MOD( alphaLock,       KEY_Caps_Lock )

    view->modifiers = modifiers;
}


#define COPY_KEY(ve,t) \
        ve.type  = t; \
        ve.code  = code; \
        ve.state = modifiers | view->buttonsHeld; \
        ve.x     = 0; \
        ve.y     = 0;

static pascal OSStatus windowEventHandler( EventHandlerCallRef myHandler,
                                           EventRef event, void* userData )
{
    GLViewEvent ve;
    GLView* view = (GLView*) userData;

    OSStatus result = eventNotHandledErr;
    UInt32   class  = GetEventClass(event);
    UInt32   kind   = GetEventKind(event);

#if 0
    result = CallNextEventHandler( myHandler, event );
    if( eventNotHandledErr != result )
        return result;
#endif

    switch( class )
    {
        case kEventClassKeyboard:
        {
            UInt32 code = 0;
            UInt32 modifiers = 0;

            switch (kind)
            {
            case kEventRawKeyDown:

                GetEventParameter( event, kEventParamKeyCode,
                                   typeUInt32, NULL,
                                   sizeof(UInt32), NULL, &code );
                GET_KEY_MOD( event, &modifiers );

                COPY_KEY( ve, GLV_EVENT_KEY_DOWN )
                view->eventHandler( view, &ve );
                //printf( "KR key down %x %x\n", (int) code, (int) modifiers );
                break;

            case kEventRawKeyUp:
                //printf( "KR key up\n" );

                GetEventParameter( event, kEventParamKeyCode,
                                   typeUInt32, NULL,
                                   sizeof(UInt32), NULL, &code );
                GET_KEY_MOD( event, &modifiers );

                COPY_KEY( ve, GLV_EVENT_KEY_UP )
                view->eventHandler( view, &ve );
                break;

            case kEventRawKeyModifiersChanged:
                GET_KEY_MOD( event, &modifiers );
                modifersChanged( view, modifiers );
                break;
            }
        }
            break;

        case kEventClassMouse:
        {
            EventMouseButton button;
            HIPoint location;
            UInt32 modifiers = 0;

            // NOTE: kEventParamWindowMouseLocation returns point in
            // structure coordinates (includes title bar).  Because of this
            // we need to use kEventParamMouseLocation and keep the rect of
            // contents in GLView bound member.

            switch (kind)
            {
            case kEventMouseDown:
                //printf( "KR mouse down\n" );

                // Let system widgets get first crack at event.
                result = CallNextEventHandler( myHandler, event );
                if( eventNotHandledErr != result )
                    break;

                GetEventParameter( event, kEventParamMouseButton,
                                   typeMouseButton, NULL,
                                   sizeof(EventMouseButton), NULL, &button );
                GetEventParameter( event, kEventParamMouseLocation,
                                   typeHIPoint, NULL,
                                   sizeof(HIPoint), NULL, &location );
                GetEventParameter( event, kEventParamKeyModifiers,
                                   typeUInt32, NULL,
                                   sizeof(UInt32), NULL, &modifiers );

                if( button < 4 )
                    view->buttonsHeld |= mouseButtonModifier[ button ];

                ve.type  = GLV_EVENT_BUTTON_DOWN;
                ve.code  = button;
                ve.state = modifiers | view->buttonsHeld;
                ve.x     = ((int) location.x) - view->bound.left;
                ve.y     = ((int) location.y) - view->bound.top;
                view->eventHandler( view, &ve );

                result = noErr;
                break;

            case kEventMouseUp:
                //printf( "KR mouse up\n" );

                GetEventParameter( event, kEventParamMouseButton,
                                   typeMouseButton, NULL,
                                   sizeof(EventMouseButton), NULL, &button );
                GetEventParameter( event, kEventParamMouseLocation,
                                   typeHIPoint, NULL,
                                   sizeof(HIPoint), NULL, &location );
                GetEventParameter( event, kEventParamKeyModifiers,
                                   typeUInt32, NULL,
                                   sizeof(UInt32), NULL, &modifiers );

                if( button < 4 )
                    view->buttonsHeld &= ~mouseButtonModifier[ button ];

                ve.type  = GLV_EVENT_BUTTON_UP;
                ve.code  = button;
                ve.state = modifiers | view->buttonsHeld;
                ve.x     = ((int) location.x) - view->bound.left;
                ve.y     = ((int) location.y) - view->bound.top;
                view->eventHandler( view, &ve );

                result = noErr;
                break;

            case kEventMouseMoved:
            case kEventMouseDragged:
                //printf( "KR mouse moved\n" );

                GetEventParameter( event, kEventParamMouseButton,
                                   typeMouseButton, NULL,
                                   sizeof(EventMouseButton), NULL, &button );
                GetEventParameter( event, kEventParamMouseLocation,
                                   typeHIPoint, NULL,
                                   sizeof(HIPoint), NULL, &location );
                GetEventParameter( event, kEventParamKeyModifiers,
                                   typeUInt32, NULL,
                                   sizeof(UInt32), NULL, &modifiers );

                ve.type  = GLV_EVENT_MOTION;
                ve.code  = button;
                ve.state = modifiers | view->buttonsHeld;
                ve.x     = ((int) location.x) - view->bound.left;
                ve.y     = ((int) location.y) - view->bound.top;
                view->eventHandler( view, &ve );

                result = noErr;
                break;

            case kEventMouseWheelMoved:
                {
                // Example uses long, docs use SInt32.
                long wheelDelta = 0;

                //printf( "KR wheel\n" );

                GetEventParameter( event, kEventParamMouseWheelDelta,
                                   typeLongInteger, NULL,
                                   sizeof(long), NULL, &wheelDelta );
                GetEventParameter( event, kEventParamKeyModifiers,
                                   typeUInt32, NULL,
                                   sizeof(UInt32), NULL, &modifiers );

                ve.type  = GLV_EVENT_WHEEL;
                ve.code  = 0;
                ve.state = modifiers;
                ve.x     = 0;
                ve.y     = (int) wheelDelta /*GLV_WHEEL_DELTA*/;
                view->eventHandler( view, &ve );

                result = noErr;
                }
                break;
            }
        }
            break;

        case kEventClassWindow:
            switch (kind)
            {
            case kEventWindowClose:
	            //printf( "KR close\n" );
	            ve.type = GLV_EVENT_CLOSE;
	            view->eventHandler( view, &ve );
                result = noErr;
                break;

            case kEventWindowBoundsChanged:
            {
                Rect* rect = &view->bound;

                // PortBounds left & top are always zero.
                //GetWindowPortBounds( view->window, rect );

                GetWindowBounds( view->window, kWindowContentRgn, rect );

                aglUpdateContext( view->ctx );

                ve.x = rect->right - rect->left;
                ve.y = rect->bottom - rect->top;

                if( (view->width != ve.x) || (view->height != ve.y) )
                {
                    // Only send event if size changed, not just position.

                    view->width  = ve.x;
                    view->height = ve.y;

                    ve.type = GLV_EVENT_RESIZE;
                    view->eventHandler( view, &ve );
                }
                result = noErr;
            }
                break;

            default:
	            //printf( "KR window event %u\n", (unsigned int) kind );
                break;
            }
            break;

        default:
            printf( "KR event class %u\n", (unsigned int) class );
            break;
    }

    return result;
}


/*
  Handle display config changes meaing we need to update the GL context via
  the resize function and check for windwo dimension changes also note we
  redraw the content here as it could be lost in a display config change
*/
void handleWindowDMEvent( void* userData, short msg, void* notifyData )
{
    // Post change notifications only.
    if( kDMNotifyEvent == msg )
    {
        GLView* view = (GLView*) userData;
        if( view->window )       // have a valid OpenGl window
        {
            Rect rectPort;
            CGRect viewRect = {{0.0f, 0.0f}, {0.0f, 0.0f}};

/*
            sprintf( pContextInfo->message,
                     "Event: Display Change at %0.1f secs", getElapsedTime() );
            view->msgTime = getElapsedTime();
*/
            GetWindowPortBounds( view->window, &rectPort );

            viewRect.size.width  = (float) (rectPort.right - rectPort.left);
            viewRect.size.height = (float) (rectPort.bottom - rectPort.top);

            // update context and handle possible resizes
            //resizeGL( view, viewRect );

            // force redrow
            InvalWindowRect( view->window, &rectPort );
        }
    }
}


OSStatus aglReportError()
{
    GLenum err = aglGetError();
/*
    if( AGL_NO_ERROR != err )
        reportError( (char*) aglErrorString(err) );
*/
    // Ensure we are returning an OSStatus noErr if no error condition.
    if (err == AGL_NO_ERROR)
        return noErr;
    else
        return (OSStatus) err;
}


OSStatus buildGL( GLView* view )
{
    OSStatus err = noErr;
    GLint attrib[] = { AGL_RGBA,
                       AGL_DOUBLEBUFFER,
                       AGL_DEPTH_SIZE, 16,
                       AGL_SAMPLE_BUFFERS_ARB, 1,
                       AGL_SAMPLES_ARB, 4, AGL_NO_RECOVERY,
                       AGL_NONE };

    //ProcessSerialNumber psn = { 0, kCurrentProcess };

    // Build context.

    if( ! (view->flags & GLV_ATTRIB_MULTISAMPLE) )
        attrib[ 4 ] = AGL_NONE;

    view->ctx        = NULL;
    view->pixFormat  = aglChoosePixelFormat(NULL, 0, attrib);
    aglReportError();

    if( view->pixFormat )
    {
        view->ctx = aglCreateContext(view->pixFormat, NULL);
        aglReportError();
    }

    if( view->ctx )
    {
        GLint swap = 1;

#if 0
        // Deprecated in Mac OS X v10.4
        GrafPtr portSave = NULL;
        GetPort( &portSave );
        SetPort( (GrafPtr) GetWindowPort(view->window) );
#endif

        if( ! aglSetDrawable(view->ctx, GetWindowPort(view->window)) )
            err = aglReportError();

        if( ! aglSetCurrentContext(view->ctx) )
            err = aglReportError();

        // VBL SYNC
        if( ! aglSetInteger(view->ctx, AGL_SWAP_INTERVAL, &swap) )
            aglReportError();

#if 0
        switch( view->modeFSAA )
        {
            case kFSAAOff:
                glDisable(GL_MULTISAMPLE_ARB);
                break;
            case kFSAAFast:
                glEnable(GL_MULTISAMPLE_ARB);
                glHint(GL_MULTISAMPLE_FILTER_HINT_NV, GL_FASTEST);
                break;
            case kFSAANice:
                glEnable(GL_MULTISAMPLE_ARB);
                glHint(GL_MULTISAMPLE_FILTER_HINT_NV, GL_NICEST);
                break;
        }
#endif

#if 0
        // Ensure we know when display configs are changed.
        view->windowEDMUPP = NewDMExtendedNotificationUPP(handleWindowDMEvent);
        DMRegisterExtendedNotifyProc( view->windowEDMUPP, (void*) view,
                                      NULL, &psn );

        SetPort( portSave );
#endif
    }

    return err;
}


static void disposeGL( GLView* view )
{
#if 0
    if( view->windowEDMUPP )
    {
        DisposeDMExtendedNotificationUPP (view->windowEDMUPP);
        view->windowEDMUPP = NULL;
    }
#endif

    aglSetCurrentContext( NULL );
    aglSetDrawable( view->ctx, NULL );

    if( view->ctx )
    {
        aglDestroyContext (view->ctx);
        view->ctx = NULL;
    }

    if( view->pixFormat )
    {
        aglDestroyPixelFormat (view->pixFormat);
        view->pixFormat = NULL;
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
    EventHandlerRef ref;
    EventTypeSpec list[] = {
        { kEventClassWindow, kEventWindowCollapsing },                                  { kEventClassWindow, kEventWindowCollapsed },
        { kEventClassWindow, kEventWindowShown },
        { kEventClassWindow, kEventWindowActivated },
        { kEventClassWindow, kEventWindowClose },
        { kEventClassWindow, kEventWindowDrawContent },
        { kEventClassWindow, kEventWindowBoundsChanged },
        { kEventClassWindow, kEventWindowZoomed },
        { kEventClassKeyboard, kEventRawKeyDown },
        { kEventClassKeyboard, kEventRawKeyUp },
        { kEventClassKeyboard, kEventRawKeyModifiersChanged },
		{ kEventClassMouse, kEventMouseDown },
		{ kEventClassMouse, kEventMouseUp }, 
		{ kEventClassMouse, kEventMouseMoved },
		{ kEventClassMouse, kEventMouseDragged },
		{ kEventClassMouse, kEventMouseWheelMoved }
    };


    view = (GLView*) calloc( 1, sizeof(GLView) );
    if( ! view )
    {
        return( 0 );
    }

    //printf( "KR glv_create\n" );

    view->window         = NULL;
    view->pixFormat      = NULL;
    view->ctx            = NULL;
    view->flags          = attributes;
    view->modeId         = -2;
    view->eventHandler   = glv_nullHandler;
    view->windowEventUUP = NewEventHandlerUPP( windowEventHandler );
    view->modifiers      = 0;
    view->buttonsHeld    = 0;


    OSStatus status;
    Rect bound;
    WindowAttributes wattr;

#if 0
    wattr = kWindowStandardDocumentAttributes;
#else
    // Must enable kWindowStandardHandlerAttribute to get close and maximize
    // to work.
    wattr = kWindowCloseBoxAttribute |
            kWindowFullZoomAttribute |
            kWindowCollapseBoxAttribute |
            kWindowResizableAttribute |
            kWindowStandardHandlerAttribute |
            kWindowLiveResizeAttribute;
#endif

    bound.top    = 50;
    bound.left   = 0;
    bound.bottom = 250;
    bound.right  = 320;

    status = CreateNewWindow( kDocumentWindowClass, wattr, &bound,
                              &view->window );
    if( status == noErr )
    {
        SetWRefCon( view->window, (long) view );

#if 0
        InstallEventHandler( GetApplicationEventTarget(), view->appEventUUP,
                             GetEventTypeCount(alist), alist,
                             (void*) view, NULL );
#endif

        InstallWindowEventHandler( view->window, view->windowEventUUP,
                                   GetEventTypeCount(list), list,
                                   (void*) view, &ref );

        buildGL( view );
        //glv_makeCurrent( view );

        RepositionWindow( view->window, NULL, kWindowCenterOnMainScreen );
        return( view );
    }
    else
    {
        fprintf( stderr, "ERROR CreateNewWindow %d\n", (int) status );
        free( view );
        return( 0 );
    }
}


#if 0
/* Restore original video mode. */
static void _restoreVideo( GLView* view )
{
    if( view->flags & FLAG_FULLSCREEN_MODE )
    {
        view->flags &= ~FLAG_FULLSCREEN_MODE;
    }
}
#endif


/**
  Closes the GLView and frees any used resources.
*/
void glv_destroy( GLView* view )
{
    printf( "KR glv_destroy\n" );
    if( view )
    {
        if( view->window )
        {
            HideWindow( view->window );
            disposeGL( view );
            DisposeWindow( view->window );
        }

        free( view );
    }
}


/**
  Returns a mask of GL_ATTRIB_* bits which apply to the view.
*/
int glv_attributes( GLView* view )
{
    return view->flags & FLAG_ATTRIB;
}


// local CF dictionary routines

static long _getDictLong (CFDictionaryRef refDict, CFStringRef key)
{
    long int_value;
    CFNumberRef num_value = (CFNumberRef)CFDictionaryGetValue(refDict, key);
    if (!num_value) // if can't get a number for the dictionary
        return -1;  // fail
    // or if cant convert it
    if (!CFNumberGetValue(num_value, kCFNumberLongType, &int_value))
        return -1; // fail
    return int_value; // otherwise return the long value
}


static double _getDictDouble (CFDictionaryRef refDict, CFStringRef key)
{
    double double_value;
    CFNumberRef num_value = (CFNumberRef)CFDictionaryGetValue(refDict, key);
    if (!num_value) // if can't get a number for the dictionary
        return -1;  // fail
    // or if cant convert it
    if (!CFNumberGetValue(num_value, kCFNumberDoubleType, &double_value))
        return -1; // fail
    return double_value; // otherwise return the long value
}


/**
  Calls func for each mode and returns the number of available fullscreen
  modes.

  \sa glv_changeMode()
*/
int glv_queryModes( GLViewMode_f func, void* data )
{
    CGDirectDisplayID dspys[ 32 ];
    CGDisplayCount count;
    CGDisplayErr err;

    err = CGGetActiveDisplayList( 32, dspys, &count );
    if( err )
        return( 0 );

    if( func )
    {
        GLViewMode vmode;
        unsigned int i;

        for( i = 0; i < count; ++i )
        {
            CGRect rect = CGDisplayBounds( dspys[i] );
            CFDictionaryRef dm = CGDisplayCurrentMode( dspys[i] );

            vmode.id          = i;
            vmode.width       = rect.size.width;
            vmode.height      = rect.size.height;
            vmode.refreshRate = (int) (_getDictDouble( dm,
                                           kCGDisplayRefreshRate) + 0.5);
            vmode.depth       = _getDictLong( dm, kCGDisplayBitsPerPixel );

            (*func)( &vmode, data );
#if 0
            printf( "%d: %dx%d  dot: %d ht: %d vt: %d\n", i,
                    vmi->hdisplay, vmi->vdisplay, vmi->dotclock,
                    vmi->htotal, vmi->vtotal );
#endif
        }
    }

    return( count );
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
    WindowRef window = view->window;


    if( mode->id == view->modeId )
        return( 1 );

    ShowWindow( window );
    SelectWindow( window );
    //SetUserFocusWindow( window );

#if 0
    if( mode->id == GLV_MODEID_WINDOW )
    {
            if( view->flags & FLAG_FULLSCREEN_MODE )
            {
                _restoreVideo( view );

                /* override_redirect does not take affect unless we unmap */
                _unmap( disp, window );

                attr.override_redirect = False;
                XChangeWindowAttributes( disp, window,
                                         CWOverrideRedirect, &attr );
            }
    }
    else
    {
            /* override_redirect does not take affect unless we unmap */
            _unmap( disp, window );

            attr.override_redirect = True;
            XChangeWindowAttributes( disp, window,
                                     CWOverrideRedirect, &attr );

            view->flags |= FLAG_FULLSCREEN_MODE;
    }
#endif

    if( view->flags & FLAG_FULLSCREEN_MODE )
    {
#if 0
        XMoveResizeWindow( disp, window, 0, 0, mode->width, mode->height );
        _map( disp, window );

        XGrabKeyboard( disp, window, True,
                       GrabModeAsync, GrabModeAsync, CurrentTime );
        XWarpPointer( disp, None, window, 0, 0, 0, 0,
                      mode->width / 2, mode->height / 2 );
        XGrabPointer( disp, window, True, 0,
                      GrabModeAsync, GrabModeAsync,
                      window, None, CurrentTime );
#endif
    }
    else
    {
        SizeWindow( view->window, mode->width, mode->height, false );
        //_map( disp, window );
    }

    aglSetCurrentContext( view->ctx );
    aglUpdateContext( view->ctx );
    view->modeId = mode->id;

    {
    GLViewEvent ve;
    Rect* rect = &view->bound;

    GetWindowBounds( window, kWindowContentRgn, rect );

    view->width  = rect->right - rect->left;
    view->height = rect->bottom - rect->top;

    ve.type = GLV_EVENT_RESIZE;
    ve.x    = view->width;
    ve.y    = view->height;
    view->eventHandler( view, &ve );
    }

    return( 1 );
}


/**
  Swaps the GL buffers of the view.
*/
void glv_swapBuffers( GLView* view )
{
    aglSwapBuffers( view->ctx );
}


/**
  Makes the GL context of the view current.
*/
void glv_makeCurrent( GLView* view )
{
    aglSetCurrentContext( view->ctx );
}


/**
  Makes the window visible.
*/
void glv_show( GLView* view )
{
    ShowWindow( view->window );
}


/**
  Hides the window.
*/
void glv_hide( GLView* view )
{
    HideWindow( view->window );
}


/**
  Sets window title and icon name.
*/
void glv_setTitle( GLView* view, const char* title )
{
    CFStringRef str;
    str = CFStringCreateWithCString( NULL, title, kCFStringEncodingASCII );
    SetWindowTitleWithCFString( view->window, str );
    CFRelease( str );
}


/**
  Positions window on screen.
  Should only be called when in windowed mode.
*/
void glv_move( GLView* view, int x, int y )
{
    MoveWindow( view->window, x, y, false );
}


/**
  Sets window dimensions.
  Should only be called when in windowed mode.
*/
void glv_resize( GLView* view, int w, int h )
{
    SizeWindow( view->window, w, h, false );
}


/**
  Show window on top of all other windows.
*/
void glv_raise( GLView* view )
{
    SelectWindow( view->window );
}


/**
  Minimizes the window.
*/
void glv_iconify( GLView* view )
{
    CollapseWindow( view->window, false );
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
        CGDisplayShowCursor( kCGDirectMainDisplay );
    }
    else
    {
        // TODO: Every call to HideCursor should be balanced with a ShowCursor.
        CGDisplayHideCursor( kCGDirectMainDisplay );
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


/**
  Waits until an event is recieved.

  \sa glv_handleEvents()
*/
void glv_waitEvent( GLView* view )
{
    EventRef event;
    ReceiveNextEvent( 0, NULL, kEventDurationForever, false, &event );

#if 0
    EventRef event;
    EventTargetRef target;
    
    target = GetEventDispatcherTarget();
    while( ReceiveNextEvent( 0, NULL, kEventDurationForever, true, &event )
           == noErr )
    {
        SendEventToEventTarget( event, target );
        ReleaseEvent( event );
    }
#endif
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
#if 1
    EventRef event;
    EventTargetRef target;
    
    //printf( "KR handle\n" );

    target = GetEventDispatcherTarget();
    while( ReceiveNextEvent( 0, NULL, kEventDurationNoWait, true, &event )
           == noErr )
    {
#if 0
        UInt32 class  = GetEventClass(event);
        UInt32 kind   = GetEventKind(event);
        printf( "KR loop %c%c%c%c %d\n",
            (char) (class >> 24),
            (char) ((class >> 16) & 0xff),
            (char) ((class >> 8)  & 0xff),
            (char) (class & 0xff),
            (int) kind );
#endif

        SendEventToEventTarget( event, target );
        ReleaseEvent( event );
    }
#else
    RunApplicationEventLoop();
#endif

#if 0
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
    {
        XNextEvent( view->display, &event );

        switch( event.type )
        {
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
    // TODO
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
    // TODO
    (void) view;
    (void) func;
    (void) user;

    return 0;
}


/*EOF*/
