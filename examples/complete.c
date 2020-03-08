/*===========================================================================/

  $Id: complete.c,v 1.11 2004/11/11 08:23:51 karl Exp $

  GLV Library
  Copyright (C) 2003-2006  Karl Robillard

  This example is a complete test of all GLV functionality.
  Here are the keyboard commands:

      w - Toggle window/fullscreen mode.
      c - Toggle cursor on/off.
      r - Toggle repeat key filter.

/===========================================================================*/


#include <stdio.h>
#include <stdlib.h>
#ifndef _WIN32
#include <unistd.h>
#endif
#include <glv.h>
#include <glv_keys.h>
#include "keystr.h"


int quit = 0;
int change = 0;
int cursorOn = 1;
int windowed = 0;
int kfilter = 1;
GLView* gView;


//#define TIMEOUT 1

#define VIEW2   0
#if VIEW2
GLView* view2;

void close2( GLView* v )
{
    printf( "close2\n" );
    glv_hide( v );
}
#endif


void pickMode( const GLViewMode* md, void* data )
{
    /* Pick a low-resolution mode for this example. */

    //if( (md->width <= 800) && (md->height <= 600) )
    if( (md->width == 800) && (md->height == 600) && (md->depth > 8) )
    {
        GLViewMode* cmode = (GLViewMode*) data;
        *cmode = *md;
    }
    printf( "mode %dx%d %d %d\n",
            md->width, md->height, md->depth, md->refreshRate );
}


#if 0
    // Triangle with gradient.

    glViewport( 0, 0, gDisplayWidth, gDisplayHeight );
    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();
    glFrustum( -1.0, 1.0, -1.0, 1.0, 5.0, 15.0 );
    glMatrixMode( GL_MODELVIEW );


    GLfloat _rotX, _rotY, _rotZ;
    _rotX = _rotY = _rotZ = 0.0;

    glShadeModel( GL_SMOOTH );
    glClear( GL_COLOR_BUFFER_BIT );

    glPushMatrix();
    glTranslatef( 0.0, 0.0, -10.0 );
    glRotatef( _rotX, 1.0, 0.0, 0.0 );
    glRotatef( _rotY, 0.0, 1.0, 0.0 );
    glRotatef( _rotZ, 0.0, 0.0, 1.0 );

    glBegin( GL_TRIANGLES );
    glColor3f( 1.0, 0.0, 0.0 );  glVertex3f( -1.0,  0.0,  0.0 );
    glColor3f( 0.0, 1.0, 0.0 );  glVertex3f(  0.0,  1.0,  0.0 );
    glColor3f( 0.0, 0.0, 1.0 );  glVertex3f(  1.0,  0.0,  0.0 );
    glEnd();

    glPopMatrix();
#endif


void redraw()
{
    int w, h;

    w = gView->width;
    h = gView->height;

    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );


    glColor3f( 1, 1, 1 );

    glBegin( GL_LINE_LOOP );
    glVertex2f( 0, 0 );
    glVertex2f( (float) w, 0 );
    glVertex2f( (float) w, (float) h );
    glVertex2f( 0, (float) h );
    glEnd();

    glBegin( GL_LINES );
    glVertex2f( 0, 0 );
    glVertex2f( (float) w, (float) h );
    glEnd();


    glv_swapBuffers( gView );
}


void clipboard( const char* data, int len, void* user )
{
    const char* end = data + len;

    (void) user;

    printf( "clipboard length: %d\n", len );
    printf( "clipboard data:   {" );
    while( data != end )
        putchar( *data++ );
    printf( "}\n" );
}


