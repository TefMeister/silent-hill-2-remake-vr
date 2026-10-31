// fire.h -- shooting without the aim stance (Tefa 2026-10-24: walk and run normally with the gun up two-handed, and
// shoot without the right grip; the Visceral RE2 way, where the game's own internal switches fire the round and the
// character never enters the aim state). Pass 0 was the probe; pass 2 (2026-10-24 16:30) makes the right trigger fire
// a lowered gun through the weapon's own fire function (door B, proven live on the simulator).
#pragma once

#include <windows.h>
#include <Xinput.h>

#include <string>

#include "common.h"

namespace sh2vr::fire {

void install();                                  // hooks the weapon's "a shot was performed" so a round is proven
void on_message(UINT msg, WPARAM wparam);        // the probe keys
void on_input(XINPUT_STATE& state);              // the right trigger, before the game sees it (plugin on_xinput_get_state)
void tick(float delta);                          // the after-shot report
std::string combat_state();                      // one line: the combat part's mode / pose / aim, the gun's rounds (game thread)

} // namespace sh2vr::fire
