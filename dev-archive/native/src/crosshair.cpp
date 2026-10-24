// crosshair.cpp -- see crosshair.h
#include "crosshair.h"

#include <set>
#include <vector>

#include "settings.h"
#include "ue.h"

namespace sh2vr::crosshair {

namespace {
std::vector<API::UObject*> g_widgets;    // the crosshair widgets alive at the last look
std::set<API::UObject*> g_reported;      // one log line per widget
float g_since_find = settings::CROSSHAIR_FIND_S;   // look on the first tick
int g_shown_again = 0;                   // times the game made a widget visible again (reported with each new widget)
} // namespace

void find(float delta) {
    g_since_find += delta;
    if (g_since_find < settings::CROSSHAIR_FIND_S) return;
    g_since_find = 0.0f;
    g_widgets.clear();
    auto* cls = ue::find_class(settings::CROSSHAIR_WIDGET_CLASS);   // loaded with the gameplay HUD
    if (cls == nullptr) return;
    for (auto* w : cls->get_objects_matching(false)) {
        if (!ue::alive(w)) continue;
        g_widgets.push_back(w);
        bool in_viewport = false;
        if (ue::call_ret(w, L"IsInViewport", {}, in_viewport) && in_viewport) ue::call(w, L"RemoveFromParent", {});
        if (g_reported.insert(w).second)
            LOGI("%s crosshair: hidden %s (was in the viewport: %s; shown again by the game so far: %d)", TAG,
                 ue::name_of(w).c_str(), in_viewport ? "yes" : "no", g_shown_again);
    }
}

void hide() {
    for (auto* w : g_widgets) {
        if (!ue::alive(w)) continue;
        uint8_t vis = settings::SLATE_COLLAPSED;
        if (!ue::call_ret(w, L"GetVisibility", {}, vis) || vis == settings::SLATE_COLLAPSED) continue;
        ++g_shown_again;
        ue::call(w, L"SetVisibility", {(int32_t)settings::SLATE_COLLAPSED});
    }
}

int shown_again() { return g_shown_again; }

} // namespace sh2vr::crosshair
