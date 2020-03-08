#include <glv.h>
#include <glv_keys.h>


void pickMode( const GLViewMode* mode, void* data )
{
    // Pick the highest resolution available which is at least 16 bits deep.
    GLViewMode* pick = (GLViewMode*) data;
    if( (mode->width > pick->width) && (mode->depth >= 16) )
    {
        *pick = *mode;
    }
}


void eventHandler( GLView* view, GLViewEvent* event )
{
    if( event->type == GLV_EVENT_KEY_DOWN )
    {
        if( event->code == KEY_Escape )
        {
            int* quit = (int*) view->user;
            *quit = 1;
        }
    }
}


int main()
{
    int quit = 0;
    GLView* view;
    GLViewMode mode;

    // Default to a window if no fullscreen modes are available.
    mode.id     = GLV_MODEID_WINDOW;
    mode.width  = 640;
    mode.height = 480;

    glv_queryModes( pickMode, &mode );

    view = glv_create( GLV_ATTRIB_DOUBLEBUFFER, 0 );
    if( view )
    {
        view->user = &quit;
        glv_setEventHandler( view, eventHandler );

        glv_changeMode( view, &mode );

        // Draw a blue screen using GL calls.
        glClearColor( 0.1f, 0.2f, 1.0f, 0 );
        glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
        glv_swapBuffers( view );

        // Wait until the escape key is pressed.
        while( ! quit )
        {
            glv_waitEvent( view );
            glv_handleEvents( view );
        }

        glv_destroy( view );
    }

    return 0;
}
