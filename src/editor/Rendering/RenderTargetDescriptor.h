#pragma once
#include <Rendering/TextureDescriptor.h>


struct RenderTargetDescriptor
{
    TextureDescriptor texDescriptor;
    int samples = 1;
};