#pragma once

#include <vector>
#include <Rendering/Texture.h>
#include <Rendering/TextureDescriptor.h>

// Textures are inmmutable, once created, cannot change.
class GLTexture : public Texture
{
public:
    GLTexture(const TextureDescriptor* desc);
    ~GLTexture() override;
    u32 GetID() const;
    void Bind(s32 index);

private:
    u32 _id = 0;
};