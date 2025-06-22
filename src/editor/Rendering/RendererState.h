#pragma once
#include <Unvoxeller/Math/VoxMatrix.h>

struct RendererState
{
    Unvoxeller::vox_mat4 ViewMatrix;
    Unvoxeller::vox_mat4 ProjectionMatrix;
    Unvoxeller::vox_mat4 ProjectionViewMatrix;
    Unvoxeller::vox_vec4 Color;
};