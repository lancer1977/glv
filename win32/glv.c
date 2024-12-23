/*===========================================================================/

  GLV Library for Windows
  Copyright (C) 2003-2024  Karl Robillard
  SPDX-License-Identifier: MIT

/===========================================================================*/


#include <windows.h>
#include <zmouse.h>
#include <stdio.h>
#include <glv.h>


#define FLAG_ATTRIB                 0x0007
#define FLAG_FULLSCREEN_MODE        0x0010
#define FLAG_FULLWINDOW_MODE        0x0020
#define FLAG_FILTER_REPEAT          0x0040


static GLView* _cv = 0;

static void glv_nullHandler( void* v, GLViewEvent* e )
{
    (void) v;
    (void) e;
}


/*--------------------------------------------------------------------------*/


static HINSTANCE gAppInstance;
static WPARAM _keyWParam;
static LPARAM _keyLParam;

#ifdef GLV_HID_KEY
// USB HID Usage Ids for keyboards (Page 7)
// https://www.usb.org/hid

static const unsigned char _virtualKeyToHid[176] = {
    0,0,0,0,0,0,0,0,
    0x2a,0x2b,      // 0x08-09 BACKSPACE, TAB
    0,0,            // Reserved
    0,0x28,         // 0x0C-0D CLEAR, ENTER
    0,0,            // Reserved
    0,0,0,          // 0x10-12 SHIFT, CTRL, ALT
    0x48,0x39,      // 0x13-14 Pause, CapsLock
    0,0,0,0,0,0,    // 0x15-1A IME keys
    0x29,0,0,0,0,               // 0x1B-1F ESC, IME...
    0x32,0x4b,0x4e,0x4d,0x4a,   // 0x20-24 SPACEBAR, PageUp, PageDn, End, Home
    0x50,0x52,0x4f,0x51,0x77,   // 0x25-29 Left, Up, Right, Down, Select
       0,0x74,0x46,0x49,0x4c,   // 0x2A-2E Print, Execute, PrScr, Ins, Del
    0x75,                       // 0x2F Help
    0x27,0x1e,0x1f,0x20,0x21,0x22,0x23,0x24,0x25,0x26,  // 0x30-39 0-9
    0,0,0,0,0,0,0,                                      // 0x3A-40 Undefined
     4, 5, 6, 7, 8, 9,10,11,12,13,                      // 0x41-5A A-Z
    14,15,16,17,18,19,20,21,22,23,
    24,25,26,27,28,29,
    0xe3,0xe7,0x65,0,0x66,                              // 0x5B-5F LWindows...
    0x62,0x59,0x5a,0x5b,0x5c,0x5d,0x5e,0x5f,0x60,0x61,  // 0x60-69 Keypad 0-9
    0x55,0x57,0,0x56,0x63,0x54,                         // 0x6A-6F Multiply...
    //        ^ Separator
    0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,0x40,0x41,0x42,0x43,  // 0x70-87 F1-F24
    0x44,0x45,0x68,0x69,0x6a,0x6b,0x6c,0x6d,0x6e,0x6f,
    0x70,0x71,0x72,0x73,
    0,0,0,0,0,0,0,0,                    // 0x88-8F Reserved
    0x83,0x84,                          // 0x90-91 NumLock, Scroll
    0,0,0,0,0,                          // 0x92-96 OEM specific
    0,0,0,0,0,0,0,0,0,                  // 0x97-9F Unassigned
    0xe1,0xe5,0xe0,0xe4,0xe2,0xe6,      // 0xA0-A5 LShift...
    0,0,0,0,0,0,0,                      // 0xA6-AC Browser keys
    0x7f,0x81,0x80                      // 0xAD-AF Volume Mute...
                                        // 0xB0 ...
};

#define VIRTKEY(vk)  ((vk < 0xB0) ? _virtualKeyToHid[vk] : 0)
#else
#define VIRTKEY(vk)  vk
#endif

