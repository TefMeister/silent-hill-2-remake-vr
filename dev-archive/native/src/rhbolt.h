// rhbolt.h -- the rifle locked in the left hand, and its bolt worked by the RIGHT hand (2026-11-01, Opus).
// Built first in Visceral RE8 the same morning (rack_right.cpp there), where Tefa wore it: "this is epic. really is. can
// you implement this in sh2 please?". The RE8 shape, kept here:
//   - LT pressed while the left grip holds the rifle's front (twohand.cpp has it two-handed) LOCKS the rifle in the left
//     hand: not frozen, it trails the left controller slowly (RH_LOCK_FOLLOW_S), and BOTH drawn hands sit on the gun (the
//     left on the front, the right on the grip), so the left hand never leaves the front (Tefa: "i need the left hand
//     docked on the front end at all times for it to look good");
//   - RG while locked: the drawn right hand glides onto the bolt (the mesh's "bone_lock", Lukas's MCP finding) and rides
//     back and forward with the right controller along the barrel; back fully, then home = one cycle. A cycle owed by the
//     pack's rifle bolt (its fire gate shut) unlocks it through the Lua (rifle_bolt.lua M.unlockFromPlugin); any other
//     cycle is cosmetic. RG let go: the hand glides back to the grip, still locked;
//   - LT (or the left grip) let go: the gun eased back to where UEVR / twohand.cpp place it (RH_HANDOVER_S).
// Not done yet: the bolt part itself does not slide (the game moves it only in its own prepare animation; a live look).
// While locked, the right grip is ours: holsters.cpp asks claims_rg() so it never draws or swaps a weapon then.
// [compile-verified 2026-11-01] until it has run.
#pragma once

namespace sh2vr::rhbolt {

void tick(float delta);   // after twohand::tick, before ik::tick: the buttons, the phases, the slow trail, the pull
void on_view();           // after twohand::on_view: the gun written where it trails, both hands' overrides
bool claims_rg();         // the rifle is locked: the right grip is the bolt, not a holster spot
bool active();            // the right hand is on (or gliding to / from) the bolt

} // namespace sh2vr::rhbolt
