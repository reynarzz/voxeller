#pragma once
#include <Rendering/RendererState.h>
#include <Rendering/RenderableTransform.h>

class Camera
{
public:
    void Update();
    void SetBackgroundColor(f32 r,f32 g, f32 b, f32 a);

    const ViewState& GetState() const;
    
private:
    RenderableTransform _transform = {};
    ViewState _state = {};
};