#ifndef GLV_H
#define GLV_H
/*===========================================================================/

  GLV Library
  Copyright (C) 2003-2025  Karl Robillard
  SPDX-License-Identifier: MIT

/===========================================================================*/


#ifdef ANDROID
#include <EGL/egl.h>
#include <GLES3/gl31.h>     /* Available in android-21 (5.0 Lollipop) */
#elif defined(_WIN32)
#include <windows.h>
#include <GL/gl.h>
#else   // X11
#include <GL/glx.h>
#endif

#define GLV_VERSION_STR "0.5.0"
#define GLV_VERSION     0x000500


typedef struct {
    int type;
    int code;
    int state;
    int x;
    int y;
} GLViewEvent;


typedef struct {
    /* Read/write */
    void* user;

    /* Read-only */
    int width;
    int height;

#ifdef ANDROID
    /* Read-only for Android */
    EGLDisplay display;
    EGLSurface surface;
    EGLContext ctx;
    char appRef;

    /* Private */
    void (*eventHandler)(void*, GLViewEvent*);
    unsigned short flags;
    unsigned short glVersion;
#elif defined(_WIN32)
    /* Read-only for Windows */
    HWND wnd;
    HDC dc;
    HGLRC rc;

    /* Private */
    void (*eventHandler)(void*, GLViewEvent*);
    unsigned short flags;
    int modeId;
    int winPos[2];
    int minW, minH, maxW, maxH;
    HICON* customCursor;
    int cursorCount;
#else
    /* Read-only for X11 */
    Display* display;
    int screen;
    Window window;
    GLXContext ctx;

    /* Private */
    void (*eventHandler)(void*, GLViewEvent*);
    unsigned short flags;
    void* omode;
    Atom deleteAtom;
    Atom wmAtom[3];
    Cursor nullCursor;
    Cursor* customCursor;
    int cursorCount;
    int activeCursor;
#endif
} GLView;


typedef struct {
    int id;
    int width;
    int height;
    int refreshRate;
    int depth;
} GLViewMode;


typedef void (*GLViewMode_f)( const GLViewMode*, void* );
typedef void (*GLViewEvent_f)( GLView*, GLViewEvent* );


#define GLV_ATTRIB_DOUBLEBUFFER     1
#define GLV_ATTRIB_STENCIL          2
#define GLV_ATTRIB_MULTISAMPLE      4
#define GLV_ATTRIB_ES               8
#define GLV_ATTRIB_DEBUG            16

#define GLV_MODEID_WINDOW       -1
#define GLV_MODEID_FULL_WINDOW  -2
#define GLV_MODEID_FIXED_WINDOW -3

#define GLV_CURSOR_ARROW    -1

/* GLViewEvent type */
#define GLV_EVENT_RESIZE        1
#define GLV_EVENT_CLOSE         2
#define GLV_EVENT_BUTTON_DOWN   3
#define GLV_EVENT_BUTTON_UP     4
#define GLV_EVENT_MOTION        5
#define GLV_EVENT_WHEEL         6
#define GLV_EVENT_KEY_DOWN      7
#define GLV_EVENT_KEY_UP        8
#define GLV_EVENT_FOCUS_IN      9
#define GLV_EVENT_FOCUS_OUT     10
#define GLV_EVENT_EXPOSE        11
#define GLV_EVENT_ICONIFY       12
// ANDROID
#define GLV_EVENT_APP           13
#define GLV_EVENT_PINCH         14
#define GLV_EVENT_DPAD          15
#define GLV_EVENT_USER          32

/* GLViewEvent y for GLV_EVENT_WHEEL events */
#define GLV_WHEEL_DELTA     120

/* GLViewEvent code for GLV_EVENT_BUTTON_DOWN/UP events */
// These match X11 Button1, Button2 & Button3.
#define GLV_BUTTON_LEFT     1
#define GLV_BUTTON_MIDDLE   2
#define GLV_BUTTON_RIGHT    3

