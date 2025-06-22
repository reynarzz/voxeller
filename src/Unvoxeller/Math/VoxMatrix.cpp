#include <Unvoxeller/Math/VoxMatrix.h>

namespace Unvoxeller
{
    //––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––
    // 2×2 float (column-major storage: mXY = column X, row Y)
    //––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––
    const vox_mat2 vox_mat2::identity = { 1, 0,  0, 1 };
    vox_mat2::vox_mat2()
        : m00(1), m01(0)
        , m10(0), m11(1)
    {
    }
    vox_mat2::vox_mat2(f32 a00, f32 a01, f32 a10, f32 a11)
        : m00(a00), m01(a01)
        , m10(a10), m11(a11)
    {
    }

    vox_mat2 vox_mat2::operator+(const vox_mat2& o) const
    {
        return {
            m00 + o.m00, m01 + o.m01,
            m10 + o.m10, m11 + o.m11
        };
    }

    vox_mat2 vox_mat2::operator*(const vox_mat2& o) const
    {
        // C = A * B  (column-vector convention v′ = A·v)
        return {
            // col0,row0
            m00 * o.m00 + m10 * o.m01,
            // col1,row0
            m01 * o.m00 + m11 * o.m01,
            // col0,row1
            m00 * o.m10 + m10 * o.m11,
            // col1,row1
            m01 * o.m10 + m11 * o.m11
        };
    }

    vox_mat2 vox_mat2::operator*(f32 s) const
    {
        return { m00 * s, m01 * s, m10 * s, m11 * s };
    }

    vox_vec2 vox_mat2::operator*(const vox_vec2& v) const
    {
        // v′ = M * v  (column-vector)
        return {
            m00 * v.x + m10 * v.y,
            m01 * v.x + m11 * v.y
        };
    }

    vox_mat2& vox_mat2::operator+=(const vox_mat2& o)
    {
        m00 += o.m00; m01 += o.m01;
        m10 += o.m10; m11 += o.m11;
        return *this;
    }
    vox_mat2& vox_mat2::operator*=(const vox_mat2& o)
    {
        *this = *this * o;
        return *this;
    }
    vox_mat2& vox_mat2::operator*=(f32 s)
    {
        m00 *= s; m01 *= s;
        m10 *= s; m11 *= s;
        return *this;
    }

    //––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––
    // 3×3 float (column-major storage)
    //––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––
    const vox_mat3 vox_mat3::identity = {
        1,0,0,   // col0: (1,0,0)
        0,1,0,   // col1: (0,1,0)
        0,0,1    // col2: (0,0,1)
    };

    vox_mat3::vox_mat3()
        : m00(1), m01(0), m02(0)
        , m10(0), m11(1), m12(0)
        , m20(0), m21(0), m22(1)
    {
    }
    vox_mat3::vox_mat3(f32 a00, f32 a01, f32 a02,
        f32 a10, f32 a11, f32 a12,
        f32 a20, f32 a21, f32 a22)
        : m00(a00), m01(a01), m02(a02)
        , m10(a10), m11(a11), m12(a12)
        , m20(a20), m21(a21), m22(a22)
    {
    }

    vox_mat3 vox_mat3::operator+(const vox_mat3& o) const
    {
        return {
            m00 + o.m00, m01 + o.m01, m02 + o.m02,
            m10 + o.m10, m11 + o.m11, m12 + o.m12,
            m20 + o.m20, m21 + o.m21, m22 + o.m22
        };
    }

    vox_mat3 vox_mat3::operator*(const vox_mat3& o) const
    {
        // C = A * B
        return {
            // col0,row0
            m00 * o.m00 + m10 * o.m01 + m20 * o.m02,
            // col1,row0
            m01 * o.m00 + m11 * o.m01 + m21 * o.m02,
            // col2,row0
            m02 * o.m00 + m12 * o.m01 + m22 * o.m02,

            // col0,row1
            m00 * o.m10 + m10 * o.m11 + m20 * o.m12,
            // col1,row1
            m01 * o.m10 + m11 * o.m11 + m21 * o.m12,
            // col2,row1
            m02 * o.m10 + m12 * o.m11 + m22 * o.m12,

            // col0,row2
            m00 * o.m20 + m10 * o.m21 + m20 * o.m22,
            // col1,row2
            m01 * o.m20 + m11 * o.m21 + m21 * o.m22,
            // col2,row2
            m02 * o.m20 + m12 * o.m21 + m22 * o.m22
        };
    }

    vox_mat3 vox_mat3::operator*(f32 s) const
    {
        return {
            m00 * s, m01 * s, m02 * s,
            m10 * s, m11 * s, m12 * s,
            m20 * s, m21 * s, m22 * s
        };
    }

