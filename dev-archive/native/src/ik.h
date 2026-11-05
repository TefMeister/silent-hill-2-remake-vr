// ik.h -- the IK arms, ported from jbusfield's ik.lua (2026-10-24): a poseable copy of the player's mesh hung on
// the pawn, the body collapsed to the shoulders, and a two-bone solver per arm that bends elbow and wrist to the
// controllers every frame. This is the pair of arms Tefa plays with (Hands Type = IK Arms).
#pragma once

#include "common.h"
#include "controllers.h"
#include "ue.h"

namespace sh2vr::ik {

void tick(float delta);     // once a frame, before the engine tick: make the rig when missing, else solve
bool exists();
API::UObject* mesh();       // the arms' PoseableMeshComponent, for the modules that hang things on it
void destroy();
void override_left(bool on, const ue::Vec3& effector_ws, const ue::Rot& rot_ws);   // rack.cpp: the left hand rides a gun part
void override_right_turn(bool on, const ue::Quat& turn_ws);                         // twohand.cpp: the right hand turns with the long gun
void override_right(bool on, const ue::Vec3& effector_ws, const ue::Quat& rot_ws);  // rhbolt.cpp: the right hand on the locked rifle (grip / bolt); wins over the turn
// frontgrip.cpp (2026-11-05): the left HAND BONE itself at this world place and turn (the game's own grip on a long gun),
// no controller offset and no end_bone_rotation; wins over override_left. The elbow still reads the real controller.
void override_left_bone(bool on, const ue::Vec3& bone_ws, const ue::Quat& rot_ws);
bool hand_world(controllers::Side side, ue::Vec3& out);                             // the arm mesh's hand bone, world

} // namespace sh2vr::ik