#ifdef ANDROID
/* GLViewEvent state masks */
#define GLV_MASK_SHIFT      0x01        // AMETA_SHIFT_ON
#define GLV_MASK_CTRL       0x1000      // AMETA_CTRL_ON
#define GLV_MASK_ALT        0x02        // AMETA_ALT_ON
#define GLV_MASK_CMD        0x10000     // AMETA_META_ON
#define GLV_MASK_CAPS       0x100000    // AMETA_CAPS_LOCK_ON
#define GLV_MASK_NUM        0x200000    // AMETA_NUM_LOCK_ON
#define GLV_MASK_LEFT       0x10        // AMOTION_EVENT_BUTTON_PRIMARY   << 4
#define GLV_MASK_MIDDLE     0x20        // AMOTION_EVENT_BUTTON_SECONDARY << 4
#define GLV_MASK_RIGHT      0x40        // AMOTION_EVENT_BUTTON_TERTIARY  << 4

/* GLViewEvent code & state mask for GLV_EVENT_DPAD */
#define GLV_DPAD_ACTIVE     0x01
#define GLV_DPAD_UP         0x04
#define GLV_DPAD_DOWN       0x08
#define GLV_DPAD_LEFT       0x10
#define GLV_DPAD_RIGHT      0x20

#elif defined(_WIN32)

/* GLViewEvent state masks */
// NOTE: There is an MK_ALT defined in OLEIDL.H as 0x20 but aparently this
// bit is not set for mouse events.
#define GLV_MASK_SHIFT      MK_SHIFT
#define GLV_MASK_CTRL       MK_CONTROL
#define GLV_MASK_ALT        0x20
#define GLV_MASK_CMD        0
#define GLV_MASK_CAPS       0
#define GLV_MASK_NUM        0
#define GLV_MASK_LEFT       MK_LBUTTON
#define GLV_MASK_MIDDLE     MK_MBUTTON
#define GLV_MASK_RIGHT      MK_RBUTTON

#else   // X11

/* GLViewEvent state masks */
#define GLV_MASK_SHIFT      ShiftMask
#define GLV_MASK_CTRL       ControlMask
#define GLV_MASK_ALT        Mod1Mask
#define GLV_MASK_CMD        Mod4Mask
#define GLV_MASK_CAPS       LockMask
#define GLV_MASK_NUM        Mod2Mask
#define GLV_MASK_LEFT       Button1Mask
#define GLV_MASK_MIDDLE     Button2Mask
#define GLV_MASK_RIGHT      Button3Mask
#endif


#ifdef __cplusplus
extern "C" {
#endif

extern int  glv_queryModes(GLViewMode_f func, void*);

extern GLView* glv_create(int attributes, int glVersion);
extern void glv_destroy(GLView* view);
extern int  glv_attributes(GLView* view);
extern int  glv_dpi(GLView* view);
extern int  glv_changeMode(GLView* view, const GLViewMode* mode);
extern void glv_swapBuffers(GLView* view);
extern void glv_makeCurrent(GLView* view);
extern void glv_show(GLView* view);
extern void glv_hide(GLView* view);
extern void glv_setTitle(GLView* view, const char* title);
extern void glv_move(GLView* view, int x, int y);
extern void glv_resize(GLView* view, int w, int h);
extern void glv_setSizeLimits(GLView* view, const int* minSize, const int* maxSize);
extern void glv_raise(GLView* view);
extern void glv_iconify(GLView* view);
extern void glv_showCursor(GLView* view, int on);

extern void glv_setEventHandler(GLView* view, GLViewEvent_f func);
extern void glv_waitEvent(GLView* view);
extern void glv_handleEvents(GLView* view);
extern void glv_filterRepeatKeys(GLView* view, int on);
extern int  glv_clipboardText(GLView* view,
                       void (*func)(const char* data, int len, void* user),
                       void* user);
extern int  glv_loadCursors(GLView* view, const short* areas, int cursorCount,
                     const unsigned char* pixels, int pixelsWidth, int argb);
extern void glv_setCursor(GLView* view, int cursorIndex);

#ifdef ANDROID
#define KEY_ASCII(e)    glv_ascii(e)
extern int  glv_ascii( const GLViewEvent* );
extern void glv_showSoftInput( GLView* view, int visible );
extern void glv_setDPadRect( GLView* view, int pad, const float* rect );
#define glv_eventPinch(ev)  ((float*) &(ev)->x)

#elif defined(_WIN32)

#define KEY_ASCII(e)    glv_ascii()
extern int  glv_ascii();
extern void glv_setAppInstance(HINSTANCE);

#else   // X11

#define KEY_ASCII(e)    glv_ascii()
extern int  glv_ascii();
extern int  glv_setIcon(GLView* view, int width, int height,
                        const unsigned char* pixels, int argb);
#endif


#ifdef __cplusplus
}
#endif

#endif // GLV_H
