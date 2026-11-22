# 2026-11-22b: Tefa's nine-point video of b038, read frame by frame

/lm, Build PC, Fable. No game run for this note. The video: `D:\Capcut export\SH2 stuff to fix.mp4` (4290 frames,
2880 x 1440, 60 a second, ONE eye). Work files (sheets, crops, the measuring scripts `diffs.py`, `shift.py`, `rel.py`):
logs folder `2026-11-22-1725-nine-point-video/`. Frame numbers below count from 0 in the video.

## What the recording is, and what it cannot show
- It was made with the headset's own recorder (the Quest quick-settings panel is on frames 3-45) `[measured]`. So it
  is the picture after Virtual Desktop's video stream and the headset's own re-aiming: fine detail is softened by the
  stream, and the picture's left edge shows a smeared block of stretched pixels in many frames (the edge fill).
- The recorder keeps 4 of every 6 headset frames (the steps go 1, 1, 2, repeat, 2 headset frames; one exact repeat
  in every 5 video frames) `[measured, parts 3 and 6: 471 of 590 and 311 of 390 frames distinct]`. Single-frame
  steps are in it, so made-up (warp) frames ARE recorded; a fault that lives on one frame can still be missed.

## The nine parts (cuts found by eye on a one-a-second sheet, then by frame difference)
| point | frames | what the frames show |
| --- | --- | --- |
| 1 picture jittery / tearing | 0-435 (prison door, torch) | No torn frame found: every frame looked at is whole (53-58, 95-100, 127-131). But while the view slides sideways (51-64, 80-118, 125-148, 162-190, 195-218) the world moves only on every OTHER headset frame: the steps read 12, 12, 0, 12, repeat (480-wide measure), where an every-frame move would read 6, 6, 12, 12. The in-between frame holds the world still while small things change (frame difference 1.5-4 against 0.12 for a true repeat). = 36 steps a second: stepped, which is Tefa's "jittery" `[measured]`. In parts 3 and 6 the world moves on EVERY frame (7, 7, 14, 14 and 3, 3, 6, 6), so the warp carries some motion and not other; which kind of move part 1 was (stick, or the head moved sideways close to the door) is not in the picture `[open]`. |
| 2 James's hair shadow | 436-901 (red cell windows) | Clear: five to eight long thin strand shadows fan down the wall from the top of the view in the torch light (470-590), swinging with the head; a small head-shaped blob low on the wall at 720 `[measured]`. Hair strands, not the head mesh. |
| 3 gun and hands shaky | 902-1491 (shotgun, both hands) | Hand and gun measured against the wall behind: 61 back-and-forth pairs in 470 frames; at 1012-1024 the hand hops about 6 px (of 1440; ~12 px full size) one way and back on alternate frames, which no arm does: real and made-up frames disagree about where the hand is `[measured]`. When the view is steady (1030-1040) the hand is steady to 1 px. |
| 4 posts flicker, too bright, thin fog | 1492-1919 (fence, road) | The fence post against the sky changes shape frame to frame: a dark doubled top at 1601-1602, thin again at 1603 `[measured]`. Brightness and fog cannot be judged without the same view from the flat game: needs the A/B pictures. |
| 5 leaves flicker | 1920-2309 (trees against sky) | Flicker measure 0.51 (as part 4, 0.55; the still puddle part 8 is 0.15), strongest at the top where leaves cross bright sky. Not yet looked at leaf by leaf. |
| 6 hand shaky, left the same | 2310-2881 (hand held out, walking) | In 2460-2850 the hand held out while walking is steady against the ground: 0 back-and-forth pairs, every step even to ~1 px `[measured]`. So the shake Tefa sees here is under 2 px full size, or on the frames the recorder skips, or in the other eye. Ghost Hand's report of the same run: the game's hand jumped 563 times while the controller was calm; left hand wanders 3.2 cm. |
| 7 puddle reflections vanish looking down | 2882-3287 | Clear: looking along the street the puddles mirror the shop, the pole and the sky (2990, 3060, 3180); looking down at the same wet ground it is flat grey with no mirror picture at all (2900, 3240) `[measured]`. The mirror picture exists only while its source is on screen. |
| 8 pixels shifting in puddles | 3288-3757 (still view) | Lowest frame change of the video (0.63) with the flicker concentrated in the puddle rows; not yet zoomed. |
| 9 flying leaves leave trails | 3758-4289 (houses, wet street) | Flicker concentrated on the wet street under the leaves; the trail itself not yet isolated in a crop. |

