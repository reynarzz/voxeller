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
