// warpscale.h -- THE FRAME WARP IS TOLD THE WORLD IS LARGER, SO ITS NEAR-THE-EYE RULES LET GO OF THE ARMS (2026-11-13).
// PureDark's frame warp treats what is close to the eyes differently from the rest: nearer than about half a unit it
// stops counting a thing as a moving object, and nearer than one unit it re-projects it by the head's rotation only.
// In a first-person VR game the arms and the held weapon live exactly there: in the combined mode the hand is carried
// right while the sleeve nearer than the line is torn off it, on every made-up frame of a turn
// [verified-live 2026-11-13, simulator: the debug view goes green only beyond the line].
// The warp's picture maths depends only on directions and ratios, so the same scene described k times larger
// (camera positions and the near plane times k, the depth picture untouched) lands every pixel in the same place,
// while every distance the warp measures is k times longer: the lines move k times closer to the eyes.
// This module does that on the way INTO the warp: it stands in the open injector's (UEVRBackend.dll, PureDark's
// public fork) import of the warp's EvaluateFrameWarp and hands it a scaled copy of the camera data. Nothing in the
// warp's own file is changed.
// The factor: settings::WARP_NEAR_SCALE, or one number in sh2vr_warpscale.txt in the profile (read once a second;
// 1 = untouched), so it can be tried live.
//
// THE MODE PER FRAME (same evening, the part that matters): the previous-frame mode (2) has clean edges but never
// carries a near object by its own motion, so the hands and the weapon are left one frame behind whenever the view
// turns or they move [verified-live 2026-11-13, simulator: no motion setting changes it]; the combined mode (3)
// carries them (and has the outline). Standing in the call, this module makes a call that asked for mode 2 as mode 3
// while the view turns faster than a threshold or a controller moves faster than a speed, and for a few frames after.
// The file's second and third numbers: degrees a frame, metres a second (0 = never). Example: "1.0 0.3 0.5".
// The scale factor itself turned out to be no fix (the lines it moves were not what tore the picture); it stays at 1
// as a proven lever on the warp's distance rules (0.25 sends the hands down its nearer-than-0.1 path).
#pragma once

namespace sh2vr::warpscale {

void tick(float delta);

} // namespace sh2vr::warpscale
