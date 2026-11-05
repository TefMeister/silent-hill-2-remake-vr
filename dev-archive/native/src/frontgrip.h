// frontgrip.h -- the left hand on the shotgun's pump and the rifle's front, held the way the GAME holds it (2026-11-05).
// Tefa: "the only way i can see the hand firmly gripping the rifle and shotgun front handle, is to use the games own
// grip independant of the wrist". The game's own aim pose is read once per gun (the hidden body's left hand against the
// weapon, and every left finger), saved in data/sh2vr_front_grips.json, and while twohand.cpp holds a long gun in two
// hands the drawn left hand and its fingers are put exactly there: the wrist no longer follows the controller at all.
#pragma once

#include <string>

#include "ue.h"

namespace sh2vr::frontgrip {

// The capture (fingers only): the equipped long gun, the game's hidden body holding it with its left hand ON the gun's
// Holder_L (checked; refused otherwise), FRONT_GRIP_CAPTURE_KEY to the game window. Writes or updates that gun's entry.
void on_message(unsigned msg, unsigned long long wparam);

// "Shotgun" / "Rifle" for a weapon whose class names it, else "" (no front grip)
std::string key_of(API::UObject* weapon);

// Where the left hand bone goes for this gun: the gun mesh's own left-hand socket (Holder_L, read live, so it rides the
// pump), else the saved place; `slide_cm` moves it back along the barrel (the shotgun's pump by hand). False: no grip.
bool target(const std::string& key, API::UObject* gun_mesh, const ue::Transform& gun_world, double slide_cm, ue::Vec3& pos, ue::Quat& rot);

void set_active(const std::string& key);   // the gun whose fingers are held in the game's pose, "" for none
void tick();                               // after fingers::tick: the left fingers in the game's pose while active

} // namespace sh2vr::frontgrip
