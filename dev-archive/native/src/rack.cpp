// rack.cpp -- see rack.h. Function and state names follow Visceral RE8's rack.cpp where the shape is the same.
#include "rack.h"

#include <cmath>
#include <string>

#include "ammo.h"
#include "ghosthand.h"
#include "gun.h"
#include "ik.h"
#include "ikmath.h"
#include "input.h"
#include "reload.h"
#include "settings.h"

namespace sh2vr::rack {

using namespace sh2vr::ikmath;
using gun::Rot;
using gun::Transform;
using gun::Vec3;

namespace {
API::UObject* g_weapon = nullptr;   // the pistol the state belongs to
bool g_needed = false, g_parked = false, g_hand_on = false, g_armed = false, g_pulled = false;
double g_travel = 0.0, g_base = 0.0, g_max_pull = 0.0;
Vec3 g_axis{1, 0, 0};              // slide -> muzzle, world, frozen when the pull is armed
bool g_open_shown = false;         // the slide drawn open (the game's empty pose) by us
int g_log_near = 0;

double clamp01(double v) { return v < 0.0 ? 0.0 : v > 1.0 ? 1.0 : v; }

void show_open(API::UObject* w, bool open) {
    if (open == g_open_shown) return;
    g_open_shown = open;
    ammo::out_of_ammo_visual(w, open);
}

// the slide socket and the barrel line, this frame
bool slide_world(API::UObject* w, Vec3& slide, Vec3& axis) {
    API::UObject* m = gun::mesh(w);
    Vec3 muzzle;
    if (!gun::socket_loc(m, settings::SLIDE_SOCKET, slide)) return false;
    axis = gun::socket_loc(m, settings::MUZZLE_SOCKET, muzzle) ? safe_normalize(sub(muzzle, slide)) : Vec3{1, 0, 0};
    return true;
}
double along(const Vec3& left) { return dot(left, g_axis); }

void complete(API::UObject* w) {
    const bool was = g_needed;
    g_needed = g_parked = g_pulled = g_armed = false;
    g_travel = 0.0;
    show_open(w, false);
    reload::on_racked();
    gun::buzz(controllers::LEFT, settings::BUZZ_DONE_AMP, settings::BUZZ_S);
    ghosthand::event("slide", "done");
    LOGI("%s rack: slide home (%s), %d rounds in the gun", TAG, was ? "was needed: it fires again" : "cosmetic", ammo::rounds(w));
}

void hand_off(API::UObject* w, const char* why) {
    if (!g_hand_on) return;
    if (g_armed && !g_pulled) { ghosthand::value("slide_max_pull_cm", (float)g_max_pull); ghosthand::event("slide", "cancel"); }
    g_hand_on = g_armed = false;
    ik::override_left(false, Vec3{}, Rot{});
    if (g_pulled) { complete(w); LOGI("%s rack: hand off the slide (%s): it went home", TAG, why); return; }
    LOGI("%s rack: hand off the slide (%s), furthest pull %.1f cm", TAG, why, g_max_pull);
}
} // namespace

bool needed() { return g_needed; }
bool active() { return g_hand_on; }
double travel() { return g_travel; }

void need(const char* why) {
    if (!g_needed) LOGI("%s rack: slide NEEDED (%s): the trigger waits until it is racked", TAG, why);
    g_needed = g_parked = true;
}

void reset(const char* why) {
    if (g_hand_on) ik::override_left(false, Vec3{}, Rot{});
    if (g_needed || g_hand_on) LOGI("%s rack: reset (%s)", TAG, why);
    g_needed = g_parked = g_hand_on = g_armed = g_pulled = false;
    g_travel = g_base = g_max_pull = 0.0;
    g_open_shown = false;
    g_weapon = nullptr;
}

void tick(float) {
    API::UObject* w = gun::equipped();
    if (!gun::is_pistol(w)) { if (g_weapon != nullptr) reset("no pistol in hand"); return; }
    if (w != g_weapon) { reset("another pistol"); g_weapon = w; }
    if (!gun::hmd_active()) { hand_off(w, "no headset"); return; }

    Vec3 slide, axis, left;
    Rot left_rot;
    if (!slide_world(w, slide, axis) || !gun::controller(controllers::LEFT, left, left_rot)) { hand_off(w, "no slide socket / no left controller"); return; }
    if (!g_armed) g_axis = axis;

    const int rounds = ammo::rounds(w);
    if (!g_parked && rounds == 0 && !reload::mag_out()) { g_parked = true; LOGI("%s rack: the pistol is empty: its slide can be racked", TAG); }
    const bool lg = input::grip(false), lt = input::left_trigger();
    const bool may_rack = g_parked || g_needed;

    if (!g_hand_on && may_rack && lg && !reload::hand_busy()) {
        const double d = size(sub(left, slide));
        if (g_log_near < 30 && input::grip_pressed(false)) { ++g_log_near; LOGI("%s rack: left grip %.1f cm from the slide (need %.0f): %s", TAG, d, settings::SLIDE_GRAB_CM, d <= settings::SLIDE_GRAB_CM ? "HAND ON" : "too far"); }
        if (d <= settings::SLIDE_GRAB_CM) {
            g_hand_on = true;
            g_armed = g_pulled = false;
            g_max_pull = 0.0;
            gun::buzz(controllers::LEFT, settings::BUZZ_GRAB_AMP, settings::BUZZ_S);
        }
    }
    if (g_hand_on) {
        if (lt && !g_armed) {
            g_armed = true;
            g_axis = axis;
            g_base = along(left) + g_travel * settings::SLIDE_TRAVEL_CM;   // a part-way pull carries on from there
            g_max_pull = 0.0;
            ghosthand::event("slide", "start");
            LOGI("%s rack: slide held with LT: pull it back along the barrel", TAG);
        } else if (!lt && g_armed) {
            g_armed = false;
            if (g_pulled) { complete(w); LOGI("%s rack: LT let go at the back: the slide went home", TAG); }
            else LOGI("%s rack: LT let go: the slide springs back", TAG);
        }
        if (!lg) hand_off(w, g_pulled ? "LG let go: it goes forward" : "LG let go");
    }
    if (g_hand_on && g_armed) {
        const double pulled = g_base - along(left);
        g_max_pull = std::fmax(g_max_pull, pulled);
        const double want = g_pulled ? 1.0 : clamp01(pulled / settings::SLIDE_TRAVEL_CM);
        g_travel += (want - g_travel) * settings::RACK_FOLLOW;
        if (!g_pulled && g_travel >= settings::RACK_PULLED_AT) {
            g_pulled = true;
            gun::buzz(controllers::LEFT, settings::BUZZ_DONE_AMP, settings::BUZZ_S);
            LOGI("%s rack: pulled back (%.1f cm)", TAG, pulled);
        }
    } else if (!g_pulled && g_travel > 0.0) {
        g_travel = std::fmax(0.0, g_travel - settings::RACK_SPRING_STEP);
    }
    // what the slide shows: open while a rack is owed or while it is pulled past half way
    show_open(w, g_needed || g_parked || g_travel > 0.5);

    // the left hand rides the slide: the socket's frame, the pack's hand sliders on top, moved back with the pull
    if (g_hand_on) {
        Transform socket;
        if (gun::socket_xform(gun::mesh(w), settings::SLIDE_SOCKET, socket)) {
            Transform local;
            local.pos = Vec3{settings::SLIDE_HAND_LOC[0], settings::SLIDE_HAND_LOC[1], settings::SLIDE_HAND_LOC[2]};
            local.rot = quat_from_euler(Rot{settings::SLIDE_HAND_ROT[0], settings::SLIDE_HAND_ROT[1], settings::SLIDE_HAND_ROT[2]});
            Transform world;
            API::UObject* kml = ue::cdo(L"Class /Script/Engine.KismetMathLibrary");
            if (ue::call_ret(kml, L"ComposeTransforms", {local, socket}, world)) {
                const Vec3 pos = sub(world.pos, mul(g_axis, g_travel * settings::SLIDE_TRAVEL_CM));
                ik::override_left(true, pos, gun::rot_of(world));
            }
        }
    }
    ghosthand::value("rack", (float)g_travel);
    ghosthand::value("hand_on", g_hand_on ? 1.0f : 0.0f);
}

} // namespace sh2vr::rack
