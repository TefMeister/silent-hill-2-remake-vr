// carry_test.cpp -- does the shipped carry (src/carry_math.h) put a pixel of the hand where the game will draw it?
// Ground truth is built a second, independent way: straight in Unreal's own world (its rotation matrix from a
// rotator, a camera's forward / right / up), never through the code under test. The views handed to the carry are
// built the way UEVR's open side builds them for the frame warp (translate(position) * yawPitchRoll(-yaw, pitch,
// -roll), position = (y, z, -x) / scale), with glm's own yawPitchRoll written out here.
// Build and run: bash dev-archive/native/tests/run.sh
#include <cstdio>
#include <random>

#include "../src/carry_math.h"

using namespace sh2vr::carry_math;

namespace {
constexpr double PI = 3.14159265358979323846, D2R = PI / 180;
int g_fail = 0;
void check(bool ok, const char* what, double got) { std::printf("%s  %-78s %.4f\n", ok ? "PASS" : "FAIL", what, got); if (!ok) ++g_fail; }

struct V { double x, y, z; };
struct R3 { double pitch, yaw, roll; };
V sub(V a, V b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
V add(V a, V b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
double dot(V a, V b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

// Unreal's FRotationMatrix axes of a rotator
void ue_axes(R3 r, V& fwd, V& right, V& up) {
    const double sp = std::sin(r.pitch * D2R), cp = std::cos(r.pitch * D2R), sy = std::sin(r.yaw * D2R), cy = std::cos(r.yaw * D2R), sr = std::sin(r.roll * D2R), cr = std::cos(r.roll * D2R);
    fwd = {cp * cy, cp * sy, sp};
    right = {sr * sp * cy - cr * sy, sr * sp * sy + cr * cy, -sr * cp};
    up = {-(cr * sp * cy + sr * sy), cy * sr - cr * sp * sy, cr * cp};
}
// Unreal's FRotator::Quaternion (x y z w)
void ue_quat(R3 r, double q[4]) {
    const double hr = r.roll * D2R * 0.5, hp = r.pitch * D2R * 0.5, hy = r.yaw * D2R * 0.5;
    const double sr = std::sin(hr), cr = std::cos(hr), sp = std::sin(hp), cp = std::cos(hp), sy = std::sin(hy), cy = std::cos(hy);
    q[0] = cr * sp * sy - sr * cp * cy; q[1] = -cr * sp * cy - sr * cp * sy; q[2] = cr * cp * sy - sr * sp * cy; q[3] = cr * cp * cy + sr * sp * sy;
}
// a point given in a rotator's own axes, in the world
V ue_local_to_world(V origin, R3 r, V local) {
    V f, rt, u; ue_axes(r, f, rt, u);
    return {origin.x + f.x * local.x + rt.x * local.y + u.x * local.z, origin.y + f.y * local.x + rt.y * local.y + u.y * local.z, origin.z + f.z * local.x + rt.z * local.y + u.z * local.z};
}
// the truth: a UE world point in front of a UE camera, as (right, up, -forward) in metres
V truth_view(V cam, R3 rot, V p, double scale) {
    V f, r, u; ue_axes(rot, f, r, u);
    const V d = sub(p, cam);
    return {dot(d, r) / scale, dot(d, u) / scale, -dot(d, f) / scale};
}

// glm::yawPitchRoll, written out (m[col * 4 + row])
void yaw_pitch_roll(double yaw, double pitch, double roll, double m[16]) {
    const double ch = std::cos(yaw), sh = std::sin(yaw), cp = std::cos(pitch), sp = std::sin(pitch), cb = std::cos(roll), sb = std::sin(roll);
    for (int i = 0; i < 16; ++i) m[i] = 0;
    m[0] = ch * cb + sh * sp * sb; m[1] = sb * cp; m[2] = -sh * cb + ch * sp * sb;
    m[4] = -ch * sb + sh * sp * cb; m[5] = cb * cp; m[6] = sb * sh + ch * sp * cb;
    m[8] = sh * cp; m[9] = -sp; m[10] = ch * cp;
    m[15] = 1;
}
// the world-to-view UEVR hands the warp for a UE camera (floats, as the call holds it)
void uevr_w2v(V cam, R3 rot, float scale, float out[16]) {
    double v2w[16];
    yaw_pitch_roll(-rot.yaw * D2R, rot.pitch * D2R, -rot.roll * D2R, v2w);
    const float pos[3] = {(float)(cam.y / (double)scale), (float)(cam.z / (double)scale), (float)(-cam.x / (double)scale)};
    // inverse of translate(pos) * R: turn = R^T, shift = -R^T pos
    for (int r = 0; r < 3; ++r) for (int c = 0; c < 3; ++c) out[c * 4 + r] = (float)v2w[r * 4 + c];
    for (int r = 0; r < 3; ++r) { double s = 0; for (int k = 0; k < 3; ++k) s -= v2w[r * 4 + k] * (double)pos[k]; out[12 + r] = (float)s; }
    out[3] = out[7] = out[11] = 0; out[15] = 1;
}
V through(const float* w2v, V p, double scale) {
    const double g[3] = {p.y / scale, p.z / scale, -p.x / scale};
    V o{};
    double* e[3] = {&o.x, &o.y, &o.z};
    for (int r = 0; r < 3; ++r) *e[r] = at(w2v, r, 0) * g[0] + at(w2v, r, 1) * g[1] + at(w2v, r, 2) * g[2] + at(w2v, r, 3);
    return o;
}
double gap(V a, V b) { const V d = sub(a, b); return std::sqrt(dot(d, d)); }

struct Result { double view_mm = 0, carry_mm = 0, carry_px = 0; };
Result run(double world_extent_cm, int n, unsigned seed) {
    std::mt19937 rng(seed);
    auto uni = [&](double a, double b) { return std::uniform_real_distribution<double>(a, b)(rng); };
    const float scale = 100.0f * 1.0f;
    Result res;
    for (int i = 0; i < n; ++i) {
        // the eye then and now: James walks, the head moves and turns between two frames
        const V cam0{uni(-1, 1) * world_extent_cm, uni(-1, 1) * world_extent_cm, uni(-0.1, 0.1) * world_extent_cm};
        const R3 rot0{uni(-60, 60), uni(-180, 180), uni(-20, 20)};
        const V cam1 = add(cam0, {uni(-6, 6), uni(-6, 6), uni(-2, 2)});
        const R3 rot1{rot0.pitch + uni(-3, 3), rot0.yaw + uni(-4, 4), rot0.roll + uni(-2, 2)};
        // the hand then and now, within arm's reach in front of the eye, moving and turning quickly
        const V hand0 = ue_local_to_world(cam0, rot0, {uni(25, 60), uni(-30, 30), uni(-35, 10)});
        const R3 hrot0{uni(-80, 80), uni(-180, 180), uni(-90, 90)};
        const V hand1 = add(hand0, {uni(-5, 5), uni(-5, 5), uni(-5, 5)});
        const R3 hrot1{hrot0.pitch + uni(-8, 8), hrot0.yaw + uni(-8, 8), hrot0.roll + uni(-8, 8)};
        // a point of the gun, fixed to the hand
        const V local{uni(-10, 40), uni(-8, 8), uni(-10, 15)};
        const V p0 = ue_local_to_world(hand0, hrot0, local), p1 = ue_local_to_world(hand1, hrot1, local);

        float w0[16], w1[16];
        uevr_w2v(cam0, rot0, scale, w0); uevr_w2v(cam1, rot1, scale, w1);
        // 1. the convention itself: UEVR's view of a world point against Unreal's own
        const V t0 = truth_view(cam0, rot0, p0, scale), t1 = truth_view(cam1, rot1, p1, scale);
        res.view_mm = std::max(res.view_mm, gap(through(w0, p0, scale), t0) * 1000);
        // 2. the shipped carry
        double q0[4], q1[4]; ue_quat(hrot0, q0); ue_quat(hrot1, q1);
        const double h0[3] = {hand0.x, hand0.y, hand0.z}, h1[3] = {hand1.x, hand1.y, hand1.z};
        const Pose a = to_warp_world(h0, q0, scale), b = to_warp_world(h1, q1, scale);
        float rows[12], then_view[3], out[3];
        if (!carry(w0, w1, a, b, rows, then_view)) { ++g_fail; continue; }
        const float in[3] = {(float)t0.x, (float)t0.y, (float)t0.z};
        apply(rows, in, out);
        const double mm = gap({out[0], out[1], out[2]}, t1) * 1000;
        res.carry_mm = std::max(res.carry_mm, mm);
        res.carry_px = std::max(res.carry_px, mm / 1000 / std::max(0.25, -t1.z) * 180 / PI * 26.0);
    }
    return res;
}
} // namespace

int main() {
    std::printf("the carry by the drawn hand's own move: the largest miss over 20000 made-up hands, views and gun points\n\n");
    const Result town = run(30000, 20000, 11);       // within 300 m of the map's origin
    const Result far = run(300000, 20000, 12);       // 3 km out
    check(town.view_mm < 0.5, "UEVR's view of a point = Unreal's own (the axes and the turn read right), mm", town.view_mm);
    check(town.carry_mm < 0.5, "a gun point carried from the last real picture lands on the drawn gun, mm", town.carry_mm);
    check(town.carry_px < 1.0, "... the same in pixels of the eye picture", town.carry_px);
    std::printf("INFO  3 km from the map's origin (the views are floats): the miss, mm %47.4f  (px %.2f)\n", far.carry_mm, far.carry_px);
    Pose none;
    float rows[12], tv[3], w[16]{};
    check(!carry(w, w, none, none, rows, tv), "a missing pose is refused", 0);
    return g_fail;
}
