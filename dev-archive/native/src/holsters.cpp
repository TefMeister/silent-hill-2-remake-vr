// holsters.cpp -- the weapons live on the body (2026-10-29, Fable; Tefa's "full VR button and body layout",
// mod-ideas games/silent-hill-2-remake.md, and today: "right grip: hold a weapon; let go and it goes back to its holster").
//
// Spots, all measured from the HEADSET (the standing rule: never on the body's joints), turned with the head's yaw
// only, never its tilt:
//   right hip      -> right grip: the handgun
//   right shoulder -> right grip: the shotgun
//   left shoulder  -> right grip: the rifle
//   left hip       -> right grip: the melee weapon (plank, pipe, chainsaw: whatever the game has equipped as melee)
// The left-grip items of the same spots (map, flashlight, syringe, drink) are later pieces; the ammo pouch stays where
// reload.cpp has it (left hip) until Tefa says where it goes.
//
// How it works, every frame on the game thread:
//   - the right hand inside a spot: one short buzz on the way in;
//   - the right grip PRESSED (the real controller, input.h) inside a spot with the hands empty: the spot's weapon is
//     equipped through the game's own weapon manager (SHWeaponManageCmbSubcomp.RequestEquipWeapon(row, Instant),
//     proven live 2026-10-24 for "Pistol"), and the layer remembers "holding";
//   - the right grip LET GO while holding: the weapon goes back (the game's own unequip; the function's name is a
//     setting, read live 2026-10-29), the hands are empty again;
//   - at the start, and after a load, a weapon the game put in the hand with no grip held is put back once, so the
//     hands begin empty (logged).
// A desk test: F6 sent to the game window is a pretend right grip (pressed / let go), since the simulator has no grip;
// the simulator's right controller is moved into a spot first. Proved on the simulator 2026-10-29: the hand in the right
// hip spot (0 cm) + F6 drew the Pistol, F6 again put it back; the left hip drew the IronPipe and put it back
// [verified-live 2026-10-29, simulator, n=1 each]. Not yet: the real grip, the headset, the hands beginning empty path.

#include "holsters.h"

#include <cmath>
#include <cstdio>
#include <string>
#include <string_view>

#include "common.h"
#include "gun.h"
#include "input.h"
#include "settings.h"
#include "ue.h"