void eventHandler( GLView* view, GLViewEvent* event )
{
    switch( event->type )
    {
        case GLV_EVENT_RESIZE:
        {
            int w, h;

            w = event->x;
            h = event->y;

            printf( "testResize %d %d\n", w, h );

            glViewport( 0, 0, w, h );
            glMatrixMode( GL_PROJECTION );
            glLoadIdentity();
            //gluPerspective( 50, (float)w / (float)h, 5, 20 );
            glOrtho( -4, w+4, -4, h+4, -1.0, 1.0 );
            glMatrixMode( GL_MODELVIEW );

            glClearColor( 0.1f, 0.2f, 1.0f, 0 );
            redraw();
        }
            break;

        case GLV_EVENT_CLOSE:
            printf( "testClose\n" );
            quit = 1;
            break;

        case GLV_EVENT_BUTTON_DOWN:
            printf( "testButtonDown %d %dx%d %x\n", event->code,
                    event->x, event->y, event->state );
            break;

        case GLV_EVENT_BUTTON_UP:
            printf( "testButtonUp %d %dx%d %x\n", event->code,
                    event->x, event->y, event->state );
            break;

        case GLV_EVENT_WHEEL:
            printf( "testWheel %d %dx%d %x\n", event->code,
                    event->x, event->y, event->state );
            break;

        case GLV_EVENT_MOTION:
            printf( "move %dx%d %x\n", event->x, event->y, event->state );
            break;

        case GLV_EVENT_KEY_DOWN:
        {
            struct KeyStr* ks = keyStrings;

            printf( "testKeyDown %d %c\n", event->code, KEY_ASCII(event) );

            while( ks->str )
            {
                if( ks->code == event->code )
                {
                    printf( "  %s\n", ks->str );
                    break;
                }
                ++ks;
            }

            switch( event->code )
            {
                case KEY_m:
                    glv_move( view, 0, 0 );
                    break;
            
                case KEY_w:
                    change = 1;
                    break;
            
                case KEY_c:
                    cursorOn ^= 1;
                    glv_showCursor( view, cursorOn );
                    break;

                case KEY_r:
                    kfilter ^= 1;
                    glv_filterRepeatKeys( view, kfilter );
                    printf( "repeat filter %s\n", kfilter ? "on" : "off" );
                    break;

                case KEY_v:
                    if( event->state & GLV_MASK_CTRL )
                    {
                        if( ! glv_clipboardText( view, clipboard, 0 ) )
                            printf( "Clipboard empty\n" );
                    }
                    break;

                case KEY_Escape:
                    quit = 1;
                    break;
            }
        }
            break;

        case GLV_EVENT_KEY_UP:
            printf( "testKeyUp %d %c\n", event->code, KEY_ASCII(event) );
            break;

        case GLV_EVENT_FOCUS_IN:
            printf( "testFocusIn\n" );
            break;

        case GLV_EVENT_FOCUS_OUT:
            printf( "testFocusOut\n" );
            break;

        case GLV_EVENT_EXPOSE:
            printf( "testExpose\n" );
            redraw();
            break;
    }
}


int main( int argc, char** argv )
{
    GLViewMode wmode;
    GLViewMode mode;
    int i;
    int windowOnly = 0;


    for( i = 1; i < argc; ++i )
    {
        char* arg = argv[i];
        if( *arg == '-' )
        {
            if( arg[1] == 'w' )
                windowOnly = 1;
        }
    }


    gView = glv_create( GLV_ATTRIB_DOUBLEBUFFER, 0 );
    if( ! gView )
        return( -1 );

    glv_setTitle( gView, "GLView Test" );

    glv_setEventHandler( gView, eventHandler );

    glv_filterRepeatKeys( gView, kfilter );


    /* Default to a 640x480 window. */
    wmode.id     = GLV_MODEID_WINDOW;
    wmode.width  = 640;
    wmode.height = 480;

    mode = wmode;

    if( ! windowOnly )
        glv_queryModes( pickMode, &mode );

    glv_changeMode( gView, windowed ? &wmode : &mode );
    //printf( "mode changed to %dx%d\n", mode.width, mode.height );


#if VIEW2
    if( windowOnly )
    {
        glv_create( view2, 0 );
        glv_funcClose( view2, close2 );
        glv_changeMode( view2, &mode );
        glv_handleEvents( view2 );
        glv_move( view2, 80, 80 );
        glv_resize( view2, 320, 200 );
    }
#endif

#ifdef TIMEOUT
    for( i = 0; i < 3000; ++i )
#else
    for(;;)
#endif
    {
        glv_handleEvents( gView );
        if( quit )
            break;

        if( change )
        {
            change = 0;
            windowed ^= 1;
            glv_changeMode( gView, windowed ? &wmode : &mode );
        }

#if VIEW2
        glv_handleEvents( view2 );
#endif

#ifdef _WIN32
        Sleep( 10 );
#else
        usleep( 4 );
#endif
    }

#ifdef TIMEOUT
    if( i == 3000 )
        printf( "Timeout\n" );
#endif

#if VIEW2
    if( windowOnly )
        glv_destroy( view2 );
#endif

    glv_destroy( gView );

    return( 0 );
}


/*EOF*/
