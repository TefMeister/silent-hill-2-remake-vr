// buttons.cpp -- ONE button layer for the whole mod (2026-10-29, Fable; Tefa 2026-10-25: "the first thing to sort out").
//
// Before this, three layers touched the pad and one press could reach two things (Tefa: "X did this and dodged at the
// same time"): the game (B = dodge), the pack's main.lua (right stick down pressed B, the stick click became LB, LT
// became the flashlight's R3) and our C++ (reload.cpp read B for the magazine without hiding it). Now the pack's button
// rules stand down (SH2VR_PORTED.buttons, scripts/sh2vr_ported.lua) and THIS file alone decides what the game sees.
// The other modules keep reading the REAL controller through input.h (input::on_state runs before this), so the hand
// reload, the rack and the lowered-trigger fire are untouched.
//
// The game's own pad, vanilla (read live from its RebindableInput binding list, 2026-10-29; the rebinding save holds
// "None", so these are the defaults) [verified-live 2026-10-29]:
//   A = Interact / InvestigationFlip / Struggling     B = Dodge / Cancel / Skip      X = Reload
//   Y = ActionHealing / InvestigationTranscript        LB = Sprint / InvestigationPreviousItem
//   RB = QuickTurn / InvestigationNextItem             LT = RangedAim / InvestigationZoom   RT = Fire / MeleeAttack
//   right stick click = FlashlightToggle   BACK = InventoryToggle   START = MainMenu   D-pad up = MapToggle
//   D-pad left/down/right = pistol / shotgun / rifle   left stick click = nothing
//
// Tefa's layout (mod-ideas games/silent-hill-2-remake.md, "The full VR button and body layout"), in play:
//   left stick click  -> run on / off (a toggle): the game sees LB held while it is on
//   (2026-10-31: the plan's letters are QUEST letters; UEVR gives Quest X as the pad's B and Quest B as the pad's X)
//   Quest B (pad X)   -> the pistol's magazine out (reload.cpp reads it); the game never sees it
//   A                 -> interact, vault, flip an item (the game's A, as is)
//   Quest X (pad B)   -> nothing in play; back in the menus (it is the game's own B = Cancel)
//   Y                 -> the inventory (the game's BACK = InventoryToggle)
//   left stick + right stick down -> dodge that way (the game's B while the stick is down)
//   right stick up    -> quick turn (the game's RB), a setting, OFF by default
//   LT alone          -> the flashlight (the game's right stick click); LT with the left grip = the rack (rack.cpp)
//   grips             -> the hands (rack, magazine, holsters); the game never sees LB / RB from them
//   RT                -> fire (fire.cpp decides, after this)
// In a menu (the inventory, map, notes, examine, pause): A, Y as above; Quest X = back; the grips -> LB / RB (previous /
// next item in examine); LT / RT pass (zoom); the sticks pass; Quest B is kept from the game.
// [compile-verified] at most until it has run.

#include "buttons.h"

#include <cstdio>

#include "common.h"
#include "input.h"
#include "settings.h"
#include "ue.h"

