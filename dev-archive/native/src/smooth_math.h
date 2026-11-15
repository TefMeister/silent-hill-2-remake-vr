// smooth_math.h -- the One Euro filter step of smooth.cpp, pure numbers and nothing of the game's, so a test can run
// the shipped code itself (dev-archive/native/tests/smooth_test.cpp). Village's visceral_re8_hand_smooth.lua step(),
// written out for a place (cm) and a turn given as its forward and up axes.
#pragma once

#include <algorithm>
#include <cmath>

#include "settings.h"

namespace sh2vr::smooth_math {

struct V3 { double x = 0, y = 0, z = 0; };
inline V3 add(V3 a, V3 b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
inline V3 sub(V3 a, V3 b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
inline V3 mul(V3 a, double s) { return {a.x * s, a.y * s, a.z * s}; }
inline double dot(V3 a, V3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline double len(V3 v) { return std::sqrt(dot(v, v)); }
inline V3 lerp(V3 a, V3 b, double t) { return add(a, mul(sub(b, a), t)); }
inline V3 nlerp(V3 a, V3 b, double t) { const V3 m = lerp(a, b, t); const double l = len(m); return l < 1e-9 ? b : mul(m, 1.0 / l); }
inline double angle(V3 a, V3 b) { return std::acos(std::clamp(dot(a, b), -1.0, 1.0)); }
inline double alpha(double cutoff_hz, double dt) { const double tau = 1.0 / (2.0 * 3.14159265358979323846 * cutoff_hz); return 1.0 / (1.0 + tau / dt); }
inline double still_hz(double strength) { return settings::SMOOTH_CUTOFF_TOP_HZ * std::pow(settings::SMOOTH_CUTOFF_STEP, strength); }

struct Hand {
    bool have = false;
    V3 p, f, u;            // the smoothed pose: place (cm), forward, up
    V3 rp, rf, ru;         // last frame's raw pose: the speed is the controller's own (Casiez's reference code)
    double w = 0, v = 0;   // filtered turn speed (rad/s) and move speed (cm/s)
    double raw_sum = 0, out_sum = 0;
    int still_n = 0;
};

// one frame: the raw pose in, the smoothed pose left in h.p / h.f / h.u
inline void step(Hand& h, V3 pc, V3 fc, V3 uc, double dt, double strength) {
    if (!h.have) { h.p = h.rp = pc; h.f = h.rf = fc; h.u = h.ru = uc; h.w = h.v = 0; h.have = true; return; }
    const double ang = std::max(angle(h.rf, fc), angle(h.ru, uc)), dist = len(sub(pc, h.rp));
    const double gap_ang = std::max(angle(h.f, fc), angle(h.u, uc)), gap = len(sub(pc, h.p));
    h.rp = pc; h.rf = fc; h.ru = uc;
    if (ang > settings::SMOOTH_SNAP_RAD || dist > settings::SMOOTH_SNAP_CM || gap_ang > settings::SMOOTH_SNAP_RAD || gap > settings::SMOOTH_SNAP_CM) {
        h.p = pc; h.f = fc; h.u = uc; h.w = h.v = 0;   // a recentre or lost tracking: taken as it is
        return;
    }
    const double ad = alpha(settings::SMOOTH_D_CUTOFF_HZ, dt);
    h.w += (ang / dt - h.w) * ad;
    h.v += (dist / dt - h.v) * ad;
    const double hz = still_hz(strength), bk = std::pow(settings::SMOOTH_BETA_STEP, strength);
    const V3 prev_f = h.f, prev_u = h.u;
    const double ar = alpha(hz + settings::SMOOTH_BETA_ROT * bk * h.w, dt);
    h.f = nlerp(h.f, fc, ar);
    h.u = nlerp(h.u, uc, ar);
    h.p = lerp(h.p, pc, alpha(hz + settings::SMOOTH_BETA_POS * bk * h.v, dt));
    if (ang / dt < settings::SMOOTH_STILL_RAD_S) {   // the proof line: held still
        h.raw_sum += ang;
        h.out_sum += std::max(angle(prev_f, h.f), angle(prev_u, h.u));
        ++h.still_n;
    }
}

} // namespace sh2vr::smooth_math
