// ghostfix.cpp -- see ghostfix.h.
#include "ghostfix.h"

#include <windows.h>

#include <string>
#include <thread>

#include "common.h"
#include "settings.h"
#include "uevr/API.hpp"

namespace sh2vr::ghostfix {

namespace {
constexpr const char* KEY = "VR_GhostingFix";
constexpr const char* ON = "true";
float g_drawing = 0.0f;   // seconds the headset has been drawing without a break
bool g_started = false;

bool kept_off() {
    const std::wstring p = appdata_dir() + L"\\" + settings::PROFILE_DIR + L"\\sh2vr_noghostfix.txt";
    return GetFileAttributesW(p.c_str()) != INVALID_FILE_ATTRIBUTES;
}
} // namespace

void tick(float delta) {
    if (!settings::GHOSTFIX_AUTO_ON || g_started) return;
    auto* vr = API::get()->param()->vr;
    if (vr == nullptr || !vr->is_hmd_active()) { g_drawing = 0.0f; return; }
    g_drawing += delta;
    if (g_drawing < settings::GHOSTFIX_AFTER_S) return;
    g_started = true;
    if (kept_off()) { LOGI("%s ghostfix: sh2vr_noghostfix.txt is in the profile, the ghosting fix stays off", TAG); return; }
    const std::string before = API::VR::get_mod_value<std::string>(KEY);
    if (before.rfind(ON, 0) == 0) {
        LOGW("%s ghostfix: the ghosting fix was ALREADY on at the start (config.txt should keep it false: a start with it on can crash)", TAG);
        return;
    }
    LOGI("%s ghostfix: the headset has drawn for %.0f s, switching the ghosting fix on (%s -> %s)", TAG, g_drawing,
         before.c_str(), ON);
    // off the game thread, like the warp's switch (an injector setting written from the tick killed the game once)
    std::thread([before] {
        API::get()->param()->vr->set_mod_value(KEY, ON);
        Sleep(settings::GHOSTFIX_READBACK_MS);
        LOGI("%s ghostfix: read back after the switch: was %s, is %s", TAG, before.c_str(),
             API::VR::get_mod_value<std::string>(KEY).c_str());
    }).detach();
}

} // namespace sh2vr::ghostfix
