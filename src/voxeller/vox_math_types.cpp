#ifndef VOXELLER_API_EXPORT
#define VOXELLER_API_EXPORT
#endif

#include <voxeller/vox_math_types.h>
#include <cmath>

const vox_imat3 vox_imat3::identity = { 1, 0, 0,
                                        0, 1, 0,
                                        0, 0, 1 };


vox_imat3 vox_imat3::transpose()
{
   return 
   {
	    m00, m10, m20,
        m01, m11, m21,
        m02, m12, m22
   };
}

float vox_math::frac(float v)
{
    return v - static_cast<long>(v);
}

float vox_math::sign(float val)
{
	return (0 < val) - (val < 0);
}

vox_vec3 vox_math::cross(const vox_vec3& a, const vox_vec3& b) 
{
	return 
	{ 
		a.y * b.z - a.z * b.y,
		a.z * b.x - a.x * b.z,
		a.x * b.y - a.y * b.x 
	};
}

float vox_math::magnitude(const vox_vec3& v)
{
	return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

vox_vec3 vox_math::normalize(vox_vec3& v) 
{
	 return v / std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

std::array<vox_vec3, 4> vox_math::makeFaceQuad(int x, int y, int z, int dx, int dy, int dz)
    {
        const float offset = 0.5f;
        vox_vec3 center{ x + 0.5f * dx, y + 0.5f * dy, z + 0.5f * dz };

        if (dx != 0) // X face
        {
            return {{
                center + vox_vec3(0, -offset, -offset),
                center + vox_vec3(0, -offset,  offset),
                center + vox_vec3(0,  offset,  offset),
                center + vox_vec3(0,  offset, -offset),
            }};
        }
        else if (dy != 0) // Y face
        {
            return {{
                center + vox_vec3(-offset, 0, -offset),
                center + vox_vec3(-offset, 0,  offset),
                center + vox_vec3( offset, 0,  offset),
                center + vox_vec3( offset, 0, -offset),
            }};
        }
        else // Z face
        {
            return {{
                center + vox_vec3(-offset, -offset, 0),
                center + vox_vec3(-offset,  offset, 0),
                center + vox_vec3( offset,  offset, 0),
                center + vox_vec3( offset, -offset, 0),
            }};
        }
    }

 std::array<vox_vec3, 4> vox_math::makeMergedQuad(
    int slice, int u, int v, int w, int h,
    int dx, int dy, int dz)
{
    float fx = static_cast<float>(dx ? slice : u);
    float fy = static_cast<float>(dy ? slice : (dx ? v : u));
    float fz = static_cast<float>(dz ? slice : (dx + dy == 0 ? u : v));

    // Normal points outward from face direction
    vox_vec3 normal{ (float)dx, (float)dy, (float)dz };

    // Determine directions for quad dimensions
    vox_vec3 du{ (float)((dx == 0) ? 1 : 0), (float)((dy == 0) ? 1 : 0), (float)((dz == 0) ? 1 : 0) };
    vox_vec3 dv;

    if (dx != 0)
        dv = { 0, 0, 1 };
    else if (dy != 0)
        dv = { 1, 0, 0 };
    else // dz != 0
        dv = { 1, 0, 0 };

    // Face offset center (from voxel center)
    vox_vec3 base = { fx + 0.5f * dx, fy + 0.5f * dy, fz + 0.5f * dz };

    return {
        base,
        base + du * static_cast<float>(w),
        base + du * static_cast<float>(w) + dv * static_cast<float>(h),
        base + dv * static_cast<float>(h),
    };
}
