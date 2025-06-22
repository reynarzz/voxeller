#pragma once
#include <Rendering/RendererState.h>


class Camera
{
    public:
        void Update(f32 width, f32 height);
        const RendererState& GetState() const;
    private:

    RendererState _state = {};
};