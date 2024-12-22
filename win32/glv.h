#ifndef GLV_H
#define GLV_H
/*===========================================================================/

  GLV Library for Windows
  Copyright (C) 2003-2024  Karl Robillard
  SPDX-License-Identifier: MIT

/===========================================================================*/


#include <windows.h>
#include <GL/gl.h>

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


    /* Read-only for Windows */

    HWND wnd;
    HDC dc;
    HGLRC rc;


    /* Private */

    void (*eventHandler)( void*, GLViewEvent* );
    unsigned short flags;
    int modeId;
    int winPos[2];
    HICON* customCursor;
    int cursorCount;
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
#define GLV_EVENT_USER          32

/* GLViewEvent code for GLV_EVENT_BUTTON_DOWN/UP events */
#define GLV_BUTTON_LEFT     1
#define GLV_BUTTON_RIGHT    2
#define GLV_BUTTON_MIDDLE   3

/* GLViewEvent y for GLV_EVENT_WHEEL events */
#define GLV_WHEEL_DELTA     120

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


#ifdef __cplusplus
extern "C" {
#endif


extern int  glv_queryModes( GLViewMode_f func, void* );

extern GLView* glv_create( int attributes, int glVersion );
extern void glv_destroy( GLView* view );
extern int  glv_attributes( GLView* view );
extern int  glv_dpi( GLView* view );
extern int  glv_changeMode( GLView* view, const GLViewMode* mode );
extern void glv_swapBuffers( GLView* view );
extern void glv_makeCurrent( GLView* view );
extern void glv_show( GLView* view );
extern void glv_hide( GLView* view );
extern void glv_setTitle( GLView* view, const char* title );
extern void glv_move( GLView* view, int x, int y );
extern void glv_resize( GLView* view, int w, int h );
extern void glv_raise( GLView* view );
extern void glv_iconify( GLView* view );
extern void glv_showCursor( GLView* view, int on );

extern void glv_setEventHandler( GLView* view, GLViewEvent_f func );
extern void glv_waitEvent( GLView* view );
extern void glv_handleEvents( GLView* view );
extern void glv_filterRepeatKeys( GLView* view, int on );
extern int  glv_ascii();
extern int  glv_clipboardText( GLView* view,
                       void (*func)(const char* data, int len, void* user),
                       void* user );
extern int  glv_loadCursors( GLView* view, const short* areas, int cursorCount,
                     const unsigned char* pixels, int pixelsWidth, int argb );
extern void glv_setCursor( GLView* view, int cursorIndex );

/* Windows Only */
extern void glv_setAppInstance(HINSTANCE);

#define KEY_ASCII(e)        glv_ascii()


#ifdef __cplusplus
}
#endif


#endif // GLV_H
