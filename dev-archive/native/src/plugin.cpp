// plugin.cpp -- sh2vr: Silent Hill 2 (2024) VR, the C++ plugin the Lua pack is ported into (started 2026-10-23).
//
// Pass 1, the skeleton (modding-notes/2026-10-23-cpp-rewrite-plan.md, decision 4 step 1): the plugin loads beside
// jbusfield's Lua pack and praydog's sh2r.dll, reads the pack's saved parameter files, and writes a heartbeat to the
// UEVR log so a run can prove it is alive: frame time, the player pawn it sees, whether the headset is active.
// Nothing is ported yet; each module arrives in its own file and is listed here as it lands (the way
// visceral-re8-vr/dev-archive/native/src/plugin.cpp does).
#include <windows.h>

#include <memory>

#include "common.h"
#include "params.h"
#include "settings.h"
#include "uevr/Plugin.hpp"

using namespace sh2vr;

namespace {
class SH2Plugin : public uevr::Plugin {
public:
    void on_initialize() override {
        LOGI("%s loaded: skeleton, API %d.%d.%d", TAG, UEVR_PLUGIN_VERSION_MAJOR, UEVR_PLUGIN_VERSION_MINOR, UEVR_PLUGIN_VERSION_PATCH);
        params::load_all();
    }

    void on_pre_engine_tick(API::UGameEngine*, float delta) override {
        ++m_frames;
        m_since_beat += delta;
        if (m_since_beat < settings::HEARTBEAT_S) return;
        const float avg_ms = m_since_beat * 1000.0f / (float)std::max<uint32_t>(1, m_frames - m_frames_at_beat);
        m_frames_at_beat = m_frames;
        m_since_beat = 0.0f;
        params::watch();

        std::string pawn = "none";
        if (auto* p = API::get()->get_local_pawn(0); p != nullptr) pawn = narrow(p->get_full_name());
        const auto* vr = API::get()->param()->vr;
        const bool hmd = vr != nullptr && vr->is_hmd_active();
        LOGI("%s alive: frame %u, %.1f ms avg, headset %s, pawn %s, params %d", TAG, m_frames, avg_ms, hmd ? "on" : "off", pawn.c_str(), params::loaded_count());
    }

private:
    uint32_t m_frames = 0, m_frames_at_beat = 0;
    float m_since_beat = 0.0f;
};

std::unique_ptr<SH2Plugin> g_plugin{new SH2Plugin()};   // constructing it registers it with the UEVR plugin base
} // namespace
