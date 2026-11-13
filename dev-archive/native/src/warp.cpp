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
    if (before.rfind(METHOD_WARP, 0) == 0) { LOGI("%s warp: the frame warp was already on", TAG); g_done.store(true); return; }
    LOGI("%s warp: the headset has drawn for %.0f s, switching the frame warp on (rendering method %s -> %s)", TAG,
         g_drawing, before.c_str(), METHOD_WARP);
    // off the game thread, like the flat screen's switch (an injector setting written from the tick killed the game once)
    std::thread([] {
        API::get()->param()->vr->set_mod_value(KEY_METHOD, METHOD_WARP);
        g_done.store(true);
    }).detach();
}

} // namespace sh2vr::warp
