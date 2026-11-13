// tuning.cpp -- see tuning.h.
#include "tuning.h"

#include <windows.h>

#include <cmath>
#include <string>

#include "common.h"
#include "settings.h"
#include "uevr/API.hpp"

namespace sh2vr::tuning {

namespace {
struct Row { const wchar_t* name; float value; };
// the set Tefa could not tell from the game's own values (modding-notes/2026-11-13c, section 6, set C)
constexpr Row ROWS[] = {
    {L"r.Lumen.Reflections.DownsampleFactor", 2.0f},                          // game: 1
    {L"r.Lumen.Reflections.ScreenSpaceReconstruction.NumSamples", 3.0f},      // game: 5
    {L"r.Lumen.Reflections.MaxRoughnessToTrace", 0.3f},                       // game: 0.4
    {L"r.AOGlobalDistanceField", 0.0f},                                       // game: 1
    {L"r.DistanceFieldAO", 0.0f},                                             // game: 1
    {L"r.Lumen.ScreenProbeGather.DownsampleFactor", 32.0f},                   // game: 16
};
float g_since = 0.0f;
int g_said = 0;
// 2026-11-13: a value the engine does not keep (MaxRoughnessToTrace read back unchanged every time) was re-set every 5 s
// all run, the suspect for Tefa's freeze every 4-5 s. Now each value gets TUNING_MAX_TRIES tries in a row, then is left.
int g_tries[6] = {};
bool g_off_said = false;

bool kept_off() {
    const std::wstring p = appdata_dir() + L"\\" + settings::PROFILE_DIR + L"\\sh2vr_notune.txt";
    return GetFileAttributesW(p.c_str()) != INVALID_FILE_ATTRIBUTES;
}
} // namespace

void tick(float delta) {
    if (!settings::TUNING_ON) return;
    g_since += delta;
    if (g_since < settings::TUNING_CHECK_S) return;
    g_since = 0.0f;
    if (API::get()->get_local_pawn(0) == nullptr) return;
    if (kept_off()) {
        if (!g_off_said) { g_off_said = true; LOGI("%s tuning: sh2vr_notune.txt is in the profile, the game's own values stay", TAG); }
        return;
    }
    auto* cm = API::get()->get_console_manager();
    if (cm == nullptr) return;
    int put = 0;
    for (size_t k = 0; k < sizeof(ROWS) / sizeof(ROWS[0]); ++k) {
        const Row& r = ROWS[k];
        if (g_tries[k] >= settings::TUNING_MAX_TRIES) continue;   // given up on: never set again this run
        auto* v = cm->find_variable(r.name);
        if (v == nullptr) continue;
        const float before = v->get_float();
        if (std::fabs(before - r.value) < 0.001f) { g_tries[k] = 0; continue; }
        v->set(r.value);
        ++put;
        if (std::fabs(v->get_float() - r.value) >= 0.001f && ++g_tries[k] >= settings::TUNING_MAX_TRIES)
            LOGI("%s tuning: %ls does not take (%g after setting %g), left alone from now on", TAG, r.name, v->get_float(), r.value);
        if (g_said < 30) { ++g_said; LOGI("%s tuning: %ls %g -> %g", TAG, r.name, before, v->get_float()); }
    }
    if (put > 0 && g_said < 30) LOGI("%s tuning: %d value(s) put (the game had its own back)", TAG, put);
}

} // namespace sh2vr::tuning
