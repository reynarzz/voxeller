#pragma once
#include <Rendering/RendererState.h>
#include <Rendering/RenderableTransform.h>

class Camera
{
public:
    static void Update();

    void SetBackgroundColor(f32 r,f32 g, f32 b, f32 a);

    const ViewState& GetState() const;
    
private:
    static void UpdateMovement();

    RenderableTransform _transform = {};
   static ViewState _state;
};