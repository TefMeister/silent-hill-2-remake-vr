# Cutscenes on a flat screen that stays put (2026-11-12 evening, Fable, simulator)

Tefa: *"silent hill 2 cutscenes in 2d screen in front of me without depth"*, then *"flat screen should stay put on a
floating big screen"*, then (watching the simulator) *"right eye on the simulator was glitching and blinking, and the
game window showed this, also James head needs to be visible in cutscenes"*.

## What is built (`dev-archive/native/src/cutscreen.cpp`, installed as b007)

- Every 0.1 s the plugin asks the game's own `SHCharacterStatics::IsCharacterInCutscene(pawn)`. NOT the pack's wider
  cutscene state (main.lua also sets that while vaulting, climbing and pushing).
- Cutscene begins: `UI_FollowView=false`, `UI_Size=3.5`, `UI_Distance=3.0`, a recentre, then `VR_2DScreenMode=true`.
  Ends: `VR_2DScreenMode=false` and the three screen settings back to what they were (2 / 2 / follow in play).
  While flat, `UI_FollowView` is put back to false if the pack turns it on (its cutscene state locks the UI).
- **The settings are written from a short thread of our own, never from the engine tick.** Set from the tick, the
  switch killed the game the moment the opening cutscene began (access violation in the game under a UEVRBackend
  frame) `[verified-live 2026-11-12, n=1]`; the pack's `set_2D_mode` says the same ("do not execute in game thread").
- **The head in cutscenes** (`shadows.cpp`): since 2026-11-06 the head is hidden by an invisible material on its slots
  and nothing ever put it back, so James was headless in every cutscene. Each head slot's own material is now kept
  and put back while a cutscene plays, invisible again after; acted on at once, not at the 1 s check.
- Dev switch: a file `sh2vr_flat.txt` in the profile forces the flat screen on in play (for trying settings).

## Seen on the simulator

- New Game's opening cutscene, 3 min 46 s: flat screen on at its start, a surface 6.2 x 3.5 m, 3 m ahead at head
  height, the same for both eyes; it stays where it is when the head turns 25 degrees; back to full VR by itself at
  the end, the play screen's size back to 3.56 x 2 at 2 m `[verified-live 2026-11-12, n=2 full cutscenes]`.
- James has his head in the cutscene `[verified-live 2026-11-12, n=1, six stills + a wide shot]`.
- Forced flat in play at the armory save: on / off twice, walking, a head turn: clean, no crash `[verified-live, n=2]`.

## OPEN: the right eye is sometimes wrong while flat

- **Run 2** (the first full cutscene, Tefa watching): the simulator's own detector logged six `ASYMMETRIC_EYE_FLASH`
  incidents during the cutscene (frames 9418-11151) and one `ASYMMETRIC_BRIGHT_BREAKTHROUGH` at its end. Its frames
  (`dev-archive/recon/2026-11-12-cutscenes-flat-screen/right-eye-white-run2-incident-10229.png`): the left eye shows
  the cutscene, **the right eye a different, white, washed-out picture of a headless James in 11 frames of 12**, the
  right picture in 1. Tefa's still of the game window at the cutscene's end shows two views laid over each other
  with bands of smeared lines `[verified-live 2026-11-12, n=1 run]`.
- **Run 3** (the next full cutscene, the head fix in): no asymmetric incident in the whole cutscene; one captured
  moment with a band of smeared lines in the right eye only, just before a cut (`...-streak-run3-incident-12724.png`).
- Not reproduced by: forcing flat in play, walking, turning the head `[measured, n=1 each]`.
- UEVR logs once a second while flat: `FFakeStereoRenderingHook.cpp:2926 Something strange is going on, the vtable
  is already hooked, maybe previous frame was not rendered?`
- Leads `[hypothesis]`: the flat screen is drawn once per eye and the right eye's comes from its own picture, which
  in flat mode is not always drawn (the injector's native stereo, `VR_RenderingMethod=0`, with the game's DLSS and
  one DLSS instance per eye in this AFW build). To try, one at a time, over the opening cutscene with the detector's
  incident list as the measure: DLSS off while flat (`r.NGX.DLSS.Enable`), `VR_GhostingFix` off while flat,
  `VR_NativeStereoFix`, another `VR_RenderingMethod` while flat. What differed between run 2 and run 3 is not known
  (the head's materials changed; the switch may also land on a different moment of the frame each time).

## Driving notes

`sh2route.py` in the recon folder: launcher `play --sim --no-install`, 62 s, Enter (warning), Down + Enter (New
Game), Down x4 + Enter (Start Game), 32 s, Enter (press any button): the cutscene starts at once. The save folder was
copied away first and put back after (identical by hash); New Game did not replace the armory save.

## Later the same evening: the mirror, and what the flat screen really shows

Tefa: *"damn the picture in the mirror is still wrong, that is one thing i really need to be figured out"*.

- **The mirror in the opening cutscene is wrong in VR-flat**: instead of James's reflection, a thin bright band (the
  ceiling light squashed) and a stretched patch of jacket (`mirror-wrong-vr-flat.png`) `[verified-live, n=4 runs]`.