#define COPY_KEY(ve,t) \
    _keyLParam = lParam; \
    _keyWParam = wParam; \
        ve.type  = t; \
        ve.code  = VIRTKEY(wParam); \
        ve.state = 0; \
    if( GetKeyState( VK_SHIFT )   & 0x8000 ) ve.state |= GLV_MASK_SHIFT; \
    if( GetKeyState( VK_CONTROL ) & 0x8000 ) ve.state |= GLV_MASK_CTRL; \
    if( GetKeyState( VK_MENU )    & 0x8000 ) ve.state |= GLV_MASK_ALT; \
        ve.x     = 0; \
        ve.y     = 0;


LRESULT CALLBACK
WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
    GLViewEvent ve;

    //printf( "WndProc %d %p\n", message, _cv );
    if( ! _cv )
        return DefWindowProc( hWnd, message, wParam, lParam );

    switch( message )
    {
        //case WM_CREATE:
        //case WM_DESTROY:
        //case WM_MOVE:

        //case WM_PAINT:
        case WM_ERASEBKGND:
            ve.type  = GLV_EVENT_EXPOSE;
            _cv->eventHandler( _cv, &ve );
            break;

        case WM_CLOSE:
            ve.type = GLV_EVENT_CLOSE;
            _cv->eventHandler( _cv, &ve );
            //PostQuitMessage( 0 );
            break;

        case WM_SETFOCUS:
            //printf( "WM_SETFOCUS %p\n", (HWND) wParam );
            ve.type  = GLV_EVENT_FOCUS_IN;
            _cv->eventHandler( _cv, &ve );
            break;

        case WM_KILLFOCUS:
            ve.type  = GLV_EVENT_FOCUS_OUT;
            _cv->eventHandler( _cv, &ve );
            break;

        case WM_SIZE:
        {
            int w, h;

            w = LOWORD(lParam);
            h = HIWORD(lParam);

            /*
            if( SIZE_MAXHIDE==wParam || SIZE_MINIMIZED==wParam )
                active = false;
            else
                active = true;
            */
            //printf( "WM_SIZE %dx%d\n", LOWORD(lParam), HIWORD(lParam) );
            if( (_cv->width != w) || (_cv->height != h) )
            {
                _cv->width  = w;
                _cv->height = h;

                ve.type = GLV_EVENT_RESIZE;
                ve.x    = w;
                ve.y    = h;
                _cv->eventHandler( _cv, &ve );
            }
        }
            break;

#if 0
        /* A WM_CHAR event is sent after a WM_KEYDOWN event of an ASCII key. */
        case WM_CHAR:
            if( (_cv->flags & FLAG_FILTER_REPEAT) && (lParam & (1 << 30)) )
                break;
            printf( "WM_CHAR %c\n", wParam );
            break;
#endif

        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
            if( (_cv->flags & FLAG_FILTER_REPEAT) && (lParam & (1 << 30)) )
                break;

            //printf( "WM_KEYDOWN %08x %08x\n", lParam, wParam );
            COPY_KEY( ve, GLV_EVENT_KEY_DOWN )
            _cv->eventHandler( _cv, &ve );

            //if( wParam == VK_ESCAPE )
            //    DestroyWindow( hWnd );
            break;

        case WM_KEYUP:
        case WM_SYSKEYUP:
            COPY_KEY( ve, GLV_EVENT_KEY_UP )
            _cv->eventHandler( _cv, &ve );
            break;

        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_MBUTTONDOWN:
            //MessageBox( hWnd, "WM_LBUTTONDOWN", "Message", MB_OK );

            ve.type  = GLV_EVENT_BUTTON_DOWN;
            switch( message )
            {
                case WM_LBUTTONDOWN: ve.code = GLV_BUTTON_LEFT;   break;
                case WM_RBUTTONDOWN: ve.code = GLV_BUTTON_RIGHT;  break;
                case WM_MBUTTONDOWN: ve.code = GLV_BUTTON_MIDDLE; break;
                default:             ve.code = 0;                 break;
            }
            ve.state = LOWORD(wParam);
            ve.x     = LOWORD(lParam);
            ve.y     = HIWORD(lParam);
            _cv->eventHandler( _cv, &ve );
            break;

        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
        case WM_MBUTTONUP:
            //event.state = _mapState( wParam );
            //event.setXY( LOWORD(lParam), _displayH - HIWORD(lParam) );

            ve.type  = GLV_EVENT_BUTTON_UP;
            switch( message )
            {
                case WM_LBUTTONUP: ve.code = GLV_BUTTON_LEFT;   break;
                case WM_RBUTTONUP: ve.code = GLV_BUTTON_RIGHT;  break;
                case WM_MBUTTONUP: ve.code = GLV_BUTTON_MIDDLE; break;
                default:           ve.code = 0;                 break;
            }
            ve.state = LOWORD(wParam);
            ve.x     = LOWORD(lParam);
            ve.y     = HIWORD(lParam);
            _cv->eventHandler( _cv, &ve );
            break;

        case WM_MOUSEMOVE:
            ve.type  = GLV_EVENT_MOTION;
            ve.code  = 0;
            ve.state = LOWORD(wParam);
            ve.x     = LOWORD(lParam);
            ve.y     = HIWORD(lParam);
            _cv->eventHandler( _cv, &ve );
            break;

        case WM_MOUSEWHEEL:
            ve.type  = GLV_EVENT_WHEEL;
            ve.code  = 0;
            ve.state = LOWORD(wParam);
            ve.x     = 0;
            ve.y     = (short) HIWORD(wParam);
            _cv->eventHandler( _cv, &ve );

            //event.setXY( LOWORD(lParam), _displayH - HIWORD(lParam) );
            break;

        default:
            return DefWindowProc( hWnd, message, wParam, lParam );
    }
    return 0;
}


