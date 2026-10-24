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

} // namespace sh2vr::ammo
