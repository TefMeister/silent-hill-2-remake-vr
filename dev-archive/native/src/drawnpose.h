// drawnpose.h -- WHICH HANDS WENT INTO WHICH PICTURE (2026-11-18, Tefa's top wish: "the gun and hands always as steady
// as the headset's framerate").
// The frame warp's made-up pictures get their hands from handpaste.cpp, which carries the hand's pixels of an eye's
// last real picture by "how the hand moved since". Up to b029 that move was read from the raw controller at the moment
// the warp was called. Two things were wrong with that (tests/handcarry_test.cpp, the shipped smoothing):
//   1. since b023 a gun hand is drawn SMOOTHED, so the raw controller's step is not the drawn gun's step: 11 px apart
//      on a "little quicker" move, 30 px on a quick one, flipping every other frame = Tefa's double gun;
//   2. a step read even ONE frame off in time misses by as much.
// The cure is to carry by the two poses the arm solve really used for the two frames in question. The game thread
// knows the poses, the render thread makes the pictures a frame or two later, so they are matched by a key both see:
// the eye's place in the world. UEVR's open side builds the view it hands the warp from the very numbers our
// on_post_calculate_stereo_view_offset sees (PureDark's public fork, FFakeStereoRenderingHook.cpp):
//     position = (location.y, location.z, -location.x) / (world to metres * VR_WorldScale)
// Game thread, every view: record (that position, both hands as ik.cpp placed them this tick) in a small ring.
// Render thread, every warp call: find the record whose position is the call's own view-to-world position.
// Several frames at one place (a dead-still head: the simulator): the view's turn decides (view-to-world =
// yawPitchRoll(-yaw, pitch, -roll), its third column), then the distance behind the newest record the last sure match had.
#pragma once

#include <cstdint>

#include "carry_math.h"

namespace sh2vr::drawnpose {

struct Entry {
    uint64_t seq = 0;
    float key[3]{};                  // the eye's place, the warp's world (metres)
    float fwd[3]{};                  // the view's own z axis in that world: tells frames apart when only the view turned (a stick turn with the head still)
    carry_math::Pose hand[2];        // left, right: as drawn in that frame
};

void tick(float delta);                                            // game thread: the world scale, the log line
void on_view(const void* position, const void* rotation, float world_to_meters, bool is_double);   // game thread: the END of on_post_calculate_stereo_view_offset
bool find(const float* view_to_world, Entry& out);                 // render thread: the record of the frame this view belongs to
void note(bool by_drawn, float differ_m);                          // render thread: one hand carried; how far the raw controller's carry was from it
void set_hand_ticks(int ticks);                                    // a test switch: pair a view with the hands of the record this many ticks later (0 = its own)
void report();                                                     // game thread: one log line now and then

} // namespace sh2vr::drawnpose
