#pragma once
#include <Unvoxeller/api.h>
#include <Unvoxeller/Types.h>

namespace Unvoxeller
{
	//–– 2D vector (f32)
	struct UNVOXELLER_API vox_vec2
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

		vox_vec2 operator-() const;
	};


	UNVOXELLER_API vox_vec2 operator/(const vox_vec2& a, const vox_vec2& b);

	//–– 3D vector (f32)
	struct UNVOXELLER_API vox_vec3
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
		vox_vec3 operator-() const;
	};

	UNVOXELLER_API vox_vec3 operator/(const vox_vec3& a, const vox_vec3& b);
	UNVOXELLER_API vox_vec3 operator-=(const vox_vec3& a, const vox_vec3& b);


	//–– 4D vector (f32)
	struct UNVOXELLER_API vox_vec4
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
		vox_vec4 operator-() const;
	};

	UNVOXELLER_API vox_vec4 operator/(const vox_vec4& a, const vox_vec4& b);

	//–– 2D vector (integer)
	struct UNVOXELLER_API vox_ivec2
	{
		s32 x, y;

		vox_ivec2();
		vox_ivec2(s32 _x, s32 _y);
	};

	vox_ivec2 operator+(const vox_ivec2& a, const vox_ivec2& b);
	vox_ivec2 operator-(const vox_ivec2& a, const vox_ivec2& b);
	vox_ivec2 operator/(const vox_ivec2& v, s32 s);

	//–– 3D vector (integer)
	struct UNVOXELLER_API vox_ivec3
	{
		s32 x, y, z;

		vox_ivec3();
		vox_ivec3(s32 _x, s32 _y, s32 _z);
	};

	vox_ivec3 operator+(const vox_ivec3& a, const vox_ivec3& b);
	vox_ivec3 operator-(const vox_ivec3& a, const vox_ivec3& b);
	vox_ivec3 operator/(const vox_ivec3& v, s32 s);

	//–– 4D vector (integer)
	struct UNVOXELLER_API vox_ivec4
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
	s32 dot(const vox_ivec2& a, const vox_ivec2& b);
	s32 dot(const vox_ivec3& a, const vox_ivec3& b);
	s32 dot(const vox_ivec4& a, const vox_ivec4& b);

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
	s32 min(s32 a, s32 b);
	s32 max(s32 a, s32 b);

	f32 sign(f32 x);
	s32 sign(s32 x);

	UNVOXELLER_API vox_vec2   abs(const vox_vec2& v);
	UNVOXELLER_API vox_vec3   abs(const vox_vec3& v);
	UNVOXELLER_API vox_vec4   abs(const vox_vec4& v);

	UNVOXELLER_API vox_vec2   floor(const vox_vec2& v);
	UNVOXELLER_API vox_vec3   floor(const vox_vec3& v);
	UNVOXELLER_API vox_vec4   floor(const vox_vec4& v);

	UNVOXELLER_API vox_vec2   ceil(const vox_vec2& v);
	UNVOXELLER_API vox_vec3   ceil(const vox_vec3& v);
	UNVOXELLER_API vox_vec4   ceil(const vox_vec4& v);

	UNVOXELLER_API vox_vec2   round(const vox_vec2& v);
	UNVOXELLER_API vox_vec3   round(const vox_vec3& v);
	UNVOXELLER_API vox_vec4   round(const vox_vec4& v);

	UNVOXELLER_API vox_vec2   clamp(const vox_vec2& v, f32 lo, f32 hi);
	UNVOXELLER_API vox_vec3   clamp(const vox_vec3& v, f32 lo, f32 hi);
	UNVOXELLER_API vox_vec4   clamp(const vox_vec4& v, f32 lo, f32 hi);

	UNVOXELLER_API vox_vec2   saturate(const vox_vec2& v);
	UNVOXELLER_API vox_vec3   saturate(const vox_vec3& v);
	UNVOXELLER_API vox_vec4   saturate(const vox_vec4& v);

	// Length / distance
	UNVOXELLER_API f32        length(const vox_vec2& v);
	UNVOXELLER_API f32        length(const vox_vec3& v);
	UNVOXELLER_API f32        length(const vox_vec4& v);

	UNVOXELLER_API f32        length2(const vox_vec2& v);
	UNVOXELLER_API f32        length2(const vox_vec3& v);
	UNVOXELLER_API f32        length2(const vox_vec4& v);

	UNVOXELLER_API f32        distance(const vox_vec2& a, const vox_vec2& b);
	UNVOXELLER_API f32        distance(const vox_vec3& a, const vox_vec3& b);
	UNVOXELLER_API f32        distance(const vox_vec4& a, const vox_vec4& b);

	// Normalize & projections
	UNVOXELLER_API vox_vec2   normalize(const vox_vec2& v);
	UNVOXELLER_API vox_vec3   normalize(const vox_vec3& v);
	UNVOXELLER_API vox_vec4   normalize(const vox_vec4& v);

	UNVOXELLER_API vox_vec3   faceforward(const vox_vec3& N, const vox_vec3& I, const vox_vec3& Nref);
	UNVOXELLER_API vox_vec3   project(const vox_vec3& v, const vox_vec3& onto);
	UNVOXELLER_API vox_vec2   perp(const vox_vec2& v);        // 2D perpendicular

	UNVOXELLER_API f32        angleBetween(const vox_vec3& a, const vox_vec3& b);

	// Interpolation (besides your existing lerp)
	UNVOXELLER_API vox_vec2   smoothstep(const vox_vec2& e0, const vox_vec2& e1, const vox_vec2& x);
	UNVOXELLER_API vox_vec3   smoothstep(const vox_vec3& e0, const vox_vec3& e1, const vox_vec3& x);
	UNVOXELLER_API vox_vec4   smoothstep(const vox_vec4& e0, const vox_vec4& e1, const vox_vec4& x);

	UNVOXELLER_API vox_vec2   smootherstep(const vox_vec2& e0, const vox_vec2& e1, const vox_vec2& x);
	UNVOXELLER_API vox_vec3   smootherstep(const vox_vec3& e0, const vox_vec3& e1, const vox_vec3& x);
	UNVOXELLER_API vox_vec4   smootherstep(const vox_vec4& e0, const vox_vec4& e1, const vox_vec4& x);

	UNVOXELLER_API vox_vec2   step(const vox_vec2& edge, const vox_vec2& x);
	UNVOXELLER_API vox_vec3   step(const vox_vec3& edge, const vox_vec3& x);
	UNVOXELLER_API vox_vec4   step(const vox_vec4& edge, const vox_vec4& x);

	// Reflection & refraction
	UNVOXELLER_API vox_vec3   reflect(const vox_vec3& I, const vox_vec3& N);
	UNVOXELLER_API vox_vec3   refract(const vox_vec3& I, const vox_vec3& N, f32 eta);
	UNVOXELLER_API f32        fresnelSchlick(f32 cosTheta, f32 F0);

	// Angle & coordinate
	UNVOXELLER_API f32        radians(f32 deg);
	UNVOXELLER_API f32        degrees(f32 rad);

	UNVOXELLER_API vox_vec2   cartesianToPolar(const vox_vec2& v);  // (angle, radius)
	UNVOXELLER_API vox_vec2   polarToCartesian(const vox_vec2& pr);

	UNVOXELLER_API vox_vec3   toSpherical(const vox_vec3& v);       // (azimuth, inclination, r)
	UNVOXELLER_API vox_vec3   fromSpherical(const vox_vec3& sph);

	// Random & noise
	UNVOXELLER_API vox_vec3   randomUnitSphere();
	UNVOXELLER_API f32        perlinNoise(const vox_vec3& p);
	UNVOXELLER_API f32        simplexNoise(const vox_vec3& p);
	UNVOXELLER_API f32        fbm(const vox_vec3& p);
	UNVOXELLER_API f32        turbulence(const vox_vec3& p);

	// Modulo, isnan/isfinite
	UNVOXELLER_API f32        mod(f32 x, f32 y);
	UNVOXELLER_API vox_vec2   mod(const vox_vec2& v, f32 m);
	UNVOXELLER_API vox_vec3   mod(const vox_vec3& v, f32 m);
	UNVOXELLER_API vox_vec4   mod(const vox_vec4& v, f32 m);

	UNVOXELLER_API bool       isnan(f32 x);
	UNVOXELLER_API bool       isfinite(f32 x);

	UNVOXELLER_API vox_vec3 noise3D(f32 t, f32 frequency = 1.0f);
}