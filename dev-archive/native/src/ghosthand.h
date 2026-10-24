// ghosthand.h -- Ghost Hand BeG0nE recorder for Silent Hill 2 (2026-10-24), moved over from Visceral RE8's ghosthand.cpp.
// The rule: any mod with hand gestures (racking, reloading by hand) records them, unasked (lanes PROTOCOL section 20).
// Writes %LOCALAPPDATA%\BeG0nE\ghost-hand\silent-hill-2-remake-vr_<date>.ghl in the Lanes recorder's line format while
// the headset is live; the Lanes rider files it. A flat run writes nothing.
//
// Space: everything in the REAL ROOM (OpenXR tracking space, metres): the controllers from UEVR's own poses, the game's
// hands and the slide carried from the game world into the room through the two controllers (known in both spaces).
// Gestures logged with start / done / fail / cancel: slide (rack.cpp), mag (reload.cpp).
#pragma once

#include "common.h"

namespace sh2vr::ghosthand {

void tick();                                          // after ik::tick: one frame of samples
void event(const char* gesture, const char* what);    // "start" | "done" | "fail" | "cancel"
void value(const char* name, float v);
void shutdown();

} // namespace sh2vr::ghosthand
