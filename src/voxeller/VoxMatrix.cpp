#include <voxeller/VoxMatrix.h>

//–– 2×2 float
const vox_mat2 vox_mat2::identity = {1,0,0,1};
vox_mat2::vox_mat2() : m00(1),m01(0), m10(0),m11(1) {}
vox_mat2::vox_mat2(float a00,float a01,float a10,float a11)
    : m00(a00),m01(a01), m10(a10),m11(a11) {}

vox_mat2 vox_mat2::operator+(const vox_mat2& o) const {
    return { m00+o.m00, m01+o.m01,
             m10+o.m10, m11+o.m11 };
}
vox_mat2 vox_mat2::operator*(const vox_mat2& o) const {
    return {
        m00*o.m00 + m01*o.m10, m00*o.m01 + m01*o.m11,
        m10*o.m00 + m11*o.m10, m10*o.m01 + m11*o.m11
    };
}
vox_mat2 vox_mat2::operator*(float s) const {
    return { m00*s, m01*s, m10*s, m11*s };
}
vox_vec2 vox_mat2::operator*(const vox_vec2& v) const {
    return { m00*v.x + m01*v.y, m10*v.x + m11*v.y };
}

vox_mat2& vox_mat2::operator+=(const vox_mat2& o) {
    m00+=o.m00; m01+=o.m01;
    m10+=o.m10; m11+=o.m11;
    return *this;
}
vox_mat2& vox_mat2::operator*=(const vox_mat2& o) {
    *this = *this * o;
    return *this;
}
vox_mat2& vox_mat2::operator*=(float s) {
    m00*=s; m01*=s;
    m10*=s; m11*=s;
    return *this;
}

//–– 3×3 float
const vox_mat3 vox_mat3::identity = {1,0,0, 0,1,0, 0,0,1};
vox_mat3::vox_mat3()
    : m00(1),m01(0),m02(0), m10(0),m11(1),m12(0), m20(0),m21(0),m22(1) {}
vox_mat3::vox_mat3(float a00,float a01,float a02,
                   float a10,float a11,float a12,
                   float a20,float a21,float a22)
    : m00(a00),m01(a01),m02(a02),
      m10(a10),m11(a11),m12(a12),
      m20(a20),m21(a21),m22(a22) {}

vox_mat3 vox_mat3::operator+(const vox_mat3& o) const {
    return { m00+o.m00, m01+o.m01, m02+o.m02,
             m10+o.m10, m11+o.m11, m12+o.m12,
             m20+o.m20, m21+o.m21, m22+o.m22 };
}
vox_mat3 vox_mat3::operator*(const vox_mat3& o) const {
    return {
        m00*o.m00 + m01*o.m10 + m02*o.m20,
        m00*o.m01 + m01*o.m11 + m02*o.m21,
        m00*o.m02 + m01*o.m12 + m02*o.m22,

        m10*o.m00 + m11*o.m10 + m12*o.m20,
        m10*o.m01 + m11*o.m11 + m12*o.m21,
        m10*o.m02 + m11*o.m12 + m12*o.m22,

        m20*o.m00 + m21*o.m10 + m22*o.m20,
        m20*o.m01 + m21*o.m11 + m22*o.m21,
        m20*o.m02 + m21*o.m12 + m22*o.m22
    };
}
vox_mat3 vox_mat3::operator*(float s) const {
    return { m00*s,m01*s,m02*s,
             m10*s,m11*s,m12*s,
             m20*s,m21*s,m22*s };
}
vox_vec3 vox_mat3::operator*(const vox_vec3& v) const {
    return { m00*v.x + m01*v.y + m02*v.z,
             m10*v.x + m11*v.y + m12*v.z,
             m20*v.x + m21*v.y + m22*v.z };
}

vox_mat3& vox_mat3::operator+=(const vox_mat3& o) { return *this = *this + o; }
vox_mat3& vox_mat3::operator*=(const vox_mat3& o) { return *this = *this * o; }
vox_mat3& vox_mat3::operator*=(float s) { return *this = *this * s; }

