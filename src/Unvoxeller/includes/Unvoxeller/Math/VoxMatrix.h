#pragma once
#include <Unvoxeller/api.h>
#include <Unvoxeller/Types.h>
#include <Unvoxeller/Math/VoxVector.h>

namespace Unvoxeller
{
	//–– 2×2 f32 matrix
	struct UNVOXELLER_API vox_mat2
	{
		f32 m00, m01;
		f32 m10, m11;

		// Constructors
		vox_mat2();
		vox_mat2(f32 a00, f32 a01,
			f32 a10, f32 a11);

		// Static identity
		static const vox_mat2 identity;

		// Member operators
		vox_mat2 operator+(const vox_mat2& o) const;
		vox_mat2 operator*(const vox_mat2& o) const;
		vox_mat2 operator*(f32 s) const;
		vox_vec2 operator*(const vox_vec2& v) const;

		// Compound assigns
		vox_mat2& operator+=(const vox_mat2& o);
		vox_mat2& operator*=(const vox_mat2& o);
		vox_mat2& operator*=(f32 s);
	};

	//–– 3×3 f32 matrix
	struct UNVOXELLER_API vox_mat3
	{
		f32 m00, m01, m02;
		f32 m10, m11, m12;
		f32 m20, m21, m22;

		// Constructors
		vox_mat3();
		vox_mat3(f32 a00, f32 a01, f32 a02,
			f32 a10, f32 a11, f32 a12,
			f32 a20, f32 a21, f32 a22);

		// Static identity
		static const vox_mat3 identity;

		// Member operators
		vox_mat3 operator+(const vox_mat3& o) const;
		vox_mat3 operator*(const vox_mat3& o) const;
		vox_mat3 operator*(f32 s) const;
		vox_vec3 operator*(const vox_vec3& v) const;

		// Compound assigns
		vox_mat3& operator+=(const vox_mat3& o);
		vox_mat3& operator*=(const vox_mat3& o);
		vox_mat3& operator*=(f32 s);
	};

	//–– 4×4 f32 matrix
	struct UNVOXELLER_API vox_mat4
	{
		f32 m00, m01, m02, m03;
		f32 m10, m11, m12, m13;
		f32 m20, m21, m22, m23;
		f32 m30, m31, m32, m33;

		// Constructors
		vox_mat4();
		vox_mat4(f32 a00, f32 a01, f32 a02, f32 a03,
			f32 a10, f32 a11, f32 a12, f32 a13,
			f32 a20, f32 a21, f32 a22, f32 a23,
			f32 a30, f32 a31, f32 a32, f32 a33);

		// Static identity
		static const vox_mat4 identity;

		// Member operators
		vox_mat4 operator+(const vox_mat4& o) const;
		vox_mat4 operator*(const vox_mat4& o) const;
		vox_mat4 operator*(f32 s) const;
		vox_vec4 operator*(const vox_vec4& v) const;

		// Compound assigns
		vox_mat4& operator+=(const vox_mat4& o);
		vox_mat4& operator*=(const vox_mat4& o);
		vox_mat4& operator*=(f32 s);

		f32* data()             { return &m00; }
    	const f32* data() const { return &m00; }
	};

	//–– 2×2 integer matrix
	struct UNVOXELLER_API vox_imat2
	{
		s32 m00, m01;
		s32 m10, m11;

		// Constructors
		vox_imat2();
		vox_imat2(s32 a00, s32 a01,
			s32 a10, s32 a11);

		static const vox_imat2 identity;

		// Member operators
		vox_imat2 operator+(const vox_imat2& o) const;
		vox_imat2 operator*(const vox_imat2& o) const;
		vox_imat2 operator*(s32 s) const;
		vox_ivec2 operator*(const vox_ivec2& v) const;

		vox_imat2& operator+=(const vox_imat2& o);
		vox_imat2& operator*=(const vox_imat2& o);
		vox_imat2& operator*=(s32 s);
	};

	//–– 3×3 integer matrix
	struct UNVOXELLER_API vox_imat3
	{
		s32 m00, m01, m02;
		s32 m10, m11, m12;
		s32 m20, m21, m22;

		// Constructors
		vox_imat3();
		vox_imat3(s32 a00, s32 a01, s32 a02,
			s32 a10, s32 a11, s32 a12,
			s32 a20, s32 a21, s32 a22);

		static const vox_imat3 identity;

		// Member operators
		vox_imat3 operator+(const vox_imat3& o) const;
		vox_imat3 operator*(const vox_imat3& o) const;
		vox_imat3 operator*(s32 s) const;
		vox_ivec3 operator*(const vox_ivec3& v) const;

		vox_imat3& operator+=(const vox_imat3& o);
		vox_imat3& operator*=(const vox_imat3& o);
		vox_imat3& operator*=(s32 s);
	};

	//–– 4×4 integer matrix
	struct UNVOXELLER_API vox_imat4 
	{
		s32 m00, m01, m02, m03;
		s32 m10, m11, m12, m13;
		s32 m20, m21, m22, m23;
		s32 m30, m31, m32, m33;

		// Constructors
		vox_imat4();
		vox_imat4(s32 a00, s32 a01, s32 a02, s32 a03,
			s32 a10, s32 a11, s32 a12, s32 a13,
			s32 a20, s32 a21, s32 a22, s32 a23,
			s32 a30, s32 a31, s32 a32, s32 a33);

		static const vox_imat4 identity;

		// Member operators
		vox_imat4 operator+(const vox_imat4& o) const;
		vox_imat4 operator*(const vox_imat4& o) const;
		vox_imat4 operator*(s32 s) const;
		vox_ivec4 operator*(const vox_ivec4& v) const;

		vox_imat4& operator+=(const vox_imat4& o);
		vox_imat4& operator*=(const vox_imat4& o);
		vox_imat4& operator*=(s32 s);
	};
}