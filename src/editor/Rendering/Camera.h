#pragma once
#include <Rendering/RendererState.h>


class Camera
{
    public:
        void Update();
        void SetBackgroundColor(f32 r,f32 g, f32 b, f32 a);

        const RendererState& GetState() const;

    private:

    RendererState _state = {};
};