/*===========================================================================/

  $Id: nocallback.c,v 1.2 2004/01/02 23:19:23 karl Exp $

  GLV Library
  Copyright (C) 2003-2006  Karl Robillard

  This example shows how to get GLViewEvents without using a callback.
  Press the escape key or click on the window close widget to exit.

  FIXME: Cannot use KEY_ASCII() on key events when not using callback!

/===========================================================================*/


#include <stdio.h>
#include <stdlib.h>
#include <glv.h>
#include <glv_keys.h>


/*--------------------------------------------------------------------------*/
/* No callback module. */


#define MAX_EVENTS  32
static GLViewEvent events[ MAX_EVENTS ];
static int currEvent  = 0;
static int eventCount = 0;


static void bufferEvent( GLView* view, GLViewEvent* event )
{
    if( eventCount < MAX_EVENTS )
    {
        events[ eventCount ] = *event;
        ++eventCount;
    }
}


void nc_setEventHandler( GLView* view )
{
    glv_setEventHandler( view, bufferEvent );
}


/**
  Call nc_readEvents before nc_nextEvent().
*/
void nc_readEvents( GLView* view )
{
    currEvent  = 0;
    eventCount = 0;
    glv_handleEvents( view );
}


/**
  Returns zero when there are no more events.
*/
GLViewEvent* nc_nextEvent()
{
    GLViewEvent* e;

    while( currEvent != eventCount )
    {
        e = events + currEvent;
        ++currEvent;
        return (e);
    }
    return (0);
}


/*--------------------------------------------------------------------------*/


int quit = 0;


void repaint( GLView* view )
{
    float w, h;

    w = (float) view->width;
    h = (float) view->height;


    glClearColor( 0.1f, 0.2f, 1.0f, 0 );
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );


    glColor3f( 1, 1, 1 );

    glBegin( GL_LINE_LOOP );
    glVertex2f( 0, 0 );
    glVertex2f( w, 0 );
    glVertex2f( w, h );
    glVertex2f( 0, h );
    glEnd();

    glBegin( GL_LINES );
    glVertex2f( 0, 0 );
    glVertex2f( w, h );
    glEnd();


    glv_swapBuffers( view );
}


void handleEvent( GLView* view, GLViewEvent* event )
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
            glOrtho( -4, w+4, -4, h+4, -1.0, 1.0 );
            glMatrixMode( GL_MODELVIEW );

            repaint( view );
        }
            break;

        case GLV_EVENT_EXPOSE:
            printf( "testExpose\n" );
            repaint( view );
            break;

        case GLV_EVENT_KEY_DOWN:
            // NOTE: Cannot call KEY_ASCII() here!
            if( event->code == KEY_Escape )
                quit = 1;
            break;

        case GLV_EVENT_CLOSE:
            printf( "testClose\n" );
            quit = 1;
            break;
    }
}


int main( int argc, char** argv )
{
    GLView* view;
    GLViewMode mode;


    if( (view = glv_create( GLV_ATTRIB_DOUBLEBUFFER )) )
    {
        int i;

        glv_setTitle( view, "GLView Library Test" );

        nc_setEventHandler( view );

        mode.id     = GLV_MODEID_WINDOW;
        mode.width  = 640;
        mode.height = 480;

        glv_changeMode( view, &mode );

        while( ! quit )
        {
            GLViewEvent* ev;

            glv_waitEvent( view );

            nc_readEvents( view );
            while( (ev = nc_nextEvent()) )
                handleEvent( view, ev );
        }

        glv_destroy( view );
    }

    return( 0 );
}


/*EOF*/
