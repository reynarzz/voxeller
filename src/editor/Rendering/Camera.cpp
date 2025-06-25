#include "Camera.h"
#include <GUI/Screen.h>
#include <Unvoxeller/Log/Log.h>
#include <glm/gtc/matrix_transform.hpp>
#include <imgui/imgui.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <Time/Time.h>


glm::vec3 Front, Right, Up;
glm::vec3 Position;
float     Yaw = -90.f;
float Pitch = 0.0f;

bool   firstMouse = true;
ImVec2 lastPos = { 0, 0 };
ViewState Camera::_state = {};

static glm::mat4 view() { return glm::lookAt(Position, Position + Front, Up); }

void updateVectors()
{
    glm::vec3 f{
        cos(glm::radians(Yaw)) * cos(glm::radians(Pitch)),
        sin(glm::radians(Pitch)),
        sin(glm::radians(Yaw)) * cos(glm::radians(Pitch))
    };
    Front = glm::normalize(f);
    Right = glm::normalize(glm::cross(Front, { 0.f, 1.f, 0.f }));
    Up = glm::normalize(glm::cross(Right, Front));
}

void look(float dx, float dy, float sensitivity = 0.1f) 
{
    Yaw += dx * sensitivity;
    Pitch += dy * sensitivity;

    Pitch = glm::clamp(Pitch, -89.f, 89.f);  // avoid gimbal lock
    updateVectors();
}

// WASD style movement
void move(float forward, float right, float up,
    float speed, float dt) 
{
    glm::vec3 dir = Front * forward + Right * right + Up * up;
    if (glm::length(dir) > 0.f)
        Position += glm::normalize(dir) * speed * dt;
}


void Camera::Update()
{
    updateVectors();

    if(_state.ScrWidth != Screen::GetTargetWidth() || _state.ScrHeight != Screen::GetTargetHeight())
    {
        _state.ScrWidth = Screen::GetTargetWidth();
        _state.ScrHeight = Screen::GetTargetHeight();
        LOG_INFO("Camera size: ({0}, {1})", _state.ScrWidth, _state.ScrHeight);
        
        _state.ProjectionMatrix = glm::perspective(glm::radians(65.0f), _state.ScrWidth / _state.ScrHeight, _state.NearPlane, _state.FarPlane);
        
       
    }

    _state.Color = { 0.1f, 0.1f, 0.1f, 1.0f };

    UpdateMovement();

    _state.ViewMatrix = glm::lookAt(Position, Position + Front, Up) * glm::inverse(glm::translate(glm::mat4(1.0f), { 0, 35, 200 }));

    _state.ProjectionViewMatrix = _state.ProjectionMatrix * _state.ViewMatrix;

}

void Camera::UpdateMovement()
{
    ImGuiIO& io = ImGui::GetIO();

    if (io.MouseDown[1])            // right-button look
    {
        if (firstMouse) { lastPos = io.MousePos; firstMouse = false; }

        float dx = io.MousePos.x - lastPos.x;
        float dy = lastPos.y - io.MousePos.y;               // invert Y
        lastPos = io.MousePos;

        look(dx, dy);                                   // sensitivity inside
    }
    else { firstMouse = true; }

     //--- keyboard movement (WSAD + Space/Ctrl) ---
   // if (!io.WantCaptureKeyboard)            // ignore when typing in a widget
    {
        auto key = [](ImGuiKey k) -> float
            {
                return ImGui::IsKeyDown(k) ? 1.0f : 0.0f;   // bool → float
            };

        float f = key(ImGuiKey_W) - key(ImGuiKey_S);          // forward / back
        float r = key(ImGuiKey_D) - key(ImGuiKey_A);          // right / left
        float up = key(ImGuiKey_E) - key(ImGuiKey_Q); // rise / descend

        move(f, r, up, /*speed*/ 35.f, Time::GetDeltaTime());
    }
}

void Camera::SetBackgroundColor(f32 r, f32 g, f32 b, f32 a)
{
    _state.Color = { r, g, b, a};
}

const ViewState &Camera::GetState() const
{
    return _state;
}