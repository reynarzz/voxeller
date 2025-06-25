#pragma once
#include <Unvoxeller/Types.h>
#include <memory>
#include <Rendering/TextureDescriptor.h>

class Texture
{
public:
    s32 GetWidth() const;
    s32 GetHeight() const;

    Texture() = default;
    virtual ~Texture() = 0;

    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    Texture(Texture&&) noexcept = default;
    Texture& operator=(Texture&&) noexcept = default;

    static std::shared_ptr<Texture> Create(const TextureDescriptor* desc);
    
protected:
    s32 Width = 0;
    s32 Height = 0;
};