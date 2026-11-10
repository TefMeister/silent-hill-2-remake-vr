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
//   fingers.cpp    hands_animation.lua + the pose half of animation.lua: finger poses on both kinds of hands
//   fire.cpp       new: the right trigger fires a lowered gun through the weapon's own fire (door B)
//   rack.cpp       new: the pistol slide by hand, the RE8 recipe (LG puts the hand on, LT makes it rackable)
//   reload.cpp     new: the pistol magazine by hand (right B out, the hip ammo spot, back into the seat)
//   ghosthand.cpp  new: the Ghost Hand BeG0nE recorder of those gestures
//   input.cpp      the pad as the plugin last saw it, for the reload modules
//   ammo.cpp       new: the rounds in the gun, read and checked at every shot (manual-reload port, step 1)
//   crosshair.cpp  new: the game's HUD crosshair hidden for good (Tefa 2026-10-24: not used in the game at all)
// Shared doors: ue.cpp (the game's functions by name), params.cpp (the pack's saved parameter files).
#include <windows.h>

#include <memory>

#include "bridge.h"
#include "buttons.h"
#include "common.h"
#include "crosshair.h"
#include "fingers.h"
#include "fire.h"
#include "frontgrip.h"
#include "ghosthand.h"
#include "holsters.h"
#include "twohand.h"
#include "rhbolt.h"
#include "devcvars.h"
#include "screenfx.h"
#include "shadows.h"
#include "selfprof.h"
#include "input.h"
#include "rack.h"
#include "reload.h"
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
        LOGI("%s loaded: hands + IK arms + finger poses + no crosshair, API %d.%d.%d", TAG, UEVR_PLUGIN_VERSION_MAJOR, UEVR_PLUGIN_VERSION_MINOR, UEVR_PLUGIN_VERSION_PATCH);
        params::load_all();
    }

    void on_custom_event(const char* name, const char* data) override { bridge::on_custom_event(name, data); }

    bool on_message(HWND, UINT msg, WPARAM wparam, LPARAM) override { fire::on_message(msg, wparam); holsters::on_message(msg, wparam); frontgrip::on_message(msg, wparam); return true; }

    void on_xinput_get_state(uint32_t*, uint32_t, XINPUT_STATE* state) override {
        if (state == nullptr) return;
        selfprof::Scope sp_(selfprof::XINPUT);
        input::on_state(*state);     // the REAL controller, for the hand modules (reload, rack, fingers)
        fingers::on_input(*state);   // the finger poses read the trigger as pulled, even when fire keeps it from the game
        buttons::on_input(*state);   // 2026-10-29: ONE layer decides what the game sees (Tefa's layout); from here on `state` is the game's
        fire::on_input(*state);
    }

    void on_pre_engine_tick(API::UGameEngine*, float delta) override {
        if (API::get()->get_local_pawn(0) != nullptr) fire::install();   // the weapon class is loaded by then
        { selfprof::Scope sp_(selfprof::FIRE); fire::tick(delta); }
        { selfprof::Scope sp_(selfprof::BUTTONS); buttons::tick(delta); }
        { selfprof::Scope sp_(selfprof::HOLSTERS); holsters::tick(delta); }   // 2026-10-29: the weapons on the body (after the buttons, before the hands)
        { selfprof::Scope sp_(selfprof::TWOHAND); twohand::tick(delta); }    // 2026-10-31: the long gun and the map in two hands (praydog's profile)
        { selfprof::Scope sp_(selfprof::RHBOLT); rhbolt::tick(delta); }     // 2026-11-01: LT locks the rifle in the left hand, RG works its bolt (RE8's right-hand bolt)
        { selfprof::Scope sp_(selfprof::CROSSHAIR); crosshair::find(delta); }
        { selfprof::Scope sp_(selfprof::SCREENFX); devcvars::tick(delta); }   // 2026-11-10: the optimiser's settings file (dev only: no file, nothing)
        { selfprof::Scope sp_(selfprof::SCREENFX); screenfx::tick(delta); }   // 2026-11-05: every blur and screen effect off in VR
        { selfprof::Scope sp_(selfprof::SCREENFX); shadows::tick(delta); }    // 2026-11-06: the weapon lit by the torch, the player's complete shadow (counted with the screen effects: both are once-a-second checks)
        { selfprof::Scope sp_(selfprof::RELOAD); reload::tick(delta); }
        { selfprof::Scope sp_(selfprof::RACK); rack::tick(delta); }       // sets the left-hand override the IK solve reads
        { selfprof::Scope sp_(selfprof::IK); ik::tick(delta); }
        { selfprof::Scope sp_(selfprof::HANDS); hands::tick(delta); }
        { selfprof::Scope sp_(selfprof::FINGERS); fingers::tick(delta); }
        { selfprof::Scope sp_(selfprof::FRONTGRIP); frontgrip::tick(); }       // 2026-11-05: after the pack's finger poses: the left fingers in the game's grip on a long gun's front
        { selfprof::Scope sp_(selfprof::GHOSTHAND); ghosthand::tick(); }       // after the solves: the hands where they are drawn
        { selfprof::Scope sp_(selfprof::BRIDGE); bridge::send_grips(delta); }   // 2026-10-31 evening: the real grips to the pack's shotgun / rifle reloads and pump / bolt
        input::end_tick();
        heartbeat(delta);
    }

    void on_post_engine_tick(API::UGameEngine*, float) override { crosshair::hide(); }

    // after UEVR has put the held things on the controllers for this view: two hands on the long gun and the map
    void on_post_calculate_stereo_view_offset(UEVR_StereoRenderingDeviceHandle, int, float, UEVR_Vector3f*, UEVR_Rotatorf*, bool) override { selfprof::Scope sp_(selfprof::VIEW); twohand::on_view(); rhbolt::on_view(); }

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
        const unsigned beat_frames = m_frames - m_last_report_frames;
        m_last_report_frames = m_frames;
        LOGI("%s alive: frame %u, %.1f ms avg, headset %s, pawn %s, hands %s, arms %s, finger poses on %d hand(s), crosshair re-hidden %d", TAG, m_frames, avg_ms, hmd ? "on" : "off",
             API::get()->get_local_pawn(0) != nullptr ? "yes" : "none", hands::exists() ? "made" : "not yet", ik::exists() ? "made" : "not yet", fingers::count(), crosshair::shown_again());
        LOGI("%s cost: %s", TAG, selfprof::report(beat_frames).c_str());   // 2026-11-05: our own share of each frame
    }

    uint32_t m_frames = 0, m_frames_at_beat = 0, m_last_report_frames = 0;
    float m_since_beat = 0.0f;
};

std::unique_ptr<SH2Plugin> g_plugin{new SH2Plugin()};   // constructing it registers it with the UEVR plugin base
} // namespace