- **What the mirror is** `[verified-live 2026-11-12, UEVR connector]`: a `SHFakeMirrorComponent` (class
  `/Script/SHProto.SHFakeMirrorComponent`, a SceneCaptureComponent2D of the game's own; here
  `FakeToiletMirror_B_C_0.SHFakeMirror`, six instances loaded). It films into `FakeMirrorRT` with
  `bUseCustomProjectionMatrix = true`, from a mirrored camera it works out itself each frame: `MirrorCameraPosition`,
  `MirrorCameraRotation`, `MirrorCameraFOV` (19.8 in the mirror shot = the cutscene camera's own angle), clip plane
  on the glass. **It has an input override: `AllowFakeInputs` (false), `FakeCameraPosition`, `FakeCameraRotation`,
  `FakeCameraFOV` (90).** No PlanarReflection is in use.
- **The plain game (no UEVR) at the same moment is a tight close-up over James's shoulder with his face in the
  mirror; VR-flat shows the same camera spot as a wide view from behind** (`plain-game-...png` against
  `vr-flat-timeline-...png`; the plain cutscene runs about 10-14 s later after the key press) `[measured, n=1 plain
  run]`. So in flat mode the picture is still drawn with the headset's wide view angle, not the camera's: every
  cutscene is framed too wide, and the mirror (filmed for the camera's 19.8 degrees, pasted by screen position) does
  not fit `[inferred from those two]`.
- Ruled out: `VR_RenderingMethod=3` (one eye at a time): the game runs, the mirror is exactly as wrong `[n=1]`.
  `VR_RenderingMethod=1` (synchronized sequential): the game dies at start, set live or in the file (`Scene state
  passed to FSceneView constructor is null`) `[n=2]`. `VR_DecoupledPitch=false` during the cutscene: the same
  pictures `[n=1]` (taken out of the code again).
- The plugin API has no hook for the view angle (only the per-eye offset and `get_ue_projection_matrix`).
- `slomo 0.01` through the connector during the cutscene turned the picture white with a headless James: the same
  look as the right eye in the bad run. So that white picture is a state of the game's own drawing (exposure, and
  the head hidden), not a foreign image `[n=1]`. Do not use slomo to hold a shot.
- The plain run counted in three "as it is" repeats: one full cutscene, no right-eye incident; the other two never
  reached the cutscene (the route's Down was lost and Continue loaded the armory save; the route needs a picture
  check before each key).
- Found on the way: `sh2vr_devcvars.txt` in the profile has held `r.NGX.DLSS.Enable 0` and `r.ScreenPercentage 100`
  since 2026-11-12 00:43, applied at every start: **DLSS is off in every run and wear since then.** Left as it is
  (it belongs to the DLSS ghost test of that night); the board row says so.

## Next for the mirror (the lead)

Feed the mirror the view that is really drawn: `AllowFakeInputs = true`, `FakeCameraPosition / Rotation` = the
camera the frame is drawn from (in play: the head), `FakeCameraFOV` = the drawn horizontal angle (from
`get_ue_projection_matrix`). To prove live first with the connector in the mirror shot (the shot lasts about 30 s;
set the three fields, look), then in `sh2vr.dll` for every `SHFakeMirrorComponent`. That is also the fix for mirrors
in VR play. For the flat screen's framing itself (too wide), no way is known yet.

## The mirror feed, tried once and taken out again (22:0x)

`dev-archive/recon/2026-11-12-cutscenes-flat-screen/mirror-feed-tried/mirror.cpp`: every tick each live
`SHFakeMirrorComponent` got `AllowFakeInputs = true`, `FakeCameraPosition / Rotation` = the left eye's view from the
plugin's `on_post_calculate_stereo_view_offset`, `FakeCameraFOV` = the angle from `get_ue_projection_matrix` (95.9
degrees on the simulator). Result `[verified-live 2026-11-12, n=1]`:

- **The position that hook hands over is not a world position**: it logged `266 5382 -338`, pitch 0, yaw 150.9, all
  through the cutscene, while the cutscene camera stood near `57874 49093 3281` (the component's own
  `MirrorCameraPosition`, which is the INPUT camera as the component sees it, with `MirrorCameraRotation` and
  `MirrorCameraFOV`: -3.3 / 63.8, 23.9 degrees in the mirror shot; the camera manager reported yet another view,
  pitch -50, 26 degrees). So the drawn view's world position has to come from somewhere else.
- **With the mirror filming from that wrong place the whole picture got brighter, and in the mirror shot it went
  white with a headless James** (`whole-picture-white-in-the-mirror-shot.png`): the same picture as the right eye
  in the bad run and as under `slomo`. So **the fake mirror's capture and the eye pictures share exposure (and very
  likely the history the smearing comes from) under UEVR** `[inferred from three sightings]`: that is the right-eye
  fault's mechanism, and why it only shows in cutscenes with a mirror in them (the opening bathroom).
- The writes themselves work (the connector's write of the same fields also took).

So the two open faults are one knot: (a) the mirror is filmed for the wrong view, (b) its capture bleeds into an
eye's picture. Next, in this order: 1. the capture's own settings against the bleed (it is a
SceneCaptureComponent2D: `bAlwaysPersistRenderingState`, `PostProcessSettings` auto-exposure override,
`CaptureSource`, its show flags for eye adaptation / temporal AA), checked on the opening cutscene with the
simulator's incident list; 2. the right input for the mirror: start from the component's own `MirrorCamera*` values
(right place, wrong angle) and change only the angle (`FakeCameraFOV` with `AllowFakeInputs`, position and rotation
copied from `MirrorCamera*` BEFORE switching the override on, refreshed by reading the game camera the component
itself reads: find that source, e.g. the sequence's camera component or the local player's view info); 3. only then
VR play, where the position must be the head's.
