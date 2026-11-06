// shadows.h -- the player's shadows in VR (2026-11-06, Fable, the headset connected, nobody wearing it; Tefa 2026-10-23:
// "weapons must cast shadows when the flashlight shines on them (they do not now); fix the player's shadow if at all
// possible, with the head showing and the hands looking right").
//
// Read live through the UEVR connector `[verified-live 2026-11-06, headset runtime, n=1]`:
//   - the flashlight's main spot light lights channels 0 and 2; James's body, his flashlight mesh and every weapon mesh
//     sit on channel 1 only (the game keeps James out of his own torch, and lights him with fill lights of his own on
//     channel 1); our poseable arm mesh is on channel 0. So the torch could never light or shadow a weapon: channels.
//   - the pack hides the head by collapsing the neck bone (HideBoneByName), which takes the head out of every shadow;
//     a mesh following the body's pose (leader pose) shares the body's bone visibility, so the head cannot be hidden on
//     one and shown on the other by bones. The game ships an invisible material (MI_Invisible, used on three of James's
//     own slots), so the head is hidden from the camera by MATERIAL instead, and the neck bone is shown again.
// What this does, once a second:
//   1. the equipped weapon's mesh gets channel 0 added (lit and shadowed by the torch; the fill lights keep channel 1);
//   2. the body: the neck bone shown, the head's material slots (face, eyes, teeth, hair, brows, lashes) set to the
//      game's invisible material, its own shadow off;
//   3. a second skeletal mesh on the pawn, following the body's pose, hidden from the camera, casting its shadow
//      anyway (the engine's "cast hidden shadow"), on channels 0 and 1: the complete shadow, head included, arms
//      hidden the way the pack hides them on the body (the copy follows the body's bone visibility).
// Proof lines in the log; the picture is Tefa's.
#pragma once

namespace sh2vr::shadows {

void tick(float delta);   // once a frame on the game thread; does its work about once a second

} // namespace sh2vr::shadows
