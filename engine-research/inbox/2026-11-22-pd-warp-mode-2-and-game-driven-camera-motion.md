# Does the frame warp's made-up picture carry game-driven camera motion? What the files say (2026-11-22, the reader, files only)

From: the background reader of the 2026-11-22 live session. Nothing launched. Read: notes 2026-11-13c/d/e, the
profile's `config.txt`, the b038 wear log, PureDark's PUBLIC fork (branch AFW, commit fc8de10, 2026-08-20, fetched
fresh), and the warp's GPU programs in the installed beta 6 `PDAFWPlugin.dll` (PureDark's permission of 2026-11-13,
this game only; findings in words only, nothing extracted is kept in git).

The video measure this answers: with the view moved by the game (stick turn, walking sideways in front of a door) the
world steps on every other headset frame (36 a second); when the head turns, every frame moves.

## 1. Measured in the b038 log

- 716-720 warp calls every 10 s in play = 72 a second, one made-up eye per call. So each eye is real 36 times and
  made-up 36 times a second: "36 steps a second" is exactly "the made-up pictures do not move the world"
  `[measured: the `drawnpose:` lines, 16:40-16:55]`.
- The DRAWN eye's view in every warp call matched one of our per-tick records of the game's own view, with one fixed
  shift per load: 0 unmatched in every 10 s window of play (33-35 only in the window around each load)
  `[measured]`. So the views the injector hands the warp DO contain the stick turn and the walking, tick by tick. The
  made-up eye's target view is not something our log sees.
- The picture was 1.0 to 2.0 records behind the game thread, changing through the run (simulator, 2026-11-18: always
  2) `[measured]`. Harmless for the matching; noted because it is new.

## 2. Which code decides `[inferred-static]`

**The open side (public August source; the installed beta 6 is newer, so this is "as far as the public source goes"):**

- The made-up eye's target view is built in the SAME callback and from the SAME game camera place and turn as the
  drawn eye's (`FFakeStereoRenderingHook.cpp`, `view_inv_matrix_other`). Nothing there leaves the stick or the walking
  out.
- `VR_AFW_UltraResponsive` only switches off the smoothing of things attached to the controllers; the fork's own menu
  text says so ("This basically just disables lerping in UObjectHook Config tab"). It does not touch the warp.
- `VR_GhostingFix` decides two things in the warp's call: the kind of motion vectors the warp is told it gets
  ("this eye's last frame" when on, "the other eye's last frame" when off), and, together with
  `VR_AFW_FixObjectMotionVector` and `VR_AFW_FixMovingObjectBrightnessFlickering`, whether the game's velocity picture
  is handed over at all (`D3D12Component.cpp`, the lines around `EvaluateFrameWarp`).
- `VR_AFW_FramewarpMode`: 0 none, 1 other eye, 2 previous frame, 3 both. `VR_AFW_FixObjectMotionRange` and
  `VR_AFW_IgnoreMotionThreshold` (default 2.5, not in our config) are passed straight through.

**The closed side, mode 2 (the four programs that place pixels; the same branch in each):**

- A pixel of the eye's previous real picture is put back into the world with that eye's PREVIOUS view and projected
  with its view NOW. Both turn and place are in those views, so stick turn and walking are carried, for every pixel
  **further than 1 m**.
- **Nearer than 1 m the previous view is first moved to the eye's place now: only the turn is applied, every
  sideways or forward move of the camera is dropped.** This confirms the 2026-11-13e reading, and adds the condition:
  it applies to every near pixel when the warp's "eye mask" is off, and only to near pixels the warp has marked as
  MOVING when the mask is on. Near pixels marked still then get the full treatment.
