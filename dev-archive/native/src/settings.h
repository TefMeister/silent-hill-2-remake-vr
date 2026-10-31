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

// ---- the button layer (buttons.cpp, 2026-10-29) ----
constexpr bool BUTTONS_LAYER_ON = true;        // one layer decides what the game sees; off = the raw pad as before
constexpr bool QUICK_TURN_ON = false;          // right stick up = quick turn (Tefa: a setting, off by default)
constexpr int STICK_DODGE_AT = 20000;          // the right stick this far down (of 32767) = the dodge press; up = quick turn
constexpr int BUTTONS_PULSE_POLLS = 3;         // a pressed button the game must see for this many polls (flashlight, quick turn)
constexpr int BUTTONS_LOG_MAX = 80;            // log lines about translated presses, per run
constexpr int BUTTONS_LOG_EVERY_POLLS = 60;    // a held dodge is logged this often
// the widgets that take the controls (the menu check); class paths read live 2026-10-29 [verified-live]
constexpr const wchar_t* MENU_WIDGET_CLASS = L"WidgetBlueprintGeneratedClass /Game/Game/UI/GameplayMenu/GameplayMenu_Widget.GameplayMenu_Widget_C";       // inventory, map, memos, examine: lives on, HAS THE FOCUS while open
constexpr const wchar_t* SAVE_MENU_WIDGET_CLASS = L"WidgetBlueprintGeneratedClass /Game/Game/UI/GameplaySaveMenu/GameplaySaveMenu_Widget.GameplaySaveMenu_Widget_C";   // the save points' menu: same kind
constexpr const wchar_t* PAUSE_WIDGET_CLASS = L"WidgetBlueprintGeneratedClass /Game/Game/UI/Menu/InGameMenuWidget_BP.InGameMenuWidget_BP_C";               // the pause menu (RESUME / LOAD / SETTINGS ...): EXISTS only while open
constexpr const wchar_t* MAP_SECTION_CLASS = L"WidgetBlueprintGeneratedClass /Game/Game/UI/GameplayMenu/Sections/MenuSectionMap_Widget.MenuSectionMap_Widget_C";   // the map (d-pad up): IsVisible() only while open; its parent menu reports no focus then

// ---- the weapons on the body (holsters.cpp, 2026-10-29) ----
constexpr bool HOLSTERS_ON = true;
struct HolsterSpot { const wchar_t* name; double right_cm, up_cm, forward_cm; const wchar_t* row; };   // from the headset; the weapon row
constexpr HolsterSpot HOLSTER_SPOTS[] = {
    {L"right hip", 20.0, -35.0, 10.0, L"Pistol"},
    {L"right shoulder", 15.0, -10.0, -10.0, L"Shotgun"},
    {L"left shoulder", -15.0, -10.0, -10.0, L"Rifle"},
    {L"left hip", -20.0, -35.0, 10.0, L"Melee"},
};
constexpr int HOLSTER_SPOT_COUNT = 4;
constexpr const wchar_t* MELEE_ROW = L"Melee";   // a spot with this row draws the game's last-used melee weapon (IronPipe, WoodenPlank, Chainsaw...)
constexpr double HOLSTER_ENTER_CM = 18.0, HOLSTER_LEAVE_CM = 26.0;   // the hand counts as in the spot / out of it
constexpr const wchar_t* WEAPON_MANAGER_CLASS = L"Class /Script/SHProto.SHWeaponManageCmbSubcomp";
constexpr const wchar_t* UNEQUIP_FN = L"RequestUnequipWeapon";      // read live 2026-10-29; see holsters.cpp for the fallback
constexpr unsigned HOLSTER_TEST_KEY = 0x75;   // VK_F6: a pretend right grip, for the simulator (no grip there)
constexpr unsigned HOLSTER_LEFT_TEST_KEY = 0x74;   // VK_F5: a pretend LEFT grip
// the left grip's items (2026-10-29, Tefa's table; 2026-10-31 Tefa: "ammo pouch needs to be on the left hip, can the map
// go to right shoulder with LG activating it?"): the map at the right shoulder; the healing item (drink or syringe) at
// the right hip, Tefa's settled way: a left grip press takes the drink out, keep holding 2 s and it is the syringe, LT
// uses what is in the hand (the game's one healing action, Y), letting go puts it back unused; the left hip is the ammo's only (the pistol magazine, the shotgun and rifle rounds one by one); the left
// shoulder's flashlight stays on LT for now (a hand-held flashlight is a later piece)
constexpr const wchar_t* LEFT_ITEM_MAP_SPOT = L"right shoulder";
constexpr const wchar_t* LEFT_ITEM_HEAL_SPOT = L"right hip";
constexpr float HEAL_SYRINGE_HOLD_S = 2.0f;    // the left grip held this long at the healing spot: the drink becomes the syringe
constexpr double USE_AT_WRIST_CM = 15.0;      // the syringe: the left hand this close to the right hand = used
constexpr double MOUTH_FORWARD_CM = 12.0, MOUTH_DOWN_CM = 10.0, USE_AT_MOUTH_CM = 15.0;   // the drink: at the mouth = drunk
constexpr int HOLSTER_PRESS_POLLS = 3;        // how many pad polls a press the game must see lasts
constexpr int HOLSTER_LOG_MAX = 80;
constexpr bool HOLSTER_LOG_EMPTY_GRIPS = true;
constexpr float HOLSTER_STATUS_S = 5.0f;
// the right grip held away from every spot with a gun out = "show the ammo" (2026-10-31, Tefa: "show ammo counter and
// optionally the crosshair while RG is held, without it triggering the AIM stance"); the on-screen part is still to come
constexpr bool SHOW_CROSSHAIR_ON_GRIP = false;   // the optional crosshair while the grip is held (not built yet)