    vox_vec3 vox_mat3::operator*(const vox_vec3& v) const
    {
        // v′ = M * v
        return {
            m00 * v.x + m10 * v.y + m20 * v.z,
            m01 * v.x + m11 * v.y + m21 * v.z,
            m02 * v.x + m12 * v.y + m22 * v.z
        };
    }

    vox_mat3& vox_mat3::operator+=(const vox_mat3& o)
    {
        *this = *this + o; return *this;
    }
    vox_mat3& vox_mat3::operator*=(const vox_mat3& o)
    {
        *this = *this * o; return *this;
    }
    vox_mat3& vox_mat3::operator*=(f32 s)
    {
        *this = *this * s; return *this;
    }

    //––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––
    // 4×4 float (column-major storage)
    //––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––
 const vox_mat4 vox_mat4::identity(
    // col0           col1           col2           col3
     1, 0, 0, 0,     0, 1, 0, 0,     0, 0, 1, 0,     0, 0, 0, 1
);

// default ctor
vox_mat4::vox_mat4()
  : m00(1), m10(0), m20(0), m30(0)
  , m01(0), m11(1), m21(0), m31(0)
  , m02(0), m12(0), m22(1), m32(0)
  , m03(0), m13(0), m23(0), m33(1)
{}

// add
vox_mat4 vox_mat4::operator+(const vox_mat4& o) const {
    return vox_mat4(
        // col0
        m00 + o.m00, m10 + o.m10, m20 + o.m20, m30 + o.m30,
        // col1
        m01 + o.m01, m11 + o.m11, m21 + o.m21, m31 + o.m31,
        // col2
        m02 + o.m02, m12 + o.m12, m22 + o.m22, m32 + o.m32,
        // col3
        m03 + o.m03, m13 + o.m13, m23 + o.m23, m33 + o.m33
    );
}

// matrix multiply (column-major convention)
vox_mat4 vox_mat4::operator*(const vox_mat4& o) const {
    return vox_mat4(
        // col0
        m00*o.m00 + m01*o.m10 + m02*o.m20 + m03*o.m30,
        m10*o.m00 + m11*o.m10 + m12*o.m20 + m13*o.m30,
        m20*o.m00 + m21*o.m10 + m22*o.m20 + m23*o.m30,
        m30*o.m00 + m31*o.m10 + m32*o.m20 + m33*o.m30,
        // col1
        m00*o.m01 + m01*o.m11 + m02*o.m21 + m03*o.m31,
        m10*o.m01 + m11*o.m11 + m12*o.m21 + m13*o.m31,
        m20*o.m01 + m21*o.m11 + m22*o.m21 + m23*o.m31,
        m30*o.m01 + m31*o.m11 + m32*o.m21 + m33*o.m31,
        // col2
        m00*o.m02 + m01*o.m12 + m02*o.m22 + m03*o.m32,
        m10*o.m02 + m11*o.m12 + m12*o.m22 + m13*o.m32,
        m20*o.m02 + m21*o.m12 + m22*o.m22 + m23*o.m32,
        m30*o.m02 + m31*o.m12 + m32*o.m22 + m33*o.m32,
        // col3
        m00*o.m03 + m01*o.m13 + m02*o.m23 + m03*o.m33,
        m10*o.m03 + m11*o.m13 + m12*o.m23 + m13*o.m33,
        m20*o.m03 + m21*o.m13 + m22*o.m23 + m23*o.m33,
        m30*o.m03 + m31*o.m13 + m32*o.m23 + m33*o.m33
    );
}

// scalar multiply
vox_mat4 vox_mat4::operator*(f32 s) const {
    return vox_mat4(
        m00*s, m10*s, m20*s, m30*s,
        m01*s, m11*s, m21*s, m31*s,
        m02*s, m12*s, m22*s, m32*s,
        m03*s, m13*s, m23*s, m33*s
    );
}

// matrix × vec4
vox_vec4 vox_mat4::operator*(const vox_vec4& v) const {
    return vox_vec4(
        // row 0
        m00*v.x + m01*v.y + m02*v.z + m03*v.w,
        // row 1
        m10*v.x + m11*v.y + m12*v.z + m13*v.w,
        // row 2
        m20*v.x + m21*v.y + m22*v.z + m23*v.w,
        // row 3
        m30*v.x + m31*v.y + m32*v.z + m33*v.w
    );
}

// compound assigns
vox_mat4& vox_mat4::operator+=(const vox_mat4& o) {
    return *this = *this + o;
}
vox_mat4& vox_mat4::operator*=(const vox_mat4& o) {
    return *this = *this * o;
}
vox_mat4& vox_mat4::operator*=(f32 s) {
    return *this = *this * s;
}

