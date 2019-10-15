#ifndef GLV_ASSET_H
#define GLV_ASSET_H


#include <android/asset_manager.h>


struct AssetFile
{
    AAsset* asset;
    FILE* fp;
    off_t len;
};


AAssetManager* glv_assetManager();
int  glv_assetOpen( struct AssetFile*, const char* file, const char* mode );
void glv_assetClose( struct AssetFile* );


#endif  // GLV_ASSET_H
