#pragma once

#include <Voxeller/Math/VoxMatrix.h>

namespace Voxeller 
{
    // Create a perspective projection matrix
    // fovY: vertical field of view in radians
    // aspect: width/height
    // near: distance to near clipping plane (>0)
    // far: distance to far clipping plane (> near)
    vox_mat4 perspective(f32 fovY, f32 aspect, f32 near, f32 far);

    // Create an orthographic projection matrix
    // left, right: x clipping planes
    // bottom, top: y clipping planes
    // near, far: near/far clipping planes
    vox_mat4 orthographic(f32 left, f32 right, f32 bottom, f32 top,  f32 near, f32 far);

} // namespace vox