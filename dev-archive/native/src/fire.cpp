// fire.cpp -- see fire.h
//
// Pass 1 (2026-10-24, /pd, the game's code read offline; modding-notes/2026-10-24d-the-fire-gate.md): RequestManualFire
// drops its weapon argument and runs the shared fire request, which returns at once when the byte IsInRangedAim() reads
// (this+0x118) is 0; then it asks the weapon's VIRTUAL IsFireReady (vtable slot 0x718, never the UFunction Lukas and F11
// hooked); then it calls the weapon's own fire (vtable slot 0x750, bool manual = 1), which checks rounds (+0xBB4) and
// shoots, with no look at the aim state.
// Pass 2 (2026-10-24 16:30, live on the OpenXR simulator, modding-notes/2026-10-24f-...): door A (the game's request
// with both gates held open) still fires nothing; door B (the weapon's own fire, called straight) fires a real round
// with the gun lowered: the pistol's rounds went 9 -> 8 -> 7, its OnFirePerformed and OnShellEjected ran
// [verified-live 2026-10-24, n=3, flat on the simulator]. So door B is the trigger now: the right trigger with a ranged
// weapon out and no aim stance fires it (and the press is kept from the game, which would swap to the melee weapon).
// In the aim stance the game keeps its own trigger. Keys stay for probes: F9 door A, F8 door B, F11 force-ready.
// The shot counter hooks the weapon's OWN class (the Blueprint overrides OnFirePerformed; a hook on the base class's
// UFunction never saw the pistol's shots, which hid door B's success for one run).
#include "fire.h"

#include <set>
#include <string>

#include "ammo.h"
#include "rack.h"
#include "reload.h"
#include "settings.h"
#include "ue.h"

