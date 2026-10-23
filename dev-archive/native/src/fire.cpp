// fire.cpp -- see fire.h
#include "fire.h"

#include <string>

#include "settings.h"
#include "ue.h"

namespace sh2vr::fire {

namespace {
int g_shots_performed = 0;          // counted by the hook on SHItemWeaponRanged::OnFirePerformed
int g_shots_at_probe = 0;
float g_report_in = -1.0f;          // seconds until the after-probe report, < 0 = none pending
bool g_installed = false;
bool g_force_ready = false;         // F10: answer "yes" to the game's own "is the gun ready to fire?" while the gun is lowered
int g_force_ready_calls = 0;

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
    return true;   // let it run
}
void on_fire_performed_post(API::UFunction*, API::UObject*, void*, void*) {}

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

// the probe: RequestManualFire(weapon) on the ranged combat part, with the gun lowered
void probe() {
    API::UObject* pawn = API::get()->get_local_pawn(0);
    API::UObject* combat = combat_of(pawn);
    API::UObject* sub = ranged_subcomp(combat);
    if (sub == nullptr) { LOGW("%s fire probe: no ranged combat part on the pawn (pawn %s, combat %s)", TAG, ue::name_of(pawn).c_str(), ue::name_of(combat).c_str()); return; }
    API::UObject* weapon = nullptr;
    ue::call_ret(sub, L"GetEquippedRangedWeapon", {}, weapon);
    LOGI("%s fire probe: before: weapon %s; %s", TAG, ue::name_of(weapon).c_str(), state_line(combat, sub, weapon).c_str());
    if (weapon == nullptr) { LOGW("%s fire probe: no ranged weapon equipped, nothing to fire", TAG); return; }
    g_shots_at_probe = g_shots_performed;
    const bool ok = ue::call(sub, L"RequestManualFire", {weapon});
    LOGI("%s fire probe: RequestManualFire(weapon) %s; right after: %s", TAG, ok ? "called" : "REFUSED", state_line(combat, sub, weapon).c_str());
    g_report_in = settings::FIRE_PROBE_REPORT_S;
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
    LOGI("%s fire: hook on OnFirePerformed %s, on IsFireReady %s; probe key F9, force-ready toggle F10", TAG, ok ? "set" : "FAILED", ok2 ? "set" : "FAILED");
    g_installed = true;
}

void on_message(UINT msg, WPARAM wparam) {
    if (msg != WM_KEYDOWN) return;
    if (wparam == settings::FIRE_PROBE_KEY) probe();
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
    LOGI("%s fire probe: %.1f s later: %d shot(s) performed since the probe; %s", TAG, settings::FIRE_PROBE_REPORT_S, g_shots_performed - g_shots_at_probe,
         sub != nullptr ? state_line(combat, sub, weapon).c_str() : "no ranged combat part");
}

} // namespace sh2vr::fire
