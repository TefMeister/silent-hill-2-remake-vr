// ammo.cpp -- see ammo.h
#include "ammo.h"

#include <cstring>
#include <string>

#include "settings.h"
#include "ue.h"

namespace sh2vr::ammo {

namespace {
int g_checks = 0, g_disagree = 0;
} // namespace

bool known_gun(API::UObject* weapon) {
    if (!ue::alive(weapon)) return false;
    const std::string cls = ue::name_of((API::UObject*)weapon->get_class());
    for (const char* name : settings::RELOAD_GUN_CLASSES)
        if (cls.find(name) != std::string::npos) return true;
    return false;
}

int rounds(API::UObject* weapon) {
    if (!known_gun(weapon)) return -1;
    int32_t n = 0;
    std::memcpy(&n, (const uint8_t*)weapon + settings::CLIP_ROUNDS_OFFSET, sizeof n);
    return n;
}

void check(API::UObject* weapon, const char* when) {
    const int n = rounds(weapon);
    if (n < 0) return;
    uint8_t any = 2;   // 2 = the game's function could not be asked
    ue::call_ret(weapon, L"HasAnyAmmoInClip", {}, any);
    ++g_checks;
    const bool agree = any == 2 || (any == 1) == (n > 0);
    if (!agree) ++g_disagree;
    LOGI("%s ammo: %s: %d round(s) at +0x%X, the game says ammo in clip %d -> %s (%d of %d checks disagreed)", TAG, when, n,
         (unsigned)settings::CLIP_ROUNDS_OFFSET, any, agree ? "agree" : "DISAGREE", g_disagree, g_checks);
}


void set_rounds(API::UObject* weapon, int n) {
    if (!known_gun(weapon)) return;
    const int32_t v = n < 0 ? 0 : n;
    std::memcpy((uint8_t*)weapon + settings::CLIP_ROUNDS_OFFSET, &v, sizeof v);
}

int clip_size(API::UObject* weapon) {
    auto* p = ue::prop<int32_t>(weapon, L"WeaponClipSize");
    return p != nullptr && *p > 0 ? *p : settings::CLIP_SIZE_FALLBACK;
}

namespace {
API::UObject* items() {
    auto** i = ue::prop<API::UObject*>(API::get()->get_local_pawn(0), L"Items");
    return i != nullptr && ue::alive(*i) ? *i : nullptr;
}
API::FName ammo_row(API::UObject* weapon) {   // WeaponAmmoItem is an FDataTableRowHandle: the table pointer, then the row name
    auto* p = weapon != nullptr ? weapon->get_property_data<uint8_t>(L"WeaponAmmoItem") : nullptr;
    if (p != nullptr) {
        API::FName n;
        std::memcpy(&n, p + sizeof(void*), sizeof n);
        if (!ue::is_none(n)) return n;
    }
    return ue::name(settings::AMMO_ROW_FALLBACK);
}
} // namespace

int carried(API::UObject* weapon) {
    API::UObject* it = items();
    int32_t n = -1;
    if (it == nullptr || !ue::call_ret(it, L"GetItemCount", {ammo_row(weapon)}, n)) return -1;
    return n < 0 || n > 100000 ? -1 : n;
}

int take_carried(API::UObject* weapon, int n) {
    if (n <= 0) return 0;
    API::UObject* it = items();
    if (it == nullptr) return 0;
    const int before = carried(weapon);
    if (before == 0) return 0;
    if (before > 0 && n > before) n = before;
    if (!ue::call(it, L"RemoveItem", {ammo_row(weapon), (int32_t)n, false})) return 0;
    const int after = carried(weapon);
    return before >= 0 && after >= 0 ? before - after : n;
}

void out_of_ammo_visual(API::UObject* weapon, bool empty) {
    auto** m = ue::prop<API::UObject*>(weapon, L"Mesh");
    auto** anim = m != nullptr && ue::alive(*m) ? ue::prop<API::UObject*>(*m, L"AnimScriptInstance") : nullptr;
    if (anim != nullptr && ue::alive(*anim)) ue::call(*anim, L"SetOutOfAmmo", {empty});
}

void fill(API::UObject* weapon, int n) {
    set_rounds(weapon, n);
    out_of_ammo_visual(weapon, n <= 0);
    check(weapon, "filled");
}

} // namespace sh2vr::ammo
