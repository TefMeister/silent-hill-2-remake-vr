// plugin.cpp -- sh2vr: Silent Hill 2 (2024) VR, the C++ plugin the Lua pack is ported into (started 2026-10-23).
//
// The plugin runs beside jbusfield's Lua pack and praydog's sh2r.dll and takes modules over one at a time
// (modding-notes/2026-10-23-cpp-rewrite-plan.md). A module taken over is listed in tools/build.sh PORTED; the deploy
// writes scripts/sh2vr_ported.lua, and the Lua module stands down when it sees its name there.
// This file only wires things together; every feature lives in its own file.
//
// Ported so far:
//   animation.cpp  animation.lua: bone transforms on a PoseableMeshComponent (a library)
//   hands.cpp      hands.lua: the forearm hands (Hands Type 2), made from hands_parameters.json, hung on the controllers
//   ik.cpp         ik.lua: the IK arms (Hands Type 3, the one in use): a poseable body copy with a two-bone solver per arm
// Shared doors: ue.cpp (the game's functions by name), params.cpp (the pack's saved parameter files).
#include <windows.h>

#include <memory>

#include "common.h"
#include "hands.h"
#include "ik.h"
#include "params.h"
#include "settings.h"
#include "uevr/Plugin.hpp"

using namespace sh2vr;

namespace {
class SH2Plugin : public uevr::Plugin {
public:
    void on_initialize() override {
        LOGI("%s loaded: hands + IK arms, API %d.%d.%d", TAG, UEVR_PLUGIN_VERSION_MAJOR, UEVR_PLUGIN_VERSION_MINOR, UEVR_PLUGIN_VERSION_PATCH);
        params::load_all();
    }

    void on_pre_engine_tick(API::UGameEngine*, float delta) override {
        ik::tick(delta);
        hands::tick(delta);
        heartbeat(delta);
    }

private:
    // one line every few seconds so a run can prove the plugin is alive and what it sees
    void heartbeat(float delta) {
        ++m_frames;
        m_since_beat += delta;
        if (m_since_beat < settings::HEARTBEAT_S) return;
        const float avg_ms = m_since_beat * 1000.0f / (float)std::max<uint32_t>(1, m_frames - m_frames_at_beat);
        m_frames_at_beat = m_frames;
        m_since_beat = 0.0f;
        params::watch();
        const auto* vr = API::get()->param()->vr;
        const bool hmd = vr != nullptr && vr->is_hmd_active();
        LOGI("%s alive: frame %u, %.1f ms avg, headset %s, pawn %s, hands %s, arms %s", TAG, m_frames, avg_ms, hmd ? "on" : "off",
             API::get()->get_local_pawn(0) != nullptr ? "yes" : "none", hands::exists() ? "made" : "not yet", ik::exists() ? "made" : "not yet");
    }

    uint32_t m_frames = 0, m_frames_at_beat = 0;
    float m_since_beat = 0.0f;
};

std::unique_ptr<SH2Plugin> g_plugin{new SH2Plugin()};   // constructing it registers it with the UEVR plugin base
} // namespace
