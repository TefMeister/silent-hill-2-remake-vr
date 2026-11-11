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
int g_vis_offset = -1;                   // where the widget keeps its visibility byte (looked up once)
bool g_vis_looked = false;
} // namespace

void find(float delta) {
    g_since_find += delta;
    if (g_since_find < settings::CROSSHAIR_FIND_S) return;
    g_since_find = 0.0f;
    g_widgets.clear();
    // 2026-11-11: with no character there is no gameplay HUD and the crosshair's class is not loaded; looking for a class
    // that is not there searches every object's name, about 20 ms once a second at the main menu (the "crosshair 0.24"
    // of the cost line there, a hitch a second). So: only in play.
    if (API::get()->get_local_pawn(0) == nullptr) return;
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

// Every engine tick. 2026-11-11: this used to ask each widget for its visibility through the game's own function call,
// which was almost all of sh2vr's cost (0.9 ms a frame at the menu, 0.2-0.4 in play). The visibility is one byte in the
// widget (UWidget's "Visibility" property): it is read straight from memory, and the game's function is called only on
// the frame the game has made the widget visible again. With no such property found (another engine build) it falls
// back to asking, as before.
void hide() {
    for (auto* w : g_widgets) {
        if (!ue::alive(w)) continue;
        uint8_t vis = settings::SLATE_COLLAPSED;
        if (g_vis_offset < 0 && !g_vis_looked) {
            g_vis_looked = true;
            auto* prop = w->get_class()->find_property(L"Visibility");
            if (prop != nullptr) g_vis_offset = prop->get_offset();
            LOGI("%s crosshair: the visibility is %s", TAG,
                 g_vis_offset >= 0 ? "read from the widget's own memory (no call a frame)" : "asked through the game's call (property not found)");
        }
        if (g_vis_offset >= 0) {
            vis = *reinterpret_cast<const uint8_t*>(reinterpret_cast<const uint8_t*>(w) + g_vis_offset);
            if (vis == settings::SLATE_COLLAPSED) continue;
        } else if (!ue::call_ret(w, L"GetVisibility", {}, vis) || vis == settings::SLATE_COLLAPSED) {
            continue;
        }
        ++g_shown_again;
        ue::call(w, L"SetVisibility", {(int32_t)settings::SLATE_COLLAPSED});
    }
}

int shown_again() { return g_shown_again; }

} // namespace sh2vr::crosshair
