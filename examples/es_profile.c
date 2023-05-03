/*
  OpenGL ES GLV Example
  Draws a multi-colored triangle on a blue background.
*/


#if 0
#include <GLES3/gl3.h>
#else
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <glv.h>
#include <glv_keys.h>


#define USER    ((UserData*) view->user)

typedef struct
{
    int    quit;
    GLuint shader;
}
UserData;


GLuint loadShader( GLenum type, const char* code )
{
   GLuint shader;
   GLint compiled;
   
   // Create the shader object.
   shader = glCreateShader( type );
   if( ! shader )
        return 0;

   glShaderSource( shader, 1, &code, NULL );
   glCompileShader( shader );

   // Check the compile status.
   glGetShaderiv( shader, GL_COMPILE_STATUS, &compiled );
   if( ! compiled ) 
   {
      GLint infoLen = 0;
      glGetShaderiv( shader, GL_INFO_LOG_LENGTH, &infoLen );
      if( infoLen > 1 )
      {
         char* infoLog = malloc( sizeof(char) * infoLen );
         glGetShaderInfoLog( shader, infoLen, NULL, infoLog );
         fprintf( stderr, "Error compiling shader:\n%s\n", infoLog );
         free( infoLog );
      }
      glDeleteShader( shader );
      return 0;
   }
   return shader;
}


static GLfloat triangleAttr[] =
{
    0.0f,  0.5f, 0.0f,   1.0f, 0.0f, 0.0f,
   -0.5f, -0.5f, 0.0f,   0.0f, 1.0f, 0.0f,
    0.5f, -0.5f, 0.0f,   0.0f, 0.0f, 1.0f
};

enum TriangleAttributes
{
    TRI_POS,
    TRI_COL
};


int setup( GLView* view )
{
    GLuint vertShader;
    GLuint fragShader;
    GLuint prog;
    GLint  linked;
    char vertProg[] =
        "attribute vec4 vPosition;\n"
        "attribute vec4 vColor;\n"
        "varying vec4 color;\n"
        "void main() {\n"
        "   gl_Position = vPosition;\n"
        "   color = vColor;\n"
        "}\n";
    char fragProg[] =
        "precision mediump float;\n"
        "varying vec4 color;\n"
        "void main() {\n"
        "    gl_FragColor = color;\n"
        "}\n";

    // Load the vertex/fragment shaders.
    vertShader = loadShader( GL_VERTEX_SHADER,   vertProg );
    fragShader = loadShader( GL_FRAGMENT_SHADER, fragProg );

    // Create the program object.
    prog = glCreateProgram();
    if( prog == 0 )
        return 0;
    glAttachShader( prog, vertShader );
    glAttachShader( prog, fragShader );

    // Bind attributes.
    glBindAttribLocation( prog, TRI_POS, "vPosition" );
    glBindAttribLocation( prog, TRI_COL, "vColor" );

    // Link the program
    glLinkProgram( prog );
    glGetProgramiv( prog, GL_LINK_STATUS, &linked );
    if( ! linked )
    {
        GLint infoLen = 0;

        glGetProgramiv( prog, GL_INFO_LOG_LENGTH, &infoLen );
        if( infoLen > 1 )
        {
            char* infoLog = malloc( sizeof(char) * infoLen );
            glGetProgramInfoLog( prog, infoLen, NULL, infoLog );
            fprintf( stderr, "Error linking program:\n%s\n", infoLog );
            free( infoLog );
        }

        glDeleteProgram( prog );
        return 0;
    }
    USER->shader = prog;

    glClearColor( 0.1f, 0.2f, 1.0f, 0 );
    return 1;
}


void draw( GLView* view )
{
    glViewport( 0, 0, view->width, view->height );
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    glUseProgram( USER->shader );
    glVertexAttribPointer( TRI_POS, 3, GL_FLOAT, GL_FALSE,
                           sizeof(GL_FLOAT) * 6, triangleAttr );
    glVertexAttribPointer( TRI_COL, 3, GL_FLOAT, GL_FALSE,
                           sizeof(GL_FLOAT) * 6, triangleAttr + 3 );
    glEnableVertexAttribArray( TRI_POS );
    glEnableVertexAttribArray( TRI_COL );
    glDrawArrays( GL_TRIANGLES, 0, 3 );

    glv_swapBuffers( view );
}


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
    switch( event->type )
    {
        case GLV_EVENT_RESIZE:
            draw( view );
            break;

        case GLV_EVENT_CLOSE:
            USER->quit = 1;
            break;

        case GLV_EVENT_KEY_DOWN:
            if( event->code == KEY_Escape )
                USER->quit = 1;
            break;
    }
}


int main(int argc, char** argv)
{
    GLView* view;
    GLViewMode mode;
    UserData data;
    int i;
    int attr = GLV_ATTRIB_DOUBLEBUFFER | GLV_ATTRIB_ES;
    int version = 0;


    // Default to a window if no fullscreen modes are available.
    mode.id     = GLV_MODEID_WINDOW;
    mode.width  = 640;
    mode.height = 480;

    for (i = 1; i < argc; ++i) {
        if (argv[i][0] == '-') {
            switch (argv[i][1]) {
                case 'm':
                    attr |= GLV_ATTRIB_MULTISAMPLE;
                    break;
                case 's':
                    attr |= GLV_ATTRIB_STENCIL;
                    break;
                case 'r':
                    setenv("GLV_REPORT", "1", 1);
                    break;
                case 'v':
                    if (++i < argc) {
                        char* vp = argv[i];
                        version  = (vp[0] - '0') << 8;
                        version |= (vp[2] - '0');
                    }
                    break;
            }
        }
    }

    view = glv_create(attr, version);
    if( view )
    {
        printf( "GL_VERSION: %s\n", (char*) glGetString( GL_VERSION ) );

        view->user = &data;
        glv_setTitle( view, "GLV ES Test" );
        glv_setEventHandler( view, eventHandler );

        glv_changeMode( view, &mode );

        if( setup( view ) )
        {
            draw( view );

            // Wait until the escape key is pressed.
            data.quit = 0;
            while( ! data.quit )
            {
                glv_waitEvent( view );
                glv_handleEvents( view );
            }
        }

        glv_destroy( view );
    }

    return 0;
}
