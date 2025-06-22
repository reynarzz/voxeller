#pragma once
#include <Unvoxeller/Types.h>


struct TextureDescriptor
{
    s32 width;
    s32 height;
    s32 channels;
    u8* image;
    
    bool GenMipMaps = false;
};
