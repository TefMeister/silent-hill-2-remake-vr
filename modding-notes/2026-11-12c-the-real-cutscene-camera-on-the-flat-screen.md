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

## The crash with DLSS on at the opening cutscene's first frame `[DISPROVED 23:4x: "our head step"; see the correction below]`

- b007 with DLSS on: the game died within a second of the key press that starts the opening cutscene, twice of twice
  (access violation reading 0x7 / 0x60000000f on the game thread, inside the engine's own tick:
  `SHProto +4c60c37 <- +49dabdd <- +4ad784a (inside UGameEngine::Tick at +4ad7540) <- UEVRBackend +1685bf`). Last
  night's runs lived because DLSS was off.
- Bisected, one change a run: cutscene pieces all off: clean (n=1). Only the head put-back off (flat screen and camera
  fix on): clean (n=1). So it was `shadows.cpp` handing the head's material back from a pointer kept since the slots
  were found; nothing held that material once ours replaced it `[inferred: a dead pointer; why DLSS decides whether it
  is already dead is not known]`.
- (Was called the cure:) shown = our override taken off (`SetMaterial(slot, null)`: the mesh's own material, which the mesh asset keeps
  alive), hidden = the invisible material as before. Two full cutscenes with DLSS on, James has his head, no crash.
- ⚠️ Two runs of this bisect were VOID and are not counted: the key route lost its first Enter on the warning screen
  and ran the Credits (Tefa saw it). `cutcompare.py` now checks the picture before each key (the warning's white
  square, the red bar on CONTINUE, the red START GAME bar) and stops if the screen is not the expected one.


## CORRECTION 2026-11-12 23:4x: the crash is NOT cured, and it is not shown to be ours

- The installed b008 crashed in 2 of 3 short runs straight after (`runs/rate-b008-*`): one at the cutscene's start
  (the same place: `+4c60c37 <- +49dabdd <- +4ad784a <- UEVRBackend +1685bf`), one 29 s into the cutscene at a NEW
  place (`+2ae8984`, reading 0xa0; UEVR logged "attempted dereference of null pointer at ...8984" just before). Two
  test builds (eyes 2, eyes 3) also died at the start, the second before our cutscene code had even noticed the
  cutscene. So the "bisect" above was single runs of a crash that comes and goes `[measured, n=3 + 4 earlier]`.
- Tefa's own wear of b008 (23:20) and four simulator runs lived. The crash rate with DLSS on, with our plugin, is
  roughly half; with DLSS off it was never seen (last night's runs, n=5, plus run 4 here) `[measured, small n]`.
- Being measured at the time of writing: three runs with our plugin taken out (`runs/rate-noplugin-*`).

## Tefa's first wear of b008 (23:20, Quest 3, DLSS on)

Tefa: *"the cutscenes played out like flat ones, so satisfying to see it!"* `[verified-live 2026-11-12, headset, n=1]`.
- *"screen has to be further from me by quite a bit, at least a meter further"*: `CUTSCENE_SCREEN_DISTANCE` 3.0 -> 4.5
  in the source (not installed yet).
- *"James has completely black eyes, super creepy"*. Simulator stills, face enlarged (`eyes-close-up-2.jpg`): b008 and
  a first fix (the shadow body keeps the game's own head materials alive and the body takes them from there) both
  have no eye shine; **with the head step switched off altogether the eyes look like the plain game's**. So it is
  the head step, and not (only) its materials `[measured, n=1 each]`. Second fix in the source, NOT SEEN YET (both
  runs crashed at the start): in a cutscene the game's own shadow setup comes back whole (the body casts its shadow,
  the hidden copy does not), and the cutscene's body changes wait one second (`CUTSCENE_BODY_DELAY_S`).
- *"i heard the windows error sound twice and my controller buttons didn't work ... i could still select things up
  and down, just pressing A didn't do anything"* (main menu; Tefa used the keyboard). The log has the controller's
  X, Y and B presses at the main menu and NO A at all: A is taken before our button layer sees it `[measured, n=1]`.
  Not looked into further.

## Left

- Tefa's eye in the headset: the screen's size and distance, the switch in and out, the ten-second blend at the end.
- Other cutscenes (only the opening was run); a cutscene whose camera uses a fixed film shape (black bars in the plain
  game) would show extra picture above and below here.
- Mirrors in VR PLAY are a different job (the frame there is really drawn for the head): the component's input
  override, with the head's place and the drawn angle.

## 2026-11-13 00:00-01:45 (Opus, the UEVR connector in from here on: Tefa, "use the uevr mcp now for everything")

### The crash and the black eyes: one cause, found with the connector `[verified-live, simulator, DLSS on]`

- Rate runs first: the head-and-shadow step off = 4 clean runs of 4; on (b008) = 2 crashes in 3; our plugin out = 4 of 4
  clean. So it was ours and it was that step `[measured, n=3-4 each]`.
- The connector: at a cutscene's start the game builds `<material in the slot>_Animated` (a dynamic material) for
  James's eyeball slot (slot 10, `CH_James_Eyes`); with the step off it is `CH_James_Eyes_Animated` (parent
  `CH_James_Eyes`), with the step on it was **`MI_Invisible_Animated` (parent `MI_Invisible`)**: the game animated
  his eyes on our invisible material. No ProcessEvent call makes it (native). Black eyes for sure; the crash
  `[inferred: the animated-eye code working on a material without its parameters; with the eyeballs left alone
  4 runs of 4 were clean]`.
