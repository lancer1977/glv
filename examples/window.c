/*===========================================================================/

  $Id: window.c,v 1.9 2004/09/28 05:13:42 karl Exp $

  GLV Library
  Copyright (C) 2003-2006  Karl Robillard

  This example shows how to open a GLView in a desktop window.
  Press the escape key or click on the window close widget to exit.

/===========================================================================*/


#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <glv.h>
#include <glv_keys.h>


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
            if( event->code == KEY_Escape )
                quit = 1;
            break;

        case GLV_EVENT_CLOSE:
            printf( "testClose\n" );
            quit = 1;
            break;
    }
}


void printExtensions()
{
    GLubyte buf[80];
    GLubyte* cp;
    const GLubyte* all;

    all = glGetString(GL_VERSION);
    printf( "GL_VERSION: %s\n", all );

    all = glGetString(GL_EXTENSIONS);
    while( *all != '\0' )
    {
        cp = buf;
        while( (*all != '\0') && (*all != ' ') )
            *cp++ = *all++;
        *cp = '\0';

        printf( "%s\n", buf );

        if( *all == ' ' )
            ++all;
    }
}


int main( int argc, char** argv )
{
    GLView* view;
    GLViewMode mode;

    (void) argc;
    (void) argv;

    view = glv_create( GLV_ATTRIB_DOUBLEBUFFER, 0 );
    if( view )
    {
        printExtensions();

        glv_setTitle( view, "GLView Library Test" );

        glv_setEventHandler( view, eventHandler );

        mode.id     = GLV_MODEID_WINDOW;
        mode.width  = 640;
        mode.height = 480;

        glv_changeMode( view, &mode );

        while( ! quit )
        {
            glv_waitEvent( view );
            glv_handleEvents( view );
        }

        glv_destroy( view );
    }

    return( 0 );
}


/*EOF*/
