#pragma once
#include <Unvoxeller/Math/VoxMatrix.h>

struct RendererState
{
    Unvoxeller::vox_mat4 ViewMatrix{};
    Unvoxeller::vox_mat4 ProjectionMatrix{};
    Unvoxeller::vox_mat4 ProjectionViewMatrix{};
    Unvoxeller::vox_vec4 Color{};

    f32 NearPlane = 0.1f;
    f32 FarPlane = 300.0f;
    f32 ScrWidth = 1;
    f32 ScrHeight = 1;
};