#pragma once
#include <voxeller/api.h>
#include <Voxeller/Math/VoxVector.h>

namespace Voxeller
{
	struct VOXELLER_API vox_quat
	{
		f32 x, y, z, w;  // vector part (x,y,z), scalar part (w)

		// identity quaternion (no rotation)
		vox_quat();
		// direct ctor
		vox_quat(f32 _x, f32 _y, f32 _z, f32 _w);

		vox_quat(const vox_vec3& eulerAngles);

		// build from unit-axis + angle (radians)
		static vox_quat fromAxisAngle(const vox_vec3& axis, f32 angle);

		// Hamilton product
		vox_quat operator*(const vox_quat& o) const;

		// component‐wise dot
		f32 dot(const vox_quat& o) const;
		// squared length, length, normalization
		f32 length() const;
		vox_quat& normalize();
		vox_quat   normalized() const;
		// conjugate (inverse for unit‐quats)
		vox_quat   conjugate() const;


		// scalar‐quat ops
		friend vox_quat operator*(const vox_quat& q, f32 s);
		friend vox_quat operator*(f32 s, const vox_quat& q);
		friend vox_quat operator+(const vox_quat& a, const vox_quat& b);
		friend vox_quat operator-(const vox_quat& a, const vox_quat& b);

		explicit operator vox_vec3() const;
	};

	vox_vec3 operator*(const vox_quat& q, const vox_vec3& v);

	// Spherical linear interpolation
	vox_quat slerp(const vox_quat& a, const vox_quat& b, f32 t);

	vox_vec3 toEuler(const vox_quat& q);
	vox_quat fromEuler(const vox_vec3& eulerAngles);


} // namespace vox
