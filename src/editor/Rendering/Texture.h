#pragma once
#include <Unvoxeller/Types.h>
#include <memory>
#include <Rendering/TextureDescriptor.h>

class Texture 
{
public:
    s32 GetWidth() const;
    s32 GetHeight() const;

    static std::shared_ptr<Texture> Create(const TextureDescriptor* desc);
    virtual ~Texture() = 0;
    
protected:
    s32 Width;
    s32 Height;
};