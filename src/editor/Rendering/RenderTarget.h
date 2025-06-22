#pragma once
#include <Unvoxeller/Types.h>

class RenderTarget
{
public:
    virtual void Resize(s32 width, s32 height) = 0;
    s32 GetWidth() const;
    s32 GetHeight() const;
protected:
    s32 _width;
    s32 _height;
};