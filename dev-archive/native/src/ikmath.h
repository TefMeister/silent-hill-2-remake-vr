// ikmath.h -- the pure-number helpers jbusfield's ik.lua took from core/math_lib.lua, written out the same way so
// the arms bend to the same numbers. Anything that touches Unreal's rotator conventions (compose, transform,
// axis vectors) is NOT here: ik.cpp asks KismetMathLibrary for those, as the Lua did.
#pragma once

#include <cmath>

#include "ue.h"

namespace sh2vr::ikmath {

using ue::Quat;
using ue::Rot;
using ue::Vec3;

constexpr double PI = 3.14159265358979323846;
constexpr double DEG2RAD = PI / 180.0;
constexpr double RAD2DEG = 180.0 / PI;

inline Vec3 add(Vec3 a, Vec3 b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
inline Vec3 sub(Vec3 a, Vec3 b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
inline Vec3 mul(Vec3 a, double s) { return {a.x * s, a.y * s, a.z * s}; }
inline double dot(Vec3 a, Vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline Vec3 cross(Vec3 a, Vec3 b) { return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x}; }
inline double size_sq(Vec3 v) { return dot(v, v); }
inline double size(Vec3 v) { return std::sqrt(size_sq(v)); }
inline bool shorter_than(Vec3 v, double len) { return size_sq(v) < len * len; }   // vectorLengthLessThan

// vectorSafeNormalize: zero when shorter than 0.0001
inline Vec3 safe_normalize(Vec3 v) {
    const double len = size(v);
    if (len < 0.0001) return Vec3{};
    return mul(v, 1.0 / len);
}

// ProjectVectorOnToPlane, the fallback form (the Lua used Kismet's when present; both give v - n * (v.n / n.n))
inline Vec3 project_on_plane(Vec3 v, Vec3 n) {
    const double denom = dot(n, n);
    if (denom <= 1e-8) return Vec3{};
    return sub(v, mul(n, dot(v, n) / denom));
}

// normalizeDeg180: Lua's % is floored, so this is the same for negative angles
inline double normalize_deg180(double a) {
    double r = std::fmod(a + 180.0, 360.0);
    if (r < 0) r += 360.0;
    return r - 180.0;
}

// quatFromEuler (math_lib.lua): the pack's own rotator-to-quaternion, not Kismet's
inline Quat quat_from_euler(Rot r) {
    const double hr = r.roll * DEG2RAD * 0.5, hp = r.pitch * DEG2RAD * 0.5, hy = r.yaw * DEG2RAD * 0.5;
    const double sr = std::sin(hr), cr = std::cos(hr), sp = std::sin(hp), cp = std::cos(hp), sy = std::sin(hy), cy = std::cos(hy);
    return Quat{(cr * sp * sy) - (sr * cp * cy), -(cr * sp * cy) - (sr * cp * sy), (cr * cp * sy) - (sr * sp * cy), (cr * cp * cy) + (sr * sp * sy)};
}

// vectorRotate_Quat
inline Vec3 rotate_quat(Vec3 v, Quat q) {
    const double tx = 2 * (q.y * v.z - q.z * v.y), ty = 2 * (q.z * v.x - q.x * v.z), tz = 2 * (q.x * v.y - q.y * v.x);
    return {v.x + q.w * tx + (q.y * tz - q.z * ty), v.y + q.w * ty + (q.z * tx - q.x * tz), v.z + q.w * tz + (q.x * ty - q.y * tx)};
}

// vectorRotate(vec, rot): rotate by the pack's own quaternion of the rotator
inline Vec3 rotate(Vec3 v, Rot r) { return rotate_quat(v, quat_from_euler(r)); }

// rotatorFromAxisAndAngle (math_lib.lua's own, not Kismet's)
inline Rot rotator_from_axis_angle(Vec3 axis, double angle_deg) {
    const double len = size(axis);
    if (len < 1e-12) return Rot{};
    axis = mul(axis, 1.0 / len);
    const double half = angle_deg * (PI / 360.0);
    const double s = std::sin(half), qw = std::cos(half);
    const double qx = axis.x * s, qy = axis.y * s, qz = axis.z * s;
    double sinp = 2.0 * ((qw * qy) - (qz * qx));
    if (sinp > 1.0) sinp = 1.0; else if (sinp < -1.0) sinp = -1.0;
    const double pitch = -std::asin(sinp);
    const double roll = -std::atan2(2.0 * ((qw * qx) + (qy * qz)), 1.0 - (2.0 * ((qx * qx) + (qy * qy))));
    const double yaw = std::atan2(2.0 * ((qw * qz) + (qx * qy)), 1.0 - (2.0 * ((qy * qy) + (qz * qz))));
    return Rot{pitch * RAD2DEG, yaw * RAD2DEG, roll * RAD2DEG};
}

// signedAngleDegAroundAxis
inline double signed_angle_deg(Vec3 a, Vec3 b, Vec3 axis) {
    return std::atan2(dot(axis, cross(a, b)), dot(a, b)) * RAD2DEG;
}

// computeSwingTwistAroundAxis_Rotators, given the two quaternions Kismet made from the rotators (Roll, Pitch, Yaw
// Euler order, as the Lua fed Quat_MakeFromEuler). Returns the twist in degrees; `swing` gets the swing quaternion
// (unit) when there is one, else stays identity.
inline double swing_twist(Quat qa, Quat qb, Vec3 axis, Quat& swing, bool& have_swing) {
    have_swing = false;
    const double len2 = size_sq(axis);
    if (len2 < 1e-8) return 0.0;
    const double aw = qa.w, ax = qa.x, ay = qa.y, az = qa.z;
    const double bw = qb.w, bx = qb.x, by = qb.y, bz = qb.z;
    const double rw = (bw * aw) + (bx * ax) + (by * ay) + (bz * az);
    const double rx = (-bw * ax) + (bx * aw) - (by * az) + (bz * ay);
    const double ry = (-bw * ay) + (bx * az) + (by * aw) - (bz * ax);
    const double rz = (-bw * az) - (bx * ay) + (by * ax) + (bz * aw);
    const double n2 = (rw * rw) + (rx * rx) + (ry * ry) + (rz * rz);
    if (n2 < 1e-12) return 0.0;
    const double d = (rx * axis.x) + (ry * axis.y) + (rz * axis.z);
    const double twist_deg = (2.0 * std::atan2(d, rw)) * RAD2DEG;
    double twx = axis.x * d, twy = axis.y * d, twz = axis.z * d, tww = rw;
    const double twist_len2 = (twx * twx) + (twy * twy) + (twz * twz) + (tww * tww);
    if (twist_len2 < 1e-12) {
        const double inv = 1.0 / std::sqrt(n2);
        swing = Quat{rx * inv, ry * inv, rz * inv, rw * inv};
        have_swing = true;
        return twist_deg;
    }
    const double inv_t = 1.0 / std::sqrt(twist_len2);
    twx *= inv_t; twy *= inv_t; twz *= inv_t; tww *= inv_t;
    const double sw = (rw * tww) + (rx * twx) + (ry * twy) + (rz * twz);
    const double sx = (-rw * twx) + (rx * tww) - (ry * twz) + (rz * twy);
    const double sy = (-rw * twy) + (rx * twz) + (ry * tww) - (rz * twx);
    const double sz = (-rw * twz) - (rx * twy) + (ry * twx) + (rz * tww);
    const double s2 = (sw * sw) + (sx * sx) + (sy * sy) + (sz * sz);
    if (s2 < 1e-12) return twist_deg;
    const double inv_s = 1.0 / std::sqrt(s2);
    swing = Quat{sx * inv_s, sy * inv_s, sz * inv_s, sw * inv_s};
    have_swing = true;
    return twist_deg;
}

} // namespace sh2vr::ikmath