#ifdef NO_WINMAIN
void glv_setAppInstance(HINSTANCE hi)
{
    gAppInstance = hi;
}
#else
extern int main(int, char**);

/*
  This WinMain just gets us to a normal main().
*/
int WINAPI WinMain(HINSTANCE hi, HINSTANCE hPrevInstance, LPSTR lpCmdLine,
                   int nCmdShow)
{
    const int MAX_NUM_ARGVS = 20;
    int argc = 1;
    char* argv[MAX_NUM_ARGVS];
    (void) nCmdShow;

    /* previous instances do not exist in Win32 */
    if (hPrevInstance)
        return 0;

    gAppInstance = hi;

    argv[0] = "prog_name";

    while (*lpCmdLine && (argc < MAX_NUM_ARGVS)) {
        while (*lpCmdLine && ((*lpCmdLine <= 32) || (*lpCmdLine > 126)))
            lpCmdLine++;

        if( *lpCmdLine ) {
            argv[ argc ] = lpCmdLine;
            argc++;

            while (*lpCmdLine && ((*lpCmdLine > 32) && (*lpCmdLine <= 126)))
                lpCmdLine++;

            if (*lpCmdLine) {
                *lpCmdLine = 0;
                lpCmdLine++;
            }
        }
    }

    return main(argc, argv);
}
#endif


/*--------------------------------------------------------------------------*/


static const char className[] = "GLView";


