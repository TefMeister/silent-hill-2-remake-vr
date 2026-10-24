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

// ---- finger poses (fingers.cpp; hands_animation.lua) ----
constexpr float FINGER_LERP_S = 0.1f;        // animDuration: a finger curls or opens in 0.1 s
constexpr int TRIGGER_PRESSED_OVER = 100;    // bRightTrigger > 100 (of 255) counts as pulled
constexpr const char* THUMBREST_LEFT = "/actions/default/in/ThumbrestTouchLeft";     // the thumb rests when touched
constexpr const char* THUMBREST_RIGHT = "/actions/default/in/ThumbrestTouchRight";

// ---- shooting without the aim stance (fire.cpp) ----
constexpr unsigned FIRE_PROBE_KEY = 0x78;   // VK_F9: the probe key, sent to the game window from the session
constexpr unsigned FIRE_FORCE_READY_KEY = 0x7A; // VK_F11 (F10 is a Windows system key and arrives differently): toggles answering "ready" to the game's fire-ready check
constexpr float FIRE_PROBE_REPORT_S = 1.0f; // how long after the probe the shot count is reported
constexpr unsigned FIRE_DIRECT_KEY = 0x77;  // VK_F8: door B, the weapon's own fire called straight
// The fire gate, read offline from this build's code (modding-notes/2026-10-24d-the-fire-gate.md) [inferred-static
// 2026-10-24]; fire.cpp checks each against the game's own UFunction answers before it touches anything.
constexpr size_t RANGED_IN_AIM_FLAG = 0x118;          // SHRangedCmbSubcomp: the byte IsInRangedAim() returns; 0 ends RequestManualFire at once
constexpr size_t WEAPON_FIRE_READY_MIN = 0x690;       // SHItemWeaponRanged: float; IsFireReady() = held item's raise value >= this
constexpr size_t WEAPON_VSLOT_IS_FIRE_READY = 0x718;  // the virtual IsFireReady the fire path calls (not the UFunction)
constexpr size_t WEAPON_VSLOT_FIRE = 0x750;           // the weapon's own fire(bool manual): rounds check, trace, shot; no aim check

// ---- no crosshair (crosshair.cpp) ----
constexpr const wchar_t* CROSSHAIR_WIDGET_CLASS =   // the game's HUD crosshair, as reticule_parameters.json names it
    L"WidgetBlueprintGeneratedClass /Game/Game/UI/GameplayHUD/Crosshairs/CrosshairContainer_WidgetBP.CrosshairContainer_WidgetBP_C";
constexpr float CROSSHAIR_FIND_S = 1.0f;       // how often the list of crosshair widgets is refreshed (a new HUD makes new ones)
constexpr int SLATE_COLLAPSED = 1;             // ESlateVisibility: Visible 0, Collapsed 1, Hidden 2, HitTestInvisible 3, SelfHitTestInvisible 4

// ---- the manual-reload port (ammo.cpp; modding-notes/2026-10-24b-manual-reloads-re8-recipe-design.md) ----
// LukasBlaster7's "MCP-confirmed" numbers for the game build he had [reported]:
constexpr size_t CLIP_ROUNDS_OFFSET = 0xBB4;   // the rounds in the gun, an int on the weapon (HasAnyAmmoInClip reads it)
constexpr const char* RELOAD_GUN_CLASSES[] = {"WeaponPistol_BP_C", "WeaponShotgun_Alt_D_BP_C", "WeaponRifle_BP_C"};

} // namespace sh2vr::settings
