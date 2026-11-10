// cornermask.h -- THE EYE-CORNER JOB, STEP 1 (2026-11-10, a dev tool for now): solid pieces that ride on the headset.
// The corners of each eye's picture that the lenses never show are drawn in full (the engine's hidden-area mask gets no
// mask from UEVR: measured 2026-11-10). A solid piece close in front of the eye lets the card skip what is behind it.
// If <profile>\sh2vr_mask.txt exists, each line "x y z pitch yaw roll sx sy sz" (cm and degrees from the headset: x
// forward, y right, z up; scale of a 100 cm cube) becomes one piece, rebuilt whenever the file changes, removed when the
// file is deleted. No file: nothing. The pieces cast no shadow and have no collision.
#pragma once

namespace sh2vr::cornermask {

void tick(float delta);

} // namespace sh2vr::cornermask