static int _createWindow( GLView* view, int fullscreen, int attributes )
{
    PIXELFORMATDESCRIPTOR pfd;
    int iFormat;
    DWORD style;

    if (fullscreen)
        style = WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
    else
        style = WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
    //style |= WS_VISIBLE;


    view->wnd = CreateWindow( className, "GLView", style,
                              0, 0,      // x, y
                              view->width, view->height,
                              NULL, NULL, gAppInstance, NULL );
    if( view->wnd == NULL )
    {
        //setError( "Failed to create window" );
        return( 0 );
    }


    // get the device context
    view->dc = GetDC( view->wnd );

    // set the pixel format for the DC
    ZeroMemory( &pfd, sizeof(pfd) );

    pfd.nSize      = sizeof(pfd);
    pfd.nVersion   = 1;
    pfd.dwFlags    = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cDepthBits = 16;
    pfd.iLayerType = PFD_MAIN_PLANE;

    if( attributes & GLV_ATTRIB_DOUBLEBUFFER )
        pfd.dwFlags |= PFD_DOUBLEBUFFER;

    if( attributes & GLV_ATTRIB_STENCIL )
        pfd.cStencilBits = 1;

    iFormat = ChoosePixelFormat( view->dc, &pfd );
    if( iFormat == 0 )
        return( 0 );

    SetPixelFormat( view->dc, iFormat, &pfd );

    view->rc = wglCreateContext( view->dc );

    wglMakeCurrent( view->dc, view->rc );

    return( 1 );
}


static void _destroyWindow( GLView* view )
{
    wglMakeCurrent( NULL, NULL );
    wglDeleteContext( view->rc );
    ReleaseDC( view->wnd, view->dc );

    // NOTE: WndProc gets called inside DestroyWindow.
    _cv = 0;
    DestroyWindow( view->wnd );
    view->wnd = 0;
}


/**
  Called once before any other glview functions.
  Returns non-zero if successful.

  If glv_create fails then no other glview function should be called
  (though it is safe to call glv_destroy).
*/
GLView* glv_create( int attributes, int glVersion )
{
    GLView* view;
    WNDCLASS wc;


    view = (GLView*) calloc( 1, sizeof(GLView) );
    if( ! view )
        return 0;

    view->user   = 0;
    view->width  = 640;
    view->height = 480;
    view->modeId = -2;
    view->eventHandler = glv_nullHandler;


    // Register window class.
    wc.style         = CS_OWNDC;    // CS_HREDRAW | CS_VREDRAW
    wc.lpfnWndProc   = WndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = gAppInstance;
    wc.hCursor       = NULL;        // Must be NULL to override.
    wc.hbrBackground = (HBRUSH) GetStockObject( BLACK_BRUSH );
    wc.lpszMenuName  = NULL;
    wc.lpszClassName = className;

    // See if an icon for GLFW is present; if not use the system default.
    wc.hIcon = LoadIcon(GetModuleHandle(NULL), "GLFW_ICON");
    if (! wc.hIcon)
        wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);

    RegisterClass( &wc );

    // DPI aware is required to query LOGPIXELSY and have
    // GLV_MODEID_FULL_WINDOW size properly.
    SetProcessDPIAware();   // Windows Vista
    //SetProcessDpiAwareness(PROCESS_SYSTEM_DPI_AWARE); // Windows 8.1

    _createWindow( view, 0, attributes );
    view->flags = attributes;

#ifdef GLEW_VERSION
    {
    GLenum err = glewInit();
    if( GLEW_OK != err )
    {
        fprintf(stderr, "GLV: %s\n", glewGetErrorString(err));
        _destroyWindow( view );
        view = 0;
    }
    }
#endif

    SetCursor(LoadCursor(NULL, IDC_ARROW));

    _cv = view;

    return( view );
}


/* Restore original video mode. */
static void _restoreVideo( GLView* view )
{
    if( view->flags & FLAG_FULLSCREEN_MODE )
    {
        view->flags &= ~FLAG_FULLSCREEN_MODE;

        ChangeDisplaySettings( NULL, 0 ); 
    }
}


#ifdef USE_CURSORS
static void glv_freeCustomCursors( GLView* view )
{
    int i;
    for (i = 0; i < view->cursorCount; ++i)
        DestroyIcon(view->customCursor[i]);
}


/*
 * Create an ARGB bitmap with top-down row order.
 */
