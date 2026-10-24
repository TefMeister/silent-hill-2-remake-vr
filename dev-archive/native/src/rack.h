// rack.h -- the pistol slide, the RE8 recipe (2026-10-24; modding-notes/2026-10-24b-manual-reloads-re8-recipe-design.md;
// Tefa: "LG puts the hand on the weapon part, LT makes it rackable"). Moved over from Visceral RE8's rack.cpp in shape:
//   when the gun is empty, or a magazine went into an empty gun: left grip with the left controller near the slide
//   puts the hand ON it (the left IK target rides the slide socket); LT held = the pull is live; move the left
//   controller back along the barrel; once fully back, letting LT (or the grip) go sends the slide home: the gun is
//   chambered and the trigger works again. The pump and the bolt come later (lock.cpp first).
// The slide itself is the game's own: while open it shows the empty pose (SetOutOfAmmo), closed on completion.
#pragma once

#include "common.h"

namespace sh2vr::rack {

void tick(float delta);       // before ik::tick: the gesture, the left-hand override
void need(const char* why);   // reload.cpp: a rack is required before the gun fires
bool needed();
bool active();                // the left hand is on the slide
double travel();              // 0 = rest, 1 = fully back
void reset(const char* why);

} // namespace sh2vr::rack
