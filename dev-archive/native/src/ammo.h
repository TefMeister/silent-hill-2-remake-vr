// ammo.h -- the rounds in the gun, step 1 of the manual-reload port (modding-notes/2026-10-24b-manual-reloads-re8-recipe-design.md).
// LukasBlaster7's scripts keep a gun's loaded rounds as an int at weapon +0xBB4 (the game's HasAnyAmmoInClip reads it)
// and write it to refill. Before any C++ writes there, this READS it at every shot and checks it against the game's own
// HasAnyAmmoInClip, so one run proves (or disproves) the door. Read only: nothing is written yet.
#pragma once

#include "common.h"

namespace sh2vr::ammo {

bool known_gun(API::UObject* weapon);        // one of the three guns Lukas mapped (pistol, shotgun, rifle)
int rounds(API::UObject* weapon);            // the rounds at +0xBB4, or -1 when the gun is not one of the three
void check(API::UObject* weapon, const char* when);   // one log line: the rounds, the game's answer, agree or not

} // namespace sh2vr::ammo
