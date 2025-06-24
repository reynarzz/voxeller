#pragma once
#include <Unvoxeller/Math/VoxMatrix.h>
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

struct ViewState
{
    glm::mat4 ViewMatrix{};
    glm::mat4 ProjectionMatrix{};
    glm::mat4 ProjectionViewMatrix{};

    f32 NearPlane = 0.1f;
    f32 FarPlane = 500.0f;
    f32 ScrWidth = 1;
    f32 ScrHeight = 1;

    glm::vec4 Color{};
};

struct LightState
{
    glm::vec3 lightDir { 0.0f, -0.5, 0.5 };   
    glm::vec3 lightColor { 1.0f, 1.0f, 1.0f };
    glm::vec3 shadowColor { 1.0f, 1.0f, 1.0f };
};

struct RendererState
{
    const ViewState* ViewState;
    const LightState* LightState;
};