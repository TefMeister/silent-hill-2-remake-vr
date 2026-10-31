// holsters.h -- the weapons live on the body: the right grip pressed in a spot draws its weapon, pressed there again puts
// it back; letting go of the grip leaves the weapon in the hand (2026-10-29; 2026-10-31 Tefa). See holsters.cpp.
#pragma once

#include <Windows.h>

namespace sh2vr::holsters {

// Once a frame, on the game thread, after input::on_state has the real controller.
void tick(float delta);

// F6 from the game window: a pretend right grip, pressed / let go (the simulator has no grip).
void on_message(UINT msg, WPARAM wparam);

// A weapon drawn from a spot is in the hand (fire and the hands may ask).
bool holding();
// The right grip is held away from every spot with a gun out: the ammo is shown (2026-10-31; the on-screen part is to come).
bool showing_ammo();
// The right grip holds the healing item from the lower back (2026-10-31): RT uses it, so it must not fire.
bool heal_in_hand();
// The left grip holds the map open (2026-10-31: taken at the right hip); twohand.cpp places it.
bool map_held();
// The left hand's distance to a spot given from the headset (cm right, up, forward), measured like the holsters.
bool left_hand_to_spot_cm(double right_cm, double up_cm, double forward_cm, double& out_cm);
// The grips as the holsters read them (the real controller, or F5 / F6 on the simulator).
bool left_grip();
bool right_grip();

} // namespace sh2vr::holsters
