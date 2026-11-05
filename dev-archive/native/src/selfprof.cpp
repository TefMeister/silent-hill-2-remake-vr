// selfprof.cpp -- see selfprof.h. [compile-verified 2026-11-05] until it has run.
#include "selfprof.h"

#include <windows.h>

#include <algorithm>
#include <cstdio>
#include <utility>
#include <vector>

#include "settings.h"

namespace sh2vr::selfprof {

namespace {
const char* const NAMES[SLOTS] = {"fire", "buttons", "holsters", "twohand", "rhbolt", "crosshair", "screenfx", "reload",
                                  "rack", "ik", "hands", "fingers", "frontgrip", "ghosthand", "bridge", "view", "xinput"};
long long g_ticks[SLOTS] = {};
double g_per_ms = 0.0;

long long now() { LARGE_INTEGER t; QueryPerformanceCounter(&t); return t.QuadPart; }
}

Scope::Scope(Slot s) : slot(s), t0(now()) {}
Scope::~Scope() { g_ticks[slot] += now() - t0; }

std::string report(unsigned frames) {
    if (g_per_ms == 0.0) { LARGE_INTEGER f; QueryPerformanceFrequency(&f); g_per_ms = (double)f.QuadPart / 1000.0; }
    if (frames == 0) frames = 1;
    std::vector<std::pair<double, int>> v;
    double total = 0.0;
    for (int i = 0; i < SLOTS; ++i) {
        const double ms = (double)g_ticks[i] / g_per_ms / frames;
        total += ms;
        v.push_back({ms, i});
        g_ticks[i] = 0;
    }
    std::sort(v.begin(), v.end(), [](const auto& a, const auto& b) { return a.first > b.first; });
    char buf[64];
    std::snprintf(buf, sizeof buf, "ours %.2f ms/frame:", total);
    std::string out = buf;
    for (int k = 0; k < settings::SELFPROF_TOP && k < (int)v.size(); ++k) {
        std::snprintf(buf, sizeof buf, " %s %.2f%s", NAMES[v[k].second], v[k].first, k + 1 < settings::SELFPROF_TOP ? "," : "");
        out += buf;
    }
    return out;
}

} // namespace sh2vr::selfprof