//–– 4×4 float
const vox_mat4 vox_mat4::identity = {
    1,0,0,0,
    0,1,0,0,
    0,0,1,0,
    0,0,0,1
};
vox_mat4::vox_mat4()
    : m00(1),m01(0),m02(0),m03(0),
      m10(0),m11(1),m12(0),m13(0),
      m20(0),m21(0),m22(1),m23(0),
      m30(0),m31(0),m32(0),m33(1) {}
vox_mat4::vox_mat4(float a00,float a01,float a02,float a03,
                   float a10,float a11,float a12,float a13,
                   float a20,float a21,float a22,float a23,
                   float a30,float a31,float a32,float a33)
    : m00(a00),m01(a01),m02(a02),m03(a03),
      m10(a10),m11(a11),m12(a12),m13(a13),
      m20(a20),m21(a21),m22(a22),m23(a23),
      m30(a30),m31(a31),m32(a32),m33(a33) {}

vox_mat4 vox_mat4::operator+(const vox_mat4& o) const {
    return {
        m00+o.m00, m01+o.m01, m02+o.m02, m03+o.m03,
        m10+o.m10, m11+o.m11, m12+o.m12, m13+o.m13,
        m20+o.m20, m21+o.m21, m22+o.m22, m23+o.m23,
        m30+o.m30, m31+o.m31, m32+o.m32, m33+o.m33
    };
}
vox_mat4 vox_mat4::operator*(const vox_mat4& o) const {
    vox_mat4 r;
    r.m00 = m00*o.m00 + m01*o.m10 + m02*o.m20 + m03*o.m30;
    r.m01 = m00*o.m01 + m01*o.m11 + m02*o.m21 + m03*o.m31;
    r.m02 = m00*o.m02 + m01*o.m12 + m02*o.m22 + m03*o.m32;
    r.m03 = m00*o.m03 + m01*o.m13 + m02*o.m23 + m03*o.m33;

    r.m10 = m10*o.m00 + m11*o.m10 + m12*o.m20 + m13*o.m30;
    r.m11 = m10*o.m01 + m11*o.m11 + m12*o.m21 + m13*o.m31;
    r.m12 = m10*o.m02 + m11*o.m12 + m12*o.m22 + m13*o.m32;
    r.m13 = m10*o.m03 + m11*o.m13 + m12*o.m23 + m13*o.m33;

    r.m20 = m20*o.m00 + m21*o.m10 + m22*o.m20 + m23*o.m30;
    r.m21 = m20*o.m01 + m21*o.m11 + m22*o.m21 + m23*o.m31;
    r.m22 = m20*o.m02 + m21*o.m12 + m22*o.m22 + m23*o.m32;
    r.m23 = m20*o.m03 + m21*o.m13 + m22*o.m23 + m23*o.m33;

    r.m30 = m30*o.m00 + m31*o.m10 + m32*o.m20 + m33*o.m30;
    r.m31 = m30*o.m01 + m31*o.m11 + m32*o.m21 + m33*o.m31;
    r.m32 = m30*o.m02 + m31*o.m12 + m32*o.m22 + m33*o.m32;
    r.m33 = m30*o.m03 + m31*o.m13 + m32*o.m23 + m33*o.m33;
    return r;
}
vox_mat4 vox_mat4::operator*(float s) const {
    return {
        m00*s, m01*s, m02*s, m03*s,
        m10*s, m11*s, m12*s, m13*s,
        m20*s, m21*s, m22*s, m23*s,
        m30*s, m31*s, m32*s, m33*s
    };
}
vox_vec4 vox_mat4::operator*(const vox_vec4& v) const {
    return {
        m00*v.x + m01*v.y + m02*v.z + m03*v.w,
        m10*v.x + m11*v.y + m12*v.z + m13*v.w,
        m20*v.x + m21*v.y + m22*v.z + m23*v.w,
        m30*v.x + m31*v.y + m32*v.z + m33*v.w
    };
}

