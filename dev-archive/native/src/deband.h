// deband.h -- THE FOG'S CONTOUR LINES (2026-11-22, Tefa: the fog has "noticable curvy lines going through it ... like
// contours on the map"). Measured the same day on the pretend headset: the eyes' pictures are 8 bits a colour, and the
// town's fog is such a slow gradient that it is drawn in flat steps of one level each: 88% of neighbouring fog pixels
// are exactly equal, and the odd/even levels draw a clean contour map. Nothing upstream breaks the steps up.
// What is ours: both eyes' pictures on their way out (warpscale.cpp stands in the call to the frame warp). Here, after
// the warp and the hands' own pass, every pixel whose surroundings are within about one level of it is given the
// average of four pixels around it (a random direction each pixel and each frame) plus one level of fine grain, so
// the 8-bit write lands on both sides of each step: the contour lines become an even, very fine grain.
// Textured things are not touched (their surroundings differ by more than the threshold).
// File sh2vr_deband.txt in the profile: "on radius_px threshold_levels grain_levels" (0 = off), read every second.
#pragma once
#include <cstdint>
struct ID3D12GraphicsCommandList; struct ID3D12Resource;
namespace sh2vr::deband {
// called once per warp call, after the hands' pass: `color` is the eye that was really drawn (0 left, 1 right)
void after_warp(ID3D12GraphicsCommandList* list, int eye, ID3D12Resource* color, uint32_t color_state);
void tick(float delta);   // game thread: the file, the counts
}
