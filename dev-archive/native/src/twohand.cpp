// twohand.cpp -- two hands on one thing (2026-10-31, Opus). Tefa: "move the map to the right hip, LG to take it out,
// right hand with RG held to two hand the map", and "please use whatever you need for two handing the map and long
// weapons" from praydog's original SH2 profile (scripts/camera.lua, credited with UEVR).
//
// What praydog's profile does, and what we take from it:
//   - the map: while the game shows the map, the item being looked at (pawn.Items.ItemExecutive.ItemContext) is put at
//     the middle of the two hands, turned so its X runs from the left hand to the right hand, its Z against the hands'
//     average up, then tipped 45 degrees ("realistic two handed map holding");
//   - the shotgun and the rifle: after UEVR has put the weapon on the right controller, it is turned about the right
//     hand so it points at the left hand ("two handed weapon aiming"); praydog does it only in the aim stance, we do it
//     whenever the left grip holds the gun's front (we never use the aim stance).
// Ours, on top of the pack (jbusfield / LukasBlaster7), which holds both the gun and the looked-at item on the RIGHT
// controller through UEVR's UObjectHook (attachments.lua, RAW_CONTROLLER):
//   - the map, held by the left grip at the right hip (holsters.cpp opens it): in the LEFT hand on its own (the pack's
//     right-hand placement, mirrored onto the left controller), between both hands while the right grip is held too;
//   - the long gun: the left grip pressed with the left hand out along the barrel starts it; the gun keeps the angle it
//     had at that moment and from then on turns about the right hand to follow the left hand, so it never jumps; letting
//     go of the left grip ends it (UEVR puts the gun back on the right hand by itself the next frame).
// Every write aims at a target (the left hand, the hands' middle), never adds to our own last write, so running twice
// in one frame (two eyes), or on a frame UEVR did not reset, gives the same answer.
// [compile-verified 2026-10-31] until it has run.

#include "twohand.h"

#include <cmath>
#include <cstdio>
#include <string>

#include "buttons.h"
#include "common.h"
#include "controllers.h"
#include "gun.h"
#include "holsters.h"
#include "ik.h"
#include "rack.h"
#include "input.h"
#include "settings.h"
#include "ue.h"

