# The game's real cutscene camera on the flat screen, and the DLSS-on crash at the opening (2026-11-12 late, Fable, simulator)

Tefa: *"cutscenes shown as the game's REAL flat cutscenes (its own camera, its own framing, the mirror right, nobody
walking through walls) on a big screen that stays put in front of me, and full VR only in gameplay. Tell me early and
plainly if that cannot be done with this VR tool."* It can. Everything below was run with DLSS on unless a line says
otherwise (`Adv_Supersampling 4`, `Adv_DLSSQuality 3`; no DLSS-off line in `sh2vr_devcvars.txt`).

Supersedes: the "Next for the mirror" and "The mirror feed" sections of `2026-11-12b-cutscenes-on-a-flat-screen.md`
(the mirror needs no feed of its own: it was right all along for the game's camera, the frame was drawn for another one).

## The cause, read in the injector's source (the reader; PureDark/UEVR branch AFW, commit fc8de10)

- In flat-screen mode the injector draws the scene with a **hard-coded 90-degree horizontal view angle**
  (`FFakeStereoRenderingHook.cpp` 5243-5273: `float fov = 90.0f; // todo, get from FMinimalViewInfo`). The camera's
  own angle is never read `[read in source]`. Head rotation and position are NOT applied in that mode; the per-eye
  sideways shift and the `VR_Camera*Offset` settings still are `[read in source]`.
- No setting changes that (projection overrides, decoupled pitch, compatibility modes, rendering method: all read) and
  the debug tick box "Disable Projection Matrix Override" is read by nothing `[read in source]`.
- `get_ue_projection_matrix` always hands a plugin the HEADSET's matrix: the 95.9 degrees of the earlier mirror-feed
  try was the wrong number `[read in source]`.
- The injector's "VR off" key (`VR_DisableVRKey`) flips a plain variable, not a setting; with it the engine would draw
  plain mono into the game window, and on OpenXR the headset would most likely keep the last VR frame, never the flat
  game `[inferred from the source, NOT tried live: not needed once the camera fix worked]`.
- Full reading: `inbox/2026-11-12-mod-uevr-2d-mode-source-reading.md`.

## Shot by shot: plain game against VR-flat (`dev-archive/recon/2026-11-12-real-flat-cutscenes/`)

`cutcompare.py` takes a still of the game window every second from the last key press, in the plain game and in VR.
The plain opening (stills `runs/plain1`, sheets `runs/sheet-plain1-*.jpg`): +11 s two pairs of hands in the mirror
frame, +21 s lips close-up, +30 s the hand rubbed, +39 s over the shoulder into the mirror, +60 s the wide bathroom,
+69 s James's face, +78 s black with two lines of text, +90 s out of the door, +93 s the car park, +123 s the wide
overlook, +156 s at the railing, +195 s behind him at the view; play from about +205 s.

Before the fix (b007, last night's stills, DLSS off): the same camera spots drawn 90 degrees wide, so a 20-26 degree
close-up became a wide view from behind; the mirror's picture (filmed for the camera's own angle) did not fit; what the
real framing hides (James against the wall on his way out) was in view.

## The fix: `dev-archive/native/src/realcam.cpp` (new), wired into `cutscreen.cpp` and `plugin.cpp`

While our cutscene flat screen is on:
1. the view's place is put back to what the game handed in, for both eyes (kept in the plugin's pre-view callback,
   written back in the post-view one): one picture, no depth, no camera offsets;
2. the camera's own angle (the camera manager's cached view, checked once against `GetFOVAngle()`: "read raw 70.00,
   asked 70.00") is written into the projection the engine asks the stereo device for. One entry of the device's
   function table is swapped for ours (entry 12, the one the injector's own log names: `Found
   CalculateStereoProjectionMatrix: 12 <address>`; refused unless the table really holds that address). Ours calls the
   old one first, then sets the two scale numbers for the camera's angle at the picture's own width / height (1.778).
   Log proof per change: `realcam: angle 28.5 degrees, aspect 1.778: the projection's first number 1.000 -> 3.943`.
It is switched by cutscreen's writer thread right beside `VR_2DScreenMode` (on just after true, off just before false).

**Result `[verified-live 2026-11-12, n=3 full opening cutscenes: 1 with DLSS off, 2 with DLSS on]`:** every still of
the VR-flat run matches the plain game's still of the same second, framing and mirror included
(`plain-against-vr-flat-with-the-camera-fix.jpg`, sheets `runs/sheet-realcam2-dlsson-*.jpg`). The simulator's two eye
pictures show the same correct picture on a 6.2 x 3.5 m surface 3 m ahead (`simulator-both-eyes-mirror-shot-dlss-on.jpg`).
The simulator's incident list for the full DLSS-on run: five `VISIBLE_TO_BLANK_FRAME` (the cutscene's own cuts to
black), **no asymmetric-eye incident**. The injector still logs `the vtable is already hooked` during the cutscene (51
lines in one run): harmless so far.

At the cutscene's end the game blends its camera to the play view while it still says "in cutscene" (the angle climbs
70 -> 80 over about ten seconds, the camera swings off James): that blend is shown flat, then VR comes back.

## The crash with DLSS on at the opening cutscene's first frame: OUR head step `[verified-live, 2 crashes of 2, then 3 clean runs of 3 without the old step]`

- b007 with DLSS on: the game died within a second of the key press that starts the opening cutscene, twice of twice
  (access violation reading 0x7 / 0x60000000f on the game thread, inside the engine's own tick:
  `SHProto +4c60c37 <- +49dabdd <- +4ad784a (inside UGameEngine::Tick at +4ad7540) <- UEVRBackend +1685bf`). Last
  night's runs lived because DLSS was off.
- Bisected, one change a run: cutscene pieces all off: clean (n=1). Only the head put-back off (flat screen and camera
  fix on): clean (n=1). So it was `shadows.cpp` handing the head's material back from a pointer kept since the slots
  were found; nothing held that material once ours replaced it `[inferred: a dead pointer; why DLSS decides whether it
  is already dead is not known]`.
- Cure: shown = our override taken off (`SetMaterial(slot, null)`: the mesh's own material, which the mesh asset keeps
  alive), hidden = the invisible material as before. Two full cutscenes with DLSS on, James has his head, no crash.
- ⚠️ Two runs of this bisect were VOID and are not counted: the key route lost its first Enter on the warning screen
  and ran the Credits (Tefa saw it). `cutcompare.py` now checks the picture before each key (the warning's white
  square, the red bar on CONTINUE, the red START GAME bar) and stops if the screen is not the expected one.

## Left

- Tefa's eye in the headset: the screen's size and distance, the switch in and out, the ten-second blend at the end.
- Other cutscenes (only the opening was run); a cutscene whose camera uses a fixed film shape (black bars in the plain
  game) would show extra picture above and below here.
- Mirrors in VR PLAY are a different job (the frame there is really drawn for the head): the component's input
  override, with the head's place and the drawn angle.
