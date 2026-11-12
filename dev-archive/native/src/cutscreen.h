// cutscreen.h -- cutscenes on a flat screen in front of the player, no depth (2026-11-12, Tefa: "silent hill 2
// cutscenes in 2d screen in front of me without depth"). While the game says the player's character is in a cutscene
// (SHCharacterStatics::IsCharacterInCutscene, the game's own answer; NOT the pack's wider "cutscene" state, which is
// also on while vaulting and pushing), the VR injector's own flat-screen mode is on (VR_2DScreenMode); it goes back to
// what it was the moment the cutscene ends.
#pragma once

namespace sh2vr::cutscreen {

void tick(float delta);
bool active();   // a cutscene is on the flat screen right now (the head is shown for it: shadows.cpp)

} // namespace sh2vr::cutscreen
