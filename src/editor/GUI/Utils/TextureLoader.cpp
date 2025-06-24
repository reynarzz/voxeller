#include "TextureLoader.h"

#include <stb/stb_image.h>


std::shared_ptr<TextureLoadData> TextureLoader::LoadRawTexture(const std::string &path)
{
    std::shared_ptr<TextureLoadData> data = std::make_shared<TextureLoadData>();

    //stbi_set_flip_vertically_on_load(1);
    
    data->desc.image = stbi_load(path.c_str(), &data->desc.width, &data->desc.height, &data->desc.channels, 4);

    return data;
}

std::shared_ptr<Texture> TextureLoader::LoadTexture(const std::string &path, bool genMipMaps)
{
    auto tex = LoadRawTexture(path);
    tex->desc.GenMipMaps = genMipMaps;
    
    return Texture::Create(&tex->desc);
}

TextureLoadData::~TextureLoadData() 
{
    stbi_image_free(desc.image);
}

