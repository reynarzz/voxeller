#pragma once
#include <Unvoxeller/Math/VoxMatrix.h>
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

struct RendererState
{
    glm::mat4 ViewMatrix{};
    glm::mat4 ProjectionMatrix{};
    glm::mat4 ProjectionViewMatrix{};
    glm::vec4 Color{};

    f32 NearPlane = 0.1f;
    f32 FarPlane = 500.0f;
    f32 ScrWidth = 1;
    f32 ScrHeight = 1;
};