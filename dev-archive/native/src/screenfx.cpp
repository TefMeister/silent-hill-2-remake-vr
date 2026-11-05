// screenfx.cpp -- see screenfx.h. [compile-verified 2026-11-05] until it has run.
#include "screenfx.h"

#include <cmath>
#include <string>

#include "common.h"
#include "settings.h"
#include "uevr/API.hpp"

namespace sh2vr::screenfx {

namespace {
constexpr size_t N = sizeof(settings::SCREEN_FX_CVARS) / sizeof(settings::SCREEN_FX_CVARS[0]);
API::IConsoleVariable* g_var[N] = {};
int g_looks[N] = {};
int g_sets[N] = {};
float g_since = settings::SCREEN_FX_CHECK_S;   // the first tick checks at once
}

void tick(float delta) {
    if (!settings::SCREEN_FX_OFF) return;
    g_since += delta;
    if (g_since < settings::SCREEN_FX_CHECK_S) return;
    g_since = 0.0f;
    auto* cm = API::get()->get_console_manager();
    if (cm == nullptr) return;
    for (size_t i = 0; i < N; ++i) {
        const auto& c = settings::SCREEN_FX_CVARS[i];
        if (g_var[i] == nullptr) {
            // looked for over the first minute (a setting can be made after the start), then given up and said once
            if (g_looks[i] >= settings::SCREEN_FX_LOOKS) continue;
            g_var[i] = cm->find_variable(c.name);
            if (g_var[i] == nullptr) {
                if (++g_looks[i] == settings::SCREEN_FX_LOOKS) LOGW("%s screenfx: %ls is not in this build (%s stays as the game has it)", TAG, c.name, c.what);
                continue;
            }
        }
        const float now = g_var[i]->get_float();
        if (std::fabs(now - c.value) < 0.0001f) continue;
        g_var[i]->set(c.value);
        ++g_sets[i];
        if (g_sets[i] <= settings::SCREEN_FX_LOG_PER_VAR)   // proof of effect: the value before and after
            LOGI("%s screenfx: %ls %g -> %g (%s off; set %d time(s))", TAG, c.name, now, g_var[i]->get_float(), c.what, g_sets[i]);
    }
}

} // namespace sh2vr::screenfx
