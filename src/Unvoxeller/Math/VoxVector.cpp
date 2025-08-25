#include <cmath>
#include <Unvoxeller/Math/VoxVector.h>

namespace Unvoxeller
{

	#ifdef _MSC_VER
    #define RAND() std::rand()
#else
    #define RAND() rand()
#endif

	vox_vec2::vox_vec2() : x(0), y(0) {}
	vox_vec2::vox_vec2(f32 _x, f32 _y) : x(_x), y(_y) {}

	vox_vec2 vox_vec2::operator+(const vox_vec2& v) const { return { x + v.x, y + v.y }; }
	vox_vec2 vox_vec2::operator-(const vox_vec2& v) const { return { x - v.x, y - v.y }; }
	vox_vec2 vox_vec2::operator*(f32 s) const { return { x * s,   y * s }; }
	vox_vec2 vox_vec2::operator/(f32 s) const { return { x / s,   y / s }; }
	vox_vec2& vox_vec2::operator*=(f32 s) { x *= s; y *= s; return *this; }
	vox_vec2& vox_vec2::operator/=(f32 s) { x /= s; y /= s; return *this; }

	vox_vec2 operator*(f32 s, const vox_vec2& v) { return v * s; }

	vox_vec2 operator/(const vox_vec2& a, const vox_vec2& b)
	{
		return { a.x / b.x, a.y / b.y };
	}
	vox_vec3 operator/(const vox_vec3& a, const vox_vec3& b) 
	{
		return { a.x / b.x, a.y / b.y, a.z / b.z };
	}

	 vox_vec3 operator-=(const vox_vec3& a, const vox_vec3& v)
	 {
		return { a.x - v.x, a.y - v.y, a.z - v.z }; 
	 }


	vox_vec4 operator/(const vox_vec4& a, const vox_vec4& b)
	{
		return { a.x / b.x, a.y / b.y, a.z / b.z, a.w / b.w };
	}

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
	vox_vec4::vox_vec4(vox_vec3 vec) : x(vec.x), y(vec.y), z(vec.z), w(0) {}

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

	//–– 2D
	vox_vec2 vox_vec2::operator-() const
	{
		return { -x, -y };
	}

	//–– 3D
	vox_vec3 vox_vec3::operator-() const
	{
		return { -x, -y, -z };
	}

	//–– 4D
	vox_vec4 vox_vec4::operator-() const
	{
		return { -x, -y, -z, -w };
	}

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



	vox_vec2 abs(const vox_vec2& v) { return { std::fabs(v.x), std::fabs(v.y) }; }
	vox_vec3 abs(const vox_vec3& v) { return { std::fabs(v.x), std::fabs(v.y), std::fabs(v.z) }; }
	vox_vec4 abs(const vox_vec4& v) { return { std::fabs(v.x), std::fabs(v.y), std::fabs(v.z), std::fabs(v.w) }; }

	vox_vec2 floor(const vox_vec2& v) { return { std::floor(v.x), std::floor(v.y) }; }
	vox_vec3 floor(const vox_vec3& v) { return { std::floor(v.x), std::floor(v.y), std::floor(v.z) }; }
	vox_vec4 floor(const vox_vec4& v) { return { std::floor(v.x), std::floor(v.y), std::floor(v.z), std::floor(v.w) }; }

	vox_vec2 ceil(const vox_vec2& v) { return { std::ceil(v.x),  std::ceil(v.y) }; }
	vox_vec3 ceil(const vox_vec3& v) { return { std::ceil(v.x),  std::ceil(v.y),  std::ceil(v.z) }; }
	vox_vec4 ceil(const vox_vec4& v) { return { std::ceil(v.x),  std::ceil(v.y),  std::ceil(v.z),  std::ceil(v.w) }; }

	vox_vec2 round(const vox_vec2& v) { return { std::round(v.x), std::round(v.y) }; }
	vox_vec3 round(const vox_vec3& v) { return { std::round(v.x), std::round(v.y), std::round(v.z) }; }
	vox_vec4 round(const vox_vec4& v) { return { std::round(v.x), std::round(v.y), std::round(v.z), std::round(v.w) }; }

