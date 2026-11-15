// smooth_test.cpp -- runs the shipped filter step (src/smooth_math.h) on made-up hands at 72 frames a second and checks
// what Tefa asked for: a gun hand much steadier when held still, a melee hand a little steadier and NOT sluggish in a
// swing, a jump (recentre) taken at once. Build and run: bash dev-archive/native/tests/run.sh
#include <cstdio>
#include <random>

#include "../src/smooth_math.h"

using namespace sh2vr;
using namespace sh2vr::smooth_math;

namespace {
constexpr double DT = 1.0 / 72, PI = 3.14159265358979323846;
int g_fail = 0;
void check(bool ok, const char* what, double got) { std::printf("%s  %-62s %.3f\n", ok ? "PASS" : "FAIL", what, got); if (!ok) ++g_fail; }

V3 rot_z(V3 v, double a) { return {v.x * std::cos(a) - v.y * std::sin(a), v.x * std::sin(a) + v.y * std::cos(a), v.z}; }
V3 rot_y(V3 v, double a) { return {v.x * std::cos(a) + v.z * std::sin(a), v.y, -v.x * std::sin(a) + v.z * std::cos(a)}; }

struct Still { double raw_deg, out_deg, raw_cm, out_cm; };
// a hand held still with tremor: 0.25 deg and 0.15 cm of white noise a frame (about what Village's Ghost Hand read)
Still still_run(double strength) {
    std::mt19937 rng(7);
    std::normal_distribution<double> n(0.0, 1.0);
    Hand h;
    V3 lp{}, lf{1, 0, 0}, lop{}, lof{1, 0, 0};
    double raw_a = 0, out_a = 0, raw_d = 0, out_d = 0;
    const int frames = 72 * 5;
    for (int i = 0; i < frames; ++i) {
        const double yaw = n(rng) * 0.25 * PI / 180, pitch = n(rng) * 0.25 * PI / 180;
        const V3 f = rot_z(rot_y({1, 0, 0}, pitch), yaw), u = rot_z(rot_y({0, 0, 1}, pitch), yaw);
        const V3 p{n(rng) * 0.15, n(rng) * 0.15, n(rng) * 0.15};
        step(h, p, f, u, DT, strength);
        if (i > 72) { raw_a += angle(lf, f); out_a += angle(lof, h.f); raw_d += len(sub(p, lp)); out_d += len(sub(h.p, lop)); }
        lf = f; lp = p; lof = h.f; lop = h.p;
    }
    const double k = 1.0 / (frames - 73);
    return {raw_a * k * 180 / PI, out_a * k * 180 / PI, raw_d * k, out_d * k};
}

// a swing: 120 degrees of yaw in 0.25 s (8.4 rad/s, a melee swing), then held; the worst lag behind the hand, degrees
double swing_lag(double strength, double* settle_ms) {
    Hand h;
    double worst = 0;
    const int swing = 18;   // 0.25 s
    const double total = 120 * PI / 180;
    for (int i = 0; i < 72 * 2; ++i) {
        const double a = i < 10 ? 0 : i < 10 + swing ? total * (i - 10) / swing : total;
        const V3 f = rot_z({1, 0, 0}, a), u{0, 0, 1};
        step(h, V3{}, f, u, DT, strength);
        const double lag = angle(h.f, f) * 180 / PI;
        if (i < 10 + swing) worst = std::max(worst, lag);
        if (i >= 10 + swing && lag < 1.0 && *settle_ms < 0) *settle_ms = (i - 10 - swing) * DT * 1000;
    }
    return worst;
}
} // namespace

int main() {
    const double gun = settings::SMOOTH_GUN_STRENGTH, melee = settings::SMOOTH_MELEE_STRENGTH;
    std::printf("still cut-off: gun %.2f Hz, melee %.2f Hz\n", still_hz(gun), still_hz(melee));

    const Still sg = still_run(gun), sm = still_run(melee);
    std::printf("held still, turn per frame: raw %.3f deg, gun %.3f, melee %.3f; move per frame: raw %.3f cm, gun %.3f, melee %.3f\n",
                sg.raw_deg, sg.out_deg, sm.out_deg, sg.raw_cm, sg.out_cm, sm.out_cm);
    check(sg.out_deg < 0.35 * sg.raw_deg, "gun: still-hand shake (turn) cut to under 35 %", sg.out_deg / sg.raw_deg);
    check(sg.out_cm < 0.35 * sg.raw_cm, "gun: still-hand shake (place) cut to under 35 %", sg.out_cm / sg.raw_cm);
    check(sm.out_deg < 0.8 * sm.raw_deg, "melee: still-hand shake cut (under 80 %)", sm.out_deg / sm.raw_deg);
    check(sm.out_deg > sg.out_deg, "melee is lighter than the gun", sm.out_deg - sg.out_deg);

    double settle_g = -1, settle_m = -1;
    const double lag_g = swing_lag(gun, &settle_g), lag_m = swing_lag(melee, &settle_m);
    std::printf("120 deg swing in 0.25 s: worst lag gun %.1f deg (settles under 1 deg in %.0f ms), melee %.1f deg (%.0f ms)\n", lag_g, settle_g, lag_m, settle_m);
    check(lag_m < 12.0, "melee: worst lag in a fast swing under 12 deg (not sluggish)", lag_m);
    check(settle_m >= 0 && settle_m < 60, "melee: settles under 1 deg within 60 ms after the swing", settle_m);
    check(settle_g >= 0 && settle_g < 250, "gun: settles under 1 deg within 250 ms after a flick", settle_g);

    Hand h;   // a recentre: one metre in one frame is taken as it is
    step(h, V3{}, {1, 0, 0}, {0, 0, 1}, DT, gun);
    step(h, V3{100, 0, 0}, {1, 0, 0}, {0, 0, 1}, DT, gun);
    check(len(sub(h.p, V3{100, 0, 0})) < 1e-9, "a 1 m jump (recentre) is taken at once", len(sub(h.p, V3{100, 0, 0})));

    std::printf(g_fail ? "%d FAILED\n" : "all passed\n", g_fail);
    return g_fail ? 1 : 0;
}
