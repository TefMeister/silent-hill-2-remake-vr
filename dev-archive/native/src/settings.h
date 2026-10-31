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

// ---- the Quest 3 face buttons as the pad sees them (2026-10-31, Tefa in the headset: "quest 3 X Y B A xbox in same order
// B Y X A"; the log agrees: Quest X ejected the magazine through the pad's B) [verified-live 2026-10-31, headset]. Tefa's
// plan names QUEST letters, so the mod reads these two names, never the pad letters directly.
constexpr const char* VR_GRIP_ACTION = "/actions/default/in/Grip";   // UEVR's own grip action (input::grip)
constexpr unsigned short QUEST_X = 0x2000;   // XINPUT_GAMEPAD_B: Quest X (left, lower) = back in the menus
constexpr unsigned short QUEST_B = 0x4000;   // XINPUT_GAMEPAD_X: Quest B (right, upper) = the pistol's magazine out

// ---- the weapons on the body (holsters.cpp, 2026-10-29) ----
constexpr bool HOLSTERS_ON = true;
constexpr const wchar_t* HEAL_ROW = L"";   // a spot with this row holds the healing item, not a weapon
struct HolsterSpot { const wchar_t* name; double right_cm, up_cm, forward_cm; const wchar_t* row; };   // from the headset; the weapon row
constexpr HolsterSpot HOLSTER_SPOTS[] = {
    {L"right hip", 20.0, -35.0, 10.0, L"Pistol"},
    {L"right shoulder", 15.0, -10.0, -10.0, L"Shotgun"},
    {L"left shoulder", -15.0, -10.0, -10.0, L"Rifle"},
    {L"left hip", -20.0, -35.0, 10.0, L"Melee"},
    // the healing item (2026-10-31, Tefa: "move the health drink and syringe to the lower back and little to the right like
    // re8 right now and sub weapon in re2"): RE2's BACK spot (0.12 right, as low as the hip, 0.30 behind); no weapon row
    {L"lower back", 12.0, -35.0, -30.0, HEAL_ROW},
};
constexpr int HOLSTER_SPOT_COUNT = 5;
constexpr const wchar_t* MELEE_ROW = L"Melee";   // a spot with this row draws the game's last-used melee weapon (IronPipe, WoodenPlank, Chainsaw...)
constexpr double HOLSTER_ENTER_CM = 18.0, HOLSTER_LEAVE_CM = 26.0;   // the hand counts as in the spot / out of it
constexpr const wchar_t* WEAPON_MANAGER_CLASS = L"Class /Script/SHProto.SHWeaponManageCmbSubcomp";
constexpr const wchar_t* UNEQUIP_FN = L"RequestUnequipWeapon";      // read live 2026-10-29; see holsters.cpp for the fallback
constexpr unsigned HOLSTER_TEST_KEY = 0x75;   // VK_F6: a pretend right grip, for the simulator (no grip there)
constexpr unsigned HOLSTER_LEFT_TEST_KEY = 0x74;   // VK_F5: a pretend LEFT grip
// the left grip's items (2026-10-29, Tefa's table; 2026-10-31 Tefa: "ammo pouch needs to be on the left hip, can the map
// go to right shoulder with LG activating it?"): the map at the right shoulder; the left hip is the ammo's only (the
// pistol magazine, the shotgun and rifle rounds one by one); the left shoulder's flashlight stays on LT for now.
// The healing item is the RIGHT hand's since 2026-10-31 (the lower back spot above; the game itself heals with the right
// hand): a right grip press there takes the drink out, keep holding and it is the syringe, RT uses what is in the hand
// (the game's one healing action, Y), letting go of the grip puts it back unused.
constexpr const wchar_t* LEFT_ITEM_MAP_SPOT = L"right hip";   // 2026-10-31 13:00, Tefa: "move the map to the right hip, LG to take it out, right hand with RG held to two hand the map"
constexpr float MAP_TOGGLE_COOLDOWN_S = 1.0f;   // after taking the map out or putting it away, LG at the hip is ignored this long (14:32: put away, out again 0.4 s later)
constexpr float MAP_GONE_S = 0.5f;          // the map not showing this long after it showed: closed another way (Quest X)
constexpr float MAP_NEVER_CAME_S = 3.0f;    // taken out but never showed in this long: give up
constexpr float HEAL_SYRINGE_HOLD_S = 0.5f;    // (Tefa 2026-10-31: "2 seconds feels like forever") the right grip held this long at the lower back: the drink becomes the syringe
// 2026-10-31 evening (Tefa 10-31: "medicine also didn't work"; the log: the buzz, nothing in the hand): the item's own model in
// the right hand, found among the game's loaded static meshes by these names (the pack's attachment keys
// ItemBase_BP_C_HealthDrink_01_B / ItemBase_BP_C_Syringe_03_A). Not found = a log line listing what is loaded.
constexpr const wchar_t* HEAL_DRINK_MESH_NAME = L"HealthDrink_01_B";
constexpr const wchar_t* HEAL_SYRINGE_MESH_NAMES[] = {L"Syringe_03_A", L"Syringe_02_A"};
constexpr double HEAL_MESH_LOC[3] = {4.0, 0.0, -2.0};   // on the right controller; guesses, to tune in the headset
constexpr double HEAL_MESH_ROT[3] = {0.0, 0.0, 0.0};
constexpr float HEAL_MESH_SEARCH_RETRY_S = 10.0f;      // a name not loaded is looked for again after this long
constexpr int HOLSTER_PRESS_POLLS = 3;        // how many pad polls a press the game must see lasts
constexpr int HOLSTER_LOG_MAX = 80;
constexpr bool HOLSTER_LOG_EMPTY_GRIPS = true;
constexpr float HOLSTER_STATUS_S = 5.0f;
// the right grip held away from every spot with a gun out = "show the ammo" (2026-10-31, Tefa: "show ammo counter and
// optionally the crosshair while RG is held, without it triggering the AIM stance"); the on-screen part is still to come
constexpr bool SHOW_CROSSHAIR_ON_GRIP = false;   // the optional crosshair while the grip is held (not built yet)

