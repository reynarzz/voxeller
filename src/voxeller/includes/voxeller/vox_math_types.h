#pragma once
#include "api.h"
#include <array>

EXPORT struct VOXELLER_API vox_vec2
{
	float x, y;
};

EXPORT struct VOXELLER_API vox_ivec3
{
	int x, y, z;
};


EXPORT struct VOXELLER_API vox_vec3
{
	float x, y, z;
	vox_vec3()
	{
		x = 0;
		y = 0;
		z = 0;
	}
	vox_vec3(float _x, float _y, float _z)
	{
		x = _x;
		y = _y;
		z = _z;
	}

	vox_vec3 operator-(const vox_vec3 v)
	{
		return { x - v.x, y - v.y, z - v.z  };
	}

	vox_vec3 operator+(const vox_vec3 v)
	{
		return { x + v.x, y + v.y, z + v.z };
	}

	

	/*vox_vec3 operator/(const vox_vec3 v)
	{
		return { x / v.x, y / v.y, z / v.z };
	}*/
	operator vox_ivec3() const
	{
		return { static_cast<int>(x), static_cast<int>(y), static_cast<int>(z) };
	}

	vox_vec3 operator/(const float v)
	{
		return { x / v, y / v, z / v };
	}

	vox_vec3 operator*(const float v)
	{
		return { x * v, y * v, z * v };
	}
};

EXPORT struct VOXELLER_API vox_imat3
{
	int m00, m01, m02,
		m10, m11, m12,
		m20, m21, m22;

	const static vox_imat3 identity;

	vox_imat3 transpose();
	
	vox_vec3 operator*(const vox_vec3& vec)
	{
		float x = m00 * vec.x + m01 * vec.y + m02 * vec.z;
		float y = m10 * vec.x + m11 * vec.y + m12 * vec.z;
		float z = m20 * vec.x + m21 * vec.y + m22 * vec.z;

		return { x, y, z };
	};

	vox_ivec3 operator*(const vox_ivec3& vec)
	{
		return
		{
			m00 * vec.x + m01 * vec.y + m02 * vec.z,
			m10 * vec.x + m11 * vec.y + m12 * vec.z,
			m20 * vec.x + m21 * vec.y + m22 * vec.z,
		};
	};
};


EXPORT struct VOXELLER_API vox_vec4
{
	float x, y, z, w;
};


EXPORT struct VOXELLER_API vox_math
{
	static float frac(float v);

	static float sign(float val);
	static vox_vec3 cross(const vox_vec3& a, const vox_vec3& b);
	static vox_vec3 normalize(vox_vec3& v);
	static float magnitude(const vox_vec3& v);
	static std::array<vox_vec3, 4> makeFaceQuad(int x, int y, int z, int dx, int dy, int dz);
    static std::array<vox_vec3, 4> makeMergedQuad(
        int slice, int u, int v, int w, int h,
        int dx, int dy, int dz);
}; 