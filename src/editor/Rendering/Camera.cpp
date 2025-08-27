#include "Camera.h"
#include <GUI/Screen.h>
#include <Unvoxeller/Log/Log.h>
#include <glm/gtc/matrix_transform.hpp>
#include <imgui/imgui.h>
#include <glm/glm.hpp>
#include <Time/Time.h>

// Camera state
glm::vec3 Position;
glm::vec3 Target = { 0.0f, 0.0f, 0.0f };  // orbit center
glm::vec3 Front, Right, Up;

float Yaw = -90.f;
float Pitch = 20.0f;
float Distance = 250;   // zoom distance
float defaultDist= 250; 
bool   firstMouse = true;
ImVec2 lastPos = { 0, 0 };

ViewState Camera::_state = {};

static glm::mat4 view() { return glm::lookAt(Position, Target, Up); }
bool _isViewingObjectViewport = false;


void updateOrbitCamera()
{
    // spherical → cartesian
    float x = Distance * cos(glm::radians(Pitch)) * cos(glm::radians(Yaw));
    float y = Distance * sin(glm::radians(Pitch));
    float z = Distance * cos(glm::radians(Pitch)) * sin(glm::radians(Yaw));

    Position = Target + glm::vec3(x, y, z);

    Front = glm::normalize(Target - Position);
    Right = glm::normalize(glm::cross(Front, { 0.f, 1.f, 0.f }));
    Up = glm::normalize(glm::cross(Right, Front));
}

void Camera::Update()
{
    ImGuiIO& io = ImGui::GetIO();

    if (ImGui::IsWindowHovered() && (ImGui::GetIO().MouseDown[1] || ImGui::GetIO().MouseDown[2] || io.MouseWheel != 0)) 
    {
        _isViewingObjectViewport = true;
    }
    else if(!ImGui::GetIO().MouseDown[1] && !ImGui::GetIO().MouseDown[2] &&  io.MouseWheel ==0)
    {
        _isViewingObjectViewport = false;
    }

    // Rotation (RMB drag)
    if (io.MouseDown[1] && _isViewingObjectViewport)
    {
        if (firstMouse) { lastPos = io.MousePos; firstMouse = false; }

        float dx = io.MousePos.x - lastPos.x;
        float dy = lastPos.y - io.MousePos.y;
        lastPos = io.MousePos;

        Yaw += dx * 0.3f;
        Pitch -= dy * 0.3f;

        Pitch = glm::clamp(Pitch, -89.f, 89.f);
    }
    // Panning (MMB drag)
    else if (io.MouseDown[2] && _isViewingObjectViewport)
    {
        
       

        if (firstMouse) { lastPos = io.MousePos; firstMouse = false; }

        float dx = io.MousePos.x - lastPos.x;
        float dy = lastPos.y - io.MousePos.y;
        lastPos = io.MousePos;

        // Pan speed depends on distance to target
        float panSpeed = 0.01f * Distance;

        Target -= Right * dx * panSpeed;
        Target -= Up * dy * panSpeed;
    }
    else
    {
        firstMouse = true;
    }

    if(_isViewingObjectViewport)
    {
        // Zoom (mouse wheel)
        Distance -= io.MouseWheel * 2.0f;
        Distance = glm::clamp(Distance, 2.0f, 1000.0f);
    }
   
    if (ImGui::IsKeyPressed(ImGuiKey_F))
    {
        // Reset orbit target & zoom
        Target = { 0.0f, 0.0f, 0.0f };
        Distance = defaultDist;
        Pitch = 20.0f;
        Yaw = -90.0f;
    }

    // Update camera vectors
    updateOrbitCamera();

    // Handle window resize
    if (_state.ScrWidth != Screen::GetTargetWidth() || _state.ScrHeight != Screen::GetTargetHeight())
    {
        _state.ScrWidth = Screen::GetTargetWidth();
        _state.ScrHeight = Screen::GetTargetHeight();
        LOG_INFO("Camera size: ({0}, {1})", _state.ScrWidth, _state.ScrHeight);

        _state.ProjectionMatrix = glm::perspective(glm::radians(65.0f),
            (float)_state.ScrWidth / (float)_state.ScrHeight,
            _state.NearPlane, _state.FarPlane);
    }

    _state.Color = { 0.1f, 0.1f, 0.1f, 1.0f };

    // Compute matrices
    _state.ViewMatrix = glm::lookAt(Position, Target, Up);
    _state.ProjectionViewMatrix = _state.ProjectionMatrix * _state.ViewMatrix;
}

void Camera::SetBackgroundColor(f32 r, f32 g, f32 b, f32 a)
{
    _state.Color = { r, g, b, a };
}

const ViewState& Camera::GetState() const
{
    return _state;
}
