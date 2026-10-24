// fingers.h -- the finger poses, ported from jbusfield's hands_animation.lua + the pose half of animation.lua
// (2026-10-24, /pd). The fingers of a hand mesh curl around the grip and the trigger, the thumb rests, and when a
// weapon is in the hand they take that weapon's grip pose: all read from hands_parameters.json ("animations":
// poses and positions, per bone, in degrees), the file the pack's menu writes.
//
// Found while porting: ik.lua put these poses on the IK arms too (createAnimationHandler on the arm mesh), so from
// the moment the C++ IK took over (2026-10-24 00:30) the arms Tefa plays with had no finger poses at all. This module
// serves both: the forearm hands (hands.cpp) and the IK arms (ik.cpp) register their mesh here.
//
// What still comes from the Lua side, over the bridge: WHICH grip pose a hand holds ("pistol", "generic", none),
// decided by the pack's attachments / accessories modules, which are not ported yet.
#pragma once

#include <windows.h>
#include <Xinput.h>

#include <string>

#include "common.h"

namespace sh2vr::fingers {

enum Hand { LEFT = 0, RIGHT = 1 };   // Handed.Left / Handed.Right

void add(const std::string& anim_id, API::UObject* component);   // createAnimationHandler: "left_arms" / "right_arms"
void remove(API::UObject* component);                           // destroyAnimationHandler
void on_input(const XINPUT_STATE& state);                        // handleInputForHands, from the plugin's XInput hook
void tick(float delta);                                          // the finger lerps; re-poses when the held weapon changes
bool exists();                                                   // any hand registered
int count();

} // namespace sh2vr::fingers
