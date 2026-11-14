# Why the hands jump in warp mode 2, and a mode picked per frame (2026-11-13 22:10-22:50, PC1, Fable, the simulator; Tefa's headset test still to come)

Continues `2026-11-13d-inside-puredarks-frame-warp-first-look.md` §7. PureDark's permission and its limits are quoted
there (this game only; findings, never extracted code). The disassembled shaders stay in the Developer PC's scratch.
Build: sh2vr b019 (`warpscale.cpp` new), beta 6 of the warp, DLSS as Tefa has it, the hotel-room save, the pipe in hand.

## 1. The fault reproduced on the simulator `[verified-live 2026-11-13, simulator, n=7 runs of 22 consecutive frames]`

A steady stick turn through the connector (right stick 0.45, about 2 px a frame at the simulator's small consecutive
frames, 160 x 175 an eye, its "flicker window" capture), the simulator's hands held still in front of the face. The
room's shift per frame by phase correlation on the top of the picture, the hands by the centre of the skin-coloured
pixels (`dev-archive/tools/warp-study/`). In `VR_AFW_FramewarpMode` 2 the room moves evenly (sd 0.3-0.5 px) and the
hands alternate every frame by 0.7 to 1.9 times the room's per-frame shift (both eyes, every run): Tefa's "two steel
pipes". With the view still: 0.05 px.

## 2. The hypothesis is disproved: our arms' motion data is fine `[disproved]`

