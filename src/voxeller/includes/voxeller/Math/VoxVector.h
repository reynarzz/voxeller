#pragma once

#include <voxeller/Types.h>
#include <cmath>

//–– 2D vector (float)
struct vox_vec2 {
    float x, y;

    // Constructors
    vox_vec2();
    vox_vec2(float _x, float _y);

    // Member arithmetic ops
    vox_vec2 operator+(const vox_vec2& v) const;
    vox_vec2 operator-(const vox_vec2& v) const;
    vox_vec2 operator*(float s) const;
    vox_vec2 operator/(float s) const;
    vox_vec2& operator*=(float s);
    vox_vec2& operator/=(float s);

    // Scalar multiplication
    friend vox_vec2 operator*(float s, const vox_vec2& v);
};

//–– 3D vector (float)
struct vox_vec3 {
    float x, y, z;

    // Constructors
    vox_vec3();
    vox_vec3(float _x, float _y, float _z);

    // Member arithmetic ops
    vox_vec3 operator+(const vox_vec3& v) const;
    vox_vec3 operator-(const vox_vec3& v) const;
    vox_vec3 operator*(float s) const;
    vox_vec3 operator/(float s) const;
    vox_vec3& operator*=(float s);
    vox_vec3& operator/=(float s);

    // Scalar multiplication
    friend vox_vec3 operator*(float s, const vox_vec3& v);
};

//–– 4D vector (float)
struct vox_vec4 {
    float x, y, z, w;

    // Constructors
    vox_vec4();
    vox_vec4(float _x, float _y, float _z, float _w);

    // Member arithmetic ops
    vox_vec4 operator+(const vox_vec4& v) const;
    vox_vec4 operator-(const vox_vec4& v) const;
    vox_vec4 operator*(float s) const;
    vox_vec4 operator/(float s) const;

    // Scalar multiplication
    friend vox_vec4 operator*(float s, const vox_vec4& v);
};

//–– 2D vector (integer)
struct vox_ivec2 {
    int x, y;

    vox_ivec2();
    vox_ivec2(int _x, int _y);
};

vox_ivec2 operator+(const vox_ivec2& a, const vox_ivec2& b);
vox_ivec2 operator-(const vox_ivec2& a, const vox_ivec2& b);
vox_ivec2 operator/(const vox_ivec2& v, int s);

//–– 3D vector (integer)
struct vox_ivec3 {
    int x, y, z;

    vox_ivec3();
    vox_ivec3(int _x, int _y, int _z);
};

vox_ivec3 operator+(const vox_ivec3& a, const vox_ivec3& b);
vox_ivec3 operator-(const vox_ivec3& a, const vox_ivec3& b);
vox_ivec3 operator/(const vox_ivec3& v, int s);

//–– 4D vector (integer)
struct vox_ivec4 {
    int x, y, z, w;

    vox_ivec4();
    vox_ivec4(int _x, int _y, int _z, int _w);
};

vox_ivec4 operator+(const vox_ivec4& a, const vox_ivec4& b);
vox_ivec4 operator-(const vox_ivec4& a, const vox_ivec4& b);
vox_ivec4 operator/(const vox_ivec4& v, int s);

//–– Utility functions

float dot(const vox_vec2& a, const vox_vec2& b);
float dot(const vox_vec3& a, const vox_vec3& b);
float dot(const vox_vec4& a, const vox_vec4& b);
int   dot(const vox_ivec2& a, const vox_ivec2& b);
int   dot(const vox_ivec3& a, const vox_ivec3& b);
int   dot(const vox_ivec4& a, const vox_ivec4& b);

vox_vec3 cross(const vox_vec3& a, const vox_vec3& b);
vox_ivec3 cross(const vox_ivec3& a, const vox_ivec3& b);

float frac(float x);
vox_vec2 frac(const vox_vec2& v);
vox_vec3 frac(const vox_vec3& v);
vox_vec4 frac(const vox_vec4& v);

float min(float a, float b);
float max(float a, float b);
int   min(int a, int b);
int   max(int a, int b);

float sign(float x);
int   sign(int x);
