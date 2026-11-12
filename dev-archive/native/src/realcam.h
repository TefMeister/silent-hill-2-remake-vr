// realcam.h -- the game's REAL cutscene camera on the flat screen (2026-11-12, Tefa: "cutscenes shown as the game's
// real flat cutscenes: its own camera, its own framing, the mirror right, nobody walking through walls").
// The injector's flat-screen mode draws every frame with a fixed 90-degree view and never reads the camera's own angle
// (its source: "float fov = 90.0f; // todo, get from FMinimalViewInfo"), and still shifts the view sideways per eye.
// While our cutscene flat screen is on, this module (1) puts the view's place back to what the game handed in, for
// both eyes (one picture, no depth, no camera offsets) and (2) writes the camera's own angle into the projection the
// engine asks the stereo device for, through ONE swapped entry of that device's function table (the entry the
// injector's own log names at start; refused unless table and log agree).
#pragma once

namespace sh2vr::realcam {

void set_active(bool on);                                   // from cutscreen's writer thread, right beside the 2D switch
void on_pre_view(void* device, void* position);             // before the injector's offsets for this view
void on_post_view(void* position);                          // after them
void tick(float delta);                                     // a slow self-check: the angle read raw against GetFOVAngle

} // namespace sh2vr::realcam