    //––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––
    // 2×2 integer (column-major)
    //––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––
    const vox_imat2 vox_imat2::identity = { 1,0,  0,1 };
    vox_imat2::vox_imat2()
        : m00(1), m01(0)
        , m10(0), m11(1)
    {
    }
    vox_imat2::vox_imat2(s32 a00, s32 a01, s32 a10, s32 a11)
        : m00(a00), m01(a01)
        , m10(a10), m11(a11)
    {
    }

    vox_imat2 vox_imat2::operator+(const vox_imat2& o) const
    {
        return {
            m00 + o.m00, m01 + o.m01,
            m10 + o.m10, m11 + o.m11
        };
    }
    vox_imat2 vox_imat2::operator*(const vox_imat2& o) const
    {
        return {
            m00 * o.m00 + m10 * o.m01,
            m01 * o.m00 + m11 * o.m01,
            m00 * o.m10 + m10 * o.m11,
            m01 * o.m10 + m11 * o.m11
        };
    }
    vox_imat2 vox_imat2::operator*(s32 s) const
    {
        return { m00 * s, m01 * s, m10 * s, m11 * s };
    }
    vox_ivec2 vox_imat2::operator*(const vox_ivec2& v) const
    {
        return {
            m00 * v.x + m10 * v.y,
            m01 * v.x + m11 * v.y
        };
    }
    vox_imat2& vox_imat2::operator+=(const vox_imat2& o)
    {
        m00 += o.m00; m01 += o.m01;
        m10 += o.m10; m11 += o.m11;
        return *this;
    }
    vox_imat2& vox_imat2::operator*=(const vox_imat2& o)
    {
        *this = *this * o; return *this;
    }
    vox_imat2& vox_imat2::operator*=(s32 s)
    {
        *this = *this * s; return *this;
    }


    //––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––
    // 3×3 integer (column-major)
    //––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––
    const vox_imat3 vox_imat3::identity = {
        1,0,0,
        0,1,0,
        0,0,1
    };
    vox_imat3::vox_imat3()
        : m00(1), m01(0), m02(0)
        , m10(0), m11(1), m12(0)
        , m20(0), m21(0), m22(1)
    {
    }
    vox_imat3::vox_imat3(s32 a00, s32 a01, s32 a02,
        s32 a10, s32 a11, s32 a12,
        s32 a20, s32 a21, s32 a22)
        : m00(a00), m01(a01), m02(a02)
        , m10(a10), m11(a11), m12(a12)
        , m20(a20), m21(a21), m22(a22)
    {
    }

    vox_imat3 vox_imat3::operator+(const vox_imat3& o) const
    {
        return {
            m00 + o.m00, m01 + o.m01, m02 + o.m02,
            m10 + o.m10, m11 + o.m11, m12 + o.m12,
            m20 + o.m20, m21 + o.m21, m22 + o.m22
        };
    }
    vox_imat3 vox_imat3::operator*(const vox_imat3& o) const
    {
        return {
            m00 * o.m00 + m10 * o.m01 + m20 * o.m02,
            m01 * o.m00 + m11 * o.m01 + m21 * o.m02,
            m02 * o.m00 + m12 * o.m01 + m22 * o.m02,

            m00 * o.m10 + m10 * o.m11 + m20 * o.m12,
            m01 * o.m10 + m11 * o.m11 + m21 * o.m12,
            m02 * o.m10 + m12 * o.m11 + m22 * o.m12,

            m00 * o.m20 + m10 * o.m21 + m20 * o.m22,
            m01 * o.m20 + m11 * o.m21 + m21 * o.m22,
            m02 * o.m20 + m12 * o.m21 + m22 * o.m22
        };
    }
    vox_imat3 vox_imat3::operator*(s32 s) const
    {
        return {
            m00 * s, m01 * s, m02 * s,
            m10 * s, m11 * s, m12 * s,
            m20 * s, m21 * s, m22 * s
        };
    }
    vox_ivec3 vox_imat3::operator*(const vox_ivec3& v) const
    {
        return {
            m00 * v.x + m10 * v.y + m20 * v.z,
            m01 * v.x + m11 * v.y + m21 * v.z,
            m02 * v.x + m12 * v.y + m22 * v.z
        };
    }
    vox_imat3& vox_imat3::operator+=(const vox_imat3& o)
    {
        *this = *this + o; return *this;
    }
    vox_imat3& vox_imat3::operator*=(const vox_imat3& o)
    {
        *this = *this * o; return *this;
    }
    vox_imat3& vox_imat3::operator*=(s32 s)
    {
        *this = *this * s; return *this;
    }

} // namespace Unvoxeller
