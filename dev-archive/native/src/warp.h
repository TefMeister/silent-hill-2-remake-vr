// warp.h -- THE ALTERNATE-EYE FRAME WARP COMES ON BY ITSELF, AFTER THE PICTURE IS UP (2026-11-13).
// PureDark's warp (the injector's VR_RenderingMethod 3) must be switched on AFTER the inject: a start with 3 already in
// config.txt crashed on the real headset (Virtual Desktop refused the per-eye pictures, 2026-11-12), while the same
// switch made in play works [verified-live 2026-11-13, n=3]. So config.txt keeps 0 and this module makes the switch
// once the headset has been drawing for settings::WARP_AFTER_S seconds. With the warp each eye is its own picture, so
// the DLSS second picture over half of the right eye (mode 0, real headset) is gone too.
// A file named sh2vr_nowarp.txt in the profile keeps it off (to compare the two modes).
#pragma once

namespace sh2vr::warp {

void tick(float delta);
bool switched();   // the switch was made this run

} // namespace sh2vr::warp