static HBITMAP bitmapCreate(int width, int height, unsigned char** bitmapData)
{
    HBITMAP handle;
    BITMAPV5HEADER bi;
    HDC dc;

    ZeroMemory(&bi, sizeof(bi));
    bi.bV5Size        = sizeof(bi);
    bi.bV5Width       = width;
    bi.bV5Height      = -height;    // Negative for top-down row order.
    bi.bV5Planes      = 1;
    bi.bV5BitCount    = 32;
    bi.bV5Compression = BI_BITFIELDS;
    bi.bV5RedMask     = 0x00ff0000;
    bi.bV5GreenMask   = 0x0000ff00;
    bi.bV5BlueMask    = 0x000000ff;
    bi.bV5AlphaMask   = 0xff000000;

    dc = GetDC(NULL);
    handle = CreateDIBSection(dc, (BITMAPINFO*) &bi, DIB_RGB_COLORS,
                             (void**) bitmapData, NULL, (DWORD) 0);
    ReleaseDC(NULL, dc);
    return handle;
}


/*
 * Create a cursor with transparency mask from a bitmap.
 */
static HICON cursorCreate(HBITMAP bitmap, int width, int height,
                          int xhot, int yhot)
{
    HICON handle;
    HBITMAP mask;
    ICONINFO info;

    mask = CreateBitmap(width, height, 1, 1, NULL);
    if (! mask)
        return NULL;

    ZeroMemory(&info, sizeof(info));
    info.fIcon    = FALSE;      // FALSE specifies cursor.
    info.xHotspot = xhot;
    info.yHotspot = yhot;
    info.hbmMask  = mask;
    info.hbmColor = bitmap;

    handle = CreateIconIndirect(&info);
    DeleteObject(mask);

    return handle;
}


/**
  Define a set of cursors.

  \param areas          Area within the pixels data for each cursor.
                        Each cursor has these six values:
                            x, y, width, height, hotx, hoty
                        The hotspot is relative to x, y.
  \param count          Number of cursors in areas array.
  \param pixels         32-bit RGBA or ARGB values.
  \param pixelsWidth    Width of image stored in pixels.
  \param argb           Pixel color channel format (0 = RGBA, 1 = ARGB)
*/
int glv_loadCursors( GLView* view, const short* areas, int count,
                     const unsigned char* pixels, int pixelsWidth, int argb )
{
    HBITMAP cmap;
    unsigned char* cp;
    const unsigned char* srcRow;
    const unsigned char* sp;
    int x, y, w, h;
    int failed = 0;
    int i;

    if (view->cursorCount)
        glv_freeCustomCursors(view);

    view->customCursor = realloc(view->customCursor, count*sizeof(HICON));
    view->cursorCount = count;

    pixelsWidth *= 4;

    for (i = 0; i < count; ++i) {
        w = areas[2];
        h = areas[3];
        cmap = bitmapCreate(w, h, &cp);
        if (! cmap) {
            view->customCursor[i] = NULL;
            ++failed;
        } else {
            srcRow = pixels + (pixelsWidth * areas[1]) + (areas[0] * 4);

            for (y = 0; y < h; ++y) {
                sp = srcRow;
                if (argb) {
                    for (x = 0; x < w; ++x, sp += 4) {
                        *cp++ = sp[0];
                        *cp++ = sp[1];
                        *cp++ = sp[2];
                        *cp++ = sp[3];
                    }
                } else {
                    for (x = 0; x < w; ++x, sp += 4) {
                        *cp++ = sp[2];
                        *cp++ = sp[1];
                        *cp++ = sp[0];
                        *cp++ = sp[3];
                    }
                }
                srcRow += pixelsWidth;
            }

            view->customCursor[i] = cursorCreate(cmap, w, h, areas[4], areas[5]);
            DeleteObject(cmap);
        }

        areas += 6;
    }
    return failed ? 0 : 1;
}


