#include <windows.h>
#include <stdio.h>

#define MAX_NUM_ARGVS    20

extern HINSTANCE hInstance;
extern int main( int, char** );

/*
  This WinMain just gets us to a normal main().
*/
int WINAPI WinMain( HINSTANCE hi, HINSTANCE hPrevInstance,
                    LPSTR lpCmdLine, int nCmdShow )
{
    int argc;
    char* argv[ MAX_NUM_ARGVS ];
    (void) nCmdShow;

    /* previous instances do not exist in Win32 */
    if( hPrevInstance )
        return 0;

    hInstance = hi;

#if 1
    /* This allows us to see printf() by piping output to 'more' in a command
     * prompt.  When using Cygwin rxvt this gives us normal UNIX behavior
     * (woohoo!).
     */
    setbuf( stdout, NULL );
#endif

    argc = 1;
    argv[0] = "prog_name";

    while( *lpCmdLine && (argc < MAX_NUM_ARGVS) )
    {
        while( *lpCmdLine && ((*lpCmdLine <= 32) || (*lpCmdLine > 126)) )
            lpCmdLine++;

        if( *lpCmdLine )
        {
            argv[ argc ] = lpCmdLine;
            argc++;

            while( *lpCmdLine && ((*lpCmdLine > 32) && (*lpCmdLine <= 126)) )
                lpCmdLine++;

            if( *lpCmdLine )
            {
                *lpCmdLine = 0;
                lpCmdLine++;
            }

        }
    }

    return main( argc, argv );
}
