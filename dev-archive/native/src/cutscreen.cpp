// cutscreen.cpp -- see cutscreen.h.
// 2026-11-12, first run: setting VR_2DScreenMode from the engine tick (the game thread) the moment the opening cutscene
// began killed the game (access violation in the game under a UEVRBackend frame) [verified-live 2026-11-12, n=1]; the
// pack's own set_2D_mode carries the line "do not execute in game thread". The settings are now written by a short
// thread of our own, never from the tick.
#include "cutscreen.h"

#include <windows.h>

#include <atomic>
#include <string>
#include <thread>

#include "common.h"
#include "settings.h"
#include "ue.h"
#include "uevr/API.hpp"

namespace sh2vr::cutscreen {

namespace {
constexpr const char* KEY_FLAT = "VR_2DScreenMode";
constexpr const char* KEY_FOLLOW = "UI_FollowView";
constexpr const char* KEY_SIZE = "UI_Size";
constexpr const char* KEY_DISTANCE = "UI_Distance";
float g_since = 0.0f;
bool g_flat = false;                 // the flat screen is on by our doing
std::atomic<bool> g_busy{false};     // the writer thread is at work
std::string g_follow, g_size, g_distance;   // what the screen's settings held before the cutscene
int g_said = 0;

// dev: a file named sh2vr_flat.txt in the profile forces the flat screen on in play (to try the injector's settings
// against the flat mode without waiting for a cutscene); looked for once a second, no file = nothing.
bool dev_forced() {
    static float since = 1.0f;
    static bool forced = false;
    since += settings::CUTSCENE_CHECK_S;
    if (since >= 1.0f) {
        since = 0.0f;
        const std::wstring p = appdata_dir() + L"\\" + settings::PROFILE_DIR + L"\\sh2vr_flat.txt";
        forced = GetFileAttributesW(p.c_str()) != INVALID_FILE_ATTRIBUTES;
    }
    return forced;
}

bool in_cutscene() {
    API::UObject* pawn = API::get()->get_local_pawn(0);
    if (pawn == nullptr) return false;
    if (dev_forced()) return true;
    API::UObject* statics = ue::cdo(L"Class /Script/SHProto.SHCharacterStatics");
    if (statics == nullptr) return false;
    bool in = false;
    if (!ue::call_ret(statics, L"IsCharacterInCutscene", {ue::Arg(pawn)}, in)) return false;
    return in;
}

std::string num(float v) { return std::to_string(v); }

// Off the game thread: the screen's place first (a big screen that stays put, in front after a recentre), then the mode.
void write(bool on, std::string follow, std::string size, std::string distance) {
    std::thread([on, follow, size, distance] {
        auto* vr = API::get()->param()->vr;
        if (on) {
            vr->set_mod_value(KEY_FOLLOW, "false");
            vr->set_mod_value(KEY_SIZE, num(settings::CUTSCENE_SCREEN_SIZE).c_str());
            vr->set_mod_value(KEY_DISTANCE, num(settings::CUTSCENE_SCREEN_DISTANCE).c_str());
            vr->recenter_view();
            vr->set_mod_value(KEY_FLAT, "true");
        } else {
            vr->set_mod_value(KEY_FLAT, "false");
            vr->set_mod_value(KEY_FOLLOW, follow.c_str());
            vr->set_mod_value(KEY_SIZE, size.c_str());
            vr->set_mod_value(KEY_DISTANCE, distance.c_str());
        }
        g_busy.store(false);
    }).detach();
}
} // namespace

bool active() { return g_flat; }

void tick(float delta) {
    if (!settings::CUTSCENE_FLAT_SCREEN) return;
    g_since += delta;
    if (g_since < settings::CUTSCENE_CHECK_S) return;
    g_since = 0.0f;
    if (g_busy.load()) return;
    const bool want = in_cutscene();
    if (want == g_flat) {
        // the pack locks the screen to the head in its own cutscene state: ours stays put
        if (g_flat && API::VR::get_mod_value<std::string>(KEY_FOLLOW).rfind("true", 0) == 0) {
            g_busy.store(true);
            std::thread([] { API::get()->param()->vr->set_mod_value(KEY_FOLLOW, "false"); g_busy.store(false); }).detach();
        }
        return;
    }
    if (want) {
        if (API::VR::get_mod_value<std::string>(KEY_FLAT).rfind("true", 0) == 0) return;   // already flat: not ours
        g_follow = API::VR::get_mod_value<std::string>(KEY_FOLLOW);
        g_size = API::VR::get_mod_value<std::string>(KEY_SIZE);
        g_distance = API::VR::get_mod_value<std::string>(KEY_DISTANCE);
    }
    if (g_said < 40) { ++g_said; LOGI("%s cutscreen: cutscene %s, flat screen going %s", TAG, want ? "began" : "ended", want ? "on" : "off"); }
    g_flat = want;
    g_busy.store(true);
    write(want, g_follow, g_size, g_distance);
}

} // namespace sh2vr::cutscreen
