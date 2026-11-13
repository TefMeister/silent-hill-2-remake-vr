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
