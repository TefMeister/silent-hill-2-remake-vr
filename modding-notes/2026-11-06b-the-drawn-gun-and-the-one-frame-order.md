# The drawn gun: why the left hand slid on the long guns, and the cure (2026-11-06 20:20-20:40, /pd, Fable)

Home PC, `/pd force silent-hill-2-remake-vr`, no game running; nothing here has been run. The row: Tefa's 11-06 wear,
*"the left hand not really being on the front of the gun, rather like stuck on to it and it does visibly move and clip
through the weapon"*. Last night's note (§4) blamed a one-frame order; today the order was read out of UEVR's source
and the engine's, and it is not quite the one written down.

## The order, as it actually is

1. **UEVR moves the motion-controller components, and everything it holds on them, at the SECOND eye's view step**
   (`UObjectHook::on_pre_calculate_stereo_view_offset`, `if ((view_index + 1) % 2 == 0) tick_attachments(...)`;
   read from praydog's UEVR `src/mods/UObjectHook.cpp` on GitHub) `[inferred-static 2026-11-06, from the source]`.
   So at our pre-engine-tick the controllers still hold LAST frame's pose; the fresh pose exists only from the second
   eye's pre-calculate onward, which is where our `on_view` (post-calculate) runs with fresh numbers.
2. **The long gun's visible mesh is not on the controller at all: it is a child of OUR arm mesh's `hand_r_bn` socket.**
   The pack's grip-update callback returns `ik.getCurrentMesh()` + `"hand_r_bn"` for IK arms (main.lua ~950) and
   `attachments.lua` does `K2_AttachTo(mesh, socket)`; the bridge hands the pack our poseable mesh (`sh2vr_ik_mesh`).
   `twohand.cpp`'s header comment ("on the right controller through UObjectHook, RAW_CONTROLLER") describes praydog's
   original profile, not the pack as installed `[inferred-static 2026-11-06, from the scripts]`. Every
   `K2_SetWorldTransform` we write becomes a relative transform against that socket.
3. **Our arm mesh is a `UPoseableMeshComponent`; its bone writes only reach the picture after the component's own
   tick.** `SetBone...ByName` stores the bone and marks the component dirty; the refresh that fills the drawn
   transforms (`RefreshBoneTransforms`) runs from the component's tick, and it is not a Blueprint function, so we
   cannot call it by name. A forum report of exactly this (a poseable mesh one frame behind what drives it, cured only
   by calling `RefreshBoneTransforms` directly) matches `[reported]`. So: a hand target set at the pre-tick of frame N
   is drawn in frame N; a target set at view time is drawn in frame N+1, whatever we do at view time.
4. Put together, per frame N: pre-tick: our IK solves the left hand toward the target set at frame N-1's view (the
   gun's grip spot `Holder_L` as it was then) -> the component ticks and refreshes -> view, second eye: UEVR moves the
   controllers to the fresh pose; `twohand::on_view` turns the gun about the right hand to point at the fresh LEFT
   CONTROLLER and reads `Holder_L` off it as the next target. The gun is therefore drawn one frame AHEAD of the hand
   that was placed on it: the hand trails by one frame of controller motion, more at 30 fps and on the rifle's long
   lever. That is the slide. The right hand cannot slide: the gun hangs from its bone.

## The cure (built, installed, not run)

Since the hand cannot catch up with the gun inside a frame, the gun waits for the hand: **the gun is DRAWN where the
hands were solved for, and only the hands' NEXT targets come from the fresh controller.**

- `twohand.cpp` `on_view`: the predicted gun (aimed at the fresh controller) is written first, `Holder_L` read off it
  for the next solve as before; then, when a grip spot exists and the arm mesh is there, the gun is turned once more
  about the grip until that spot (pulled back by the slide the LAST target used, `g_slide_prev`) meets the drawn left
  hand bone as the game refreshed it this frame, and that is what is written last and drawn. The hand sits on the
  gun's line by construction; what remains is radial play along the barrel (the solve's reach) and one frame of the
  hand's own turn. The gun now lags the real hand by one frame, as the drawn hands always have.
- `rhbolt.cpp` (the locked rifle, LT): the trail `g_gun_s` keeps feeding both hands' targets; `tick` remembers the
  pose the hands are being solved against (`g_gun_shown`, the trail as of the last view) and `on_view` draws the gun
  there, reading `Holder_L` for the next target with the mesh briefly at the fresh pose.
- Settings `TWO_HAND_DRAW_AT_DRAWN_HAND`, `RH_DRAW_WHERE_SOLVED` (both on): off = the 0.1.1 behaviour.
- **The proof is self-measuring now.** The old status line ("the drawn left hand is 0.0 cm from the game's grip spot")
  compared the hand with the target it had been solved toward, both one frame old, so it read 0.0 while the hand slid.
  The new line, every 3 s while two-handing: *"the drawn left hand to the drawn gun's grip spot: mean X cm, max Y
  (seen); to its last target: mean, max (the one-frame lag), N views"*. "Seen" is what the eye gets and should sit
  near 0; "lag" is the slide the old build showed and should read a few cm while the hands move. If "seen" is NOT
  near 0, the derivation is wrong (most likely: the bone read at view time is not the refreshed one), not a knob.
- Built with the VS 2022 Build Tools, both UEVR exports present `[compile-verified 2026-11-06]`; sh2vr.dll f7989800;
  installed by `build.sh --deploy`, backup `D:/SH2 builds/_backup-2026-11-06-2033-before-sh2vr-deploy` (undo:
  `build.sh --undo <that folder>`).

## Not established

- That the bone read at view time (`GetBoneLocationByName`) returns the refreshed pose rather than the written one;
  both agree after the tick, which is when `on_view` runs, so it should; the "seen" number is the check.
- Whether UEVR's AFW build renders the eyes from one flush of the game's transforms (the second write of the gun in
  one view must reach both eyes); the old code already wrote per eye.
- The hand's turn still lags the drawn gun by one frame (its rotation target comes from the predicted gun). A few
  degrees at most while turning; if Tefa sees a wrist twitch, that is it.
- A real catch-up (the hand drawn where the gun is, no lag at all) needs `RefreshBoneTransforms` called at view time:
  a virtual, not a UFunction; its vtable slot could be found live with the UEVR MCP (`uevr_vtable`) on the poseable
  mesh's class. A `[FLAT]` recon row, not needed if the drawn-gun cure reads near 0.

## Next

1. Tefa's wear (shotgun and rifle two-handed, the rifle locked with LT, the pump): good and bad, and the two numbers
   from the status line. `MODEL: OPUS`.
2. The open fingers (palm up, fingers spread on the shotgun's pump): find what overwrites the left finger pose after
   `frontgrip::tick`, or capture the game's own grip fingers (F2 with the body's hand on `Holder_L`). `MODEL: OPUS`.
