// warp.cpp -- see warp.h.
#include "warp.h"

#include <windows.h>

#include <atomic>
#include <string>
#include <thread>

#include "common.h"
#include "settings.h"
#include "uevr/API.hpp"

namespace sh2vr::warp {

namespace {
constexpr const char* KEY_METHOD = "VR_RenderingMethod";
constexpr const char* KEY_MODE = "VR_AFW_FramewarpMode";   // 0 none, 1 the other eye, 2 the previous frame, 3 both
constexpr const char* METHOD_WARP = "3";   // 0 both eyes, 1 synchronized, 2 alternating, 3 alternate-eye frame warp
float g_drawing = 0.0f;                    // seconds the headset has been drawing without a break
std::atomic<bool> g_done{false};
bool g_started = false;

bool kept_off() {
    const std::wstring p = appdata_dir() + L"\\" + settings::PROFILE_DIR + L"\\sh2vr_nowarp.txt";
    return GetFileAttributesW(p.c_str()) != INVALID_FILE_ATTRIBUTES;
}
} // namespace

bool switched() { return g_done.load(); }

void tick(float delta) {
    if (!settings::WARP_AUTO_ON || g_started) return;
    auto* vr = API::get()->param()->vr;
    if (vr == nullptr || !vr->is_hmd_active()) { g_drawing = 0.0f; return; }
    g_drawing += delta;
    if (g_drawing < settings::WARP_AFTER_S) return;
    g_started = true;
    if (kept_off()) { LOGI("%s warp: sh2vr_nowarp.txt is in the profile, the frame warp stays off", TAG); return; }
    const std::string before = API::VR::get_mod_value<std::string>(KEY_METHOD);
    // 2026-11-22 (b041): the warp's mode is put to "both" (3) here as well. Mode 2 (config.txt's value until now) gives
    // everything nearer than 1 m only the head's turn, not its move: Tefa's "dome" ("ANY close thing does that, as if
    // james has a dome around him"). Mode 3, switched live in the headset: "dome jitter gone", "unbelivably good".
    const std::string mode_before = API::VR::get_mod_value<std::string>(KEY_MODE);
    LOGI("%s warp: the warp's mode %s -> %s (both: the previous frame and the other eye)", TAG, mode_before.c_str(), settings::WARP_MODE);
    if (before.rfind(METHOD_WARP, 0) == 0) {
        std::thread([] { API::get()->param()->vr->set_mod_value(KEY_MODE, settings::WARP_MODE); }).detach(); LOGI("%s warp: the frame warp was already on", TAG); g_done.store(true); return; }
    LOGI("%s warp: the headset has drawn for %.0f s, switching the frame warp on (rendering method %s -> %s)", TAG,
         g_drawing, before.c_str(), METHOD_WARP);
    // off the game thread, like the flat screen's switch (an injector setting written from the tick killed the game once)
    std::thread([] {
        API::get()->param()->vr->set_mod_value(KEY_MODE, settings::WARP_MODE);
        API::get()->param()->vr->set_mod_value(KEY_METHOD, METHOD_WARP);
        Sleep(1000);
        LOGI("%s warp: read back: the warp's mode is %s", TAG, API::VR::get_mod_value<std::string>(KEY_MODE).c_str());
        g_done.store(true);
    }).detach();
}

} // namespace sh2vr::warp
