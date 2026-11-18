// handcarry_test.cpp -- how far apart are the hand the game DRAWS and the hand the warp's made-up frame SHOWS?
// (2026-11-18, Tefa's top wish: the gun and hands as steady as the headset's rate.)
// With the frame warp each eye is really drawn every second frame; on the frames between, handpaste.cpp carries the
// hand's pixels from that eye's last real picture by "how the hand moved since". Up to b029 that move was the RAW
// controller's (API::VR::get_pose), while since b023 the right hand and its gun are drawn SMOOTHED (smooth.cpp).
// So on a made-up frame the eye shows  drawn(k-1) + raw step(k)  where the next real frame shows  drawn(k):
// the difference flips on and off every other frame = a double image whenever the two steps differ.
// This test runs the SHIPPED filter (src/smooth_math.h) on made-up hand moves and prints that difference, for
//   A. the carry by the raw controller's step (b020-b029), and
//   B. the carry by the drawn hand's own step (the fix), read exactly on time and 1 or 2 frames late.
// Build and run: bash dev-archive/native/tests/run.sh
#include <cstdio>
#include <vector>

#include "../src/smooth_math.h"

using namespace sh2vr;
using namespace sh2vr::smooth_math;

namespace {
constexpr double PI = 3.14159265358979323846;
constexpr double EYE_CM = 45.0;        // a gun held in front of the face
constexpr double MUZZLE_CM = 20.0;     // a point of the gun this far ahead of the hand: the turn's error shows there
constexpr double PX_PER_DEG = 26.0;    // the eye picture here: 2688 px across about 104 degrees
int g_fail = 0;
void check(bool ok, const char* what, double got) { std::printf("%s  %-74s %.2f\n", ok ? "PASS" : "FAIL", what, got); if (!ok) ++g_fail; }

double ease(double t) { t = std::clamp(t, 0.0, 1.0); return t * t * t * (10 + t * (-15 + 6 * t)); }   // a hand's natural start and stop
V3 rot_z(V3 v, double a) { return {v.x * std::cos(a) - v.y * std::sin(a), v.x * std::sin(a) + v.y * std::cos(a), v.z}; }

struct Pose { V3 p, f; };
V3 muzzle(const Pose& s) { return add(s.p, mul(s.f, MUZZLE_CM)); }
// a point of the gun carried from pose a to pose b's frame: place by the step, the muzzle by the turn too
V3 carried(const Pose& shown, const Pose& from, const Pose& to) {
    // the shown muzzle, moved the way (from -> to) moved: the hand's place by the difference, its direction likewise
    const V3 f = add(shown.f, sub(to.f, from.f));
    return add(add(shown.p, sub(to.p, from.p)), mul(f, MUZZLE_CM / std::max(len(f), 1e-9)));
}

struct Out { double raw_px, exact_px, late1_px, late2_px; };
// move_cm sideways and turn_deg about the up axis, both eased over `secs`, at `fps`, with the filter at `strength`
Out run(double move_cm, double turn_deg, double secs, double fps, double strength) {
    const double dt = 1.0 / fps;
    const int lead = 20, n = lead + (int)(secs * fps) + 40;
    std::vector<Pose> raw(n), drawn(n);
    Hand h;
    for (int k = 0; k < n; ++k) {
        const double e = ease((k - lead) * dt / secs);
        raw[k].p = {0, move_cm * e, 0};
        raw[k].f = rot_z({1, 0, 0}, turn_deg * e * PI / 180);
        const V3 u{0, 0, 1};
        if (strength > 0) { step(h, raw[k].p, raw[k].f, u, dt, strength); drawn[k] = {h.p, h.f}; }
        else drawn[k] = raw[k];
    }
    auto px = [](double cm) { return cm / EYE_CM * 180 / PI * PX_PER_DEG; };
    Out o{};
    for (int k = 3; k < n; ++k) {
        const V3 truth = muzzle(drawn[k]);   // what the next real frame (and the other eye, this very frame) shows
        o.raw_px = std::max(o.raw_px, px(len(sub(carried(drawn[k - 1], raw[k - 1], raw[k]), truth))));
        o.exact_px = std::max(o.exact_px, px(len(sub(carried(drawn[k - 1], drawn[k - 1], drawn[k]), truth))));
        if (k + 1 < n) o.late1_px = std::max(o.late1_px, px(len(sub(carried(drawn[k - 1], drawn[k], drawn[k + 1]), truth))));
        if (k + 2 < n) o.late2_px = std::max(o.late2_px, px(len(sub(carried(drawn[k - 1], drawn[k + 1], drawn[k + 2]), truth))));
    }
    return o;
}
} // namespace

