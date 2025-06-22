#pragma once
#include <Unvoxeller/Types.h>

class Screen
{
public:
    static f32 GetTargetWidth();
    static f32 GetTargetHeight();
private:
    friend class VoxToProcessView;
    static f32 _width;
    static f32 _height;
};