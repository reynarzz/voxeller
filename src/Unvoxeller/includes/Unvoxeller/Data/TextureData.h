#pragma once
#include <vector>
#include <Unvoxeller/api.h>
#include <Unvoxeller/Types.h>
#include "TextureFormat.h"
#include <string>

namespace Unvoxeller 
{
    struct UNVOXELLER_API TextureData
    {
        std::string Name = "";
        std::vector<unsigned char> Buffer;
        TextureFormat Format;
        f32 Width;
        f32 Height;
    };
}