	vox_vec2 clamp(const vox_vec2& v, f32 lo, f32 hi)
	{
		f32 cx = (v.x < lo) ? lo : (v.x > hi ? hi : v.x);
		f32 cy = (v.y < lo) ? lo : (v.y > hi ? hi : v.y);
		return vox_vec2{ cx, cy };
	}

	vox_vec3 clamp(const vox_vec3& v, f32 lo, f32 hi)
	{
		f32 cx = (v.x < lo) ? lo : (v.x > hi ? hi : v.x);
		f32 cy = (v.y < lo) ? lo : (v.y > hi ? hi : v.y);
		f32 cz = (v.z < lo) ? lo : (v.z > hi ? hi : v.z);
		return vox_vec3{ cx, cy, cz };
	}

	vox_vec4 clamp(const vox_vec4& v, f32 lo, f32 hi) 
	{
		f32 cx = (v.x < lo) ? lo : (v.x > hi ? hi : v.x);
		f32 cy = (v.y < lo) ? lo : (v.y > hi ? hi : v.y);
		f32 cz = (v.z < lo) ? lo : (v.z > hi ? hi : v.z);
		f32 cw = (v.w < lo) ? lo : (v.w > hi ? hi : v.w);
		return vox_vec4{ cx, cy, cz, cw };
	}

	vox_vec2 saturate(const vox_vec2& v) { return clamp(v, 0.f, 1.f); }
	vox_vec3 saturate(const vox_vec3& v) { return clamp(v, 0.f, 1.f); }
	vox_vec4 saturate(const vox_vec4& v) { return clamp(v, 0.f, 1.f); }

	/////////////////////////////////////////////////////////////////////////////
	// Length & distance
	/////////////////////////////////////////////////////////////////////////////

