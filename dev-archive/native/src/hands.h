// hands.h -- the two VR hands, ported from jbusfield's hands.lua (2026-10-23, step 2 part 2 of the rewrite plan).
// Owns one record per hand (design note 2026-10-23c); attachments and finger poses will call in, nothing calls out.
#pragma once

#include <string>

#include "common.h"

namespace sh2vr::hands {

enum Side { LEFT = 0, RIGHT = 1 };   // the Lua's Handed.Left / Handed.Right

struct Hand {
    Side side = LEFT;
    API::UObject* component = nullptr;    // our PoseableMeshComponent
    API::UObject* controller = nullptr;   // the MotionControllerComponent it hangs on
    std::string pose;                     // the pose in use (step 3), "" for none
    std::string attachment;               // the attachment in hand (step 4), "" for none
    bool hidden = false;
};

void tick(float delta);           // once a frame: create the hands when they are missing (once a second, like the Lua)
bool exists();
const Hand& get(Side side);
void hide(Side side, bool hidden);
void destroy();                   // takes both hands down (pawn changed, level changed, or the module stood down)

} // namespace sh2vr::hands
