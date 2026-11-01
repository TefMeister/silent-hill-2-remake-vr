// rhbolt.cpp -- see rhbolt.h
#include "rhbolt.h"

#include <cmath>
#include <cstdio>
#include <string>

#include "bridge.h"
#include "buttons.h"
#include "common.h"
#include "controllers.h"
#include "gun.h"
#include "holsters.h"
#include "ik.h"
#include "input.h"
#include "settings.h"
#include "twohand.h"
#include "ue.h"

namespace sh2vr::rhbolt {

namespace {

using ue::Quat;
using ue::Transform;
using ue::Vec3;

// ---- small maths (Unreal's quaternions are ordinary Hamilton ones), as twohand.cpp ----
Vec3 add(Vec3 a, Vec3 b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
Vec3 sub(Vec3 a, Vec3 b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
Vec3 mul(Vec3 a, double s) { return {a.x * s, a.y * s, a.z * s}; }
double dot(Vec3 a, Vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
Vec3 cross(Vec3 a, Vec3 b) { return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x}; }
Quat qmul(Quat a, Quat b) {
    return {a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y, a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
            a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w, a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z};
}
Quat qconj(Quat q) { return {-q.x, -q.y, -q.z, q.w}; }
Quat qnorm(Quat q) { const double l = std::sqrt(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w); return l > 1e-12 ? Quat{q.x / l, q.y / l, q.z / l, q.w / l} : Quat{}; }
Vec3 qrot(Quat q, Vec3 v) { const Vec3 u{q.x, q.y, q.z}; const Vec3 t = mul(cross(u, v), 2.0); return add(add(v, mul(t, q.w)), cross(u, t)); }
Quat nlerp(Quat a, Quat b, double u) {
    if (a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w < 0) b = {-b.x, -b.y, -b.z, -b.w};
    return qnorm({a.x + (b.x - a.x) * u, a.y + (b.y - a.y) * u, a.z + (b.z - a.z) * u, a.w + (b.w - a.w) * u});
}
Vec3 lerp(Vec3 a, Vec3 b, double u) { return add(a, mul(sub(b, a), u)); }
double ease(double t) { return t <= 0 ? 0 : t >= 1 ? 1 : t * t * (3.0 - 2.0 * t); }
double clamp01(double v) { return v < 0 ? 0 : v > 1 ? 1 : v; }

// a pose without scale (the gun keeps its own scale when written)
struct Pose { Vec3 pos; Quat rot; };
Pose pmul(const Pose& a, const Pose& b) { return {add(a.pos, qrot(a.rot, b.pos)), qnorm(qmul(a.rot, b.rot))}; }
Pose pinv(const Pose& a) { const Quat iq = qconj(qnorm(a.rot)); return {qrot(iq, mul(a.pos, -1.0)), iq}; }
Pose pblend(const Pose& a, const Pose& b, double u) { return {lerp(a.pos, b.pos, u), nlerp(a.rot, b.rot, u)}; }
Pose of(const Transform& t) { return {t.pos, qnorm(t.rot)}; }

bool world_of(API::UObject* c, Transform& out) { return ue::alive(c) && ue::call_ret(c, L"K2_GetComponentToWorld", {}, out); }
bool set_world(API::UObject* c, const Transform& t) { return ue::call(c, L"K2_SetWorldTransform", {t, false}); }
bool hand(controllers::Side s, Pose& out) { Transform t; if (!world_of(controllers::find(s), t)) return false; out = of(t); return true; }
ue::Rot rot_of(Quat q) { Transform t; t.rot = q; return gun::rot_of(t); }
bool is_rifle(API::UObject* w) { return w != nullptr && ue::name_of((API::UObject*)w->get_class()).find("Rifle") != std::string::npos; }
Vec3 rear_local() { return {-settings::TWO_HAND_BARREL_AXIS[0], -settings::TWO_HAND_BARREL_AXIS[1], -settings::TWO_HAND_BARREL_AXIS[2]}; }

void say(const char* line) { LOGI("%s rhbolt: %s", TAG, line); }

// LOCKED: the rifle trails the left controller, both hands drawn on it. ON: RG held, the right hand gliding onto / riding
// the bolt. BACK: gliding back to the grip (LOCKED again while LT is held). HANDOVER: the gun eased back to UEVR's place.
enum class Phase { OFF, LOCKED, ON, BACK, HANDOVER };
Phase g_phase = Phase::OFF;
bool g_captured = false;          // the poses below are taken at the first view after LT (once twohand / UEVR placed the gun)
API::UObject* g_mesh = nullptr;
Vec3 g_scale{1, 1, 1};
Pose g_gun_k{};                   // the gun in the left controller's frame at the lock
Pose g_left_in_gun{};             // the drawn left hand (its place) and the left controller's turn, in the gun's frame
Pose g_right_in_gun{};            // the drawn right hand and the right controller's turn, in the gun's frame
Vec3 g_bolt_in_gun{};             // bone_lock in the gun's frame
bool g_have_bolt = false;
Pose g_gun_s{};                   // the gun as drawn: trailing the left controller
Pose g_last_written{};
double g_t0 = 0;                  // the current glide / handover's start (seconds)
double g_u = 0, g_u0 = 0;         // 0 = the right hand on the grip, 1 = on the bolt
double g_now = 0;
bool g_rg_was = false;
// the pull: the right controller against the left one, along the barrel toward the stock (cm)
double g_base = 0, g_pull = 0;
int g_bstate = 0;                 // 0 home, 1 pulled back (waiting to go home)
int g_cycles = 0, g_unlocks = 0;
double g_report_in = 0;

double along_rear() {
    Pose l, r;
    if (!hand(controllers::LEFT, l) || !hand(controllers::RIGHT, r)) return 0;
    return dot(sub(r.pos, l.pos), qrot(g_gun_s.rot, rear_local()));
}
void glide_start() { g_u0 = g_u; g_t0 = g_now; }
double glide_k() { return clamp01((g_now - g_t0) / settings::RH_GLIDE_S); }

void to_handover(const char* why) {
    char line[200];
    std::snprintf(line, sizeof line, "%s: the rifle eased back to the right hand (%d bolt cycles, %d unlocks this lock)", why, g_cycles, g_unlocks);
    say(line);
    ik::override_right(false, {}, {});
    g_phase = Phase::HANDOVER;
    g_t0 = g_now;
}
void to_off() {
    ik::override_right(false, {}, {});
    g_phase = Phase::OFF;
    g_captured = false;
    g_mesh = nullptr;
    g_u = 0;
}
} // namespace

bool claims_rg() { return g_phase == Phase::LOCKED || g_phase == Phase::ON || g_phase == Phase::BACK; }
bool active() { return g_phase == Phase::ON || g_phase == Phase::BACK; }

void tick(float delta) {
    g_now += delta;
    if (!settings::RH_BOLT_ON) return;
    API::UObject* w = gun::equipped();
    const bool rifle = is_rifle(w) && !buttons::menu_open() && !holsters::map_held();
    const bool lg = holsters::left_grip(), lt = input::left_trigger();
    const bool rg = holsters::right_grip();
    const bool rg_pressed = rg && !g_rg_was;
    g_rg_was = rg;
    char line[200];

    if (g_phase != Phase::OFF && g_phase != Phase::HANDOVER && (!rifle || (g_mesh != nullptr && gun::mesh(w) != g_mesh))) {
        say("the rifle went away: unlocked at once");
        to_off();
        return;
    }
    switch (g_phase) {
    case Phase::OFF:
        if (rifle && lg && input::left_trigger_pressed() && twohand::gun_two_handed()) {
            g_phase = Phase::LOCKED;
            g_captured = false;
            g_mesh = gun::mesh(w);
            g_cycles = g_unlocks = 0;
            gun::buzz(controllers::LEFT, settings::BUZZ_GRAB_AMP, settings::BUZZ_S);
            say("LT with the left hand on the rifle's front: the rifle LOCKED in the left hand (RG works the bolt)");
        }
        break;
    case Phase::LOCKED:
        if (!lt || !lg) { to_handover(!lt ? "LT let go" : "left grip let go"); break; }
        if (rg_pressed && g_captured) {
            if (!g_have_bolt) { say("RG: the rifle has no bone_lock: no bolt to work"); break; }
            g_phase = Phase::ON;
            glide_start();
            g_base = along_rear();
            g_pull = 0;
            g_bstate = 0;
            gun::buzz(controllers::RIGHT, settings::BUZZ_GRAB_AMP, settings::BUZZ_S);
            std::snprintf(line, sizeof line, "RG: the right hand onto the bolt (the pack's bolt gate %s)", bridge::long_gun_gate() ? "SHUT: a bolt is owed" : "open: a cosmetic cycle");
            say(line);
        }
        break;
    case Phase::ON: {
        g_u = g_u0 + (1.0 - g_u0) * ease(glide_k());
        if (!rg || !lt || !lg) {
            g_phase = Phase::BACK;
            glide_start();
            say(!rg ? "RG let go: the right hand back to the grip" : "LT / left grip let go: the right hand back to the grip, then the lock ends");
            break;
        }
        g_pull = along_rear() - g_base;
        const double t = clamp01(g_pull / settings::RH_BOLT_TRAVEL_CM);
        if (g_bstate == 0 && t >= settings::RH_BOLT_BACK_AT) {
            g_bstate = 1;
            gun::buzz(controllers::RIGHT, settings::BUZZ_GRAB_AMP, settings::BUZZ_S);
            say("bolt pulled back: push it forward");
        } else if (g_bstate == 1 && t <= settings::RH_BOLT_HOME_AT) {
            g_bstate = 0;
            ++g_cycles;
            gun::buzz(controllers::RIGHT, settings::BUZZ_DONE_AMP, settings::BUZZ_S);
            if (bridge::long_gun_gate()) {
                ++g_unlocks;
                bridge::tell_lua("sh2vr_bolt", "unlock");
                say("bolt home: a round chambered (asked the pack's rifle bolt to unlock)");
            } else {
                say("bolt home: a cosmetic cycle (no bolt was owed)");
            }
        }
        break;
    }
    case Phase::BACK:
        g_u = g_u0 * (1.0 - ease(glide_k()));
        g_pull = 0;
        if (glide_k() >= 1.0) {
            if (lt && lg) { g_phase = Phase::LOCKED; say("the right hand back on the grip: still locked (LT held)"); }
            else to_handover(!lt ? "LT let go" : "left grip let go");
        }
        break;
    case Phase::HANDOVER:
        if (g_now - g_t0 >= settings::RH_HANDOVER_S) to_off();
        break;
    }

    // the slow trail, once a frame: the gun eases toward where the left controller carries it
    if (g_captured && g_phase != Phase::OFF) {
        Pose l;
        if (hand(controllers::LEFT, l)) {
            const double k = 1.0 - std::exp(-(double)delta / settings::RH_LOCK_FOLLOW_S);
            if (g_phase != Phase::HANDOVER) g_gun_s = pblend(g_gun_s, pmul(l, g_gun_k), k);
        }
    }
    // proof of effect while locked: the drawn gun against where it was put, and the pull
    g_report_in -= delta;
    if (claims_rg() && g_captured && g_report_in <= 0) {
        g_report_in = settings::RH_REPORT_S;
        Transform mw;
        if (world_of(g_mesh, mw)) {
            std::snprintf(line, sizeof line, "status: %s, the gun drawn %.1f cm from where it was put, pull %.1f cm, %d cycles",
                          g_phase == Phase::LOCKED ? "locked" : "the right hand on the bolt", std::sqrt(dot(sub(mw.pos, g_last_written.pos), sub(mw.pos, g_last_written.pos))),
                          g_pull, g_cycles);
            say(line);
        }
    }
}

void on_view() {
    if (!settings::RH_BOLT_ON || g_phase == Phase::OFF || !ue::alive(g_mesh)) return;
    Transform mw;
    if (!world_of(g_mesh, mw)) return;
    if (!g_captured) {   // the first view after LT: twohand / UEVR have placed the gun this frame
        if (g_phase == Phase::HANDOVER) return;
        Pose l, r;
        Vec3 hl, hr;
        if (!hand(controllers::LEFT, l) || !hand(controllers::RIGHT, r)) return;
        const Pose g = of(mw);
        const Pose ig = pinv(g);
        g_scale = mw.scale;
        g_gun_k = pmul(pinv(l), g);
        if (!ik::hand_world(controllers::LEFT, hl)) hl = l.pos;
        if (!ik::hand_world(controllers::RIGHT, hr)) hr = r.pos;
        g_left_in_gun = {pmul(ig, Pose{hl, {}}).pos, qnorm(qmul(ig.rot, l.rot))};
        g_right_in_gun = {pmul(ig, Pose{hr, {}}).pos, qnorm(qmul(ig.rot, r.rot))};
        Vec3 b;
        g_have_bolt = gun::socket_loc(g_mesh, settings::RH_BOLT_SOCKET, b);
        if (g_have_bolt) g_bolt_in_gun = pmul(ig, Pose{b, {}}).pos;
        g_gun_s = g;
        g_captured = true;
        char line[220];
        std::snprintf(line, sizeof line, "lock taken: the bolt (bone_lock) %s, %.0f cm from the drawn right hand, %.0f cm from the drawn left hand",
                      g_have_bolt ? "found" : "NOT FOUND", g_have_bolt ? std::sqrt(dot(sub(b, hr), sub(b, hr))) : 0.0, g_have_bolt ? std::sqrt(dot(sub(b, hl), sub(b, hl))) : 0.0);
        say(line);
    }
    if (g_phase == Phase::HANDOVER) {   // from where it trailed to where UEVR / twohand put it this view
        const Pose p = pblend(g_gun_s, of(mw), ease((g_now - g_t0) / settings::RH_HANDOVER_S));
        Transform t;
        t.rot = p.rot; t.pos = p.pos; t.scale = g_scale;
        set_world(g_mesh, t);
        return;
    }
    Transform t;
    t.rot = g_gun_s.rot; t.pos = g_gun_s.pos; t.scale = g_scale;
    if (set_world(g_mesh, t)) g_last_written = g_gun_s;
    // both hands on the gun: the left on the front, the right on the grip or the bolt
    const Pose left = pmul(g_gun_s, g_left_in_gun);
    ik::override_left(true, left.pos, rot_of(left.rot));
    const Pose grip = pmul(g_gun_s, g_right_in_gun);
    Pose want = grip;
    if (g_have_bolt && g_u > 0) {
        const double travel = g_pull < 0 ? 0 : g_pull > settings::RH_BOLT_TRAVEL_CM ? settings::RH_BOLT_TRAVEL_CM : g_pull;
        const Vec3 nudge{settings::RH_BOLT_HAND_NUDGE_CM[0], settings::RH_BOLT_HAND_NUDGE_CM[1], settings::RH_BOLT_HAND_NUDGE_CM[2]};
        const Vec3 on_bolt = add(add(g_bolt_in_gun, nudge), mul(rear_local(), travel));
        want = pblend(grip, Pose{pmul(g_gun_s, Pose{on_bolt, {}}).pos, grip.rot}, g_u);
    }
    ik::override_right(true, want.pos, want.rot);
}

} // namespace sh2vr::rhbolt
