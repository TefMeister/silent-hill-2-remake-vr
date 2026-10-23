// fire.h -- shooting without the aim stance (Tefa 2026-10-24: walk and run normally with the gun up two-handed, and
// shoot without the right grip; the Visceral RE2 way, where the game's own internal switches fire the round and the
// character never enters the aim state). Pass 0 is the probe: on a key, ask the game's ranged combat part to fire the
// equipped gun while it is lowered, and log every answer, so a run can tell whether the door exists.
#pragma once

#include <windows.h>

#include "common.h"

namespace sh2vr::fire {

void install();                                  // hooks the weapon's "a shot was performed" so a round is proven
void on_message(UINT msg, WPARAM wparam);        // the probe key
void tick(float delta);                          // the after-shot report

} // namespace sh2vr::fire
