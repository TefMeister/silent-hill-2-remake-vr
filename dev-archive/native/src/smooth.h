// smooth.h -- weapon smoothing, switched by what is held (2026-11-15, /pd; Tefa 2026-11-14: "yeah let's build a proper
// one", melee "just less to get rid of the hand shaking, but not enough to make melee feel sluggish").
//
// A One Euro filter (Casiez, Roussel, Vogel 2012) on the right hand's target pose, inside the arm solve, so the arm,
// the hand and what the hand holds (the gun is a child of our arm mesh's hand_r_bn) move as one steadier piece. Filtered
// in the controller's own tracking space (its attach parent), so walking and turning are not smeared.
//   gun in hand    -> SMOOTH_GUN_STRENGTH (Village's default)
//   melee in hand  -> SMOOTH_MELEE_STRENGTH (light: shake only)
//   bare hands, the healing item, menus, no headset -> raw
// Live: "sh2vr smooth_gun <0..10>" / "sh2vr smooth_melee <0..10>" in sh2vr_devcvars.txt (0 = raw).
// Proves itself: every SMOOTH_REPORT_S the log gives the hand's frame-to-frame turn while held still, raw and drawn.
#pragma once

#include "common.h"
#include "ue.h"

namespace sh2vr::smooth {

enum class Kind { RAW, MELEE, GUN };

void tick(float delta);   // once a frame, before ik::tick: the clock and what is in hand
// the right hand's target (effector place and turn, world), smoothed for what is held; untouched when raw
void apply(API::UObject* controller, ue::Vec3& pos_ws, ue::Rot& rot_ws);
void set_strength(Kind kind, double strength);   // the dev file's live lines
Kind kind();

} // namespace sh2vr::smooth
