// ik.h -- the IK arms, ported from jbusfield's ik.lua (2026-10-24): a poseable copy of the player's mesh hung on
// the pawn, the body collapsed to the shoulders, and a two-bone solver per arm that bends elbow and wrist to the
// controllers every frame. This is the pair of arms Tefa plays with (Hands Type = IK Arms).
#pragma once

#include "common.h"

namespace sh2vr::ik {

void tick(float delta);     // once a frame, before the engine tick: make the rig when missing, else solve
bool exists();
API::UObject* mesh();       // the arms' PoseableMeshComponent, for the modules that hang things on it
void destroy();

} // namespace sh2vr::ik
