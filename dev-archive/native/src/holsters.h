// holsters.h -- the weapons live on the body: the right grip draws one from its spot and holds it; let go and it
// goes back (2026-10-29, Tefa's "full VR button and body layout"). See holsters.cpp.
#pragma once

#include <Windows.h>

namespace sh2vr::holsters {

// Once a frame, on the game thread, after input::on_state has the real controller.
void tick(float delta);

// F6 from the game window: a pretend right grip, pressed / let go (the simulator has no grip).
void on_message(UINT msg, WPARAM wparam);

// The right grip holds a weapon drawn from a spot (fire and the hands may ask).
bool holding();

} // namespace sh2vr::holsters
