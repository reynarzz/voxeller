#include <Voxeller/Math/VoxTransforms.h>
#include <Voxeller/Math/VoxVector.h>
#include <cmath>

namespace Voxeller
{
    vox_mat4 perspective(f32 fovY, f32 aspect, f32 near, f32 far)
    {
        f32 f = 1.0f / std::tan(fovY * 0.5f);
        // zero‐init all elements
        vox_mat4 m(
            0, 0, 0, 0,
            0, 0, 0, 0,
            0, 0, 0, 0,
            0, 0, 0, 0
        );
        // column‐major: assign M_c[row][col]
        m.m00 = f / aspect;                                       // M[0][0]
        m.m11 = f;                                                // M[1][1]
        m.m22 = (far + near) / (near - far);                      // M[2][2]
        m.m23 = -1.0f;                                            // M[2][3]
        m.m32 = (2.0f * far * near) / (near - far);               // M[3][2]
        // M[3][3] stays 0
        return m;
    }

    vox_mat4 orthographic(f32 left, f32 right, f32 bottom, f32 top, f32 near, f32 far)
    {
        // start with zeros, but set M[3][3]=1
        vox_mat4 m(
            0, 0, 0, 0,
            0, 0, 0, 0,
            0, 0, 0, 0,
            0, 0, 0, 1
        );
        // diagonal
        m.m00 = 2.0f / (right - left);   // M[0][0]
        m.m11 = 2.0f / (top - bottom); // M[1][1]
        m.m22 = -2.0f / (far - near);   // M[2][2]
        // translation in column 3 (rows 0–2)
        m.m30 = -(right + left) / (right - left);  // M[3][0]
        m.m31 = -(top + bottom) / (top - bottom); // M[3][1]
        m.m32 = -(far + near) / (far - near);   // M[3][2]
        return m;
    }

    vox_mat4 translate(const vox_vec3& v)
    {
        vox_mat4 m = vox_mat4::identity;
        // put translation in 4th column (column-major!)
        m.m30 = v.x;  // M[3][0]
        m.m31 = v.y;  // M[3][1]
        m.m32 = v.z;  // M[3][2]
        return m;
    }

    vox_mat4 translate(const vox_mat4& M, const vox_vec3& v)
    {
        return M * translate(v);
    }

    vox_mat4 rotate(f32 angle, const vox_vec3& axis)
    {
        auto q = vox_quat::fromAxisAngle(axis, angle).normalized();
        return rotate(q);
    }

    vox_mat4 rotate(const vox_quat& q_)
    {
        // ensure unit‐quaternion
        auto q = q_.normalized();

        float xx = q.x * q.x, yy = q.y * q.y, zz = q.z * q.z;
        float xy = q.x * q.y, xz = q.x * q.z, yz = q.y * q.z;
        float wx = q.w * q.x, wy = q.w * q.y, wz = q.w * q.z;

        // transpose of the row-major form → column-major mat
        return vox_mat4(
            // row 0
            1.f - 2.f * (yy + zz), 2.f * (xy + wz), 2.f * (xz - wy), 0.f,
            // row 1
            2.f * (xy - wz), 1.f - 2.f * (xx + zz), 2.f * (yz + wx), 0.f,
            // row 2
            2.f * (xz + wy), 2.f * (yz - wx), 1.f - 2.f * (xx + yy), 0.f,
            // row 3
            0.f, 0.f, 0.f, 1.f
        );
    }

    vox_mat4 rotate(const vox_mat4& M, f32 angle, const vox_vec3& axis)
    {
        return M * rotate(angle, axis);
    }

    vox_mat4 rotate(const vox_mat4& M, const vox_quat& q)
    {
        return M * rotate(q);
    }

    vox_mat4 scale(const vox_vec3& s)
    {
        vox_mat4 m = vox_mat4::identity;
        m.m00 = s.x;  // M[0][0]
        m.m11 = s.y;  // M[1][1]
        m.m22 = s.z;  // M[2][2]
        return m;
    }

    vox_mat4 scale(const vox_mat4& M, const vox_vec3& s)
    {
        return M * scale(s);
    }
}
