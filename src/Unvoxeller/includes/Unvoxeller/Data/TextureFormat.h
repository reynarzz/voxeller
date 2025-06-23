#pragma once

namespace Unvoxeller 
{
    enum class TextureFormat
    {
        // Raw texture: 4 channels, 4 bytes per pixel, 8 bits per channel.
        RGBA8,
        // Encode as .png
        PGN,
        // Encode as .jpg
        JPG,
        // Encode as .tga
        TGA,
    };
}