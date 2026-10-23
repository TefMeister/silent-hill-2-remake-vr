// controllers.h -- the two MotionControllerComponents jbusfield's controllers.lua makes (MotionSource "Left" /
// "Right"); the Lua keeps making them, the C++ finds them (controllers.getController).
#pragma once

#include "common.h"

namespace sh2vr::controllers {

enum Side { LEFT = 0, RIGHT = 1 };   // Handed.Left / Handed.Right

API::UObject* find(Side side);       // nullptr until the Lua has made it

} // namespace sh2vr::controllers
