#include "Camera.h"
#include <Unvoxeller/Math/VoxMath.h>

void Camera::Update(f32 width, f32 height)
{
    if(_state.ScrWidth != width || _state.ScrHeight != height)
    {
        _state.ScrWidth = width;
        _state.ScrHeight = height;

        _state.ProjectionMatrix = Unvoxeller::perspective(Unvoxeller::radians(65), width / height, _state.NearPlane, _state.FarPlane);
    }
}

const RendererState &Camera::GetState() const
{
    return _state;
}