namespace sh2vr::holsters {


namespace {

struct Spot {
    const settings::HolsterSpot* def = nullptr;
    bool hand_in = false;
};
Spot g_spots[settings::HOLSTER_SPOT_COUNT];
bool g_holding = false;            // the right grip holds a weapon drawn from a spot
int g_holding_spot = -1;
bool g_started = false;            // the first-frame "hands begin empty" done for this pawn
API::UObject* g_pawn_seen = nullptr;
bool g_pretend_grip = false;       // F6
bool g_grip_was = false;
int g_logs = 0;
float g_status_in = 0.0f;

void say(const char* line) {
    if (g_logs >= settings::HOLSTER_LOG_MAX) return;
    ++g_logs;
    LOGI("%s holsters: %s", TAG, line);
}

// Everything here is measured in the HEADSET'S OWN TRACKING SPACE (UEVR's poses: metres, +X right, +Y up, -Z
// forward), not in the game's world: on the simulator the game's camera manager did not follow the headset (the
// camera location sat 1.5 m from the controllers, its rotation read 0,0,0) [measured 2026-10-29], and a spot built
// from it was never where the hand was. Head and hands from the same source agree everywhere.
struct TV { double x, y, z; };
TV rot_q(const UEVR_Quaternionf& q, TV v) {   // v turned by q: v + 2 * (q.xyz x (q.xyz x v + w v))
    const TV u{q.x, q.y, q.z};
    const TV t{u.y * v.z - u.z * v.y + q.w * v.x, u.z * v.x - u.x * v.z + q.w * v.y, u.x * v.y - u.y * v.x + q.w * v.z};
    return {v.x + 2.0 * (u.y * t.z - u.z * t.y), v.y + 2.0 * (u.z * t.x - u.x * t.z), v.z + 2.0 * (u.x * t.y - u.y * t.x)};
}
bool head_pose(TV& pos, UEVR_Quaternionf& rot) {
    const auto* vr = API::get()->param()->vr;
    if (vr == nullptr || !vr->is_hmd_active()) return false;
    UEVR_Vector3f p{};
    vr->get_pose(0, &p, &rot);
    pos = {p.x, p.y, p.z};
    return true;
}
bool right_hand(TV& pos) {
    const auto* vr = API::get()->param()->vr;
    if (vr == nullptr || !vr->is_hmd_active()) return false;
    UEVR_Vector3f p{};
    UEVR_Quaternionf q{};
    vr->get_grip_pose(vr->get_right_controller_index(), &p, &q);
    pos = {p.x, p.y, p.z};
    return true;
}
// the spot now, in tracking metres: from the head, along its yaw-only right / forward, and straight up
bool spot_tracking(const settings::HolsterSpot& s, TV& out) {
    TV h;
    UEVR_Quaternionf q;
    if (!head_pose(h, q)) return false;
    TV f = rot_q(q, {0, 0, -1});
    f.y = 0;
    const double len = std::sqrt(f.x * f.x + f.z * f.z);
    if (len < 1e-6) f = {0, 0, -1}; else { f.x /= len; f.z /= len; }
    const TV right{-f.z, 0, f.x};
    const double cm = 0.01;
    out = {h.x + right.x * s.right_cm * cm + f.x * s.forward_cm * cm,
           h.y + s.up_cm * cm,
           h.z + right.z * s.right_cm * cm + f.z * s.forward_cm * cm};
    return true;
}
double dist_cm(TV a, TV b) { return 100.0 * std::sqrt((a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y) + (a.z - b.z) * (a.z - b.z)); }

API::UObject* weapon_manager() {
    auto** c = ue::prop<API::UObject*>(gun::pawn(), L"Combat");
    auto* cls = ue::find_class(settings::WEAPON_MANAGER_CLASS);
    API::UObject* sub = nullptr;
    if (c == nullptr || *c == nullptr || cls == nullptr || !ue::call_ret(*c, L"FindSubcomponentByClass", {(API::UObject*)cls}, sub)) return nullptr;
    return ue::alive(sub) ? sub : nullptr;
}

// the row to equip for a spot: the melee spot takes whatever melee weapon the game last had in hand
// (GetLastUsedMeleeWeapon, "IronPipe" on the prison-armory save, read live 2026-10-29)
bool row_for(API::UObject* wm, const settings::HolsterSpot& s, API::FName& out) {
    if (std::wstring_view(s.row) != settings::MELEE_ROW) { out = ue::name(s.row); return true; }
    API::FName n{};
    if (!ue::call_ret(wm, L"GetLastUsedMeleeWeapon", {}, n) || ue::is_none(n)) return false;
    out = n;
    return true;
}

bool equip(const settings::HolsterSpot& s, std::string& what) {
    API::UObject* wm = weapon_manager();
    if (wm == nullptr) { say("no weapon manager: cannot equip"); return false; }
    API::FName row{};
    if (!row_for(wm, s, row)) { say("the game has no melee weapon to draw"); return false; }
    what = narrow(ue::str(row));
    bool ok = false;
    return ue::call_ret(wm, L"RequestEquipWeapon", {row, true}, ok) && ok;
}

bool unequip() {
    API::UObject* wm = weapon_manager();
    if (wm == nullptr) { say("no weapon manager: cannot put the weapon back"); return false; }
    bool ok = false;
    return ue::call_ret(wm, settings::UNEQUIP_FN, {true}, ok) && ok;   // RequestUnequipWeapon(Instant) [read live 2026-10-29]
}

bool grip_now() { return g_pretend_grip || input::held(XINPUT_GAMEPAD_RIGHT_SHOULDER); }

} // namespace

bool holding() { return g_holding; }

void on_message(UINT msg, WPARAM wparam) {
    if (msg != WM_KEYDOWN && msg != WM_SYSKEYDOWN) return;
    if (wparam == settings::HOLSTER_TEST_KEY) {
        g_pretend_grip = !g_pretend_grip;
        say(g_pretend_grip ? "F6: pretend right grip PRESSED" : "F6: pretend right grip let go");
    }
}

void tick(float delta) {
    if (!settings::HOLSTERS_ON) return;
    API::UObject* pawn = gun::pawn();
    if (pawn == nullptr) { g_started = false; g_pawn_seen = nullptr; g_holding = false; return; }
    if (pawn != g_pawn_seen) { g_pawn_seen = pawn; g_started = false; g_holding = false; g_holding_spot = -1; }
    for (int i = 0; i < settings::HOLSTER_SPOT_COUNT; ++i) g_spots[i].def = &settings::HOLSTER_SPOTS[i];

    const bool grip = grip_now();
    const bool pressed = grip && !g_grip_was;
    const bool released = !grip && g_grip_was;
    g_grip_was = grip;

    // the hands begin empty: a weapon the game put in the hand with no grip held goes back once per pawn
    if (!g_started) {
        g_started = true;
        if (gun::equipped() != nullptr && !grip) {
            say(unequip() ? "start: the game's weapon put back, the hands begin empty" : "start: could not put the game's weapon back");
        }
    }

    TV hand{};
    const bool have_hand = right_hand(hand);
    int in_spot = -1;
    double nearest = 1e9;
    if (have_hand) {
        for (int i = 0; i < settings::HOLSTER_SPOT_COUNT; ++i) {
            TV p;
            if (!spot_tracking(*g_spots[i].def, p)) continue;
            const double d = dist_cm(hand, p);
            nearest = std::fmin(nearest, d);
            const bool was = g_spots[i].hand_in;
            g_spots[i].hand_in = was ? d < settings::HOLSTER_LEAVE_CM : d < settings::HOLSTER_ENTER_CM;
            if (g_spots[i].hand_in && !was && !g_holding) gun::buzz(controllers::RIGHT, settings::BUZZ_GRAB_AMP, settings::BUZZ_S);
            if (g_spots[i].hand_in && in_spot < 0) in_spot = i;
        }
    }

    char line[200];
    if (pressed && !g_holding && in_spot >= 0) {
        const auto& s = *g_spots[in_spot].def;
        std::string what = narrow(s.row);
        const bool ok = equip(s, what);
        if (ok) { g_holding = true; g_holding_spot = in_spot; gun::buzz(controllers::RIGHT, settings::BUZZ_DONE_AMP, settings::BUZZ_S); }
        std::snprintf(line, sizeof line, "right grip at the %s: %s %s", narrow(s.name).c_str(), ok ? "drew the" : "could not draw the", what.c_str());
        say(line);
    } else if (pressed && !g_holding && in_spot < 0 && settings::HOLSTER_LOG_EMPTY_GRIPS) {
        say("right grip with the hand away from every spot: nothing drawn");
    } else if (released && g_holding) {
        const bool ok = unequip();
        std::snprintf(line, sizeof line, "right grip let go: %s (%s)", ok ? "the weapon goes back to its spot" : "could not put the weapon back",
                      g_holding_spot >= 0 ? narrow(g_spots[g_holding_spot].def->name).c_str() : "?");
        say(line);
        g_holding = false;
        g_holding_spot = -1;
        gun::buzz(controllers::RIGHT, settings::BUZZ_DENY_AMP, settings::BUZZ_S);
    }

    g_status_in -= delta;
    if (g_status_in <= 0.0f) {
        g_status_in = settings::HOLSTER_STATUS_S;
        // the weapon manager's own name for what is in hand ("None" when empty); gun::equipped() knows ranged weapons only
        API::FName eq{};
        API::UObject* wm = weapon_manager();
        std::string eq_name = (wm != nullptr && ue::call_ret(wm, L"GetEquippedWeaponName", {}, eq)) ? narrow(ue::str(eq)) : "?";
        std::snprintf(line, sizeof line, "status: holding=%d spot=%s hand in=%s (nearest spot %.0f cm) equipped=%s grip=%d%s", g_holding ? 1 : 0,
                      g_holding_spot >= 0 ? narrow(g_spots[g_holding_spot].def->name).c_str() : "-",
                      in_spot >= 0 ? narrow(g_spots[in_spot].def->name).c_str() : "-", have_hand ? nearest : -1.0,
                      eq_name.c_str(), grip ? 1 : 0, g_pretend_grip ? " (F6)" : "");
        LOGI("%s holsters: %s", TAG, line);
    }
}

} // namespace sh2vr::holsters
