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

    GLTexture(const GLTexture&) = delete;
    GLTexture& operator=(const GLTexture&) = delete;

    // These in a cpp
    GLTexture(GLTexture&&) noexcept;
    GLTexture& operator=(GLTexture&&) noexcept;

    u32 GetID() const;
    void Bind(s32 index) const;

private:
    u32 _id = 0;
};