// ---- two hands on one thing (twohand.cpp, 2026-10-31, from praydog's original SH2 profile) ----
constexpr bool TWO_HAND_ON = true;
constexpr double TWO_HAND_BARREL_AXIS[3] = {0.0, 1.0, 0.0};   // the long guns' barrel in the gun mesh's own frame (praydog: forward turned 90 deg about up); checked live by the press log
constexpr double TWO_HAND_MIN_CM = 12.0, TWO_HAND_MAX_CM = 90.0;   // the left hand this far from the right to take the gun's front
constexpr double TWO_HAND_CONE_DEG = 50.0;    // and within this angle of the barrel (40 let the simulator's rifle in at exactly 40, 2026-10-31)
constexpr bool TWO_HAND_AIM_AT_DRAWN_HAND = true;   // aim at the drawn left hand, not the controller point
constexpr bool TWO_HAND_TURN_RIGHT = true;   // the drawn right hand turns with the two-handed long gun
constexpr bool TWO_HAND_LOCK_LEFT = true;    // the drawn left hand stays on the gun's line at its starting distance (until the pump is built)
// 2026-10-31 evening (Tefa: "is there a way to lock the left hand onto a pump handle and rifle front, right now i can
// move my wrist freely"): the drawn left hand also takes its TURN from the gun, the turn it had against the gun at the
// grab, kept from then on (the wrist no longer follows the controller). On the shotgun it slides back along the gun
// with the controller, up to TWO_HAND_PUMP_SLIDE_CM, so the hand rides the pump; on the rifle it stays put.
constexpr bool TWO_HAND_LOCK_TURN = true;
constexpr double TWO_HAND_PUMP_SLIDE_CM = 10.0;   // the pack's pump counts 8 cm of pull (shotgun_pump.lua PUMP_TRAVEL)
constexpr double MAP_TWO_HAND_TILT_DEG = 45.0; // praydog's tilt of the two-handed map
constexpr float TWO_HAND_REPORT_S = 3.0f;
constexpr int TWO_HAND_LOG_MAX = 80;

