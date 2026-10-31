// input.h -- the controller's buttons as the plugin's XInput hook last saw them (the raw state, before fire.cpp keeps
// anything from the game), read by the reload modules on the game thread. UEVR maps the VR controllers onto one
// XInput pad: left grip = left shoulder, right grip = right shoulder, triggers = triggers, right B = B.
#pragma once

#include <windows.h>
#include <Xinput.h>

namespace sh2vr::input {

void on_state(const XINPUT_STATE& state);   // every poll, from the plugin's on_xinput_get_state
void end_tick();                            // once a frame, after the modules ran: the edges reset

bool held(WORD button);                     // XINPUT_GAMEPAD_* mask
bool pressed(WORD button);                  // held now, not held at the last end_tick
bool left_trigger();
bool right_trigger();
bool left_trigger_pressed();
bool right_trigger_pressed();
// A grip held: the pad's LB / RB OR UEVR's own VR grip action on that hand (2026-10-31: when the map opened, the pad's LB
// read "let go" 33 ms later while Tefa still held the grip; the VR action does not go through the pad at all).
bool grip(bool right);

} // namespace sh2vr::input
