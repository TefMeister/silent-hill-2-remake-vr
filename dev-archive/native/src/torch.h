// torch.h -- THE FLASHLIGHT'S LIGHT BETWEEN THE EYES, NOT AT ONE OF THEM (2026-11-13).
// Tefa, real headset, frame warp on: the torch's shadow falls on opposite sides of the shotgun for each eye (left eye
// open: shadow on the left of the gun; right eye: on the right), "both eyes open and it gets trippy". The pack puts the
// torch's main light (SHFlashlight.LightMain) on the HEAD through UEVR's attachments, which are placed from the view
// being worked out; with the alternate-eye warp each frame is one eye, and a moved light reaches the picture a frame
// later, so every eye is lit from the OTHER eye's place [hypothesis from the source and the report; this module is the
// test]. Here, after UEVR has placed it for each view, the light is put at the middle of the two latest eye positions.
// A file sh2vr_notorchfix.txt in the profile leaves the pack's placement alone.
#pragma once

namespace sh2vr::torch {

void tick(float delta);                       // finds the light (game thread)
void on_post_view(int view_index, void* position);   // after UEVR's offsets: one eye's final place

} // namespace sh2vr::torch