// ---- shooting without the aim stance (fire.cpp) ----
constexpr unsigned FIRE_PROBE_KEY = 0x78;   // VK_F9: the probe key, sent to the game window from the session
constexpr unsigned FIRE_FORCE_READY_KEY = 0x7A; // VK_F11 (F10 is a Windows system key and arrives differently): toggles answering "ready" to the game's fire-ready check
constexpr float FIRE_PROBE_REPORT_S = 1.0f; // how long after the probe the shot count is reported
constexpr unsigned FIRE_DIRECT_KEY = 0x72;  // VK_F3 (was F8 until 2026-10-31: F8 is the 21 Sacraments patch's own on/off key): door B, the weapon's own fire called straight
constexpr bool FIRE_LOWERED_ON = true;      // the right trigger fires a lowered gun through door B (Tefa: the RE2 way)
constexpr float FIRE_MIN_INTERVAL_S = 0.15f; // our own pace between lowered pistol shots (door B skips the game's cooldown)
// 2026-10-31 evening (Tefa 10-31: "fire rate is still really fast"): the long guns' own pace. The real gate is the pump / the
// bolt (the pack's scripts, through the bridge's gate); these are the floor under it, and they cover the moment
// before the pack's "pump owed" reaches the plugin. Guesses, to tune in the headset.
constexpr float FIRE_MIN_INTERVAL_SHOTGUN_S = 0.8f;
constexpr float FIRE_MIN_INTERVAL_RIFLE_S = 1.0f;
// the empty gun's click (Tefa 10-31, fault 5): the trigger on an empty gun plays this and never calls the weapon's fire
// (the pack's pistol reload uses the same event for its dry fire; no shotgun / rifle event is known yet)
constexpr const wchar_t* DRY_CLICK_EVENT = L"AkAudioEvent /Game/WWiseAudio/Events/Characters/James/James_Weapons/Pistol/Play_Wpn_Pistol_Shot_No_Ammo.Play_Wpn_Pistol_Shot_No_Ammo";
constexpr float BUZZ_CLICK_AMP = 0.25f;
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
// 2026-10-31 evening (Tefa 10-31: "the mag looks like the actual ammo box from the game and not the pistol mag"): the hand
// magazine is a copy of the pistol's own model with every part but the magazine shrunk away and hidden (LukasBlaster7's
// eject visual, main.lua showOnlyPistolMagazine); HAND_MAG_MESH above stays as the invisible carrier of the fit, and is
// shown only when the copy cannot be made.
constexpr bool HAND_MAG_FROM_PISTOL = true;
constexpr const wchar_t* HAND_MAG_BONE = L"magazine_joint";
constexpr const wchar_t* HAND_MAG_KEEP[] = {L"magazine_joint", L"magazine_socket_joint"};
constexpr const wchar_t* HAND_MAG_ANCESTORS[] = {L"Holder", L"offset_joint"};   // kept unhidden (shrunk only), or the magazine under them would hide too
constexpr float HAND_MAG_BOUNDS_SCALE = 16.0f;
constexpr float RELOAD_GRAB_COOLDOWN_S = 0.5f;
constexpr float BUZZ_GRAB_AMP = 0.5f, BUZZ_DONE_AMP = 1.0f, BUZZ_DENY_AMP = 0.3f, BUZZ_S = 0.08f, BUZZ_HZ = 1000.0f;
constexpr bool GHOST_HAND_ON = true;         // the Ghost Hand BeG0nE recorder (the rule for any mod with hand gestures)
constexpr double GHOST_MAP_MIN_CM = 15.0;    // the hands this far apart give the world-to-room turn; closer keeps the last one

} // namespace sh2vr::settings
