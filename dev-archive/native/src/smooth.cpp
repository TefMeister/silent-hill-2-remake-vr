// smooth.cpp -- see smooth.h. The filter's maths is smooth_math.h (tested on its own); this file finds what is held,
// moves the hand into its tracking space and back, and writes the proof line.
#include "smooth.h"

#include <algorithm>
#include <string>

#include "gun.h"
#include "holsters.h"
#include "settings.h"
#include "smooth_math.h"

namespace sh2vr::smooth {

using ue::Rot;
using ue::Transform;
using ue::Vec3;
namespace sm = smooth_math;

namespace {
constexpr double RAD2DEG = 57.29577951308232;
API::UObject* kml() { return ue::cdo(L"Class /Script/Engine.KismetMathLibrary"); }
Vec3 kv(const wchar_t* fn, const Transform& t, Vec3 v) { Vec3 o; ue::call_ret(kml(), fn, {t, v}, o); return o; }
Vec3 kr(const wchar_t* fn, Rot r) { Vec3 o; ue::call_ret(kml(), fn, {r}, o); return o; }
Rot from_xz(Vec3 x, Vec3 z) { Rot o; ue::call_ret(kml(), L"MakeRotFromXZ", {x, z}, o); return o; }
sm::V3 v3(Vec3 v) { return {v.x, v.y, v.z}; }
Vec3 uv(sm::V3 v) { return {v.x, v.y, v.z}; }

sm::Hand g_hand;
Kind g_kind = Kind::RAW;
double g_gun = settings::SMOOTH_GUN_STRENGTH, g_melee = settings::SMOOTH_MELEE_STRENGTH;
double g_dt = 1.0 / 72;
bool g_stepped = false;   // the filter steps once a frame
float g_since_kind = settings::SMOOTH_KIND_EVERY_S, g_since_report = 0.0f;

Kind look_up() {
    if (!settings::SMOOTH_ON || !gun::hmd_active() || holsters::heal_in_hand()) return Kind::RAW;
    const std::string n = holsters::in_hand();
    if (n.empty() || n == "None" || n == "?") return Kind::RAW;
    return gun::equipped() != nullptr ? Kind::GUN : Kind::MELEE;
}
const char* name_of(Kind k) { return k == Kind::GUN ? "gun" : k == Kind::MELEE ? "melee" : "raw"; }
double strength() { return g_kind == Kind::GUN ? g_gun : g_kind == Kind::MELEE ? g_melee : 0.0; }
} // namespace

Kind kind() { return g_kind; }

void set_strength(Kind k, double s) {
    s = std::clamp(s, 0.0, 10.0);
    if (k == Kind::GUN) g_gun = s;
    else if (k == Kind::MELEE) g_melee = s;
    LOGI("%s smooth: %s strength %.1f (live)", TAG, name_of(k), s);
}

void tick(float delta) {
    g_dt = std::clamp((double)delta, settings::SMOOTH_DT_MIN, settings::SMOOTH_DT_MAX);
    g_stepped = false;
    g_since_kind += delta;
    if (g_since_kind >= settings::SMOOTH_KIND_EVERY_S) {
        g_since_kind = 0.0f;
        const Kind k = look_up();
        if (k != g_kind) {
            g_kind = k;
            g_hand.have = false;   // start again from the hand as it is: no catch-up glide
            LOGI("%s smooth: %s in hand, strength %.1f", TAG, name_of(k), strength());
        }
    }
    g_since_report += delta;
    if (g_since_report >= settings::SMOOTH_REPORT_S) {
        g_since_report = 0.0f;
        if (g_kind != Kind::RAW && g_hand.still_n > 0)
            LOGI("%s smooth: %s, strength %.1f: hand held still %d frames, turn per frame raw %.3f deg, drawn %.3f deg", TAG, name_of(g_kind),
                 strength(), g_hand.still_n, g_hand.raw_sum / g_hand.still_n * RAD2DEG, g_hand.out_sum / g_hand.still_n * RAD2DEG);
        g_hand.raw_sum = g_hand.out_sum = 0;
        g_hand.still_n = 0;
    }
}

void apply(API::UObject* controller, Vec3& pos_ws, Rot& rot_ws) {
    const double st = strength();
    if (st <= 0.0 || !ue::alive(controller)) return;
    // the tracking space: the controller's attach parent (moves and turns with James); none = world
    API::UObject* parent = nullptr;
    Transform t;   // identity
    if (ue::call_ret(controller, L"GetAttachParent", {}, parent) && ue::alive(parent)) ue::call_ret(parent, L"K2_GetComponentToWorld", {}, t);
    if (!g_stepped) {
        g_stepped = true;
        sm::step(g_hand, v3(kv(L"InverseTransformLocation", t, pos_ws)), v3(kv(L"InverseTransformDirection", t, kr(L"GetForwardVector", rot_ws))),
                 v3(kv(L"InverseTransformDirection", t, kr(L"GetUpVector", rot_ws))), g_dt, st);
    }
    if (!g_hand.have) return;
    pos_ws = kv(L"TransformLocation", t, uv(g_hand.p));
    rot_ws = from_xz(kv(L"TransformDirection", t, uv(g_hand.f)), kv(L"TransformDirection", t, uv(g_hand.u)));
}

} // namespace sh2vr::smooth