namespace sh2vr::twohand {

namespace {

using ue::Quat;
using ue::Transform;
using ue::Vec3;

// ---- small maths (Unreal's quaternions are ordinary Hamilton ones: x, y, z, w) ----
Vec3 add(Vec3 a, Vec3 b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
Vec3 sub(Vec3 a, Vec3 b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
Vec3 mul(Vec3 a, double s) { return {a.x * s, a.y * s, a.z * s}; }
double dot(Vec3 a, Vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
Vec3 cross(Vec3 a, Vec3 b) { return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x}; }
double len(Vec3 v) { return std::sqrt(dot(v, v)); }
Vec3 norm(Vec3 v) { const double l = len(v); return l > 1e-9 ? mul(v, 1.0 / l) : Vec3{1, 0, 0}; }
Quat qmul(Quat a, Quat b) {
    return {a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y, a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
            a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w, a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z};
}
Quat qconj(Quat q) { return {-q.x, -q.y, -q.z, q.w}; }
Quat qnorm(Quat q) { const double l = std::sqrt(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w); return l > 1e-12 ? Quat{q.x / l, q.y / l, q.z / l, q.w / l} : Quat{}; }
Vec3 qrot(Quat q, Vec3 v) {   // q v q*
    const Vec3 u{q.x, q.y, q.z};
    const Vec3 t = mul(cross(u, v), 2.0);
    return add(add(v, mul(t, q.w)), cross(u, t));
}
Quat from_to(Vec3 a, Vec3 b) {   // the shortest turn taking direction a onto direction b
    a = norm(a); b = norm(b);
    const double d = dot(a, b);
    if (d < -0.999999) {
        Vec3 axis = cross({1, 0, 0}, a);
        if (len(axis) < 1e-6) axis = cross({0, 1, 0}, a);
        axis = norm(axis);
        return {axis.x, axis.y, axis.z, 0};
    }
    const Vec3 c = cross(a, b);
    return qnorm({c.x, c.y, c.z, 1.0 + d});
}
Quat from_axes(Vec3 x, Vec3 y, Vec3 z) {   // the rotation whose local X, Y, Z are these (orthonormal) world axes
    const double m00 = x.x, m11 = y.y, m22 = z.z, tr = m00 + m11 + m22;
    Quat q;
    if (tr > 0) {
        const double s = std::sqrt(tr + 1.0) * 2;
        q = {(y.z - z.y) / s, (z.x - x.z) / s, (x.y - y.x) / s, 0.25 * s};
    } else if (m00 > m11 && m00 > m22) {
        const double s = std::sqrt(1.0 + m00 - m11 - m22) * 2;
        q = {0.25 * s, (y.x + x.y) / s, (z.x + x.z) / s, (y.z - z.y) / s};
    } else if (m11 > m22) {
        const double s = std::sqrt(1.0 + m11 - m00 - m22) * 2;
        q = {(y.x + x.y) / s, 0.25 * s, (z.y + y.z) / s, (z.x - x.z) / s};
    } else {
        const double s = std::sqrt(1.0 + m22 - m00 - m11) * 2;
        q = {(z.x + x.z) / s, (z.y + y.z) / s, 0.25 * s, (x.y - y.x) / s};
    }
    return qnorm(q);
}
// FRotator(roll only).Quaternion(), the way Kismet's Quat_MakeFromEuler(X = roll) builds praydog's tilt
Quat roll_quat(double deg) { const double h = deg * 3.14159265358979323846 / 360.0; return {-std::sin(h), 0, 0, std::cos(h)}; }
Vec3 to_world(const Transform& t, Vec3 p) { return add(t.pos, qrot(t.rot, {p.x * t.scale.x, p.y * t.scale.y, p.z * t.scale.z})); }
Vec3 to_local(const Transform& t, Vec3 p) {
    const Vec3 v = qrot(qconj(t.rot), sub(p, t.pos));
    return {v.x / (t.scale.x != 0 ? t.scale.x : 1), v.y / (t.scale.y != 0 ? t.scale.y : 1), v.z / (t.scale.z != 0 ? t.scale.z : 1)};
}
double angle_deg(Vec3 a, Vec3 b) { double c = dot(norm(a), norm(b)); c = c > 1 ? 1 : c < -1 ? -1 : c; return std::acos(c) * 180.0 / 3.14159265358979323846; }

// ---- the game ----
bool world_of(API::UObject* c, Transform& out) { return ue::alive(c) && ue::call_ret(c, L"K2_GetComponentToWorld", {}, out); }
bool set_world(API::UObject* c, const Transform& t) { return ue::call(c, L"K2_SetWorldTransform", {t, false}); }   // no sweep; ue::call fills parameters in order, so the hit result and teleport after it stay zero
bool hand(controllers::Side s, Transform& out) { return world_of(controllers::find(s), out); }
// where the gun's front is held: the DRAWN left hand (the arm mesh's hand bone), so the gun points where the player sees
// the hand; on the simulator the drawn hand sat well above the controller point (2026-10-31); the controller if no arms
Vec3 front_hand(const Transform& l) {
    Vec3 v;
    // not while rack.cpp pins the drawn hand to the gun's slide: aiming at a point on the gun itself would chase itself
    return (settings::TWO_HAND_AIM_AT_DRAWN_HAND && !rack::active() && ik::hand_world(controllers::LEFT, v)) ? v : l.pos;
}

int g_logs = 0;
void say(const char* line) {
    if (g_logs >= settings::TWO_HAND_LOG_MAX) return;
    ++g_logs;
    LOGI("%s twohand: %s", TAG, line);
}

// the long gun
bool g_gun = false;                // two-handed now
bool g_gun_capture = false;        // started this frame: capture at the next view, once UEVR has placed the gun
API::UObject* g_gun_mesh = nullptr;
Vec3 g_barrel_local{};             // the hands' line at the start, in the gun mesh's own frame
Vec3 g_grip_local{};               // the right hand at the start, in the gun mesh's own frame
Transform g_gun_written{};
bool g_gun_have_written = false;
int g_gun_writes = 0;
Transform g_gun_rel{};             // the gun's place in the right hand before two hands
Vec3 g_barrel_ctrl{};              // the hands' line at the start, in the RIGHT controller's frame (the whole turn for the right hand)
Vec3 g_aim_off{};                  // the drawn left hand at the start, in the LEFT controller's frame (aim there, not at the drawn hand once it is locked)
Quat g_left_rel{};                 // the drawn left hand's turn against the gun at the grab (TWO_HAND_LOCK_TURN)
bool g_left_rel_due = false;       // take it on the first write after the grab
bool g_gun_is_shotgun = false;     // the hand slides with the pump only on the shotgun
double g_front_cm = 0;             // the right hand to the drawn left hand at the start: the left hand stays this far up the gun
bool g_gun_have_rel = false;

// the map
API::UObject* g_map_mesh = nullptr;
Transform g_map_written{};
bool g_map_have_written = false;
bool g_map_two = false;
bool g_map_have_rel = false;
Vec3 g_map_rel_pos{};
Quat g_map_rel_rot{};
int g_map_writes = 0;

bool g_lg_was = false;
float g_report_in = 0.0f;

bool is_long_gun(API::UObject* w) {
    if (!ue::alive(w)) return false;
    const std::string n = ue::name_of((API::UObject*)w->get_class());
    return n.find("Shotgun") != std::string::npos || n.find("Rifle") != std::string::npos;
}

// the map being looked at: pawn.Items.ItemExecutive (named Map...) . ItemContext . Mesh, as the pack's main.lua finds it
API::UObject* map_mesh() {
    auto** items = ue::prop<API::UObject*>(gun::pawn(), L"Items");
    if (items == nullptr || !ue::alive(*items)) return nullptr;
    auto** exec = ue::prop<API::UObject*>(*items, L"ItemExecutive");
    if (exec == nullptr || !ue::alive(*exec)) return nullptr;
    const std::string en = ue::name_of(*exec);
    if (en.compare(0, 4, "Map_") != 0) return nullptr;   // its class: "Map_ItemInvestigationExe_BP_C /Game/..." (read live 2026-10-31)
    auto** ctx = ue::prop<API::UObject*>(*exec, L"ItemContext");
    if (ctx == nullptr || !ue::alive(*ctx)) return nullptr;
    auto** m = ue::prop<API::UObject*>(*ctx, L"Mesh");
    return (m != nullptr && ue::alive(*m)) ? *m : nullptr;
}

} // namespace

bool gun_two_handed() { return g_gun; }

void tick(float delta) {
    if (!settings::TWO_HAND_ON) return;
    const bool lg = holsters::left_grip();
    const bool lg_pressed = lg && !g_lg_was;
    g_lg_was = lg;
    char line[220];

    // ---- the long gun ----
    API::UObject* w = gun::equipped();
    const bool long_gun = is_long_gun(w) && !buttons::menu_open() && !holsters::map_held();
    if (g_gun && (!lg || !long_gun || gun::mesh(w) != g_gun_mesh)) {
        // put the gun back where it sat in the right hand: the pack places it on the hand once, not every frame, so our turn
        // would stay (Tefa's first wear, 2026-10-31: "the weapon stays at the angle it was left")
        const bool back = g_gun_have_rel && ue::alive(g_gun_mesh) && ue::call(g_gun_mesh, L"K2_SetRelativeTransform", {g_gun_rel, false});
        std::snprintf(line, sizeof line, "%s; its place in the right hand %s", !lg ? "left grip let go: the long gun back in the right hand alone" : "the long gun went away: two hands off",
                      back ? "put back" : g_gun_have_rel ? "could NOT be put back" : "was never read");
        say(line);
        ik::override_left(false, {}, {});
        ik::override_right_turn(false, {});
        g_gun = g_gun_capture = g_gun_have_written = g_gun_have_rel = false;
        g_left_rel_due = false;
        g_gun_mesh = nullptr;
    }
    if (lg_pressed && long_gun && !g_gun) {
        API::UObject* m = gun::mesh(w);
        Transform mw, r, l;
        if (world_of(m, mw) && hand(controllers::RIGHT, r) && hand(controllers::LEFT, l)) {
            const Vec3 line_rl = sub(front_hand(l), r.pos);
            const double d = len(line_rl);
            const Vec3 barrel = qrot(mw.rot, {settings::TWO_HAND_BARREL_AXIS[0], settings::TWO_HAND_BARREL_AXIS[1], settings::TWO_HAND_BARREL_AXIS[2]});
            const double a = angle_deg(barrel, line_rl);
            const bool ok = d >= settings::TWO_HAND_MIN_CM && d <= settings::TWO_HAND_MAX_CM && a <= settings::TWO_HAND_CONE_DEG;
            std::snprintf(line, sizeof line, "left grip pressed with the %s out: the left hand %.0f cm from the right, %.0f deg off the barrel -> %s",
                          ue::name_of((API::UObject*)w->get_class()).c_str(), d, a, ok ? "TWO HANDS" : "not on the gun's front");
            say(line);
            const Vec3 ax[6] = {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};
            std::snprintf(line, sizeof line, "  the hands' line against the gun mesh's axes: +X %.0f -X %.0f +Y %.0f -Y %.0f +Z %.0f -Z %.0f deg",
                          angle_deg(qrot(mw.rot, ax[0]), line_rl), angle_deg(qrot(mw.rot, ax[1]), line_rl), angle_deg(qrot(mw.rot, ax[2]), line_rl),
                          angle_deg(qrot(mw.rot, ax[3]), line_rl), angle_deg(qrot(mw.rot, ax[4]), line_rl), angle_deg(qrot(mw.rot, ax[5]), line_rl));
            say(line);
            if (ok) {
                g_gun = true; g_gun_capture = true; g_gun_mesh = m; g_gun_have_written = false;
                g_gun_have_rel = ue::call_ret(m, L"GetRelativeTransform", {}, g_gun_rel);   // its place in the right hand, for the let-go
                gun::buzz(controllers::LEFT, settings::BUZZ_GRAB_AMP, settings::BUZZ_S);
            }
        }
    }

    // ---- the map ----
    API::UObject* mm = holsters::map_held() ? map_mesh() : nullptr;
    if (mm != g_map_mesh) {
        std::snprintf(line, sizeof line, "the map %s (%s)", mm != nullptr ? "is in the left hand" : "is put away", mm != nullptr ? ue::name_of(mm).c_str() : "-");
        say(line);
        g_map_mesh = mm;
        g_map_have_written = false;
        g_map_have_rel = false;
    }
    const bool two = mm != nullptr && holsters::right_grip();
    if (two != g_map_two && mm != nullptr) {
        say(two ? "right grip held: the map in both hands" : "right grip let go: the map in the left hand alone");
        gun::buzz(controllers::RIGHT, settings::BUZZ_GRAB_AMP, settings::BUZZ_S);
    }
    g_map_two = two;

    // proof of effect, every few seconds while something is held: where the gun points against the left hand
    g_report_in -= delta;
    if ((g_gun || g_map_mesh != nullptr) && g_report_in <= 0.0f) {
        g_report_in = settings::TWO_HAND_REPORT_S;
        if (g_gun) {
            Transform mw, l;
            if (world_of(g_gun_mesh, mw) && hand(controllers::LEFT, l)) {
                const Vec3 grip = to_world(mw, g_grip_local);
                std::snprintf(line, sizeof line, "status: the gun's start line is %.1f deg off the left hand (0 = following it), %d writes",
                              angle_deg(qrot(mw.rot, g_barrel_local), sub(add(l.pos, qrot(l.rot, g_aim_off)), grip)), g_gun_writes);
                LOGI("%s twohand: %s", TAG, line);
            }
        }
        if (g_map_mesh != nullptr) LOGI("%s twohand: status: the map %s, %d writes", TAG, g_map_two ? "in both hands" : "in the left hand", g_map_writes);
    }
}

void on_view() {
    if (!settings::TWO_HAND_ON) return;

    if (g_gun && g_gun_mesh != nullptr) {
        Transform mw, r, l;
        if (world_of(g_gun_mesh, mw) && hand(controllers::RIGHT, r) && hand(controllers::LEFT, l)) {
            if (g_gun_capture) {   // the gun where UEVR put it, the hands now: remember the line and the grip in the gun's frame
                const Vec3 drawn = front_hand(l);
                g_barrel_local = norm(qrot(qconj(mw.rot), sub(drawn, r.pos)));
                g_barrel_ctrl = norm(qrot(qconj(r.rot), sub(drawn, r.pos)));
                g_aim_off = qrot(qconj(l.rot), sub(drawn, l.pos));
                g_front_cm = len(sub(drawn, r.pos));
                g_grip_local = to_local(mw, r.pos);
                g_gun_capture = false;
                g_left_rel_due = true;
                g_gun_is_shotgun = ue::name_of(g_gun_mesh).find("Shotgun") != std::string::npos || ue::name_of(gun::equipped()).find("Shotgun") != std::string::npos;
            }
            // aimed at the target, not added to the last write: run again on a gun already turned, it turns no further
            const Vec3 aim = add(l.pos, qrot(l.rot, g_aim_off));
            const Vec3 grip = to_world(mw, g_grip_local);
            const Quat q = from_to(qrot(mw.rot, g_barrel_local), sub(aim, grip));
            Transform t = mw;
            t.rot = qnorm(qmul(q, mw.rot));
            t.pos = add(grip, qrot(q, sub(mw.pos, grip)));
            if (set_world(g_gun_mesh, t)) { g_gun_written = t; g_gun_have_written = true; ++g_gun_writes; }
            // the hands follow the gun (Tefa's second wear, 2026-10-31: "right hand doesn't turn with the long weapon" and
            // the left hand "moves around the weapon freely forward and back"): the right hand turns by the whole turn
            // from where the gun sat in it; the drawn left hand stays on the gun's line at its starting distance
            if (settings::TWO_HAND_TURN_RIGHT) ik::override_right_turn(true, from_to(qrot(r.rot, g_barrel_ctrl), sub(aim, r.pos)));
            if (settings::TWO_HAND_LOCK_LEFT) {
                // where along the gun: the grab's distance; on the shotgun the hand follows the controller back by up to the
                // pump's slide (the pack's pump reads the controller's own pull, so the drawn hand only shows it)
                const double reach = len(sub(aim, grip));
                double at = g_front_cm;
                if (g_gun_is_shotgun) at = std::fmax(g_front_cm - settings::TWO_HAND_PUMP_SLIDE_CM, std::fmin(g_front_cm, reach));
                const Vec3 where = add(grip, mul(norm(sub(aim, grip)), at));
                if (g_left_rel_due) { g_left_rel = qnorm(qmul(qconj(t.rot), l.rot)); g_left_rel_due = false; }
                Vec3 lp; ue::Rot lr;
                if (settings::TWO_HAND_LOCK_TURN) {
                    Transform ht;
                    ht.rot = qnorm(qmul(t.rot, g_left_rel));   // the gun's turn, then the hand's own turn against it from the grab
                    ik::override_left(true, where, gun::rot_of(ht));
                } else if (gun::controller(controllers::LEFT, lp, lr)) ik::override_left(true, where, lr);
            }
        }
    }

    if (g_map_mesh != nullptr) {
        Transform mw, r, l;
        if (world_of(g_map_mesh, mw) && hand(controllers::RIGHT, r) && hand(controllers::LEFT, l)) {
            if (!g_map_have_rel) {   // the pack's right-hand hold, once, before our first write: the map's place in the right hand
                g_map_rel_pos = qrot(qconj(r.rot), sub(mw.pos, r.pos));
                g_map_rel_rot = qmul(qconj(r.rot), mw.rot);
                g_map_have_rel = true;
            }
            Transform t = mw;
            if (g_map_two) {   // praydog's two-handed map: the middle of the hands, X left -> right, Z against the hands' up, tipped
                const Vec3 x = norm(sub(r.pos, l.pos));
                const Vec3 up = norm(add(qrot(l.rot, {0, 0, 1}), qrot(r.rot, {0, 0, 1})));
                const Vec3 zw = mul(up, -1.0);
                const Vec3 y = norm(cross(zw, x));
                const Vec3 z = cross(x, y);
                t.rot = qnorm(qmul(from_axes(x, y, z), roll_quat(settings::MAP_TWO_HAND_TILT_DEG)));
                t.pos = mul(add(r.pos, l.pos), 0.5);
            } else {           // the left hand alone: the pack's right-hand hold moved onto the left controller, its side mirrored
                // (only the place: mirroring the turn too showed the map upside down on the simulator, 2026-10-31)
                Vec3 rel_pos = g_map_rel_pos;
                const Quat rel_rot = g_map_rel_rot;
                rel_pos.y = -rel_pos.y;
                t.rot = qnorm(qmul(l.rot, rel_rot));
                t.pos = add(l.pos, qrot(l.rot, rel_pos));
            }
            if (set_world(g_map_mesh, t)) { g_map_written = t; g_map_have_written = true; ++g_map_writes; }
        }
    }
}

} // namespace sh2vr::twohand
