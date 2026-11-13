// handpaste.h -- THE HANDS AND THE HELD WEAPON ARE TAKEN OFF THE FRAME WARP'S PLATE (2026-11-13, Tefa: "worth a good try").
// The frame warp makes every second picture of each eye up from that eye's last real one. In its clean mode (2) it
// moves every pixel by the camera alone, so the hands and the weapon are drawn where scenery would be: one frame
// behind in every turn (notes 2026-11-13e). No setting cures it and its file is not ours to change.
// What is ours: the made-up picture on its way out. warpscale.cpp stands in the call to the warp; right after it,
// on the same command list, this module draws one full-picture pass over the warp's result:
//   1. THE HANDS, PASTED: where that eye's last REAL picture had something close to the eyes that the game itself
//      marked as a moving object (its velocity picture), that pixel is copied over as it was (plus a shift, later:
//      the hands' own motion).
//   2. THE WARP'S COPY, PAINTED OVER: where the warp put those same pixels (the head's turn carries a near pixel
//      there, the same for every depth), the background from just beyond is pulled in.
//   3. Everything else is the warp's picture, untouched.
// It keeps its own copy of each eye's last real picture (the warp writes its result over the injector's).
// Switch: the 4th number in sh2vr_warpscale.txt (0 off, 1 on, 2 on with a tint: pasted green, painted-over red,
// 3 on without the hands' own shift: b020's step one, to compare).
// THE HANDS' OWN MOTION (b021, Tefa 2026-11-14: "the hands are moving with less FPS than the game world"): each hand's
// pixels are moved by how far that controller moved against the head since the eye's last real picture, projected
// into the picture; a pixel belongs to the hand whose last place is nearer.
// b022 (Tefa's clip of 00:43: bands of the pipe jumping sideways for a frame): the shift is now worked out per pixel in
// depth: each pixel is carried the way its controller moved AND turned (a rigid move of the point in front of the eye),
// blended between the two hands by distance, instead of one flat shift per hand with a hard border between them.
// b023: "marked moving by the game" was no test at all (this game writes every pixel's motion, so a near table was
// pasted and carried with the hands): a pixel is hand or weapon when it is near AND its motion in the game's velocity
// picture differs from what a still thing at its depth would show (worked out from the eye's two last real views).
#pragma once

#include <cstdint>

struct ID3D12GraphicsCommandList;
struct ID3D12Resource;

namespace sh2vr::handpaste {

struct Call {
    ID3D12GraphicsCommandList* list = nullptr;   // the injector's open command list (the warp just recorded into it)
    int eye = 0;                                 // the eye that was really drawn this frame; the other one is made up
    const float* camera = nullptr;               // the call's 12 matrices (dest set = the made-up eye now, src set = the drawn eye)
    ID3D12Resource* color = nullptr;             // the drawn eye's picture, and (next frame) where the warp writes this eye's made-up one
    uint32_t color_state = 0;                    // its resting state
    ID3D12Resource* depth = nullptr;             // the drawn eye's depth (the injector's per-eye copy: it lasts until this eye is drawn again)
    ID3D12Resource* velocity = nullptr;          // the game's velocity picture for the drawn eye, or null
    bool previous_frame_mode = false;            // the call asked for mode 2
};

void set_mode(int mode);          // 0 off, 1 on, 2 on with the debug tint
void set_near(float metres);      // how close to the eyes counts as "hands and weapon"
void set_against_room(float amount);   // ... and how far a pixel must move against the room (picture units; 0 = closeness alone)
void after_warp(const Call& call);
void report();                    // one log line now and then (the game thread)

} // namespace sh2vr::handpaste
