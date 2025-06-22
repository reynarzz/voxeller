#include "Camera.h"
#include <Unvoxeller/Math/VoxMath.h>
#include <GUI/Screen.h>

void Camera::Update()
{
    if(_state.ScrWidth != Screen::GetTargetWidth() || _state.ScrHeight != Screen::GetTargetHeight())
    {
        _state.ScrWidth = Screen::GetTargetWidth();
        _state.ScrHeight = Screen::GetTargetHeight();

        _state.ProjectionMatrix = Unvoxeller::perspective(Unvoxeller::radians(65), _state.ScrWidth / _state.ScrHeight, _state.NearPlane, _state.FarPlane);
    }

    _state.Color = {0.05f,0.05f,0.05f,1.0f};
}

void Camera::SetBackgroundColor(f32 r, f32 g, f32 b, f32 a)
{
    _state.Color = { r, g, b, a};
}

const RendererState &Camera::GetState() const
{
    return _state;
}