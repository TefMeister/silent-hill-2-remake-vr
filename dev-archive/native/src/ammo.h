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

// the reload's doors (2026-10-24 16:50; Lukas's scripts proved each on his build [reported])
void set_rounds(API::UObject* weapon, int n);        // writes +0xBB4 (refused for a gun that is not one of the three)
int clip_size(API::UObject* weapon);                 // WeaponClipSize
int carried(API::UObject* weapon);                   // the ammo carried, from the pawn's Items component (-1 = unknown)
int take_carried(API::UObject* weapon, int n);       // removes up to n from the carried ammo; returns what was taken
void out_of_ammo_visual(API::UObject* weapon, bool empty);   // the gun's own empty pose (the pistol's slide locked back)
void fill(API::UObject* weapon, int n);              // rounds = n, the empty pose off when n > 0

} // namespace sh2vr::ammo
