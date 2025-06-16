#include <Voxeller/Math/VoxTransforms.h>
#include <Voxeller/Math/VoxVector.h>
#include <cmath>


namespace Voxeller
{
    vox_mat4 perspective(f32 fovY, f32 aspect, f32 near, f32 far) 
    {
        f32 f = 1.0f / std::tan(fovY * 0.5f);
        vox_mat4 m(0, 0, 0, 0,
            0, 0, 0, 0,
            0, 0, 0, 0,
            0, 0, 0, 0);
        m.m00 = f / aspect;
        m.m11 = f;
        m.m22 = (far + near) / (near - far);
        m.m23 = (2.0f * far * near) / (near - far);
        m.m32 = -1.0f;
        return m;
    }

    vox_mat4 orthographic(f32 left, f32 right, f32 bottom, f32 top, f32 near, f32 far)
    {
        vox_mat4 m(0, 0, 0, 0,
            0, 0, 0, 0,
            0, 0, 0, 0,
            0, 0, 0, 1);
        m.m00 = 2.0f / (right - left);
        m.m11 = 2.0f / (top - bottom);
        m.m22 = -2.0f / (far - near);
        m.m03 = -(right + left) / (right - left);
        m.m13 = -(top + bottom) / (top - bottom);
        m.m23 = -(far + near) / (far - near);
        return m;
    }

    vox_mat4 translate(const vox_vec3& v);
    // Rotate by angle (radians) around axis
    vox_mat4 rotate(float angle, const vox_vec3& axis);
    // Scale by vector
    vox_mat4 scale(const vox_vec3& s);
} 