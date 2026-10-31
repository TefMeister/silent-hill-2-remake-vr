// twohand.h -- two hands on one thing (2026-10-31, Tefa: "move the map to the right hip, LG to take it out, right hand
// with RG held to two hand the map" and "use whatever you need for two handing the map and long weapons" from praydog's
// original SH2 profile, camera.lua). See twohand.cpp.
#pragma once

namespace sh2vr::twohand {

// Once a frame on the game thread (pre engine tick): the grips' edges, what is in hand, start / stop.
void tick(float delta);

// After UEVR has placed the held things on the controllers for this view (post calculate stereo view offset):
// turn the long gun toward the left hand, put the map in the left hand or between both hands.
void on_view();

// The long gun is held with both hands right now (rack / fire may ask).
bool gun_two_handed();

} // namespace sh2vr::twohand
