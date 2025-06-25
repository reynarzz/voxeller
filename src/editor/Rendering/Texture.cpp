#include "Texture.h"
#include <Rendering/RenderingSystem.h> 

std::shared_ptr<Texture> Texture::Create(const TextureDescriptor *desc)
{
    return RenderingSystem::GetDevice().lock()->CreateTexture(desc);
}

s32 Texture::GetWidth() const
{
    return Width;
}

s32 Texture::GetHeight() const
{
    return Height;
}

Texture::~Texture() = default;