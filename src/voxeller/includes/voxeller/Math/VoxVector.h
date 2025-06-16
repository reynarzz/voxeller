#pragma once
#include <voxeller/api.h>
#include <Voxeller/Types.h>

namespace Voxeller
{
	//–– 2D vector (f32)
	struct VOXELLER_API vox_vec2
	{
		f32 x, y;

		// Constructors
		vox_vec2();
		vox_vec2(f32 _x, f32 _y);

		// Member arithmetic ops
		vox_vec2 operator+(const vox_vec2& v) const;
		vox_vec2 operator-(const vox_vec2& v) const;
		vox_vec2 operator*(f32 s) const;
		vox_vec2 operator/(f32 s) const;
		vox_vec2& operator*=(f32 s);
		vox_vec2& operator/=(f32 s);

		// Scalar multiplication
		friend vox_vec2 operator*(f32 s, const vox_vec2& v);
	};

	//–– 3D vector (f32)
	struct VOXELLER_API vox_vec3
	{
		f32 x, y, z;

		// Constructors
		vox_vec3();
		vox_vec3(f32 _x, f32 _y, f32 _z);

		// Member arithmetic ops
		vox_vec3 operator+(const vox_vec3& v) const;
		vox_vec3 operator-(const vox_vec3& v) const;
		vox_vec3 operator*(f32 s) const;
		vox_vec3 operator/(f32 s) const;
		vox_vec3& operator*=(f32 s);
		vox_vec3& operator/=(f32 s);

		// Scalar multiplication
		friend vox_vec3 operator*(f32 s, const vox_vec3& v);
	};

	//–– 4D vector (f32)
	struct VOXELLER_API vox_vec4
	{
		f32 x, y, z, w;

		// Constructors
		vox_vec4();
		vox_vec4(f32 _x, f32 _y, f32 _z, f32 _w);

		// Member arithmetic ops
		vox_vec4 operator+(const vox_vec4& v) const;
		vox_vec4 operator-(const vox_vec4& v) const;
		vox_vec4 operator*(f32 s) const;
		vox_vec4 operator/(f32 s) const;

		// Scalar multiplication
		friend vox_vec4 operator*(f32 s, const vox_vec4& v);
	};

	//–– 2D vector (integer)
	struct VOXELLER_API vox_ivec2
	{
		s32 x, y;

		vox_ivec2();
		vox_ivec2(s32 _x, s32 _y);
	};

	vox_ivec2 operator+(const vox_ivec2& a, const vox_ivec2& b);
	vox_ivec2 operator-(const vox_ivec2& a, const vox_ivec2& b);
	vox_ivec2 operator/(const vox_ivec2& v, s32 s);

	//–– 3D vector (integer)
	struct VOXELLER_API vox_ivec3
	{
		s32 x, y, z;

		vox_ivec3();
		vox_ivec3(s32 _x, s32 _y, s32 _z);
	};

	vox_ivec3 operator+(const vox_ivec3& a, const vox_ivec3& b);
	vox_ivec3 operator-(const vox_ivec3& a, const vox_ivec3& b);
	vox_ivec3 operator/(const vox_ivec3& v, s32 s);

	//–– 4D vector (integer)
	struct VOXELLER_API vox_ivec4
	{
		s32 x, y, z, w;

		vox_ivec4();
		vox_ivec4(s32 _x, s32 _y, s32 _z, s32 _w);
	};

	vox_ivec4 operator+(const vox_ivec4& a, const vox_ivec4& b);
	vox_ivec4 operator-(const vox_ivec4& a, const vox_ivec4& b);
	vox_ivec4 operator/(const vox_ivec4& v, s32 s);

	//–– Utility functions

	f32 dot(const vox_vec2& a, const vox_vec2& b);
	f32 dot(const vox_vec3& a, const vox_vec3& b);
	f32 dot(const vox_vec4& a, const vox_vec4& b);
	s32   dot(const vox_ivec2& a, const vox_ivec2& b);
	s32   dot(const vox_ivec3& a, const vox_ivec3& b);
	s32   dot(const vox_ivec4& a, const vox_ivec4& b);

	vox_vec3 cross(const vox_vec3& a, const vox_vec3& b);
	vox_ivec3 cross(const vox_ivec3& a, const vox_ivec3& b);

	f32 frac(f32 x);
	vox_vec2 frac(const vox_vec2& v);
	vox_vec3 frac(const vox_vec3& v);
	vox_vec4 frac(const vox_vec4& v);

	vox_vec2 lerp(const vox_vec2& a, const vox_vec2& b, f32 t);
	vox_vec3 lerp(const vox_vec3& a, const vox_vec3& b, f32 t);
	vox_vec4 lerp(const vox_vec4& a, const vox_vec4& b, f32 t);

	f32 min(f32 a, f32 b);
	f32 max(f32 a, f32 b);
	s32   min(s32 a, s32 b);
	s32   max(s32 a, s32 b);

	f32 sign(f32 x);
	s32   sign(s32 x);
}