int main() {
    std::printf("the made-up frame's gun against the drawn gun: the largest gap during one move, in pixels of the eye picture\n");
    std::printf("(a gun %.0f cm from the eye, a point %.0f cm ahead of the hand, %.0f px a degree)\n\n", EYE_CM, MUZZLE_CM, PX_PER_DEG);
    struct Case { const char* what; double cm, deg, secs; };
    const Case cases[] = {
        {"slow drift: 10 cm in 1.0 s", 10, 0, 1.0},
        {"a little quicker: 20 cm in 0.4 s", 20, 0, 0.4},
        {"quick: 30 cm in 0.25 s", 30, 0, 0.25},
        {"wrist turn: 30 deg in 0.3 s", 0, 30, 0.3},
        {"aim across a room: 25 cm and 40 deg in 0.4 s", 25, 40, 0.4},
    };
    const double gun = settings::SMOOTH_GUN_STRENGTH, melee = settings::SMOOTH_MELEE_STRENGTH;
    double worst_raw_gun = 0, worst_exact = 0, worst_late1 = 0, worst_raw_none = 0, quicker_raw = 0;
    for (double fps : {72.0, 55.0}) {
        std::printf("-- %.0f frames a second --------------------------------- raw carry | drawn carry: exact, 1 late, 2 late\n", fps);
        for (const Case& c : cases) {
            const Out g = run(c.cm, c.deg, c.secs, fps, gun), m = run(c.cm, c.deg, c.secs, fps, melee), z = run(c.cm, c.deg, c.secs, fps, 0);
            std::printf("  gun   %-46s %7.1f | %5.2f %6.1f %6.1f\n", c.what, g.raw_px, g.exact_px, g.late1_px, g.late2_px);
            std::printf("  melee %-46s %7.1f | %5.2f %6.1f %6.1f\n", c.what, m.raw_px, m.exact_px, m.late1_px, m.late2_px);
            std::printf("  bare  %-46s %7.1f | %5.2f %6.1f %6.1f\n", c.what, z.raw_px, z.exact_px, z.late1_px, z.late2_px);
            worst_raw_gun = std::max(worst_raw_gun, g.raw_px); worst_exact = std::max({worst_exact, g.exact_px, m.exact_px, z.exact_px});
            worst_late1 = std::max({worst_late1, g.late1_px, m.late1_px, z.late1_px}); worst_raw_none = std::max(worst_raw_none, z.raw_px);
            if (fps == 72.0 && c.cm == 20 && c.deg == 0) quicker_raw = g.raw_px;
        }
    }
    std::printf("\n");
    check(worst_raw_none < 0.01, "no smoothing (b025, bare hands): the raw carry lands on the drawn hand, px", worst_raw_none);
    check(quicker_raw > 5.0, "a gun, 'a little quicker': the raw carry misses the drawn gun by (the fault), px", quicker_raw);
    check(worst_raw_gun > 20.0, "a gun, the worst of the moves above, raw carry, px", worst_raw_gun);
    check(worst_exact < 0.01, "the drawn hand's own step, read on time: the gap is gone, px", worst_exact);
    std::printf("INFO  the drawn hand's own step read ONE frame late: the worst gap, px %38.2f\n", worst_late1);
    return g_fail;
}
