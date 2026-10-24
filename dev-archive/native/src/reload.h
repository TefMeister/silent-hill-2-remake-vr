// reload.h -- the pistol magazine by hand, the RE8 recipe (2026-10-24; modding-notes/2026-10-24b-...-design.md):
//   right B          the magazine comes out (the gun reads empty; its rounds stay with that magazine)
//   left grip in the ammo spot on the left hip (from the headset)   a magazine in the left hand (the game's own
//                    HandgunAmmoClosed mesh on the left controller)
//   bring it to the gun's magazine seat   it goes in: the old magazine's rounds come back, topped up from the ammo
//                    carried (the game's own item count goes down by what was taken)
//   let go of the left grip with it in hand   it drops (its rounds wait at the spot)
//   A magazine into a gun that was EMPTY needs a rack (rack.cpp) before it fires.
// The game's own reload (the X button and the body animation) is kept out while a pistol is managed.
#pragma once

#include "common.h"

namespace sh2vr::reload {

void tick(float delta);       // before rack::tick
bool managed();               // the gun in hand reloads by hand (a pistol, headset on)
bool mag_out();               // the magazine is not seated
bool mag_in_hand();
bool hand_busy();             // the left hand carries a magazine: the slide must not take it
bool blocks_fire();           // the magazine is out: the trigger waits
void on_racked();             // rack.cpp: the slide cycled
void reset(const char* why);

} // namespace sh2vr::reload
