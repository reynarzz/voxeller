#pragma once

#include <voxeller/Types.h>
#include <voxeller/VoxVector.h>
#include <cmath>

//–– 2×2 float matrix
struct vox_mat2 {
    float m00, m01;
    float m10, m11;

    // Constructors
    vox_mat2();
    vox_mat2(float a00, float a01,
             float a10, float a11);

    // Static identity
    static const vox_mat2 identity;

    // Member operators
    vox_mat2 operator+(const vox_mat2& o) const;
    vox_mat2 operator*(const vox_mat2& o) const;
    vox_mat2 operator*(float s) const;
    vox_vec2 operator*(const vox_vec2& v) const;

    // Compound assigns
    vox_mat2& operator+=(const vox_mat2& o);
    vox_mat2& operator*=(const vox_mat2& o);
    vox_mat2& operator*=(float s);
};

//–– 3×3 float matrix
struct vox_mat3 {
    float m00, m01, m02;
    float m10, m11, m12;
    float m20, m21, m22;

    // Constructors
    vox_mat3();
    vox_mat3(float a00, float a01, float a02,
             float a10, float a11, float a12,
             float a20, float a21, float a22);

    // Static identity
    static const vox_mat3 identity;

    // Member operators
    vox_mat3 operator+(const vox_mat3& o) const;
    vox_mat3 operator*(const vox_mat3& o) const;
    vox_mat3 operator*(float s) const;
    vox_vec3 operator*(const vox_vec3& v) const;

    // Compound assigns
    vox_mat3& operator+=(const vox_mat3& o);
    vox_mat3& operator*=(const vox_mat3& o);
    vox_mat3& operator*=(float s);
};

//–– 4×4 float matrix
struct vox_mat4 {
    float m00, m01, m02, m03;
    float m10, m11, m12, m13;
    float m20, m21, m22, m23;
    float m30, m31, m32, m33;

    // Constructors
    vox_mat4();
    vox_mat4(float a00, float a01, float a02, float a03,
             float a10, float a11, float a12, float a13,
             float a20, float a21, float a22, float a23,
             float a30, float a31, float a32, float a33);

    // Static identity
    static const vox_mat4 identity;

    // Member operators
    vox_mat4 operator+(const vox_mat4& o) const;
    vox_mat4 operator*(const vox_mat4& o) const;
    vox_mat4 operator*(float s) const;
    vox_vec4 operator*(const vox_vec4& v) const;

    // Compound assigns
    vox_mat4& operator+=(const vox_mat4& o);
    vox_mat4& operator*=(const vox_mat4& o);
    vox_mat4& operator*=(float s);
};

//–– 2×2 integer matrix
struct vox_imat2 {
    int m00, m01;
    int m10, m11;

    // Constructors
    vox_imat2();
    vox_imat2(int a00, int a01,
              int a10, int a11);

    static const vox_imat2 identity;

    // Member operators
    vox_imat2 operator+(const vox_imat2& o) const;
    vox_imat2 operator*(const vox_imat2& o) const;
    vox_imat2 operator*(int s) const;
    vox_ivec2 operator*(const vox_ivec2& v) const;

    vox_imat2& operator+=(const vox_imat2& o);
    vox_imat2& operator*=(const vox_imat2& o);
    vox_imat2& operator*=(int s);
};

//–– 3×3 integer matrix
struct vox_imat3 {
    int m00, m01, m02;
    int m10, m11, m12;
    int m20, m21, m22;

    // Constructors
    vox_imat3();
    vox_imat3(int a00, int a01, int a02,
              int a10, int a11, int a12,
              int a20, int a21, int a22);

    static const vox_imat3 identity;

    // Member operators
    vox_imat3 operator+(const vox_imat3& o) const;
    vox_imat3 operator*(const vox_imat3& o) const;
    vox_imat3 operator*(int s) const;
    vox_ivec3 operator*(const vox_ivec3& v) const;

    vox_imat3& operator+=(const vox_imat3& o);
    vox_imat3& operator*=(const vox_imat3& o);
    vox_imat3& operator*=(int s);
};

//–– 4×4 integer matrix
struct vox_imat4 {
    int m00, m01, m02, m03;
    int m10, m11, m12, m13;
    int m20, m21, m22, m23;
    int m30, m31, m32, m33;

    // Constructors
    vox_imat4();
    vox_imat4(int a00, int a01, int a02, int a03,
              int a10, int a11, int a12, int a13,
              int a20, int a21, int a22, int a23,
              int a30, int a31, int a32, int a33);

    static const vox_imat4 identity;

    // Member operators
    vox_imat4 operator+(const vox_imat4& o) const;
    vox_imat4 operator*(const vox_imat4& o) const;
    vox_imat4 operator*(int s) const;
    vox_ivec4 operator*(const vox_ivec4& v) const;

    vox_imat4& operator+=(const vox_imat4& o);
    vox_imat4& operator*=(const vox_imat4& o);
    vox_imat4& operator*=(int s);
};
