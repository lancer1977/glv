/*
*/


#include <stdio.h>
#include "glv_activity.h"
#include "glv_asset.h"


extern struct android_app* gGlvApp;


AAssetManager* glv_assetManager()
{
    return gGlvApp ? gGlvApp->activity->assetManager : NULL;
}


/*
  Open Android package asset for FILE* access.

  Return non-zero if successful.
*/
int glv_assetOpen( struct AssetFile* af, const char* file, const char* mode )
{
    af->asset = AAssetManager_open( gGlvApp->activity->assetManager, file,
                                    AASSET_MODE_UNKNOWN );
    if( af->asset )
    {
        off_t offset;
        int fd = AAsset_openFileDescriptor( af->asset, &offset, &af->len );
        if( fd >= 0 )
        {
            af->fp = fdopen( fd, mode );
            if( af->fp )
            {
                if( offset )
                    fseek( af->fp, offset, SEEK_SET );
                return 1;
            }
        }
        AAsset_close( af->asset );
        af->asset = 0;
    }
    af->fp = 0;
    return 0;
}


void glv_assetClose( struct AssetFile* af )
{
    if( af->fp )
    {
        fclose( af->fp );
        af->fp = 0;
    }
    if( af->asset )
    {
        AAsset_close( af->asset );
        af->asset = 0;
    }
}