// ---- shooting without the aim stance (fire.cpp) ----
constexpr unsigned FIRE_PROBE_KEY = 0x78;   // VK_F9: the probe key, sent to the game window from the session
constexpr unsigned FIRE_FORCE_READY_KEY = 0x7A; // VK_F11 (F10 is a Windows system key and arrives differently): toggles answering "ready" to the game's fire-ready check
constexpr float FIRE_PROBE_REPORT_S = 1.0f; // how long after the probe the shot count is reported
constexpr unsigned FIRE_DIRECT_KEY = 0x77;  // VK_F8: door B, the weapon's own fire called straight
constexpr bool FIRE_LOWERED_ON = true;      // the right trigger fires a lowered gun through door B (Tefa: the RE2 way)
constexpr float FIRE_MIN_INTERVAL_S = 0.15f; // our own pace between lowered shots (door B skips the game's cooldown)
constexpr int FIRE_LOWERED_LOG_FIRST = 20;  // how many lowered shots get a full log line
constexpr unsigned FIRE_TRIGGER_TEST_KEY = 0x76; // VK_F7: a pretend trigger pull through the same path (the simulator's pad never reaches the plugin)
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
constexpr int CLIP_SIZE_FALLBACK = 10;                  // the pistol's WeaponClipSize when the property cannot be read
constexpr const wchar_t* AMMO_ROW_FALLBACK = L"HandgunAmmo";   // the carried-ammo item row when the weapon does not name one

// ---- the manual reloads, the RE8 recipe (rack.cpp, reload.cpp, ghosthand.cpp; 2026-10-24) ----
// distances in the game's centimetres; Lukas's numbers for his build [reported]; the hand fits are the pack's sliders
constexpr const wchar_t* PISTOL_CLASS = L"WeaponPistol_BP_C";
constexpr const wchar_t* SLIDE_SOCKET = L"slide_joint";
constexpr const wchar_t* MAG_SOCKET = L"magazine_joint";
constexpr const wchar_t* MUZZLE_SOCKET = L"FX_muzzle";
constexpr const wchar_t* MAG_BONES[] = {L"magazine_joint", L"magazine_socket_joint"};   // hidden while the magazine is out
constexpr double SLIDE_GRAB_CM = 22.0;       // the left controller this close to the slide takes it (left grip)
constexpr double SLIDE_TRAVEL_CM = 12.0;     // a full pull
constexpr double RACK_PULLED_AT = 0.97;      // travel that counts as fully back
constexpr double RACK_FOLLOW = 0.6;          // how fast the slide follows the hand, per tick
constexpr double RACK_SPRING_STEP = 0.1;     // how fast a let-go slide springs forward, per tick
constexpr double SLIDE_HAND_LOC[3] = {0.0, 0.0, 0.0};   // the hand on the slide, in the slide socket's frame (the pack's sliders)
constexpr double SLIDE_HAND_ROT[3] = {0.0, 0.0, 90.0};  // pitch, yaw, roll: roll 90 = the palm on top of the slide
constexpr double MAG_DOCK_CM = 18.0;         // a magazine this close to its seat goes in
constexpr double HIP_RIGHT_CM = -20.0, HIP_UP_CM = -35.0, HIP_FORWARD_CM = 10.0;   // the ammo spot from the headset (the pack's holster)
constexpr double HIP_ENTER_CM = 18.0, HIP_LEAVE_CM = 28.0;
constexpr double HAND_MAG_LOC[3] = {4.0, 1.5, -2.0};    // the magazine on the left controller (the pack's fit)
constexpr double HAND_MAG_ROT[3] = {-15.0, 90.0, 0.0};
constexpr const wchar_t* HAND_MAG_MESH = L"StaticMesh /Game/Game/Art/Items/Handgun/HandgunAmmoClosed_01_A.HandgunAmmoClosed_01_A";
constexpr float RELOAD_GRAB_COOLDOWN_S = 0.5f;
constexpr float BUZZ_GRAB_AMP = 0.5f, BUZZ_DONE_AMP = 1.0f, BUZZ_DENY_AMP = 0.3f, BUZZ_S = 0.08f, BUZZ_HZ = 1000.0f;
constexpr bool GHOST_HAND_ON = true;         // the Ghost Hand BeG0nE recorder (the rule for any mod with hand gestures)
constexpr double GHOST_MAP_MIN_CM = 15.0;    // the hands this far apart give the world-to-room turn; closer keeps the last one

} // namespace sh2vr::settings
