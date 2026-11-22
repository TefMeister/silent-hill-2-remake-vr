// ghostfix.h -- THE INJECTOR'S GHOSTING FIX COMES ON BY ITSELF, AFTER THE START (2026-11-22).
// With VR_GhostingFix on, the injector lends the second eye one of the game's own scene states (so each eye keeps its
// own picture memory for DLSS) and holds a raw pointer to it. During the start the game throws that state away about
// half a second after the injector took it, and the render thread then calls a dead object: the start crash, 3 of 11
// starts with the fix on from the start, 0 of 20 with it off [verified-live 2026-11-22, simulator]. With it off both
// eyes share one picture memory: shimmer on every edge, trails behind the hands, a weapon that is not solid (Tefa, worn
// 2026-11-22). Switched on by hand in play it took and did not crash [verified-live 2026-11-22, n=2].
// So config.txt keeps VR_GhostingFix=false and this module switches it on once the headset has been drawing for
// settings::GHOSTFIX_AFTER_S seconds, then reads it back and logs both values.
// A file named sh2vr_noghostfix.txt in the profile keeps it off (to compare the two).
#pragma once

namespace sh2vr::ghostfix {

void tick(float delta);

} // namespace sh2vr::ghostfix
