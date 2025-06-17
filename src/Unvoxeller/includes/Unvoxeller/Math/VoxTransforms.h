#pragma once

#include <Unvoxeller/Math/VoxMatrix.h>
#include <Unvoxeller/Math/VoxQuat.h>

namespace Unvoxeller 
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

    vox_mat4 translate(const vox_vec3& v);
    /// Post-multiply an existing transform by a translation
    vox_mat4 translate(const vox_mat4& m, const vox_vec3& v);


    // Rotate by angle (radians) around axis
    vox_mat4 rotate(f32 angle, const vox_vec3& axis);
    vox_mat4 rotate(const vox_quat& q);

    /// Post-multiply an existing transform by an axis-angle rotation
    vox_mat4 rotate(const vox_mat4& m, f32 angle, const vox_vec3& axis);

    /// Post-multiply an existing transform by a quaternion rotation
    vox_mat4 rotate(const vox_mat4& m, const vox_quat& q);

    // Scale by vector
    vox_mat4 scale(const vox_vec3& s);

    /// Post-multiply an existing transform by a non-uniform scale
    vox_mat4 scale(const vox_mat4& m, const vox_vec3& s);

}