	f32 length(const vox_vec2& v) { return std::hypot(v.x, v.y); }
	f32 length(const vox_vec3& v) { return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z); }
	f32 length(const vox_vec4& v) { return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w); }

	f32 length2(const vox_vec2& v) { return v.x * v.x + v.y * v.y; }
	f32 length2(const vox_vec3& v) { return v.x * v.x + v.y * v.y + v.z * v.z; }
	f32 length2(const vox_vec4& v) { return v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w; }

	f32 distance(const vox_vec2& a, const vox_vec2& b) { return length(a - b); }
	f32 distance(const vox_vec3& a, const vox_vec3& b) { return length(a - b); }
	f32 distance(const vox_vec4& a, const vox_vec4& b) { return length(a - b); }

	/////////////////////////////////////////////////////////////////////////////
	// Normalize & related
	/////////////////////////////////////////////////////////////////////////////

	vox_vec2 normalize(const vox_vec2& v) {
		f32 len = length(v);
		return (len > 0) ? v / len : vox_vec2(0, 0);
	}
	vox_vec3 normalize(const vox_vec3& v) {
		f32 len = length(v);
		return (len > 0) ? v / len : vox_vec3(0, 0, 0);
	}
	vox_vec4 normalize(const vox_vec4& v) {
		f32 len = length(v);
		return (len > 0) ? v / len : vox_vec4(0, 0, 0, 0);
	}

	vox_vec3 faceforward(const vox_vec3& N, const vox_vec3& I, const vox_vec3& Nref) 
	{
		return (dot(Nref, I) < 0.f) ? N : -N;
	}

	f32 angleBetween(const vox_vec3& a, const vox_vec3& b)
	{
		// compute lengths
		f32 la = length(a);
		f32 lb = length(b);
		f32 denom = la * lb;
		if (denom <= 0.f)
			return 0.f;

		// compute cosine of the angle
		f32 cosTheta = dot(a, b) / denom;

		// clamp to [-1,1] explicitly
		if (cosTheta < -1.f)       cosTheta = -1.f;
		else if (cosTheta > 1.f)   cosTheta = 1.f;

		// return the angle
		return std::acos(cosTheta);
	}


	vox_vec3 project(const vox_vec3& v, const vox_vec3& onto) 
	{
		f32 d = dot(onto, onto);
		return (d > 0) ? onto * (dot(v, onto) / d) : vox_vec3(0, 0, 0);
	}

	vox_vec2 perp(const vox_vec2& v) {
		return { -v.y, v.x };
	}

	
	/////////////////////////////////////////////////////////////////////////////
	// Smooth & step
	/////////////////////////////////////////////////////////////////////////////

	vox_vec2 step(const vox_vec2& e, const vox_vec2& x) 
	{
		return { x.x < e.x ? 0.f : 1.f, x.y < e.y ? 0.f : 1.f };
	}

	vox_vec3 step(const vox_vec3& e, const vox_vec3& x)
	{
		return { x.x < e.x ? 0.f : 1.f,
				 x.y < e.y ? 0.f : 1.f,
				 x.z < e.z ? 0.f : 1.f };
	}

	vox_vec4 step(const vox_vec4& e, const vox_vec4& x) 
	{
		return { x.x < e.x ? 0.f : 1.f,
				 x.y < e.y ? 0.f : 1.f,
				 x.z < e.z ? 0.f : 1.f,
				 x.w < e.w ? 0.f : 1.f };
	}

	vox_vec2 smoothstep(const vox_vec2& e0, const vox_vec2& e1, const vox_vec2& x) {
		// compute t = clamp((x - e0)/(e1 - e0), 0, 1) per‐component
		f32 tx = (x.x - e0.x) / (e1.x - e0.x);
		tx = tx < 0.f ? 0.f : (tx > 1.f ? 1.f : tx);
		f32 ty = (x.y - e0.y) / (e1.y - e0.y);
		ty = ty < 0.f ? 0.f : (ty > 1.f ? 1.f : ty);

		// result = t^2 * (3 - 2 t)
		f32 sx = tx * tx * (3.f - 2.f * tx);
		f32 sy = ty * ty * (3.f - 2.f * ty);

		return vox_vec2{ sx, sy };
	}

	vox_vec3 smoothstep(const vox_vec3& e0, const vox_vec3& e1, const vox_vec3& x) {
		f32 tx = (x.x - e0.x) / (e1.x - e0.x);
		tx = tx < 0.f ? 0.f : (tx > 1.f ? 1.f : tx);
		f32 ty = (x.y - e0.y) / (e1.y - e0.y);
		ty = ty < 0.f ? 0.f : (ty > 1.f ? 1.f : ty);
		f32 tz = (x.z - e0.z) / (e1.z - e0.z);
		tz = tz < 0.f ? 0.f : (tz > 1.f ? 1.f : tz);

		f32 sx = tx * tx * (3.f - 2.f * tx);
		f32 sy = ty * ty * (3.f - 2.f * ty);
		f32 sz = tz * tz * (3.f - 2.f * tz);

		return vox_vec3{ sx, sy, sz };
	}

	vox_vec4 smoothstep(const vox_vec4& e0, const vox_vec4& e1, const vox_vec4& x) {
		f32 tx = (x.x - e0.x) / (e1.x - e0.x);
		tx = tx < 0.f ? 0.f : (tx > 1.f ? 1.f : tx);
		f32 ty = (x.y - e0.y) / (e1.y - e0.y);
		ty = ty < 0.f ? 0.f : (ty > 1.f ? 1.f : ty);
		f32 tz = (x.z - e0.z) / (e1.z - e0.z);
		tz = tz < 0.f ? 0.f : (tz > 1.f ? 1.f : tz);
		f32 tw = (x.w - e0.w) / (e1.w - e0.w);
		tw = tw < 0.f ? 0.f : (tw > 1.f ? 1.f : tw);

		f32 sx = tx * tx * (3.f - 2.f * tx);
		f32 sy = ty * ty * (3.f - 2.f * ty);
		f32 sz = tz * tz * (3.f - 2.f * tz);
		f32 sw = tw * tw * (3.f - 2.f * tw);

		return vox_vec4{ sx, sy, sz, sw };
	}


	vox_vec2 smootherstep(const vox_vec2& e0, const vox_vec2& e1, const vox_vec2& x) {
		f32 tx = (x.x - e0.x) / (e1.x - e0.x);
		tx = tx < 0.f ? 0.f : (tx > 1.f ? 1.f : tx);
		f32 ty = (x.y - e0.y) / (e1.y - e0.y);
		ty = ty < 0.f ? 0.f : (ty > 1.f ? 1.f : ty);

		// t^3*(10 - 15 t + 6 t^2)
		f32 sx = tx * tx * tx * (10.f - 15.f * tx + 6.f * tx * tx);
		f32 sy = ty * ty * ty * (10.f - 15.f * ty + 6.f * ty * ty);

		return vox_vec2{ sx, sy };
	}

	vox_vec3 smootherstep(const vox_vec3& e0, const vox_vec3& e1, const vox_vec3& x) {
		f32 tx = (x.x - e0.x) / (e1.x - e0.x);
		tx = tx < 0.f ? 0.f : (tx > 1.f ? 1.f : tx);
		f32 ty = (x.y - e0.y) / (e1.y - e0.y);
		ty = ty < 0.f ? 0.f : (ty > 1.f ? 1.f : ty);
		f32 tz = (x.z - e0.z) / (e1.z - e0.z);
		tz = tz < 0.f ? 0.f : (tz > 1.f ? 1.f : tz);

		f32 sx = tx * tx * tx * (10.f - 15.f * tx + 6.f * tx * tx);
		f32 sy = ty * ty * ty * (10.f - 15.f * ty + 6.f * ty * ty);
		f32 sz = tz * tz * tz * (10.f - 15.f * tz + 6.f * tz * tz);

		return vox_vec3{ sx, sy, sz };
	}

	vox_vec4 smootherstep(const vox_vec4& e0, const vox_vec4& e1, const vox_vec4& x) {
		f32 tx = (x.x - e0.x) / (e1.x - e0.x);
		tx = tx < 0.f ? 0.f : (tx > 1.f ? 1.f : tx);
		f32 ty = (x.y - e0.y) / (e1.y - e0.y);
		ty = ty < 0.f ? 0.f : (ty > 1.f ? 1.f : ty);
		f32 tz = (x.z - e0.z) / (e1.z - e0.z);
		tz = tz < 0.f ? 0.f : (tz > 1.f ? 1.f : tz);
		f32 tw = (x.w - e0.w) / (e1.w - e0.w);
		tw = tw < 0.f ? 0.f : (tw > 1.f ? 1.f : tw);

		f32 sx = tx * tx * tx * (10.f - 15.f * tx + 6.f * tx * tx);
		f32 sy = ty * ty * ty * (10.f - 15.f * ty + 6.f * ty * ty);
		f32 sz = tz * tz * tz * (10.f - 15.f * tz + 6.f * tz * tz);
		f32 sw = tw * tw * tw * (10.f - 15.f * tw + 6.f * tw * tw);

		return vox_vec4{ sx, sy, sz, sw };
	}

	/////////////////////////////////////////////////////////////////////////////
	// Reflect / Refract / Fresnel
	/////////////////////////////////////////////////////////////////////////////

	vox_vec3 reflect(const vox_vec3& I, const vox_vec3& N) {
		return I - N * (2.f * dot(N, I));
	}
	vox_vec3 refract(const vox_vec3& I, const vox_vec3& N, f32 eta) {
		f32 cosI = dot(N, I);
		f32 k = 1.f - eta * eta * (1.f - cosI * cosI);
		return k < 0.f
			? vox_vec3(0, 0, 0)
			: I * eta - N * (eta * cosI + std::sqrt(k));
	}
	f32 fresnelSchlick(f32 cosTheta, f32 F0) {
		return F0 + (1.f - F0) * std::pow(1.f - cosTheta, 5.f);
	}

	/////////////////////////////////////////////////////////////////////////////
	// Angle & coordinate
	/////////////////////////////////////////////////////////////////////////////

	static constexpr f32 PI = 3.14159265358979323846f;
	f32 radians(f32 deg) { return deg * (PI / 180.f); }
	f32 degrees(f32 rad) { return rad * (180.f / PI); }

	vox_vec2 cartesianToPolar(const vox_vec2& v) {
		return { std::atan2(v.y, v.x), length(v) };
	}
	vox_vec2 polarToCartesian(const vox_vec2& pr) {
		return { pr.y * std::cos(pr.x), pr.y * std::sin(pr.x) };
	}

	vox_vec3 toSpherical(const vox_vec3& v) {
		f32 r = length(v);
		if (r == 0) return vox_vec3(0, 0, 0);
		f32 az = std::atan2(v.y, v.x);
		f32 el = std::acos(v.z / r);
		return { az, el, r };
	}
	vox_vec3 fromSpherical(const vox_vec3& s) {
		f32 az = s.x, el = s.y, r = s.z;
		f32 sinE = std::sin(el), cosE = std::cos(el);
		return { r * cosE * std::cos(az), r * cosE * std::sin(az), r * sinE };
	}

	/////////////////////////////////////////////////////////////////////////////
	// Random & noise (stubs)
	/////////////////////////////////////////////////////////////////////////////

	vox_vec3 randomUnitSphere()
	{
		float z = 2.f * (RAND() / float(RAND_MAX)) - 1.f;
		float t = 2.f * PI * (RAND() / float(RAND_MAX));
		float r = std::sqrt(1.f - z * z);
		return { r * std::cos(t), r * std::sin(t), z };
	}


	/////////////////////////////////////////////////////////////////////////////
	// Mod, isnan, isfinite
	/////////////////////////////////////////////////////////////////////////////

	f32 mod(f32 x, f32 y)
	{
		return x - y * std::floor(x / y);
	}
	vox_vec2 mod(const vox_vec2& v, f32 m) { return { mod(v.x,m), mod(v.y,m) }; }
	vox_vec3 mod(const vox_vec3& v, f32 m) { return { mod(v.x,m), mod(v.y,m), mod(v.z,m) }; }
	vox_vec4 mod(const vox_vec4& v, f32 m) { return { mod(v.x,m), mod(v.y,m), mod(v.z,m), mod(v.w,m) }; }

	bool isnan(f32 x) { return std::isnan(x); }
	bool isfinite(f32 x) { return std::isfinite(x); }

	// Permutation table for Perlin & Simplex (Ken Perlin’s improved noise)
	static const int perm[512] = 
	{
		151,160,137,91,90,15,131,13,201,95,96,53,194,233,7,225,
		140,36,103,30,69,142,8,99,37,240,21,10,23,190, 6,148,
		247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,
		57,177,33,88,237,149,56,87,174,20,125,136,171,168, 68,
		175,74,165,71,134,139,48,27,166,77,146,158,231,83,111,
		229,122,60,211,133,230,220,105,92,41,55,46,245,40,244,
		102,143,54, 65,25,63,161, 1,216,80,73,209,76,132,187,
		208,89,18,169,200,196,135,130,116,188,159,86,164,100,
		109,198,173,186, 3,64,52,217,226,250,124,123,5,202,38,
		147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,
		182,189,28,42,223,183,170,213,119,248,152, 2,44,154,
		163, 70,221,153,101,155,167, 43,172, 9,129,22,39,253,
		19, 98,108,110,79,113,224,232,178,185,112,104,218,246,
		97,228,251,34,242,193,238,210,144,12,191,179,162,241,
		81,51,145,235,249,14,239,107,49,192,214,31,181,199,106,
		157,184,84,204,176,115,121,50,45,127, 4,150,254,138,236,
		205,93,222,114, 67,29,24,72,243,141,128,195,78,66,215,
		61,156,180,
		// repeat
		151,160,137,91,90,15,131,13,201,95,96,53,194,233,7,225,
		140,36,103,30,69,142,8,99,37,240,21,10,23,190, 6,148,
		247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,
		57,177,33,88,237,149,56,87,174,20,125,136,171,168, 68,
		175,74,165,71,134,139,48,27,166,77,146,158,231,83,111,
		229,122,60,211,133,230,220,105,92,41,55,46,245,40,244,
		102,143,54, 65,25,63,161, 1,216,80,73,209,76,132,187,
		208,89,18,169,200,196,135,130,116,188,159,86,164,100,
		109,198,173,186, 3,64,52,217,226,250,124,123,5,202,38,
		147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,
		182,189,28,42,223,183,170,213,119,248,152, 2,44,154,
		163, 70,221,153,101,155,167, 43,172, 9,129,22,39,253,
		19, 98,108,110,79,113,224,232,178,185,112,104,218,246,
		97,228,251,34,242,193,238,210,144,12,191,179,162,241,
		81,51,145,235,249,14,239,107,49,192,214,31,181,199,106,
		157,184,84,204,176,115,121,50,45,127, 4,150,254,138,236,
		205,93,222,114, 67,29,24,72,243,141,128,195,78,66,215,
		61,156,180
	};

	inline float fade(float t) {
		return t * t * t * (t * (t * 6 - 15) + 10);
	}
	inline float lerp(float a, float b, float t) {
		return a + t * (b - a);
	}
	inline float grad(int hash, float x, float y, float z) {
		int h = hash & 15;
		float u = h < 8 ? x : y;
		float v = h < 4 ? y : (h == 12 || h == 14 ? x : z);
		return ((h & 1) ? -u : u) + ((h & 2) ? -v : v);
	}

	// gradients for simplex noise
	static const int grad3[12][3] = {
		{1,1,0},{-1,1,0},{1,-1,0},{-1,-1,0},
		{1,0,1},{-1,0,1},{1,0,-1},{-1,0,-1},
		{0,1,1},{0,-1,1},{0,1,-1},{0,-1,-1}
	};
	constexpr float F3 = 1.0f / 3.0f;
	constexpr float G3 = 1.0f / 6.0f;

	inline float dot3(const int* g, float x, float y, float z) {
		return g[0] * x + g[1] * y + g[2] * z;
	}

	// --- Perlin Noise (Improved) ---
	f32 perlinNoise(const vox_vec3& v) 
	{
		float x = v.x, y = v.y, z = v.z;
		int X = int(std::floor(x)) & 255;
		int Y = int(std::floor(y)) & 255;
		int Z = int(std::floor(z)) & 255;
		x -= std::floor(x);
		y -= std::floor(y);
		z -= std::floor(z);
		float u = fade(x), w = fade(y), t = fade(z);

		int A = perm[X] + Y, AA = perm[A] + Z, AB = perm[A + 1] + Z;
		int B = perm[X + 1] + Y, BA = perm[B] + Z, BB = perm[B + 1] + Z;

		return lerp(
			lerp(
				lerp(grad(perm[AA], x, y, z),
					grad(perm[BA], x - 1, y, z), u),
				lerp(grad(perm[AB], x, y - 1, z),
					grad(perm[BB], x - 1, y - 1, z), u),
				w),
			lerp(
				lerp(grad(perm[AA + 1], x, y, z - 1),
					grad(perm[BA + 1], x - 1, y, z - 1), u),
				lerp(grad(perm[AB + 1], x, y - 1, z - 1),
					grad(perm[BB + 1], x - 1, y - 1, z - 1), u),
				w),
			t);
	}

	// --- 3D Simplex Noise ---
	f32 simplexNoise(const vox_vec3& v) {
		float x = v.x, y = v.y, z = v.z;
		// Skew into simplex cell
		float s = (x + y + z) * F3;
		int i = int(std::floor(x + s));
		int j = int(std::floor(y + s));
		int k = int(std::floor(z + s));
		float t = (i + j + k) * G3;
		float X0 = i - t, Y0 = j - t, Z0 = k - t;
		x -= X0; y -= Y0; z -= Z0;

		int i1, j1, k1, i2, j2, k2;
		if (x >= y) {
			if (y >= z) { i1 = 1; j1 = 0; k1 = 0;  i2 = 1; j2 = 1; k2 = 0; }
			else if (x >= z) { i1 = 1; j1 = 0; k1 = 0;  i2 = 1; j2 = 0; k2 = 1; }
			else { i1 = 0; j1 = 0; k1 = 1;  i2 = 1; j2 = 0; k2 = 1; }
		}
		else {
			if (y < z) { i1 = 0; j1 = 0; k1 = 1;  i2 = 0; j2 = 1; k2 = 1; }
			else if (x < z) { i1 = 0; j1 = 1; k1 = 0;  i2 = 0; j2 = 1; k2 = 1; }
			else { i1 = 0; j1 = 1; k1 = 0;  i2 = 1; j2 = 1; k2 = 0; }
		}

		float x1 = x - i1 + G3, y1 = y - j1 + G3, z1 = z - k1 + G3;
		float x2 = x - i2 + 2 * G3, y2 = y - j2 + 2 * G3, z2 = z - k2 + 2 * G3;
		float x3 = x - 1 + 3 * G3, y3 = y - 1 + 3 * G3, z3 = z - 1 + 3 * G3;

		int ii = i & 255, jj = j & 255, kk = k & 255;
		int gi0 = perm[ii + perm[jj + perm[kk]]] % 12;
		int gi1 = perm[ii + i1 + perm[jj + j1 + perm[kk + k1]]] % 12;
		int gi2 = perm[ii + i2 + perm[jj + j2 + perm[kk + k2]]] % 12;
		int gi3 = perm[ii + 1 + perm[jj + 1 + perm[kk + 1]]] % 12;

		auto contrib = [&](float t0, int gi, float xi, float yi, float zi) {
			if (t0 < 0) return 0.f;
			t0 *= t0;
			return t0 * t0 * dot3(grad3[gi], xi, yi, zi);
			};

		float n0 = contrib(0.6f - x * x - y * y - z * z, gi0, x, y, z);
		float n1 = contrib(0.6f - x1 * x1 - y1 * y1 - z1 * z1, gi1, x1, y1, z1);
		float n2 = contrib(0.6f - x2 * x2 - y2 * y2 - z2 * z2, gi2, x2, y2, z2);
		float n3 = contrib(0.6f - x3 * x3 - y3 * y3 - z3 * z3, gi3, x3, y3, z3);

		// scale to [-1,1]
		return 32.f * (n0 + n1 + n2 + n3);
	}

	// --- fBm & Turbulence (using Perlin) ---
	f32 fbm(const vox_vec3& v) {
		f32 sum = 0.f, amp = 0.5f;
		vox_vec3 pos = v;
		for (int i = 0; i < 6; ++i) {
			sum += amp * perlinNoise(pos);
			pos *= 2.f;
			amp *= 0.5f;
		}
		return sum;
	}

	f32 turbulence(const vox_vec3& v)
	{
		f32 sum = 0.f, amp = 1.f;
		vox_vec3 pos = v;
		for (int i = 0; i < 6; ++i) {
			sum += amp * std::fabs(perlinNoise(pos));
			pos *= 2.f;
			amp *= 0.5f;
		}
		return sum;
	}

	vox_vec3 noise3D(f32 t, f32 frequency)
	{
		// sample Perlin noise along X, Y, Z with the same time but different axes
		f32 tf = t * frequency;
		return 
		{
			perlinNoise(vox_vec3(tf, 0.f, 0.f)),
			perlinNoise(vox_vec3(0.f, tf, 0.f)),
			perlinNoise(vox_vec3(0.f, 0.f, tf))
		};
	}

	vox_vec3 fbmNoise3D(f32 t, f32 frequency = 1.f, int octaves = 4)
	{
		vox_vec3 sum{ 0,0,0 };
		f32    amp = 1.0f;
		f32    total = 0.0f;

		for (int i = 0; i < octaves; ++i)
		{
			// sample your 3-component noise
			vox_vec3 sample = noise3D(t * frequency);
			// scale it by the current amplitude
			vox_vec3 scaled = sample * amp;
			// accumulate by *reassigning* sum = sum + scaled
			sum = sum + scaled;

			total += amp;
			frequency *= 2.0f;  // lacunarity
			amp *= 0.5f;  // gain
		}

		// normalize by the total amplitude so result stays in [-1,1]
		return sum / total;
	}
}