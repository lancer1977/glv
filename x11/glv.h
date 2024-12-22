#ifndef GLV_H
#define GLV_H
/*===========================================================================/

  GLV Library for X11
  Copyright (C) 2003-2024  Karl Robillard
  SPDX-License-Identifier: MIT

/===========================================================================*/


#include <GL/glx.h>

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


    /* Read-only for X11 */

    Display* display;
    int screen;
    Window window;
    GLXContext ctx;


    /* Private */

    void (*eventHandler)( void*, GLViewEvent* );
    unsigned short flags;
    void* omode;
    Atom deleteAtom;
    Atom wmAtom[3];
    Cursor nullCursor;
    Cursor* customCursor;
    int cursorCount;
    int activeCursor;
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
#define GLV_BUTTON_LEFT     Button1
#define GLV_BUTTON_MIDDLE   Button2
#define GLV_BUTTON_RIGHT    Button3

/* GLViewEvent y for GLV_EVENT_WHEEL events */
#define GLV_WHEEL_DELTA     120

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

/* X11 Only */
extern int  glv_setIcon( GLView* view, int width, int height,
                         const unsigned char* pixels, int argb );
extern int  glv_loadCursors( GLView* view, const short* areas, int cursorCount,
                     const unsigned char* pixels, int pixelsWidth, int argb );
extern void glv_setCursor( GLView* view, int cursorIndex );

#define KEY_ASCII(e)        glv_ascii()


#ifdef __cplusplus
}
#endif


#endif // GLV_H
