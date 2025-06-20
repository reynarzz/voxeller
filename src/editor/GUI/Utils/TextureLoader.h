#pragma once
#include <Unvoxeller/Types.h>
#include <vector>
#include <string>
#include <Rendering/Texture.h>


struct TextureLoadData
{
    TextureDescriptor desc;
    
    ~TextureLoadData();
};


std::shared_ptr<TextureLoadData> LoadRawTexture(const std::string& path);
std::shared_ptr<Texture> Loadexture(const std::string& path);