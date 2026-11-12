// mirror.h -- the game's mirrors under VR (2026-11-12, Tefa: "the picture in the mirror is still wrong, that is one
// thing i really need to be figured out").
// A mirror in this game is a SHFakeMirrorComponent: a scene capture of the game's own that films the room from behind
// the glass for the camera it believes the frame is drawn from (the game camera, with the game camera's angle) and is
// pasted by screen position. Under UEVR the frame is drawn from the headset's view with the headset's angle, so the
// picture does not fit. The component has an input override (AllowFakeInputs, FakeCameraPosition, FakeCameraRotation,
// FakeCameraFOV): this module feeds it the view each frame is really drawn from.
#pragma once

namespace sh2vr::mirror {

// from the stereo view-offset hook: the position and rotation one eye's view is drawn from (after UEVR's changes)
void on_view(int view_index, const void* position, const void* rotation, bool is_double);
void tick(float delta);

} // namespace sh2vr::mirror