## The sort (said to Tefa before any build)
(a) What ray tracing off costs: 7 for certain (screen-only mirror pictures); 4's brightness and fog and 8 likely.
    Test: one switch at a time, pictures at the same spot, and the cost in frames.
(b) The warp and DLSS on thin moving things: 1 (the world held still on made-up frames for one kind of move), 4's post,
    5, 8, 9.
(c) Ours: 2 (hair strands' shadow), 3 and 6 (the carried hand disagreeing between real and made-up frames).

## 17:35 to 17:50: point 4 found and fixed (b039): one of our own switches removed the town's fog
Simulator, town start save, the plugin's value-keeper paused (`sh2vr_notune.txt`), values set through the connector one
at a time, 12 s to settle, 45 frames of the game window averaged (`dev-archive/recon/2026-11-22-light-ab/ab.py`,
picture beside it) `[measured 2026-11-22, simulator, n=1 per row; b038 row repeated at the end, n=2]`:

| what was set | brightness: whole / top third / bottom third | flicker |
| --- | --- | --- |
| b038 as worn | 52.5 / 51.4 / 57.6 | 0.17 |
| + `r.AOGlobalDistanceField 1` (the game's own) | 57.5 / 82.9 / 47.8 | 0.18 |
| + `r.DistanceFieldAO 1` | 57.0 / 81.3 / 47.7 | 0.15 |
| + probes 16 | 57.6 / 82.7 / 47.5 | 0.15 |
| + the three reflection values at the game's own | 57.1 / 82.2 / 47.0 | 0.15 |
| + hardware ray tracing on (the game's full own look) | 54.0 / 77.6 / 46.3 | 0.13 |
| b038 again | 53.5 / 51.7 / 59.3 | 0.18 |

With `r.AOGlobalDistanceField 0` the dense fog over the street is simply not there and the wet road shines; with 1 the
fog is back and the ground darker, within a few points of the game's own picture with ray tracing on. The other
switches change almost nothing at this spot. The 2026-11-13 "Tefa could not tell" verdict on that switch was made
indoors (the armory). **b039** = tuning.cpp without that row; in play the value reads 1, the other seven held
`[verified-live 2026-11-22, simulator, n=1]`. Not measured: what it costs in frames (the 2026-11-10 waterfall put the
global distance field at 1.7 ms of a 25 ms frame): real headset next. Not checked: whether puddle mirrors (point 7) gain
anything from it (software tracing can use the field for off-screen hits).

## 17:50: point 2, the hair shadow (b040), from the reader's static pass
The reader's three inbox files of 2026-11-22 (`engine-research/inbox/2026-11-22-pd-*`). For point 2 `[inferred-static,
each link read from a file; the chain not seen live]`: shadows.cpp hides the head's 14 material slots on the body and
lets a hidden COPY of the body, holding those materials (eight are hair), cast the shadow; that copy was on lighting
channels 0 and 1, the torch lights 0 and 2 and sits at the eyes (the pack's light type "Head"), so a light inside the
hidden head shone through its hair. b038's log: "channels 110" `[measured]`.
**b040:** the copy is on channel 1 only (`settings::SHADOWS_COPY_ON_TORCH = false`). Log on the simulator, armory:
"channels 010", no crash `[verified-live 2026-11-22, simulator, n=1: the setting, NOT the look]`. The strands were not
looked for on the simulator; Tefa's eyes decide. What goes with it: the torch no longer draws any shadow of James's
body or head (as in the flat game; from a light at the eyes those fall behind what casts them); gun and arms unchanged.

## Still open, from the reader (nothing of this tested)
- Point 1: the frame warp (mode 2) places pixels nearer than 1 m by the TURN only and drops every sideways move of the
  camera, by design `[inferred-static, the installed warp's programs]`; a door at arm's length while stepping sideways
  would therefore step at 36 a second. A stick turn should not. Which it was in part 1: a question to Tefa. Live tries,
  20 s each through `sh2vr_devcvars.txt`: `uevr VR_AFW_IgnoreMotionThreshold 100`, mode 1, the debug tint.
- Point 3: the leading reading is "the hand is right, the wall is one tick old" (the same 1 m rule) `[hypothesis]`;
  refused carries, wrong records and raw-controller carries are ruled out by the b038 log `[measured]`. Decides it:
  the 7th number 0 / 2, the 4th number 0 and 2 (tint), walking.
- With no velocity picture our hand pass treats everything nearer than 1 m as hand: likely b035's "things close by are
  still tearing" `[inferred-static]`.

## 18:05 to 18:50: b040 on the real headset (lying still), and the sidestep test that did not settle
Tefa: headset connected; and to the question about part 1 of the video: "sidestepping" (in front of the prison door).
That fits the reader's reading of the warp (pixels nearer than 1 m get the turn only) `[reported + inferred-static]`.

**b040, real headset lying still, the f4decomp sweeps paused** `[measured 2026-11-22, n=1 per spot, 20 s of the card's
load once a second (nvidia-smi) and the plugin's own "alive" frame average; NOT the frame meter, see below]`:

| spot | frame average | card | b038 the same afternoon (the meter) |
| --- | --- | --- | --- |
| town start | 13.9 ms (72 a second) | 92% (top 96) | 72.0, card 74% |
| prison armory | 14.0-14.1 ms (71) | 96% (top 97) | 71.7, card 95% |
| room after the garage | 13.9 ms (72) | 95% (top 97) | 72.0, card 86% |

So the fog (the global distance field back on) holds 72 lying still but takes the town start's spare card power: the
card is at its limit at all three spots. The slowest 1% is NOT known for b040. Candidates to buy room back, each to be
judged by Tefa's eyes: shadow size 1024, corner shadows off, local brightness balance off (the Optimizer's tour of the
same morning, never made permanent).
**The frame meter wrote nothing in three rounds** though its layer was registered, switched on for the game and loaded
in the game (`lanes_tune_meter.dll` in the module list) and the game drew 72 a second: no new csv, the live file
untouched since 16:16. It was "not registered" when this session began (round 1 = my miss: I did not check before
measuring; round 2 = registered and still silent). Cause not found; on the fault list. Left as found (off).

**Stick turn on the pretend headset (armory, mode 2, as built):** the picture moves on every frame, both eyes
(every-other-frame ratio 0.91-0.97, twice) `[measured, simulator, n=2]`: a stick turn IS carried by the made-up frames.
**Sidestep on the pretend headset: not settled.** `dev-archive/tools/warp-study/strafe.py` (new) pushes the stick
sideways and compares each frame's whole-picture step. One run read uneven steps (ratio 0.55 and 0.70, right eye
0.9 / 0.6 / 1.1 / 0.6 ...), which is what "near things held still on made-up frames" would give; the repeats read
0.86-0.97, and later runs had no motion at all (James against a wall). The captures stayed 160 px wide although the
simulator window was made 2560 x 1400, and each run started from a different place. Tries with
`VR_AFW_IgnoreMotionThreshold 100` and modes 1 and 3 are therefore worth nothing `[not established]`. What the test
needs: the same start place every run (a teleport or a reload), a wall at arm's length filling the view, and
full-size captures. The settings were put back (mode 2, threshold 2.5; config.txt unchanged).

## 18:55: Tefa on point 1, word for word
"if you are doing this to measure the jitter and tearing around close objects, then i doesn't matter which way you
move, ANY close thing does that, as if james has a dome around him and everything in that dome causes things to
jitter" `[reported, Tefa, worn, b038 and before]`. That is the observation; it matches the reader's static reading
(the warp's mode 2 treats pixels nearer than 1 m differently: the turn only, every move of the camera dropped) better
than anything measured here, and it says the test need not be a sidestep: any move with something inside ~1 m. Two
things own a 1 m line: the warp's own programs, and our hand pass (`sh2vr_warpscale.txt`, "near 1 m"). Telling them
apart, live, in the headset: our pass off (4th number 0): dome still there = the warp's own; then
`uevr VR_AFW_IgnoreMotionThreshold 100`, then mode 1.