namespace sh2vr::buttons {

namespace {

bool g_sprint = false;             // the toggle
bool g_ls_was = false;             // edges of the real controller, poll to poll
bool g_lt_was = false, g_rs_up_was = false;
int g_flash_pulse = 0;             // polls left to hold the flashlight click
int g_turn_pulse = 0;              // polls left to hold the quick turn
bool g_menu = false;               // the frame's menu-or-gameplay answer (set on the game thread)
WORD g_queued = 0;                 // press_for_game: what the game must see next
int g_queued_polls = 0;
bool g_keep_lg = false;            // keep_left_grip: the map is out (holsters.cpp)
bool g_map_showing = false;        // the map widget is visible (game thread)
bool g_page_lt_was = false, g_page_rt_was = false;
int g_page_prev = 0, g_page_next = 0;   // polls left of a page turn press
bool g_menu_was = false;
int g_logs = 0;
uint32_t g_polls = 0, g_rewrites = 0;
char g_why[48] = "not checked yet";

void say(const char* line) {
    if (g_logs >= settings::BUTTONS_LOG_MAX) return;
    ++g_logs;
    LOGI("%s buttons: %s", TAG, line);
}

inline bool down(const XINPUT_STATE& s, WORD b) { return (s.Gamepad.wButtons & b) != 0; }
inline void set(XINPUT_STATE& s, WORD b, bool on) { if (on) s.Gamepad.wButtons |= b; else s.Gamepad.wButtons &= (WORD)~b; }

// A menu widget has the controls when it has the user's focus: the gameplay menu (inventory, map, memos, examine)
// answered HasAnyUserFocus true with the inventory open and false after it closed [verified-live 2026-10-29, n=1 each,
// simulator]. (Its InputComponent stayed null both ways, and IsInViewport / IsVisible are true all the time.)
// The pause menu is different: its widget is CREATED when it opens and DESTROYED when it closes (it answered focus false
// while open, and "object no longer valid" after B closed it) [verified-live 2026-10-29, n=1], so for it alive = open.
bool widget_alive(const wchar_t* class_path) {
    auto* cls = ue::find_class(class_path);
    if (cls == nullptr) return false;
    for (auto* w : cls->get_objects_matching<API::UObject>(false))
        if (ue::alive(w)) return true;
    return false;
}
// The map is a section widget that answers IsVisible() true only while the map is open (its parent, the gameplay menu,
// reports no focus for it) [verified-live 2026-10-29, n=1 open / close].
bool widget_visible(const wchar_t* class_path) {
    auto* cls = ue::find_class(class_path);
    if (cls == nullptr) return false;
    for (auto* w : cls->get_objects_matching<API::UObject>(false)) {
        if (!ue::alive(w)) continue;
        bool vis = false;
        if (ue::call_ret(w, L"IsVisible", {}, vis) && vis) return true;
    }
    return false;
}
bool widget_focused(const wchar_t* class_path) {
    auto* cls = ue::find_class(class_path);
    if (cls == nullptr) return false;
    for (auto* w : cls->get_objects_matching<API::UObject>(false)) {
        if (!ue::alive(w)) continue;
        bool focused = false;
        if (ue::call_ret(w, L"HasAnyUserFocus", {}, focused) && focused) return true;
    }
    return false;
}

} // namespace

bool sprinting() { return g_sprint; }
bool menu_open() { return g_menu; }
bool map_showing() { return g_map_showing; }
void press_for_game(WORD buttons, int polls) { g_queued = buttons; g_queued_polls = polls; }
void keep_left_grip(bool keep) { g_keep_lg = keep; }

namespace {
void apply_queued(XINPUT_STATE& state) {
    if (g_queued_polls <= 0) return;
    state.Gamepad.wButtons |= g_queued;
    if (--g_queued_polls == 0) g_queued = 0;
}
} // namespace

void on_input(XINPUT_STATE& state) {
    ++g_polls;
    if (!settings::BUTTONS_LAYER_ON) return;
    const XINPUT_STATE raw = state;
    const bool lg = down(raw, XINPUT_GAMEPAD_LEFT_SHOULDER), rg = down(raw, XINPUT_GAMEPAD_RIGHT_SHOULDER);
    const bool lt = raw.Gamepad.bLeftTrigger > settings::TRIGGER_PRESSED_OVER;
    const bool ls_click = down(raw, XINPUT_GAMEPAD_LEFT_THUMB);
    const bool rs_down = raw.Gamepad.sThumbRY < -settings::STICK_DODGE_AT;
    const bool rs_up = raw.Gamepad.sThumbRY > settings::STICK_DODGE_AT;
    char line[120];

    static WORD faces_was = 0;
    const WORD faces = raw.Gamepad.wButtons & (XINPUT_GAMEPAD_A | XINPUT_GAMEPAD_B | XINPUT_GAMEPAD_X | XINPUT_GAMEPAD_Y);
    const WORD fresh = (WORD)(faces & ~faces_was);
    faces_was = faces;
    if (fresh != 0) {
        std::snprintf(line, sizeof line, "pressed:%s%s%s%s (Quest letters; the pad's A B X Y = Quest A X B Y)", (fresh & XINPUT_GAMEPAD_A) ? " Quest A" : "",
                      (fresh & settings::QUEST_X) ? " Quest X" : "", (fresh & settings::QUEST_B) ? " Quest B" : "", (fresh & XINPUT_GAMEPAD_Y) ? " Quest Y" : "");
        say(line);
    }

    if (g_menu) {
        // the menus: the game's own buttons, with Tefa's two (in QUEST letters, settings.h): Quest X = back (it IS the pad's
        // B, the game's Cancel, so it passes as it is), Quest Y = inventory; Quest B (the pad's X) kept out
        set(state, XINPUT_GAMEPAD_B, down(raw, settings::QUEST_X));
        set(state, XINPUT_GAMEPAD_X, false);
        set(state, XINPUT_GAMEPAD_BACK, down(raw, XINPUT_GAMEPAD_BACK) || down(raw, XINPUT_GAMEPAD_Y));
        set(state, XINPUT_GAMEPAD_Y, false);
        set(state, XINPUT_GAMEPAD_LEFT_THUMB, false);
        set(state, XINPUT_GAMEPAD_RIGHT_THUMB, false);
        if (g_keep_lg) {
            // the map is out (2026-10-31, Tefa): the grips hold it and never reach the game; LT / RT turn its pages
            // (the game's previous / next map = LB / RB); the triggers themselves never reach the game
            const bool rt = raw.Gamepad.bRightTrigger > settings::TRIGGER_PRESSED_OVER;
            if (lt && !g_page_lt_was) { g_page_prev = settings::BUTTONS_PULSE_POLLS; say("map: LT -> previous page (the game's LB)"); }
            if (rt && !g_page_rt_was) { g_page_next = settings::BUTTONS_PULSE_POLLS; say("map: RT -> next page (the game's RB)"); }
            g_page_lt_was = lt; g_page_rt_was = rt;
            set(state, XINPUT_GAMEPAD_LEFT_SHOULDER, g_page_prev > 0);
            set(state, XINPUT_GAMEPAD_RIGHT_SHOULDER, g_page_next > 0);
            if (g_page_prev > 0) --g_page_prev;
            if (g_page_next > 0) --g_page_next;
            state.Gamepad.bLeftTrigger = 0;
            state.Gamepad.bRightTrigger = 0;
        }
        apply_queued(state);
        g_ls_was = ls_click; g_lt_was = lt; g_rs_up_was = rs_up;
        g_flash_pulse = g_turn_pulse = 0;
        ++g_rewrites;
        return;
    }

    // ---- in play ----
    // run: the stick click toggles; the game sees LB held while it is on (the grips never reach LB / RB)
    if (ls_click && !g_ls_was) {
        g_sprint = !g_sprint;
        say(g_sprint ? "left stick click -> run ON (the game sees LB held)" : "left stick click -> run off");
    }
    g_ls_was = ls_click;
    set(state, XINPUT_GAMEPAD_LEFT_SHOULDER, g_sprint);
    set(state, XINPUT_GAMEPAD_LEFT_THUMB, false);

    // the face buttons
    set(state, XINPUT_GAMEPAD_B, false);                 // Quest X: nothing in play for now; never the game's dodge
    set(state, XINPUT_GAMEPAD_X, false);                 // Quest B: the magazine (reload.cpp reads it); never the game's reload
    set(state, XINPUT_GAMEPAD_BACK, down(raw, XINPUT_GAMEPAD_BACK) || down(raw, XINPUT_GAMEPAD_Y));   // Y = inventory
    set(state, XINPUT_GAMEPAD_Y, false);
    if (down(raw, XINPUT_GAMEPAD_Y) && !input::held(XINPUT_GAMEPAD_Y)) say("Y -> the inventory (the game's BACK)");

    // dodge: the left stick gives the direction, right stick down is the press (the game's B)
    if (rs_down) set(state, XINPUT_GAMEPAD_B, true);
    if (rs_down && g_polls % settings::BUTTONS_LOG_EVERY_POLLS == 0) say("right stick down -> dodge (the game's B)");

    // quick turn: right stick up, a setting
    if (settings::QUICK_TURN_ON && rs_up && !g_rs_up_was) { g_turn_pulse = settings::BUTTONS_PULSE_POLLS; say("right stick up -> quick turn (the game's RB)"); }
    g_rs_up_was = rs_up;
    set(state, XINPUT_GAMEPAD_RIGHT_SHOULDER, g_turn_pulse > 0);
    if (g_turn_pulse > 0) --g_turn_pulse;
    state.Gamepad.sThumbRY = 0;                          // the right stick's up / down is ours; the pack keeps its turn (X)

    // the flashlight: LT alone (with the left grip it is the rack, rack.cpp); the game's aim stance is never asked for
    if (lt && !g_lt_was && !lg) { g_flash_pulse = settings::BUTTONS_PULSE_POLLS; say("left trigger alone -> flashlight (the game's right stick click)"); }
    g_lt_was = lt;
    set(state, XINPUT_GAMEPAD_RIGHT_THUMB, g_flash_pulse > 0);
    if (g_flash_pulse > 0) --g_flash_pulse;
    state.Gamepad.bLeftTrigger = 0;
    apply_queued(state);
    (void)rg;
    ++g_rewrites;
    (void)line;
}

void tick(float) {
    const bool no_pawn = API::get()->get_local_pawn(0) == nullptr;
    bool open = no_pawn;
    const char* why = no_pawn ? "no player yet" : "play";
    if (!no_pawn) {
        if (widget_focused(settings::MENU_WIDGET_CLASS)) { open = true; why = "the gameplay menu has the focus"; }
        else if (widget_focused(settings::SAVE_MENU_WIDGET_CLASS) || widget_visible(settings::SAVE_MENU_WIDGET_CLASS)) { open = true; why = "the save menu is open"; }   // 2026-10-31: the focus check missed it (X / B were dead in it); IsVisible is false while closed [measured 2026-10-31]
        else if (widget_alive(settings::PAUSE_WIDGET_CLASS)) { open = true; why = "the pause menu is up"; }
        else if (widget_visible(settings::MAP_SECTION_CLASS)) { open = true; why = "the map is open"; }
    }
    g_menu = open;
    g_map_showing = !no_pawn && widget_visible(settings::MAP_SECTION_CLASS);
    if (open != g_menu_was) {
        g_menu_was = open;
        std::snprintf(g_why, sizeof g_why, "%s", why);
        char line[160];
        std::snprintf(line, sizeof line, "%s (%s): %u polls, %u rewritten; run %s", open ? "MENU buttons" : "PLAY buttons", why, g_polls, g_rewrites, g_sprint ? "on" : "off");
        say(line);
    }
}

} // namespace sh2vr::buttons
