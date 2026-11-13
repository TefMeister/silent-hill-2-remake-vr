// roomscale.cpp -- see roomscale.h.
#include "roomscale.h"

#include <atomic>
#include <string>
#include <thread>

#include "common.h"
#include "settings.h"
#include "ue.h"
#include "uevr/API.hpp"

namespace sh2vr::roomscale {

namespace {
constexpr const char* KEY = "VR_RoomscaleMovement";
float g_since = 0.0f;
bool g_off = false;                 // switched off by us
std::string g_before;               // the value it had before we switched it off
std::atomic<bool> g_busy{false};
int g_said = 0;

bool in_cutscene(API::UObject* pawn) {
    API::UObject* statics = ue::cdo(L"Class /Script/SHProto.SHCharacterStatics");
    bool in = false;
    return statics != nullptr && ue::call_ret(statics, L"IsCharacterInCutscene", {ue::Arg(pawn)}, in) && in;
}

// why the game is steering James right now, or nullptr when it is not
const char* game_steers() {
    API::UObject* pawn = API::get()->get_local_pawn(0);
    if (pawn == nullptr) return nullptr;
    API::UObject* pc = API::get()->get_player_controller(0);
    bool ignored = false;
    if (pc != nullptr && ue::call_ret(pc, L"IsMoveInputIgnored", {}, ignored) && ignored) return "movement input ignored";
    if (in_cutscene(pawn)) return "in a cutscene";
    // memory spots: the game's camera takes over and James is no longer the view target [measured 2026-11-13]
    if (!pawn->get_bool_property(L"bIsLocalViewTarget")) return "the camera is the game's";
    // squeezes, crawls, vaults: the movement component's mode goes from walking (1) to custom (6) for exactly the
    // traversal [measured 2026-11-13: 9 squeezes, CurrentlyPlayingTraversal set over the same 2 s]
    auto** move = pawn->get_property_data<API::UObject*>(L"CharacterMovement");
    if (move != nullptr && *move != nullptr) {
        const auto* mode = (*move)->get_property_data<uint8_t>(L"MovementMode");
        if (mode != nullptr && *mode == settings::ROOMSCALE_CUSTOM_MOVEMENT_MODE) return "a traversal (custom movement)";
    }
    return nullptr;
}

// off the game thread, like every injector setting this plugin writes
void write(const std::string& value) {
    g_busy.store(true);
    std::thread([value] { API::get()->param()->vr->set_mod_value(KEY, value.c_str()); g_busy.store(false); }).detach();
}
} // namespace

void tick(float delta) {
    if (!settings::ROOMSCALE_STEPS_ASIDE) return;
    g_since += delta;
    if (g_since < settings::ROOMSCALE_CHECK_S) return;
    g_since = 0.0f;
    if (g_busy.load()) return;
    const char* why = game_steers();
    if (why != nullptr && !g_off) {
        g_before = API::VR::get_mod_value<std::string>(KEY);
        if (g_before.rfind("true", 0) != 0) return;   // roomscale is not on: nothing to step aside
        g_off = true;
        if (g_said < 200) { ++g_said; LOGI("%s roomscale: off (%s)", TAG, why); }
        write("false");
    } else if (why == nullptr && g_off) {
        g_off = false;
        if (g_said < 200) { ++g_said; LOGI("%s roomscale: back on (the player has James again)", TAG); }
        write(g_before);
    }
}

} // namespace sh2vr::roomscale
