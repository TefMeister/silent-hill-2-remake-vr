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

} // namespace sh2vr::buttons