- A pixel is marked moving when its motion vector differs from the motion the camera alone would give (worked out
  from the warp's own views) by more than `IgnoreMotionThreshold` / 2160 of the picture; nearer than 0.5 m the bar
  rises toward 10 / 2160.
- The warp's "previous view" is kept inside the closed file (the call only carries the views of now). Whether the
  mask is in use is also set there. Neither can be read from the open side.
- Two settings named `_IsMoving` and `_IsRotating` exist in the warp's settings block; none of the placing programs
  reads them.

## 3. Is "the world holds on made-up frames" expected for mode 2?

- **Walking sideways close to a door or a wall: YES, by design, if it is nearer than 1 m** (and marked moving, or
  the mask is off). The made-up picture turns it with the view but does not slide it. `[inferred-static]`
- **A stick turn: NO.** By the program a turn is carried on every path, near and far, and on 2026-11-13 the simulator
  showed the room moving evenly on every frame of a stick turn in mode 2 (`[verified-live 2026-11-13, simulator,
  n=7]`, beta 6, before our hands' pass existed). If far things also stand still on made-up frames during a stick
  turn in the b038 video, something outside what was read is wrong: the warp's kept "previous view", or the marking
  (a whole view wrongly marked moving), or a difference between the real headset and the simulator.
- Head turns looking smooth proves little: the headset's own last-moment turn correction smooths head turns on any
  frame, made-up or not `[hypothesis]`.
- What differs from 2026-11-13: the ghosting fix is now switched on 20 s after the start instead of from the start
  (b036), ray tracing is off, the game holds a full 72, and our hands' pass runs. Tefa on b035 (fix OFF, otherwise
  the same): "running super smooth"; on b038 (fix ON): "picture jittery/tearing" `[reported]`. That points at the
  ghosting fix's side of the call before anything else `[hypothesis]`.
- Our own pass does not touch anything further than 1 m (`handpaste.cpp`: every test starts with "nearer than the
  near line") `[inferred-static]`.

## 4. What the 46-setting sweep found about it

Nothing: it could not. The sweep of 2026-11-13 (note 13c section 6) had the headset lying still in one view and
measured frame time and a still-picture difference; all 46 settings were the game's own drawing settings. The warp's
settings were only timed (same note, section 3): Ultra Responsive off: no change in frame time; mode 1 for 3: -0.4 ms;
shading rate: -0.5 ms. No test has ever looked at how the world moves on made-up frames under stick or walking on the
real headset. `[measured, from the note and the sweep's own list]`

## 5. Cheapest live A/B (lines in `sh2vr_devcvars.txt`, 20 s each, same corridor, a steady stick turn, then a sideways walk)

First, from the video, with no game: **is the thing that stands still nearer or further than about 1 m?** Far things
standing still in a stick turn = the unexpected case. Only near things, only when walking = the 1 m rule.

1. `uevr VR_AFW_IgnoreMotionThreshold 100` (now 2.5). Raises the "moving" bar forty times. World moves on every frame
   now = the world was being marked as moving under game-driven motion. Put back to 2.5.
2. `uevr VR_GhostingFix false` (b035's state). World moves on every frame = the fix's motion-vector side. ⚠️ With the
   fix off no velocity picture reaches our hands' pass, and it then treats EVERYTHING nearer than 1 m as hand
   (`handpaste.cpp` `hand()`: no velocity picture = closeness alone): near walls and tables get carried with the hands
   and tear. That is very likely b035's "things close by are still tearing" `[inferred-static]`. For a clean look
   also set the 4th number of `sh2vr_warpscale.txt` to 0 for this step.
3. `uevr VR_AFW_FixMovingObjectBrightnessFlickering false`, then `uevr VR_AFW_FixObjectMotionVector false`: each takes
   the velocity picture away from the warp (same caution as step 2).
4. `sh2vr_warpscale.txt` 4th number 0 (`1.0 0 0 0 1.0 0.004 1 0`): our pass off. Expected: no change to the far
   world. A change = ours.
5. `uevr VR_AFW_FramewarpMode 1` for 20 s: the made-up eye is then built from the other eye's fresh picture, so the
   world must move on every frame. If it does there and not in 2, the fault is in the previous-frame path alone.
6. Mode 3 with the debug tint (Numpad 6 as a real key): green = marked moving. The whole world going green in a
   stick turn or a sideways walk, and not in a head turn, is the marking fault seen directly.

Each is undone by writing the old value back; none needs a restart.

## 6. Side finding for the start-crash notes

The "Inserting new scene state" / "Setting scene state to" lines are in the public fork
(`FFakeStereoRenderingHook.cpp`, about lines 3190-3220), so that code can be read as source. The "Setting" line is a
log-ONCE line: "one Setting line in the log" does not mean one swap. `[inferred-static, public August source]`

## Tags

- 72 warp calls a second, every drawn view matched `[measured, b038 log]`.
- Mode 2 drops the camera's move for near (under 1 m) pixels, keeps the turn, carries both further out
  `[inferred-static, beta 6's own programs]`.
- Ultra Responsive is not involved `[inferred-static, public source]`.
- Why far things would stand still in a stick turn `[not established]`; candidates `[hypothesis]`.
