// settings.h -- every number the plugin uses, named, in one place (code-shape rule: no bare numbers in the code).
#pragma once

namespace sh2vr::settings {

// How often the heartbeat line is written to the UEVR log while the game runs (seconds).
constexpr float HEARTBEAT_S = 5.0f;

// Where jbusfield's saved parameter files live, under %APPDATA%: the same folder UEVR keeps the profile in.
constexpr const wchar_t* PROFILE_DIR = L"UnrealVRMod\\SHProto-Win64-Shipping";
constexpr const wchar_t* PARAMS_SUBDIR = L"data";

// ---- animation (animation.lua) ----
constexpr int MAX_BONE_DEPTH = 64;            // a guard for the walk up to the root bone
constexpr double HIDDEN_BONE_SCALE = 0.001;   // the Lua's scale for bones that should not show

// ---- hands (hands.lua) ----
constexpr float HANDS_RETRY_S = 1.0f;         // the Lua tried to create the hands once a second (setInterval 1000)
constexpr double HAND_BOUNDS_SCALE = 16.0;    // component.BoundsScale = 16.0: never culled at arm's length
constexpr const char* HANDS_PROFILE = "Main"; // the first (only) profile in hands_parameters.json
constexpr int HANDS_TYPE_FOREARMS = 2;        // main.lua HandsType: None 1, Forearms 2, IK Arms 3 (Tefa plays with 3)
constexpr const wchar_t* LEFT_SOURCE = L"Left";    // controllers.lua sourceNames: the MotionSource of each controller
constexpr const wchar_t* RIGHT_SOURCE = L"Right";

// ---- the IK arms (ik.lua's locals; the names there are kept) ----
constexpr double IK_MIN_SWING_DEG = 0.02;              // IK_MIN_SWING_DEG
constexpr double IK_MIN_TWIST_DEG = 0.02;              // IK_MIN_TWIST_DEG
constexpr double IK_ALIGN_SWING_DEADBAND_DEG = 0.0;    // ALIGN_SWING_DEADBAND_DEG (0.6 only while a scope is active)
constexpr double FOREARM_TWIST_MAX_DEG = 100.0;        // FOREARM_TWIST_MAX_DEG_DEFAULT
constexpr double IK_POLE_MIN_LEN = 0.15;               // the pole projection too short to trust (the non-aggressive path)
constexpr double IK_CTRL_POLE_MIN_LEN = 0.25;          // upProjLen > 0.25 before the controller's up steers the elbow
constexpr double IK_OUTWARD_FRACTION = 0.35;           // outwardDist = upperLen * 0.35
constexpr double IK_POLE_SMOOTH_ALPHA_BASE = 0.72;     // POLE_SMOOTH_ALPHA_BASE
constexpr double IK_POLE_SMOOTH_ALPHA_NEAR = 0.90;     // POLE_SMOOTH_ALPHA_NEAR
constexpr double IK_POLE_SMOOTH_NEAR_LEN = 0.30;       // POLE_SMOOTH_NEAR_LEN
constexpr double IK_DIR_SMOOTH_ALPHA = 0.2;            // nlerpSmoothDir alpha for the arm directions (smoothing > 0 only)
constexpr double IK_POLE_DIR_SMOOTH_ALPHA = 0.4;       // and for the pole
constexpr float IK_TELL_LUA_S = 1.0f;                  // how often the plugin names its arm mesh to the Lua pack

// ---- shooting without the aim stance (fire.cpp) ----
constexpr unsigned FIRE_PROBE_KEY = 0x78;   // VK_F9: the probe key, sent to the game window from the session
constexpr float FIRE_PROBE_REPORT_S = 1.0f; // how long after the probe the shot count is reported

} // namespace sh2vr::settings
