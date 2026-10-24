// fire.cpp -- see fire.h
//
// Pass 1 (2026-10-24, /pd, the game's code read offline; modding-notes/2026-10-24d-the-fire-gate.md): the gate the live
// probe hit is now known. RequestManualFire(weapon) on SHRangedCmbSubcomp drops its weapon argument and runs the shared
// fire request, which returns at once when the byte IsInRangedAim() reads (this+0x118) is 0; then it asks the weapon's
// VIRTUAL IsFireReady (vtable slot 0x718, never the UFunction Lukas and F11 hooked), which is "the held item's raise
// value >= the weapon's float at +0x690"; then it calls the weapon's own fire (vtable slot 0x750, bool manual = 1),
// which checks rounds (+0xBB4) and shoots, with no look at the aim state. Two doors, both on keys for the next run:
//   F9  door A: force the two gates (aim byte = 1, threshold = 0) around the game's own RequestManualFire, restore after
//   F8  door B: call the weapon's fire straight, skipping the combat part altogether
// Before either, the offsets prove themselves against the game's own answers (UFunction IsInRangedAim == the byte,
// UFunction IsFireReady == the virtual), so a run can tell "wrong offset for this build" from "wrong door".
#include "fire.h"

#include <string>

#include "ammo.h"
#include "settings.h"
#include "ue.h"

namespace sh2vr::fire {

namespace {
int g_shots_performed = 0;          // counted by the hook on SHItemWeaponRanged::OnFirePerformed
int g_shots_at_probe = 0;
float g_report_in = -1.0f;          // seconds until the after-probe report, < 0 = none pending
const char* g_probe_name = "";
bool g_installed = false;
bool g_force_ready = false;         // F11: answer "yes" to the game's own "is the gun ready to fire?" (UFunction only; proven not enough)
int g_force_ready_calls = 0;
int g_offsets_checked = 0;          // 0 not yet, 1 agree, -1 disagree (then the doors stay shut)

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

// SHItemWeaponRanged::OnFirePerformed(FVector ShootEndWorldLoc): the game calls it for every real shot
bool on_fire_performed_pre(API::UFunction*, API::UObject* weapon, void*, void*) {
    ++g_shots_performed;
    LOGI("%s fire: shot performed #%d by %s", TAG, g_shots_performed, ue::name_of(weapon).c_str());
    ammo::check(weapon, "shot, before");
    return true;   // let it run
}
void on_fire_performed_post(API::UFunction*, API::UObject* weapon, void*, void*) { ammo::check(weapon, "shot, after"); }

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
    uint8_t mode = 255, pose = 255, in_aim = 2, ammo = 2, ready = 2;
    ue::call_ret(combat, L"GetCombatInputMode", {}, mode);
    ue::call_ret(combat, L"GetCombatPose", {}, pose);
    ue::call_ret(sub, L"IsInRangedAim", {}, in_aim);
    if (weapon != nullptr) { ue::call_ret(weapon, L"HasAnyAmmoInClip", {}, ammo); ue::call_ret(weapon, L"IsFireReady", {}, ready); }
    char buf[256];
    snprintf(buf, sizeof buf, "input mode %d, pose %d, in ranged aim %d, ammo in clip %d, fire ready %d, shots so far %d", mode, pose, in_aim, ammo, ready, g_shots_performed);
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
bool find_parts(Parts& p, const char* who) {
    p.pawn = API::get()->get_local_pawn(0);
    p.combat = combat_of(p.pawn);
    p.sub = ranged_subcomp(p.combat);
    if (p.sub == nullptr) { LOGW("%s fire %s: no ranged combat part on the pawn (pawn %s, combat %s)", TAG, who, ue::name_of(p.pawn).c_str(), ue::name_of(p.combat).c_str()); return false; }
    ue::call_ret(p.sub, L"GetEquippedRangedWeapon", {}, p.weapon);
    LOGI("%s fire %s: before: weapon %s; %s", TAG, who, ue::name_of(p.weapon).c_str(), state_line(p.combat, p.sub, p.weapon).c_str());
    if (p.weapon == nullptr) { LOGW("%s fire %s: no ranged weapon equipped, nothing to fire", TAG, who); return false; }
    return true;
}
void report_after(const char* who) {
    g_shots_at_probe = g_shots_performed;
    g_probe_name = who;
    g_report_in = settings::FIRE_PROBE_REPORT_S;
}

// F9, door A: the game's own request with its two gates held open for the length of the call
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

// F8, door B: the weapon's own fire, the step the combat part takes after its gates
void probe_direct() {
    Parts p;
    if (!find_parts(p, "door B (weapon fire direct)")) return;
    if (!offsets_agree(p.sub, p.weapon)) return;
    const bool fired = vslot<FireFn>(p.weapon, settings::WEAPON_VSLOT_FIRE)(p.weapon, true);
    LOGI("%s fire door B: weapon fire (slot 0x%zX, manual) returned %d; right after: %s", TAG, settings::WEAPON_VSLOT_FIRE, fired ? 1 : 0, state_line(p.combat, p.sub, p.weapon).c_str());
    report_after("door B");
}
} // namespace

void install() {
    if (g_installed) return;
    auto* cls = ue::find_class(L"Class /Script/SHProto.SHItemWeaponRanged");
    auto* fn = cls != nullptr ? cls->find_function(L"OnFirePerformed") : nullptr;
    if (fn == nullptr) { LOGW("%s fire: SHItemWeaponRanged.OnFirePerformed not found, shots cannot be counted", TAG); g_installed = true; return; }
    const bool ok = fn->hook_ptr(on_fire_performed_pre, on_fire_performed_post);
    auto* ready = cls->find_function(L"IsFireReady");
    const bool ok2 = ready != nullptr && ready->hook_ptr(on_is_fire_ready_pre, on_is_fire_ready_post);
    LOGI("%s fire: hook on OnFirePerformed %s, on IsFireReady %s; keys: F9 door A (gates forced), F8 door B (weapon fire direct), F11 force-ready toggle", TAG, ok ? "set" : "FAILED", ok2 ? "set" : "FAILED");
    g_installed = true;
}

void on_message(UINT msg, WPARAM wparam) {
    if (msg != WM_KEYDOWN && msg != WM_SYSKEYDOWN) return;   // F10 and friends arrive as system keys
    if (wparam == settings::FIRE_PROBE_KEY) probe_through_gate();
    if (wparam == settings::FIRE_DIRECT_KEY) probe_direct();
    if (wparam == settings::FIRE_FORCE_READY_KEY) {
        g_force_ready = !g_force_ready;
        LOGI("%s fire: force-ready %s (the game asked %d times while it was on)", TAG, g_force_ready ? "ON" : "OFF", g_force_ready_calls);
    }
}

void tick(float delta) {
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
