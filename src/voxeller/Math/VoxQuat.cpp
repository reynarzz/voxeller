#include <Voxeller/Math/VoxQuat.h>
#include <cmath>

namespace Voxeller
{

	// -- Constructors & axis‐angle

	vox_quat::vox_quat()
		: x(0.f), y(0.f), z(0.f), w(1.f)
	{
	}

	vox_quat::vox_quat(f32 _x, f32 _y, f32 _z, f32 _w)
		: x(_x), y(_y), z(_z), w(_w)
	{
	}

	vox_quat vox_quat::fromAxisAngle(const vox_vec3& axis, f32 angle) {
		f32 half = angle * 0.5f;
		f32 s = std::sin(half);
		return vox_quat(axis.x * s,
			axis.y * s,
			axis.z * s,
			std::cos(half));
	}

	// -- Construct from Euler

	vox_quat::vox_quat(const vox_vec3& euler)
	{
		// euler.x = roll  (rotation about X)
		// euler.y = pitch (rotation about Y)
		// euler.z = yaw   (rotation about Z)
		f32 hx = euler.x * 0.5f;
		f32 hy = euler.y * 0.5f;
		f32 hz = euler.z * 0.5f;

		f32 cx = std::cos(hx), sx = std::sin(hx);
		f32 cy = std::cos(hy), sy = std::sin(hy);
		f32 cz = std::cos(hz), sz = std::sin(hz);

		w = cx * cy * cz + sx * sy * sz;
		x = sx * cy * cz - cx * sy * sz;
		y = cx * sy * cz + sx * cy * sz;
		z = cx * cy * sz - sx * sy * cz;
	}

	// -- Hamilton product

	vox_quat vox_quat::operator*(const vox_quat& o) const {
		return vox_quat(
			w * o.x + x * o.w + y * o.z - z * o.y,
			w * o.y - x * o.z + y * o.w + z * o.x,
			w * o.z + x * o.y - y * o.x + z * o.w,
			w * o.w - x * o.x - y * o.y - z * o.z
		);
	}

	// -- dot, length, normalize, conjugate

	f32 vox_quat::dot(const vox_quat& o) const {
		return x * o.x + y * o.y + z * o.z + w * o.w;
	}

	f32 vox_quat::length() const {
		return std::sqrt(dot(*this));
	}

	vox_quat& vox_quat::normalize() {
		f32 len = length();
		if (len > 0.f) {
			x /= len; y /= len; z /= len; w /= len;
		}
		return *this;
	}

	vox_quat vox_quat::normalized() const {
		vox_quat q = *this;
		return q.normalize();
	}

	vox_quat vox_quat::conjugate() const {
		return vox_quat(-x, -y, -z, w);
	}

	// -- scalar/quaternion arithmetic

	vox_quat operator*(const vox_quat& q, f32 s) {
		return vox_quat(q.x * s, q.y * s, q.z * s, q.w * s);
	}

	vox_quat operator*(f32 s, const vox_quat& q) {
		return q * s;
	}

	vox_quat operator+(const vox_quat& a, const vox_quat& b) {
		return vox_quat(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w);
	}

	vox_quat operator-(const vox_quat& a, const vox_quat& b) {
		return vox_quat(a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w);
	}

	// -- slerp

	vox_quat slerp(const vox_quat& qa, const vox_quat& qb, f32 t) {
		// compute cosine of angle between
		f32 cosTheta = qa.dot(qb);
		vox_quat  qb2 = qb;

		// if cosTheta < 0, invert to take shortest path
		if (cosTheta < 0.f) {
			cosTheta = -cosTheta;
			qb2 = vox_quat(-qb.x, -qb.y, -qb.z, -qb.w);
		}

		// if very close, fallback to normalized lerp
		if (cosTheta > 0.9995f) {
			vox_quat r = qa + (qb2 - qa) * t;
			return r.normalize();
		}

		f32 theta = std::acos(cosTheta);
		f32 sinTheta = std::sin(theta);

		f32 w1 = std::sin((1 - t) * theta) / sinTheta;
		f32 w2 = std::sin(t * theta) / sinTheta;
		return (qa * w1) + (qb2 * w2);
	}

	vox_vec3 operator*(const vox_quat& q_, const vox_vec3& v)
	{
		// ensure q is unit
		vox_quat q = q_.normalized();
		// convert vector to quat
		vox_quat vq(v.x, v.y, v.z, 0.f);
		// rotate: q * v * q⁻¹
		vox_quat rq = q * vq * q.conjugate();

		return vox_vec3(rq.x, rq.y, rq.z);
	}

	// -- conversion operator → Euler

	vox_quat::operator vox_vec3() const {
		return toEuler(*this);
	}

	// -- free‐function toEuler

	vox_vec3 toEuler(const vox_quat& q)
	{
		// make sure q is unit-length!
		f32 sinr_cosp = 2.f * (q.w * q.x + q.y * q.z);
		f32 cosr_cosp = 1.f - 2.f * (q.x * q.x + q.y * q.y);
		f32 roll = std::atan2(sinr_cosp, cosr_cosp);

		constexpr f32 PI = 3.14159265358979323846f;
		constexpr f32 PI_2 = PI * 0.5f;

		f32 sinp = 2.f * (q.w * q.y - q.z * q.x);
		f32 pitch;
		if (sinp >= 1.f) pitch = PI_2;  // gimbal-lock, clamp
		else if (sinp <= -1.f) pitch = -PI_2;
		else                   pitch = std::asin(sinp);

		f32 siny_cosp = 2.f * (q.w * q.z + q.x * q.y);
		f32 cosy_cosp = 1.f - 2.f * (q.y * q.y + q.z * q.z);
		f32 yaw = std::atan2(siny_cosp, cosy_cosp);

		return vox_vec3(roll, pitch, yaw);
	}


	vox_quat fromEuler(const vox_vec3& e) 
	{
		return vox_quat(e);
	}

} 