The hypothesis of §7 (UE's velocity says "still" for our arms and the weapon, so the warp moves them like walls):

- **No motion setting changes mode 2's flip** `[verified-live, n=1 run each]`: `VR_AFW_FixObjectMotionVector` off,
  `VR_AFW_FixMovingObjectBrightnessFlickering` off, `VR_GhostingFix` off, `VR_AFW_IgnoreMotionThreshold` 0: the hands
  alternate just the same (0.5-1.9 of the room's shift). If mode 2 used the motion of objects, taking it away would show.
- **In mode 3 the warp's debug view paints the hand and the pipe green on every made-up frame of the turn**
  `[verified-live]`: it knows they are moving objects, from our meshes' velocity. (Debug: Numpad 6 sent as a real key;
  the setting name does nothing in beta 6. It tints only in modes 1 and 3; mode 2 shows no tint at all.)
- The sleeve near the eyes is not green, and no setting or scale changes that: a turn by stick pivots a little in front
  of the eyes, and what sits at the pivot's distance does not move against the scenery, so there is nothing to mark and
  nothing wrong there `[inferred-static]`.
- The reader's static reading of the warp's programs (30, 41-52), `[inferred-static]`: in the previous-frame path a
  pixel lands by the camera matrices and depth only; an object's own motion only chooses the colour, and only in the
  path the combined mode uses. Nearer than 1 unit (metres) mode 2 re-projects by the head's rotation alone. So mode 2
  shows any near thing that moves against the world one frame behind, by design. Units are metres
  `[verified-live: describing the scene 0.25 times its size sends the hands down the nearer-than-0.1 path, magenta]`.

So nothing in how we pose the arms can cure mode 2. What would, inside the warp: in mode 2, carry the pixels it already
marks as moving objects by their own motion (the mask and the velocity are there), filling behind them from the previous
frame. That is PureDark's to change; this is what to tell him.

## 3. Tried and of no use: describing the scene larger to the warp

`warpscale.cpp` stands in the open injector's delay-load import of the warp's `EvaluateFrameWarp` (UEVRBackend.dll,
PureDark's public fork; the warp's own file is untouched) and can hand it the camera data scaled k times (positions
and the near plane; every pixel lands the same, every distance the warp measures is k times longer). It reaches the
warp (k = 0.25: magenta hands) but the picture's faults do not come from those distance lines: k = 4 and 16 change
nothing useful. Left in at 1 (off).

## 4. What is built: mode 2 while calm, mode 3 while the view turns or a hand moves fast (b019, unworn)

The same stand-in makes a call that asked for mode 2 as mode 3 while the view turns more than a threshold (degrees a
frame, from the camera data itself) or a controller moves faster than a speed through the room, plus 12 frames. So
the clean edges of mode 2 stay while calm, and the hands are carried while something moves, when mode 3's outline is
hardest to see. Off by default (`settings::WARP_TURN_DEG_PER_FRAME` 0); `sh2vr_warpscale.txt` in the profile holds
"scale degrees speed", read once a second, so it can be flipped live. In Tefa's profile now: `1.0 0.3 0.5`.

Simulator `[verified-live 2026-11-13, n=3 runs]`: with 0.3 degrees the turn's calls are made in mode 3 (121 and 357 of
573 in ten seconds, 2-3 switches), the hands' alternation falls from 1.0-1.9 to 0.3-0.8 of the room's shift (the skin
measure is coarse: mode 3's fill colours move it too). View still: mode 2 (24 of 647 calls, from placing the hands).
A hand swung at 2 Hz, view still: 368 of 628 calls in mode 3. No refused call, no crash, the profile's settings
unchanged after the quit.

NOT established: how it looks in the headset (the outline during turns, a visible change at the switch between the
modes, the frame cost: mode 3 was 0.4 ms dearer than 1 on 2026-11-13 morning, mode 2 never measured); the right
thresholds at the real headset's 55-72 frames a second (0.3 degrees a frame is about 20 degrees a second there).

## 5. For Tefa's headset test

Start as always (Steam, UEVR by the connector). With the file as it is, the switch is on. Live, by the session:
`1.0 0 0` = plain mode 2 (the old picture), `1.0 0.3 0` = for turns only, `1.0 0.3 0.5` = turns and fast hands.
Look at: the pipe and the left hand while turning by stick and by body; waving them with the head still; the edges of
the hands while turning (the outline); any flash at the moment the turn starts or stops.

## 6. Tefa in the headset (22:50-22:56): the per-frame mode is a flop `[reported 2026-11-13, n=1 wear]`

With `1.0 0.3 0.5` the log shows mode 3 on most frames while Tefa moved (408-662 of 720 calls in ten seconds), and
Tefa: *"shimmering around the edges is back and the hands underneath still do what they did, like teleport real
quick"*. So in the headset mode 3 does not carry the hands either; it only repaints them (the shimmer). Put back live
to plain mode 2 (`1.0 0 0`), Tefa: *"it's like it was"*. The switch stays in the code, off.

## 7. Next: take the hands off the warp's plate (Tefa: "that is worth a good try")

Our stand-in sees the warp's made-up picture on its way out, the last real picture of the same eye (colour and depth),
and the controllers' poses now. Plan: after the warp's call, on its own command list, our own drawing step cuts the
near pixels (the arms and the weapon, by depth) out of that eye's last real picture, pastes them where the controllers
say they are now, and paints over the copy the warp left behind. Step one on the simulator: pasted back unshifted
during a stick turn (hands locked to the view), measured with the flip measure. Then the shift by the hands' own
motion, then the clean-up. `MODEL: FABLE`.

## 8. b020: the hands' own pass works on the simulator (23:05-23:45, Fable)

`dev-archive/native/src/handpaste.cpp`, called from the stand-in right after the warp's call, on the injector's own
command list (after that call the injector only closes the list, so nothing of its state is disturbed). One full-picture
pass over the made-up eye's picture:
- **pasted:** where that eye's last real picture had something nearer than 1 m that the game's own velocity picture
  marks as a moving object, the pixel is copied as it was (no shift yet). Our own copy of each eye's real picture is
  kept each frame (the warp writes its result over the injector's per-eye picture); the injector's per-eye depth and
  velocity pictures last until that eye is drawn again and are read in place.
- **painted over:** where the warp put those pixels (for near pixels: the head's turn alone, the same for every depth,
  so one 3x3 map built from the two views' matrices), the warp's picture from just beyond, along the way the room went.
- the rest: the warp's picture.
Two small programs compiled at start (D3DCompile); drawn into our own target, then copied over the warp's picture.

`[verified-live 2026-11-13, simulator]`: with the tint (file's 4th number 2) the whole arm, hand and pipe are pasted
on every made-up frame of a stick turn and nothing of the room is; the painted-over copy is a strip a few pixels wide
on one side. The hands' flip, left eye: 1.25-1.5 of the room's shift without, 0.3-0.85 with (n=3 runs each; the right
eye's skin measure is spoiled by warm lamp light in view). Frame time 18.3 ms with and without (n=2 each). 600-900
made-up pictures redrawn every ten seconds, none refused. A start and a load with the pass on: clean (n=1).
Faults met: the first build held plain pointers to the injector's pictures and the game died at the level load, when
the injector replaces them (now each is held for the frame it is still needed); one run sat on a black picture after
the load with the tint on (not seen again in two starts, cause not found; watch for it).
NOT built yet: the shift by the hands' own motion (a waved hand is still drawn where it was one frame ago, as in plain
mode 2, only without the room's shift on top); soft edges (the depth picture is coarser than the colour picture);
a filler better than "the picture from just beyond"; what happens with an enemy nearer than 1 m.
In Tefa's profile now: `1.0 0 0 1 1.0` (the pass on, the per-frame mode off). Headset verdict open.

## 9. Tefa wears b020 (2026-11-14 00:30) `[reported, n=1 wear]`

First start: hung on the loading screen and crashed: the old UEVR crash of 2026-10-31 and 2026-11-12 (reading 0xa0,
UEVR's stereo hook meeting an engine null at the load; sh2vr not in the stack). Second start fine.
Tefa: *"this is definitely a big step up!! ... there is now one pipe and it is just smudging when moving"*, *"it
still leaves like a trail behind when moving"*, and *"the hands are moving with less FPS than the game world"*. As
expected of step one: the pasted hands are the last real picture unshifted, so per eye they update only on real
frames (half rate). Next: the shift by the hands' own motion (head-relative controller motion since that picture).

## 10. b021-b023 (2026-11-14 00:35-01:20, Fable): the hands carried by their own motion, and what "hands" means

- b021 (a flat shift per hand, the hand chosen by which is nearer in the picture): Tefa `[reported, n=1 wear]`:
  *"smoother now"*, but close up the gun *"teleporting around seemingly randomly just a little bit"*. Tefa's clip
  (`VirtualDesktop.Android-20261114-004344-0.mp4`, 4.6-5.2 s, every frame): bands of the pipe's edge jump sideways for a
  frame or two `[measured, n=1 clip]`. And Tefa on simulator pictures with the tint: the straight edge of the pasted
  area (the 1 m line across a table) *"is exactly where the glitching starts"*.
- b022: each pixel carried in depth by its controller's rigid move (place AND turn, head-relative, from then to now),
  blended between the hands over 15 cm; a few "try, see where it lands, correct" steps find the source pixel.
- b023: "marked moving by the game" was no test. This game's velocity picture: a still thing's pixel is NOT exactly 0
  in red (it decodes to about -2 screens), a movable thing's pixel holds its motion in the picture. So the test is now:
  near, written (red >= 0.01), and its motion differs from what a still thing at that depth shows between the eye's
  last two real views by more than 0.004 (file's 6th number). `[verified-live 2026-11-14, simulator, tint, n=2 captures
  each]`: all still: nothing pasted; one hand swinging: that arm and the pipe only; stick turn: both arms and the pipe;
  the near table and lamp never. Diagnostic view: 4th number 4.
- The file now: `scale degrees speed pass near against`; in Tefa's profile `1.0 0 0 1 1.0 0.004`.
- The DLL Tefa found in Downloads (PureDark's, shared once for a close-up fault): a third build, neither beta 5 nor 6:
  beta 5 with shaders 30, 48-51 changed `[measured]`. Kept out of git; to be tried once the hands are settled.

## 11. Tefa wears b023 (01:15-01:25), five clips; b024 (01:35)

Tefa `[reported, n=1 wear]`: *"it's much better again!"*; walking with the gun out in a good frame-rate area *"looks
superb"*. Left: the gun trailing and the gun's shadow doubling while stepping left and right; *"table still tearing in
front of me, like a clear line where it begins"*. Clips `VirtualDesktop.Android-20261114-0117xx ... 0123xx` (the middle
three fifths are the useful part; the recording is ONE 2880 x 1440 view, not two eyes), read frame by frame:
- table (011722): the tabletop holds; the red sheet on it (a movable thing, written in the velocity picture) shows a
  doubled corner on alternate frames: the "against the room" test lets a near movable thing through when the head moves.
- shadow (012220): the gun is the same in every frame; beside it a band of evenly striped picture on alternate frames:
  the painted-over strip (the picture from beside, taken in steps). The gun's shadow on the wall is the wall's shading:
  the warp shows it one frame old on made-up frames. Not ours to carry without knowing which wall pixels are shadow.
b024: a pixel must also be within reach of a controller (16 cm of the line it points along or stands up along, 22 cm
of the line to that shoulder); the painted-over strip is filled from the other eye's picture of the same frame
(carried over with the background's depth taken from just beyond the strip; the picture from beside only where that
eye's own gun is in the way). Simulator, tint: still / swinging / turning as b023 `[verified-live, n=2 captures each]`.

## 12. Tefa wears b024 and b025 (01:40-02:05), twelve clips; where it stands

- b024 `[reported + measured, clips 0141xx-0143xx]`: Tefa: *"didn't really improve nothing ... maybe the pipe looked like it
  had doubling"*. Frames: the pistol is the same in every frame; the pipe and the shotgun's pump are CUT and shifted
  on alternate frames along a curved line: the reach test's lines along the controller's axes are too tight for a long
  weapon held at an angle. b025: the reach test off (`settings::HANDPASTE_REACH_ON` false), the other-eye fill kept.
- b025 `[reported + measured, clips 0154xx-0204xx]`: Tefa: *"this works well!"*. Frames: both hands waved outdoors:
  single and whole in every frame; the pipe whole again; pistol, shotgun, rifle whole.
- Still there:
  1. a loose movable thing lying near (the red save sheet, a note on a desk) shows a notched corner or a torn edge
     on alternate frames while the head moves: it passes "near, written, moving against the room". The clear line
     Tefa sees is where the 1 m line crosses it. To do: the held weapon's real size from the game (its bounds against
     the controller) for a reach test that fits, and the "still thing" motion checked against the game's own motion
     vectors (the DLSS input) instead of our matrices alone.
  2. the weapon's shadow on the floor flips between two places on alternate frames, in ONE view (Tefa: one eye closed,
     like the pipe did): the warp shows the lit floor one frame old. Not carried by our pass.
  3. a soft darker edge beside the pipe on some made-up frames (the fill).
- Tefa: weapon smoothing wanted, the proper one (*"yeah let's build a proper one"*): gun and arm together, guns only,
  never melee, switched by what is held (the standing rule). jbusfield's "Smoothing" in the arm settings (0 in the
  profile, carried into ik.cpp) smooths the arms only and has no guns-only gate: not used for this.
- Tefa on the whole: with settings lowered or adaptive for a solid 72, *"a very solid way to play this game in VR"*.
