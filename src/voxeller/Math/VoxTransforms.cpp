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

    vox_mat4 translate(const vox_vec3& v) 
    {
        vox_mat4 m = vox_mat4::identity;
        m.m03 = v.x;
        m.m13 = v.y;
        m.m23 = v.z;
        return m;
    }

    vox_mat4 translate(const vox_mat4& m, const vox_vec3& v)
    {
        return m * translate(v);
    }


    // Rotate by axis-angle: build a vox_quaternion then delegate
    vox_mat4 rotate(float angle, const vox_vec3& axis) 
    {
        vox_quat q = vox_quat::fromAxisAngle(axis, angle).normalized();
        return rotate(q);
    }

    // Rotate by unit vox_quaternion q
    vox_mat4 rotate(const vox_quat& q_) 
    {
        // ensure unit length
        vox_quat q = q_.normalized();

        float xx = q.x * q.x;
        float yy = q.y * q.y;
        float zz = q.z * q.z;
        float xy = q.x * q.y;
        float xz = q.x * q.z;
        float yz = q.y * q.z;
        float wx = q.w * q.x;
        float wy = q.w * q.y;
        float wz = q.w * q.z;

        return vox_mat4
        {
            1.f - 2.f * (yy + zz),  2.f * (xy - wz),       2.f * (xz + wy),       0.f,
            2.f * (xy + wz),        1.f - 2.f * (xx + zz), 2.f * (yz - wx),       0.f,
            2.f * (xz - wy),        2.f * (yz + wx),       1.f - 2.f * (xx + yy), 0.f,
            0.f,                    0.f,                   0.f,                   1.f
        };
    }

    // rotate(m, angle, axis) ≡ m * R(axis, angle)
    vox_mat4 rotate(const vox_mat4& m, float angle, const vox_vec3& axis)
    {
        return m * rotate(angle, axis);
    }

    // rotate(m, q) ≡ m * R(q)
    vox_mat4 rotate(const vox_mat4& m, const vox_quat& q)
    {
        return m * rotate(q);
    }

    // Scale by vector s
    vox_mat4 scale(const vox_vec3& s) 
    {
        vox_mat4 m = vox_mat4::identity;
        m.m00 = s.x;
        m.m11 = s.y;
        m.m22 = s.z;
        return m;
    }

    // scale(m, s) ≡ m * S(s)
    vox_mat4 scale(const vox_mat4& m, const vox_vec3& s)
    {
        return m * scale(s);
    }
} 