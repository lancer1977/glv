#ifndef GLV_H
#define GLV_H
/*===========================================================================/

  GLV Library for Android
  Copyright (C) 2012,2019  Karl Robillard
  SPDX-License-Identifier: MIT

/===========================================================================*/


#include <EGL/egl.h>
#include <GLES3/gl31.h>     /* Available in android-21 (5.0 Lollipop) */

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


    /* Read-only for Android */

    EGLDisplay display;
    EGLSurface surface;
    EGLContext ctx;
    char appRef;


    /* Private */

    void (*eventHandler)( void*, GLViewEvent* );
    unsigned short flags;
    unsigned short glVersion;
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
#define GLV_EVENT_APP           12
#define GLV_EVENT_PINCH         13
#define GLV_EVENT_DPAD          14
#define GLV_EVENT_USER          32

/* GLViewEvent code for GLV_EVENT_BUTTON_DOWN/UP events */
#define GLV_BUTTON_LEFT     1
#define GLV_BUTTON_MIDDLE   2
#define GLV_BUTTON_RIGHT    3

/* GLViewEvent y for GLV_EVENT_WHEEL events */
#define GLV_WHEEL_DELTA     120

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
extern int  glv_clipboardText( GLView* view,
                       void (*func)(const char* data, int len, void* user),
                       void* user );

/* Android Only */
extern int  glv_ascii( const GLViewEvent* );
extern void glv_showSoftInput( GLView* view, int visible );
extern void glv_setDPadRect( GLView* view, int pad, const float* rect );
#define glv_eventPinch(ev)  ((float*) &(ev)->x)


#ifdef __cplusplus
}
#endif


#endif // GLV_H
