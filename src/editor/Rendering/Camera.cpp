#include "Camera.h"
#include <GUI/Screen.h>
#include <Unvoxeller/Log/Log.h>
#include <glm/gtc/matrix_transform.hpp>

void Camera::Update()
{
    if(_state.ScrWidth != Screen::GetTargetWidth() || _state.ScrHeight != Screen::GetTargetHeight())
    {
        _state.ScrWidth = Screen::GetTargetWidth();
        _state.ScrHeight = Screen::GetTargetHeight();
        LOG_INFO("Camera size: ({0}, {1})", _state.ScrWidth, _state.ScrHeight);
        
        _state.ProjectionMatrix = glm::perspective(glm::radians(65.0f), _state.ScrWidth / _state.ScrHeight, _state.NearPlane, _state.FarPlane);
        
        _state.ViewMatrix = glm::inverse(glm::translate(glm::mat4(1.0f), { 0, 35, 200}));

        _state.ProjectionViewMatrix = _state.ProjectionMatrix * _state.ViewMatrix;
    }

    _state.Color = { 0.1f, 0.1f, 0.1f, 1.0f };
}

void Camera::SetBackgroundColor(f32 r, f32 g, f32 b, f32 a)
{
    _state.Color = { r, g, b, a};
}

const RendererState &Camera::GetState() const
{
    return _state;
}