namespace sh2vr::fire {

namespace {
int g_shots_performed = 0;          // counted by the hooks on <weapon class>::OnFirePerformed
int g_shots_at_probe = 0;
float g_report_in = -1.0f;          // seconds until the after-probe report, < 0 = none pending
const char* g_probe_name = "";
bool g_installed = false;
bool g_force_ready = false;         // F11: answer "yes" to the game's own "is the gun ready to fire?" (UFunction only; proven not enough)
int g_force_ready_calls = 0;
int g_offsets_checked = 0;          // 0 not yet, 1 agree, -1 disagree (then the doors stay shut)
std::set<API::UClass*> g_hooked_classes;   // weapon classes whose OnFirePerformed / OnShellEjected are hooked
bool g_rt_was = false;              // the right trigger last frame (on_input)
bool g_eating_rt = false;           // the press is being kept from the game until it is let go
bool g_fire_queued = false;         // on_input asked for a shot; tick fires it on the game thread
float g_since_shot = 999.0f;        // seconds since our last door-B shot (the game's own cooldown is skipped)
int g_lowered_shots = 0;

// raw reads at the offsets read from this build's code; every use is behind the agree-check below
template <typename T> T& at(API::UObject* o, size_t off) { return *reinterpret_cast<T*>(reinterpret_cast<uint8_t*>(o) + off); }
template <typename Fn> Fn vslot(API::UObject* o, size_t byte_off) { return reinterpret_cast<Fn>((*reinterpret_cast<void***>(o))[byte_off / sizeof(void*)]); }
using IsFireReadyFn = bool (*)(API::UObject*);
using FireFn = bool (*)(API::UObject*, bool);

// SHItemWeaponRanged::IsFireReady(): the game's gate before a shot; with the gun lowered it says no. The params
// struct holds only ReturnValue (offset 0), so the answer is written there and the game's own check is skipped.
bool on_is_fire_ready_pre(API::UFunction*, API::UObject*, void* params, void* result) {
    if (!g_force_ready) return true;
    ++g_force_ready_calls;
    if (params != nullptr) *static_cast<uint8_t*>(params) = 1;
    if (result != nullptr) *static_cast<uint8_t*>(result) = 1;
    return false;
}
void on_is_fire_ready_post(API::UFunction*, API::UObject*, void* params, void*) {
    if (g_force_ready && params != nullptr) *static_cast<uint8_t*>(params) = 1;
}

// <weapon class>::OnFirePerformed(FVector ShootEndWorldLoc): the game calls it for every real shot
bool on_fire_performed_pre(API::UFunction*, API::UObject* weapon, void*, void*) {
    ++g_shots_performed;
    LOGI("%s fire: shot performed #%d by %s", TAG, g_shots_performed, ue::name_of(weapon).c_str());
    ammo::check(weapon, "shot, before");
    return true;   // let it run
}
void on_fire_performed_post(API::UFunction*, API::UObject* weapon, void*, void*) { ammo::check(weapon, "shot, after"); }
bool on_shell_ejected_pre(API::UFunction*, API::UObject* weapon, void*, void*) { LOGI("%s fire: shell ejected by %s", TAG, ue::name_of(weapon).c_str()); return true; }

// the Blueprint weapon classes override OnFirePerformed, so the hook goes on the class of the weapon in hand, once each
void hook_weapon_class(API::UObject* weapon) {
    auto* cls = weapon != nullptr ? weapon->get_class() : nullptr;
    if (cls == nullptr || g_hooked_classes.count(cls) != 0) return;
    g_hooked_classes.insert(cls);
    auto* fired = cls->find_function(L"OnFirePerformed");
    auto* shell = cls->find_function(L"OnShellEjected");
    const bool ok = fired != nullptr && fired->hook_ptr(on_fire_performed_pre, on_fire_performed_post);
    const bool ok2 = shell != nullptr && shell->hook_ptr(on_shell_ejected_pre, nullptr);
    LOGI("%s fire: hooks on %s: OnFirePerformed %s, OnShellEjected %s", TAG, ue::name_of((API::UObject*)cls).c_str(), ok ? "set" : "FAILED", ok2 ? "set" : "FAILED");
}

API::UObject* combat_of(API::UObject* pawn) {
    auto** c = ue::prop<API::UObject*>(pawn, L"Combat");
    return c != nullptr ? *c : nullptr;
}
API::UObject* ranged_subcomp(API::UObject* combat) {
    auto* cls = ue::find_class(L"Class /Script/SHProto.SHRangedCmbSubcomp");
    API::UObject* sub = nullptr;
    if (combat == nullptr || cls == nullptr || !ue::call_ret(combat, L"FindSubcomponentByClass", {(API::UObject*)cls}, sub)) return nullptr;
    return sub;
}
std::string state_line(API::UObject* combat, API::UObject* sub, API::UObject* weapon) {
    uint8_t mode = 255, pose = 255, in_aim = 2, ammo_any = 2, ready = 2;
    ue::call_ret(combat, L"GetCombatInputMode", {}, mode);
    ue::call_ret(combat, L"GetCombatPose", {}, pose);
    ue::call_ret(sub, L"IsInRangedAim", {}, in_aim);
    if (weapon != nullptr) { ue::call_ret(weapon, L"HasAnyAmmoInClip", {}, ammo_any); ue::call_ret(weapon, L"IsFireReady", {}, ready); }
    char buf[256];
    snprintf(buf, sizeof buf, "input mode %d, pose %d, in ranged aim %d, rounds %d, ammo in clip %d, fire ready %d, shots so far %d", mode, pose, in_aim, ammo::rounds(weapon), ammo_any, ready, g_shots_performed);
    return buf;
}

// the offsets prove themselves: the game's own UFunction answers must equal what the raw reads say
bool offsets_agree(API::UObject* sub, API::UObject* weapon) {
    uint8_t in_aim_fn = 2, ready_fn = 2;
    if (!ue::call_ret(sub, L"IsInRangedAim", {}, in_aim_fn) || !ue::call_ret(weapon, L"IsFireReady", {}, ready_fn)) { LOGW("%s fire: offset check could not call IsInRangedAim / IsFireReady", TAG); return false; }
    const uint8_t in_aim_raw = at<uint8_t>(sub, settings::RANGED_IN_AIM_FLAG);
    const bool ready_virtual = vslot<IsFireReadyFn>(weapon, settings::WEAPON_VSLOT_IS_FIRE_READY)(weapon);
    const float threshold = at<float>(weapon, settings::WEAPON_FIRE_READY_MIN);
    const bool ok = (in_aim_fn != 0) == (in_aim_raw != 0) && (ready_fn != 0) == ready_virtual;
    LOGI("%s fire: offset check: IsInRangedAim() %d vs byte +0x%zX %d; IsFireReady() %d vs virtual slot 0x%zX %d; ready threshold +0x%zX = %.3f -> %s", TAG,
         in_aim_fn, settings::RANGED_IN_AIM_FLAG, in_aim_raw, ready_fn, settings::WEAPON_VSLOT_IS_FIRE_READY, ready_virtual ? 1 : 0, settings::WEAPON_FIRE_READY_MIN, threshold,
         ok ? "AGREE, the doors are open" : "DISAGREE, this build's layout differs, the doors stay shut");
    g_offsets_checked = ok ? 1 : -1;
    return ok;
}

struct Parts { API::UObject* pawn = nullptr, *combat = nullptr, *sub = nullptr, *weapon = nullptr; };
bool find_parts(Parts& p, const char* who, bool loud = true) {
    p.pawn = API::get()->get_local_pawn(0);
    p.combat = combat_of(p.pawn);
    p.sub = ranged_subcomp(p.combat);
    if (p.sub == nullptr) { if (loud) LOGW("%s fire %s: no ranged combat part on the pawn (pawn %s, combat %s)", TAG, who, ue::name_of(p.pawn).c_str(), ue::name_of(p.combat).c_str()); return false; }
    ue::call_ret(p.sub, L"GetEquippedRangedWeapon", {}, p.weapon);
    if (loud) LOGI("%s fire %s: before: weapon %s; %s", TAG, who, ue::name_of(p.weapon).c_str(), state_line(p.combat, p.sub, p.weapon).c_str());
    if (p.weapon == nullptr) { if (loud) LOGW("%s fire %s: no ranged weapon equipped, nothing to fire", TAG, who); return false; }
    hook_weapon_class(p.weapon);
    return true;
}
void report_after(const char* who) {
    g_shots_at_probe = g_shots_performed;
    g_probe_name = who;
    g_report_in = settings::FIRE_PROBE_REPORT_S;
}

// F9, door A: the game's own request with its two gates held open for the length of the call (fires nothing, kept as the control)
void probe_through_gate() {
    Parts p;
    if (!find_parts(p, "door A (gate forced)")) return;
    if (!offsets_agree(p.sub, p.weapon)) { LOGW("%s fire door A: plain RequestManualFire instead (the old probe)", TAG); ue::call(p.sub, L"RequestManualFire", {p.weapon}); report_after("door A, plain"); return; }
    uint8_t& in_aim = at<uint8_t>(p.sub, settings::RANGED_IN_AIM_FLAG);
    float& threshold = at<float>(p.weapon, settings::WEAPON_FIRE_READY_MIN);
    const uint8_t saved_aim = in_aim;
    const float saved_threshold = threshold;
    in_aim = 1;
    threshold = 0.0f;
    const bool ok = ue::call(p.sub, L"RequestManualFire", {p.weapon});
    in_aim = saved_aim;
    threshold = saved_threshold;
    LOGI("%s fire door A: RequestManualFire(weapon) %s with aim byte 1 and threshold 0 (restored to %d / %.3f); right after: %s", TAG, ok ? "called" : "REFUSED", saved_aim, saved_threshold, state_line(p.combat, p.sub, p.weapon).c_str());
    report_after("door A");
}

// door B: the weapon's own fire, the step the combat part takes after its gates. `who` names the caller in the log.
bool fire_direct(const char* who, bool loud) {
    Parts p;
    if (!find_parts(p, who, loud)) return false;
    if (g_offsets_checked != 1 && !offsets_agree(p.sub, p.weapon)) return false;
    const bool fired = vslot<FireFn>(p.weapon, settings::WEAPON_VSLOT_FIRE)(p.weapon, true);
    if (loud) LOGI("%s fire %s: weapon fire (slot 0x%zX, manual) returned %d; right after: %s", TAG, who, settings::WEAPON_VSLOT_FIRE, fired ? 1 : 0, state_line(p.combat, p.sub, p.weapon).c_str());
    return fired;
}
void probe_direct() { if (fire_direct("door B (weapon fire direct)", true)) report_after("door B"); }

// the trigger: a ranged weapon out, no aim stance (the game keeps its own trigger while aiming)
bool lowered_gun_out(Parts& p) {
    if (!find_parts(p, "trigger", false)) return false;
    uint8_t in_aim = 2;
    ue::call_ret(p.sub, L"IsInRangedAim", {}, in_aim);
    return in_aim == 0;
}
} // namespace

void install() {
    if (g_installed) return;
    auto* cls = ue::find_class(L"Class /Script/SHProto.SHItemWeaponRanged");
    auto* ready = cls != nullptr ? cls->find_function(L"IsFireReady") : nullptr;
    const bool ok2 = ready != nullptr && ready->hook_ptr(on_is_fire_ready_pre, on_is_fire_ready_post);
    LOGI("%s fire: hook on IsFireReady %s; the shot hooks go on each weapon's own class when it is first in hand; keys: F9 door A (gates forced), F8 door B (weapon fire direct), F11 force-ready toggle; the right trigger fires a lowered gun: %s", TAG,
         ok2 ? "set" : "FAILED", settings::FIRE_LOWERED_ON ? "ON" : "off");
    g_installed = true;
}

void on_message(UINT msg, WPARAM wparam) {
    if (msg != WM_KEYDOWN && msg != WM_SYSKEYDOWN) return;   // F10 and friends arrive as system keys
    if (wparam == settings::FIRE_PROBE_KEY) probe_through_gate();
    if (wparam == settings::FIRE_DIRECT_KEY) probe_direct();
    if (wparam == settings::FIRE_TRIGGER_TEST_KEY) {   // a pretend pull and release through on_input, so the trigger path can be tried flat
        XINPUT_STATE fake{};
        fake.Gamepad.bRightTrigger = 255;
        on_input(fake);
        LOGI("%s fire: F7 pretend trigger: the game would have seen trigger %d (0 = kept from it); shot queued %s", TAG, fake.Gamepad.bRightTrigger, g_fire_queued ? "yes" : "no");
        fake.Gamepad.bRightTrigger = 0;
        on_input(fake);
    }
    if (wparam == settings::FIRE_FORCE_READY_KEY) {
        g_force_ready = !g_force_ready;
        LOGI("%s fire: force-ready %s (the game asked %d times while it was on)", TAG, g_force_ready ? "ON" : "OFF", g_force_ready_calls);
    }
}

// the right trigger, read before the game sees it: with a ranged weapon out and no aim stance, a fresh press queues
// a door-B shot and the press is kept from the game (which would otherwise swap to the melee weapon); while aiming,
// the game keeps its trigger untouched
void on_input(XINPUT_STATE& state) {
    if (!settings::FIRE_LOWERED_ON || !g_installed) return;
    if (reload::managed()) state.Gamepad.wButtons &= (WORD)~XINPUT_GAMEPAD_X;   // the game's own reload stays out while a pistol reloads by hand
    const bool rt = state.Gamepad.bRightTrigger > settings::TRIGGER_PRESSED_OVER;
    const bool edge = rt && !g_rt_was;
    g_rt_was = rt;
    if (rt && (rack::needed() || reload::blocks_fire())) { state.Gamepad.bRightTrigger = 0; g_eating_rt = false; return; }   // the trigger waits for the rack / the magazine
    if (!rt) { g_eating_rt = false; return; }
    if (edge) {
        Parts p;
        if (!lowered_gun_out(p)) { g_eating_rt = false; return; }   // aiming, or no gun: the game's own trigger
        g_eating_rt = true;
        if (g_since_shot >= settings::FIRE_MIN_INTERVAL_S) g_fire_queued = true;
    }
    if (g_eating_rt) state.Gamepad.bRightTrigger = 0;
}

void tick(float delta) {
    g_since_shot += delta;
    if (g_fire_queued) {   // on the game thread, where the game itself fires from
        g_fire_queued = false;
        g_since_shot = 0.0f;
        const bool fired = fire_direct("trigger (lowered)", g_lowered_shots < settings::FIRE_LOWERED_LOG_FIRST);
        if (fired) ++g_lowered_shots;
        if (g_lowered_shots <= settings::FIRE_LOWERED_LOG_FIRST) LOGI("%s fire: trigger with the gun lowered: %s (lowered shot #%d)", TAG, fired ? "FIRED" : "nothing", g_lowered_shots);
    }
    if (g_report_in < 0.0f) return;
    g_report_in -= delta;
    if (g_report_in > 0.0f) return;
    g_report_in = -1.0f;
    API::UObject* combat = combat_of(API::get()->get_local_pawn(0));
    API::UObject* sub = ranged_subcomp(combat);
    API::UObject* weapon = nullptr;
    if (sub != nullptr) ue::call_ret(sub, L"GetEquippedRangedWeapon", {}, weapon);
    LOGI("%s fire %s: %.1f s later: %d shot(s) performed since; %s", TAG, g_probe_name, settings::FIRE_PROBE_REPORT_S, g_shots_performed - g_shots_at_probe,
         sub != nullptr ? state_line(combat, sub, weapon).c_str() : "no ranged combat part");
}

} // namespace sh2vr::fire