- Fix (b012): the eyeball slot is never touched (`SHADOWS_HEAD_KEEP_WORDS`); in play the eyeballs are hidden by their
  BONES instead (`eye_l`, `eye_r`, `eye_Lt`, `eye_Rt`, read live: `head_bn` is 15, `eye_r` 17, `eye_l` 22,
  `eye_Rt` 52, `eye_Lt` 53), shown in a cutscene. b010 (eyeballs kept, no bone hiding) had **the two eyeballs floating
  in front of the camera in play** (simulator still, `runs/rt-check.png`, not committed); hidden live by bone they went
  and nothing else did. Simulator: cutscene eyes as in the plain game, play clear, no crash `[n=1 + 3 with the eyes
  kept]`. The other head slots still swap to the invisible material in play and back in cutscenes (the shadow body
  holds their own materials alive).

### Tefa's second wear (b010, 00:4x, Quest 3)

Eyes right; the cutscene screen further (4.5 m) not commented. New: *"the mirror in the far back [main-menu bathroom]
has a glint of light run across it in the left eye, right one is steady"*; *"I can see double image in my right eye,
but not left"*; A dead and no button advancing the main menu (keyboard used); *"the windows error/crashed sound in the
beginning of the cutscene and at the end of it"* (the two moments our flat screen switches); *"during the cutscene
when James is already outside talking a monologue, the background keeps, after every 7 seconds maybe, smoothly turning
darker and then smoothly coming back"*.

### The darkening: jbusfield's 10-second renderer reset `[measured + read]`

Simulator stills: the monologue dips darker for a moment exactly every 10 s in VR (157, 167, 177, 187 s), never in the
plain game. Tefa: *"it's something of jbusfields"*; it is `fix_no_raytracing_visual_glitches` (main.lua
`onRaytracingVisualGlitchTimer`, `fix_visual_glitch_seconds` 10 in `data/sh2_config.json`): ray tracing on for 0.5 s
and off again every 10 s, against mismatched lighting in the two eyes with ray tracing off. jbusfield (to Tefa): turn it
off, *"but then you will likely get mismatched shadows indoors. If you now have the extra headroom from ASW you can
check Enable Raytracing"*. Tefa: AFW + DLSS + ray tracing is the way. So: reset off (b011), ray tracing on (b013;
checked live: `r.Lumen.HardwareRayTracing` 1 at start). **Cost: +2.1 ms a frame (+15 %), 13.9 -> 16.0 -> 13.9 ms, the
armory, simulator A/B/A; card 59 C max** `[measured 2026-11-13, n=1]`. Unworn.

### The right eye's one-frame wrong picture `[measured, simulator; Tefa: "that only occurs in right eye"]`

OBS's recording of the game window (60 fps), every frame scored against its neighbours: in play the window shows for
ONE frame a different picture (a see-through, squeezed and shifted copy of the scene beside another view;
`runs/flicker/obs-odd-frames.jpg`), never during the flat-screen cutscene. Ray tracing off (01:12 run): 10 in about
45 s of play; ray tracing on (01:19 run): 416 in about 4 min (median gap 0.27 s). The window recordings of Tefa's real
headset wear show none, yet Tefa sees a double image in the right eye there: the window does not show it in that set-up.
Very likely the same thing as the see-through second picture of 2026-11-12 00:35 (DLSS on, real headset) and the right
eye fault while flat. Ray tracing makes it about eight times as frequent. NOT looked into further.
