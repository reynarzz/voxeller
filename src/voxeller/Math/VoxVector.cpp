#include <cmath>
#include <Voxeller/Math/VoxVector.h>

namespace Voxeller
{
	vox_vec2::vox_vec2() : x(0), y(0) {}
	vox_vec2::vox_vec2(f32 _x, f32 _y) : x(_x), y(_y) {}

	vox_vec2 vox_vec2::operator+(const vox_vec2& v) const { return { x + v.x, y + v.y }; }
	vox_vec2 vox_vec2::operator-(const vox_vec2& v) const { return { x - v.x, y - v.y }; }
	vox_vec2 vox_vec2::operator*(f32 s) const { return { x * s,   y * s }; }
	vox_vec2 vox_vec2::operator/(f32 s) const { return { x / s,   y / s }; }
	vox_vec2& vox_vec2::operator*=(f32 s) { x *= s; y *= s; return *this; }
	vox_vec2& vox_vec2::operator/=(f32 s) { x /= s; y /= s; return *this; }

	vox_vec2 operator*(f32 s, const vox_vec2& v) { return v * s; }

	vox_vec3::vox_vec3() : x(0), y(0), z(0) {}
	vox_vec3::vox_vec3(f32 _x, f32 _y, f32 _z) : x(_x), y(_y), z(_z) {}

	vox_vec3 vox_vec3::operator+(const vox_vec3& v) const { return { x + v.x, y + v.y, z + v.z }; }
	vox_vec3 vox_vec3::operator-(const vox_vec3& v) const { return { x - v.x, y - v.y, z - v.z }; }
	vox_vec3 vox_vec3::operator*(f32 s) const { return { x * s,   y * s,   z * s }; }
	vox_vec3 vox_vec3::operator/(f32 s) const { return { x / s,   y / s,   z / s }; }
	vox_vec3& vox_vec3::operator*=(f32 s) { x *= s; y *= s; z *= s; return *this; }
	vox_vec3& vox_vec3::operator/=(f32 s) { x /= s; y /= s; z /= s; return *this; }

	vox_vec3 operator*(f32 s, const vox_vec3& v) { return v * s; }

	vox_vec4::vox_vec4() : x(0), y(0), z(0), w(0) {}
	vox_vec4::vox_vec4(f32 _x, f32 _y, f32 _z, f32 _w) : x(_x), y(_y), z(_z), w(_w) {}

	vox_vec4 vox_vec4::operator+(const vox_vec4& v) const { return { x + v.x, y + v.y, z + v.z, w + v.w }; }
	vox_vec4 vox_vec4::operator-(const vox_vec4& v) const { return { x - v.x, y - v.y, z - v.z, w - v.w }; }
	vox_vec4 vox_vec4::operator*(f32 s) const { return { x * s,   y * s,   z * s,   w * s }; }
	vox_vec4 vox_vec4::operator/(f32 s) const { return { x / s,   y / s,   z / s,   w / s }; }

	vox_vec4 operator*(f32 s, const vox_vec4& v) { return v * s; }

	vox_ivec2::vox_ivec2() : x(0), y(0) {}
	vox_ivec2::vox_ivec2(s32 _x, s32 _y) : x(_x), y(_y) {}

	vox_ivec2 operator+(const vox_ivec2& a, const vox_ivec2& b) { return { a.x + b.x, a.y + b.y }; }
	vox_ivec2 operator-(const vox_ivec2& a, const vox_ivec2& b) { return { a.x - b.x, a.y - b.y }; }
	vox_ivec2 operator/(const vox_ivec2& v, s32 s) { return { v.x / s,   v.y / s }; }

	vox_ivec3::vox_ivec3() : x(0), y(0), z(0) {}
	vox_ivec3::vox_ivec3(s32 _x, s32 _y, s32 _z) : x(_x), y(_y), z(_z) {}

	vox_ivec3 operator+(const vox_ivec3& a, const vox_ivec3& b) { return { a.x + b.x, a.y + b.y, a.z + b.z }; }
	vox_ivec3 operator-(const vox_ivec3& a, const vox_ivec3& b) { return { a.x - b.x, a.y - b.y, a.z - b.z }; }
	vox_ivec3 operator/(const vox_ivec3& v, s32 s) { return { v.x / s,   v.y / s,   v.z / s }; }

	vox_ivec4::vox_ivec4() : x(0), y(0), z(0), w(0) {}
	vox_ivec4::vox_ivec4(s32 _x, s32 _y, s32 _z, s32 _w) : x(_x), y(_y), z(_z), w(_w) {}

	vox_ivec4 operator+(const vox_ivec4& a, const vox_ivec4& b) { return { a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w }; }
	vox_ivec4 operator-(const vox_ivec4& a, const vox_ivec4& b) { return { a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w }; }
	vox_ivec4 operator/(const vox_ivec4& v, s32 s) { return { v.x / s,   v.y / s,   v.z / s,   v.w / s }; }

	f32 dot(const vox_vec2& a, const vox_vec2& b) { return a.x * b.x + a.y * b.y; }
	f32 dot(const vox_vec3& a, const vox_vec3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
	f32 dot(const vox_vec4& a, const vox_vec4& b) { return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w; }
	s32 dot(const vox_ivec2& a, const vox_ivec2& b) { return a.x * b.x + a.y * b.y; }
	s32 dot(const vox_ivec3& a, const vox_ivec3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
	s32 dot(const vox_ivec4& a, const vox_ivec4& b) { return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w; }

	vox_vec3 cross(const vox_vec3& a, const vox_vec3& b) { return { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x }; }
	vox_ivec3 cross(const vox_ivec3& a, const vox_ivec3& b) { return { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x }; }

	f32 frac(f32 x) { return x - std::floor(x); }
	vox_vec2 frac(const vox_vec2& v) { return { frac(v.x), frac(v.y) }; }
	vox_vec3 frac(const vox_vec3& v) { return { frac(v.x), frac(v.y), frac(v.z) }; }
	vox_vec4 frac(const vox_vec4& v) { return { frac(v.x), frac(v.y), frac(v.z), frac(v.w) }; }


	vox_vec2 lerp(const vox_vec2& a, const vox_vec2& b, float t) 
	{
		return a + (b - a) * t;
	}

	vox_vec3 lerp(const vox_vec3& a, const vox_vec3& b, float t)
	{
		return a + (b - a) * t;
	}

	vox_vec4 lerp(const vox_vec4& a, const vox_vec4& b, float t) 
	{
		return a + (b - a) * t;
	}

	f32 min(f32 a, f32 b) { return (a < b) ? a : b; }
	f32 max(f32 a, f32 b) { return (a > b) ? a : b; }
	s32 min(s32 a, s32 b) { return (a < b) ? a : b; }
	s32 max(s32 a, s32 b) { return (a > b) ? a : b; }

	f32 sign(f32 x) { return (x > 0) ? 1.f : ((x < 0) ? -1.f : 0.f); }
	s32 sign(s32 x) { return (x > 0) - (x < 0); }
}