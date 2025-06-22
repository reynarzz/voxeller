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

class TextureLoader
{
public:
    static std::shared_ptr<TextureLoadData> LoadRawTexture(const std::string& path);
    static std::shared_ptr<Texture> LoadTexture(const std::string& path, bool genMipMaps = false);
};