vox_mat4& vox_mat4::operator+=(const vox_mat4& o) { return *this = *this + o; }
vox_mat4& vox_mat4::operator*=(const vox_mat4& o) { return *this = *this * o; }
vox_mat4& vox_mat4::operator*=(float s) { return *this = *this * s; }

//–– 2×2 integer
const vox_imat2 vox_imat2::identity = {1,0,0,1};
vox_imat2::vox_imat2() : m00(1),m01(0), m10(0),m11(1) {}
vox_imat2::vox_imat2(int a00,int a01,int a10,int a11)
    : m00(a00),m01(a01), m10(a10),m11(a11) {}

vox_imat2 vox_imat2::operator+(const vox_imat2& o) const {
    return { m00+o.m00, m01+o.m01,
             m10+o.m10, m11+o.m11 };
}
vox_imat2 vox_imat2::operator*(const vox_imat2& o) const {
    return {
        m00*o.m00 + m01*o.m10, m00*o.m01 + m01*o.m11,
        m10*o.m00 + m11*o.m10, m10*o.m01 + m11*o.m11
    };
}
vox_imat2 vox_imat2::operator*(int s) const {
    return { m00*s, m01*s, m10*s, m11*s };
}
vox_ivec2 vox_imat2::operator*(const vox_ivec2& v) const {
    return { m00*v.x + m01*v.y, m10*v.x + m11*v.y };
}
vox_imat2& vox_imat2::operator+=(const vox_imat2& o) {
    m00+=o.m00; m01+=o.m01;
    m10+=o.m10; m11+=o.m11;
    return *this;
}
vox_imat2& vox_imat2::operator*=(const vox_imat2& o) { return *this = *this * o; }
vox_imat2& vox_imat2::operator*=(int s) { return *this = *this * s; }

//–– 3×3 integer
const vox_imat3 vox_imat3::identity = {1,0,0, 0,1,0, 0,0,1};
vox_imat3::vox_imat3()
    : m00(1),m01(0),m02(0), m10(0),m11(1),m12(0), m20(0),m21(0),m22(1) {}
vox_imat3::vox_imat3(int a00,int a01,int a02,
                     int a10,int a11,int a12,
                     int a20,int a21,int a22)
    : m00(a00),m01(a01),m02(a02),
      m10(a10),m11(a11),m12(a12),
      m20(a20),m21(a21),m22(a22) {}

vox_imat3 vox_imat3::operator+(const vox_imat3& o) const {
    return { m00+o.m00, m01+o.m01, m02+o.m02,
             m10+o.m10, m11+o.m11, m12+o.m12,
             m20+o.m20, m21+o.m21, m22+o.m22 };
}
vox_imat3 vox_imat3::operator*(const vox_imat3& o) const {
    return {
        m00*o.m00 + m01*o.m10 + m02*o.m20,
        m00*o.m01 + m01*o.m11 + m02*o.m21,
        m00*o.m02 + m01*o.m12 + m02*o.m22,

        m10*o.m00 + m11*o.m10 + m12*o.m20,
        m10*o.m01 + m11*o.m11 + m12*o.m21,
        m10*o.m02 + m11*o.m12 + m12*o.m22,

        m20*o.m00 + m21*o.m10 + m22*o.m20,
        m20*o.m01 + m21*o.m11 + m22*o.m21,
        m20*o.m02 + m21*o.m12 + m22*o.m22
    };
}
vox_imat3 vox_imat3::operator*(int s) const {
    return { m00*s,m01*s,m02*s,
             m10*s,m11*s,m12*s,
             m20*s,m21*s,m22*s };
}
vox_ivec3 vox_imat3::operator*(const vox_ivec3& v) const {
    return { m00*v.x + m01*v.y + m02*v.z,
             m10*v.x + m11*v.y + m12*v.z,
             m20*v.x + m21*v.y + m22*v.z };
}
