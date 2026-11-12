// cutscreen.cpp -- see cutscreen.h. [compile-verified 2026-11-12] until it has run.
#include "cutscreen.h"

#include <string>

#include "common.h"
#include "settings.h"
#include "ue.h"
#include "uevr/API.hpp"

namespace sh2vr::cutscreen {

namespace {
constexpr const char* KEY = "VR_2DScreenMode";
float g_since = 0.0f;
bool g_flat = false;          // we switched the flat screen on
std::string g_before;         // what the setting held before we did
int g_said = 0;

bool in_cutscene() {
    API::UObject* pawn = API::get()->get_local_pawn(0);
    if (pawn == nullptr) return false;
    API::UObject* statics = ue::cdo(L"Class /Script/SHProto.SHCharacterStatics");
    if (statics == nullptr) return false;
    bool in = false;
    if (!ue::call_ret(statics, L"IsCharacterInCutscene", {ue::Arg(pawn)}, in)) return false;
    return in;
}
} // namespace

void tick(float delta) {
    if (!settings::CUTSCENE_FLAT_SCREEN) return;
    g_since += delta;
    if (g_since < settings::CUTSCENE_CHECK_S) return;
    g_since = 0.0f;
    const bool want = in_cutscene();
    if (want == g_flat) return;
    auto* vr = API::get()->param()->vr;
    if (want) {
        g_before = API::VR::get_mod_value<std::string>(KEY);
        if (g_before.rfind("true", 0) == 0) { g_flat = true; g_before = "true"; return; }   // already flat: not ours to undo
        vr->set_mod_value(KEY, "true");
        g_before = "false";
    } else if (g_before != "true") {
        vr->set_mod_value(KEY, "false");
    }
    g_flat = want;
    if (g_said < 40) { ++g_said; LOGI("%s cutscreen: cutscene %s, flat screen %s", TAG, want ? "began" : "ended", want ? "on" : "off"); }
}

} // namespace sh2vr::cutscreen
