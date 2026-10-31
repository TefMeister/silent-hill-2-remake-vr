// buttons.h -- ONE button layer for the whole mod (2026-10-29). See buttons.cpp.
#pragma once

#include <Windows.h>
#include <Xinput.h>

namespace sh2vr::buttons {

// From the plugin's on_xinput_get_state, after input::on_state has kept the real controller: rewrites `state` into
// what the game is allowed to see (Tefa's layout), every poll.
void on_input(XINPUT_STATE& state);

// Once a frame, on the game thread: the menu-or-gameplay check and the log lines.
void tick(float delta);

// The sprint toggle is on (the left stick was clicked): the game sees the sprint button held.
bool sprinting();

// A menu, the inventory, the map, a note or an examine screen has the controls (buttons pass through as menu buttons).
bool menu_open();
// The map is showing on screen (2026-10-31: holsters.cpp notices Quest X closing it).
bool map_showing();

// Another module asks the game to see these pad buttons (XINPUT_GAMEPAD_* mask) for `polls` polls, in play or in a menu
// (the holsters: the map toggle, the healing action). Queued, applied on the next polls.
void press_for_game(WORD buttons, int polls);

// While true the real left grip never reaches the game as LB, in menus either (the holsters hold the map with it).
void keep_left_grip(bool keep);

} // namespace sh2vr::buttons