/**
  Use one of the cursors defined by glv_loadCursors() or the default
  GLV_CURSOR_ARROW.

  The pointer visibility is controlled separately by glv_showCursor().
*/
void glv_setCursor( GLView* view, int cursorIndex )
{
    if (cursorIndex < view->cursorCount) {
        if (cursorIndex < 0)
            SetCursor(LoadCursor(NULL, IDC_ARROW));
        else
            SetCursor(view->customCursor[cursorIndex]);
    }
}
#endif


/**
  Called to close the glview and free any used resources.
*/
void glv_destroy( GLView* view )
{
    if( view )
    {
        if( view->wnd )
        {
#ifdef USE_CURSORS
            glv_freeCustomCursors(view);
            free(view->customCursor);
            view->customCursor = NULL;
            view->cursorCount = 0;
#endif

            _restoreVideo( view );
            _destroyWindow( view );
            UnregisterClass( className, gAppInstance );
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


/**
  Returns vertical dots per inch of display.
*/
int glv_dpi( GLView* view )
{
#if 1
    double vr = ((double) GetDeviceCaps(view->dc, VERTRES) * 25.4) /
                 (double) GetDeviceCaps(view->dc, VERTSIZE);
    return (int) (vr + 0.5);
#else
    // Must cancel DPI scaling or the default value of 96 is returned.
    return GetDeviceCaps( view->dc, LOGPIXELSY );
#endif
}


/**
  Returns the number of available fullscreen modes.
  Calls func for each mode.
*/
int glv_queryModes( GLViewMode_f func, void* data )
{
    GLViewMode mode;
    DEVMODE devmode;
    DWORD modeIndex = 0;

    ZeroMemory( &devmode, sizeof(DEVMODE) );
    devmode.dmSize = sizeof(DEVMODE);
    devmode.dmDriverExtra = 0;

    /* NOTE: There may be hundreds of display modes.  There were 322 modes
     * on my Windows test box.
     */

    while( EnumDisplaySettings( NULL, modeIndex, &devmode ) )
    {
        mode.id          = modeIndex;
        mode.width       = devmode.dmPelsWidth;
        mode.height      = devmode.dmPelsHeight;
        mode.refreshRate = devmode.dmDisplayFrequency;
        mode.depth       = devmode.dmBitsPerPel;

        /* NOTE: dmDisplayFrequency can be 0 or 1 to represent a hardware
         * default refresh rate */

        //printf( "KR %d freq %d, depth %d\n", modeIndex,
        //        devmode.dmDisplayFrequency, devmode.dmBitsPerPel );

        func( &mode, data );

        modeIndex++;
    }

    return( 0 );
}


static BOOL _desktopDisplay(int adapter, DISPLAY_DEVICE* dd)
{
    int devCount = 0;
    int acount = 0;

    ZeroMemory(dd, sizeof(DISPLAY_DEVICE));
    dd->cb = sizeof(DISPLAY_DEVICE);

    while (EnumDisplayDevices(NULL, devCount, dd, 0)) {
        if (dd->StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP) {
            if (adapter == acount)
                return TRUE;
            ++acount;
        }
        ++devCount;
    }
    return FALSE;
}


static BOOL _monitorSize(int adapter, int* size)
{
    DISPLAY_DEVICE dd;
    DEVMODE dm;

    if (_desktopDisplay(adapter, &dd)) {
        ZeroMemory(&dm, sizeof(dm));
        dm.dmSize = sizeof(dm);
        if (! EnumDisplaySettings(dd.DeviceName, ENUM_CURRENT_SETTINGS, &dm))
            return FALSE;

        if (dm.dmFields & DM_PELSWIDTH &&
            dm.dmFields & DM_PELSHEIGHT) {
            size[0] = dm.dmPelsWidth;
            size[1] = dm.dmPelsHeight;
            return TRUE;
        }
    }
    return FALSE;
}


#define IN_WINDOW_MODE(view) \
    (! (view->flags & (FLAG_FULLSCREEN_MODE | FLAG_FULLWINDOW_MODE)))

static void _saveWindowPos(GLView* view)
{
    RECT rect;
    GetWindowRect(view->wnd, &rect);
    view->winPos[0] = rect.left;
    view->winPos[1] = rect.top;
}


/**
  Returns non-zero if successful.
  Must not be called from within input handler.
*/
int glv_changeMode( GLView* view, const GLViewMode* mode )
{
    int oldModeFS = (view->flags & FLAG_FULLSCREEN_MODE) ? 1 : 0;
    int newModeFS = (mode->id > GLV_MODEID_WINDOW) ? 1 : 0;

    /* Return if mode is current */
    if (oldModeFS == newModeFS) {
        if (mode->id == GLV_MODEID_FULL_WINDOW) {
            if (view->flags & FLAG_FULLWINDOW_MODE)
                return 1;
        } else {
            if (mode->width == view->width && mode->height == view->height)
                return 1;
        }
    }

    if (! newModeFS)
    {
        /* Change to desktop window. */

        if (oldModeFS) {
            _restoreVideo(view);
            _destroyWindow(view);

            view->width  = mode->width;
            view->height = mode->height;

            _createWindow(view, 0, view->flags);
        }

        if (mode->id == GLV_MODEID_FULL_WINDOW) {
            int size[2];
            HWND wnd = view->wnd;

            if (IN_WINDOW_MODE(view))
                _saveWindowPos(view);

            if (! _monitorSize(0, size)) {
                size[0] = mode->width;      // Fallback to mode size.
                size[1] = mode->height;
            }

            SetWindowLong(wnd, GWL_STYLE, WS_VISIBLE | WS_POPUP);
            SetWindowLong(wnd, GWL_EXSTYLE, WS_EX_APPWINDOW);
            SetWindowPos(wnd, HWND_TOP, 0, 0, size[0], size[1],
                         SWP_FRAMECHANGED);
            view->flags |= FLAG_FULLWINDOW_MODE;
        } else {
            RECT fr;
            HWND wnd = view->wnd;
            LONG style = (mode->id == GLV_MODEID_FIXED_WINDOW) ?
                        WS_VISIBLE | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX :
                        WS_VISIBLE | WS_OVERLAPPEDWINDOW;
            UINT swpFlags = SWP_FRAMECHANGED;

            if (IN_WINDOW_MODE(view))
                 swpFlags |= SWP_NOMOVE;

            SetRect(&fr, 0, 0, mode->width, mode->height);
            AdjustWindowRect(&fr, style, FALSE);

            SetWindowLong(wnd, GWL_STYLE, style);
            SetWindowPos(wnd, HWND_TOP, view->winPos[0], view->winPos[1],
                         fr.right - fr.left, fr.bottom - fr.top, swpFlags);
            view->flags &= ~FLAG_FULLWINDOW_MODE;
#if 0
            if (mode->id == GLV_MODEID_FIXED_WINDOW) {
                // Disable resize by menu.
                HMENU menu = GetSystemMenu(wnd, FALSE);
                DeleteMenu(menu, SC_SIZE, MF_BYCOMMAND);
                DeleteMenu(menu, SC_MAXIMIZE, MF_BYCOMMAND);
                DrawMenuBar(wnd);
            }
#endif
        }
    }
    else
    {
        DEVMODE devmode;

        ZeroMemory(&devmode, sizeof(DEVMODE));
        devmode.dmSize = sizeof(DEVMODE);
        devmode.dmDriverExtra = 0;

        if (EnumDisplaySettings(NULL, mode->id, &devmode)) {
            devmode.dmFields = DM_BITSPERPEL |
                               DM_PELSWIDTH |
                               DM_PELSHEIGHT |
                               DM_DISPLAYFLAGS |
                               DM_DISPLAYFREQUENCY;

#ifdef DM_POSITION
            /* Not sure if we should care about multi-monitor setups. */
            devmode.dmPosition.x  = 0;
            devmode.dmPosition.y  = 0;
            devmode.dmFields     |= DM_POSITION;
#endif

            if (IN_WINDOW_MODE(view))
                _saveWindowPos(view);

            if (ChangeDisplaySettings(&devmode, CDS_FULLSCREEN) ==
                DISP_CHANGE_SUCCESSFUL) {
                if ((view->flags & FLAG_FULLSCREEN_MODE) == 0) {
                    _destroyWindow(view);
                    _createWindow(view, 1, view->flags);
                }

                MoveWindow(view->wnd, 0, 0,
                           devmode.dmPelsWidth, devmode.dmPelsHeight, FALSE);
                view->flags |= FLAG_FULLSCREEN_MODE;
            }
        }
    }

    glv_show(view);

    view->modeId = mode->id;
    return 1;
}


void glv_swapBuffers( GLView* view )
{
    SwapBuffers( view->dc );
}


void glv_makeCurrent( GLView* view )
{
    wglMakeCurrent( view->dc, view->rc );
}


void glv_show( GLView* view )
{
    ShowWindow( view->wnd, SW_SHOW );
}


void glv_hide( GLView* view )
{
    ShowWindow( view->wnd, SW_HIDE );
}


/**
  Sets window title and icon name.
*/
void glv_setTitle( GLView* view, const char* title )
{
    SetWindowText( view->wnd, title );
}


/**
  Positions window on screen.
  Should only be called when in windowed mode.
*/
void glv_move( GLView* view, int x, int y )
{
    MoveWindow( view->wnd, x, y, view->width, view->height, FALSE );
}


/**
  Sets window dimensions.
  Should only be called when in windowed mode.
*/
void glv_resize( GLView* view, int w, int h )
{
    RECT rect;
    GetWindowRect( view->wnd, &rect );
    MoveWindow( view->wnd, rect.left, rect.top, w, h, FALSE );
}


/**
  Show window on top of all other windows.
*/
void glv_raise( GLView* view )
{
    ShowWindow( view->wnd, SW_SHOW );
    SetForegroundWindow( view->wnd );
    SetFocus( view->wnd );
}


void glv_iconify( GLView* view )
{
    ShowWindow( view->wnd, SW_HIDE );
}


/**
  Show or hide the mouse pointer.

  \sa glv_setCursor()
*/
void glv_showCursor( GLView* view, int on )
{
    (void) view;
    ShowCursor(on ? TRUE : FALSE);
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
  Wait until an event is recieved.
*/
void glv_waitEvent( GLView* view )
{
    WaitMessage();
}


/**
  Calls the appropriate event handler funtion for all pending events.
*/
void glv_handleEvents( GLView* view )
{
    MSG msg;

    _cv = view;

    while( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) )
    {
        TranslateMessage( &msg );
        DispatchMessage( &msg );
    }
}


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
    unsigned char keyState[256];
    WORD ascii;
    UINT scanCode;

    if (GetKeyboardState(keyState)) {
        scanCode = (_keyLParam >> 16) & 127;
        if (ToAscii(_keyWParam, scanCode, keyState, &ascii, 0) == 1)
            return ascii & 0xff;
    }

    /*
    unsigned short buf[2];
    if( ToUnicode( _keyWParam, scanCode, keyState, buf, 2, 0 ) > 0 )
    {
        char ascii[2];
        *ascii = 0;
        WideCharToMultiByte( CP_ACP, 0, buf, 2, ascii, 1, NULL, FALSE );
        return *ascii;
    }
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
    LPTSTR clip;
    (void) view;
   
    if( ! IsClipboardFormatAvailable(CF_TEXT) )
        return 0;
    if( ! OpenClipboard(NULL) )
        return 0;

    clip = GlobalLock( GetClipboardData(CF_TEXT) );
    func( clip, strlen(clip), user );
    GlobalUnlock( clip );

    CloseClipboard();
    return 1;
}


/*EOF*/
