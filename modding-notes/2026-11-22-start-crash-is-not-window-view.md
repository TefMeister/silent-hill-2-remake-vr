# 2026-11-22: the start crash is not Window View; it is UEVR's Ghosting Fix holding a dead scene state

/lm, Build PC, OpenXR Simulator (the pretend headset), nobody at the PC. Build b033 as installed (unchanged).
Hand-on job: firm up last night's "no start crash with Window View's OpenXR layer off", then The Optimizer's tour.

## The count: Window View's layer off, 11 menu starts

Window View's two OpenXR layer values set to 1 (off) in the registry list, backed up first, put back to 0 after. The
other layers (OBS mirror, xrFPS, Hand Freeze, and the machine-wide ones) as they were. The Optimizer's frame meter on.
Each start: the mod's launcher `SH2VR.exe play --sim --no-install`, Enter through the warning screen by the connector,
stop at the main menu widget (`MainMenuWidget_BP_C /Engine/Transient`), 20 s at the menu, the meter read, the console
`quit`.

| start | time | result | to the menu | meter frames / waits |
| --- | --- | --- | --- | --- |
| 0 (probe, by hand) | 12:20 | clean | ~75 s | 5726 / 5726 |
| 1 | 12:23 | clean | 68 s | 5139 / 5140 |
| 2 | 12:24 | clean | 68 s | 5162 / 5163 |
| 3 | 12:26 | clean | 72 s | 5762 / 5763 |
| 4 | 12:28 | **crash** 26 s (`[Exception Handler] Exception address is null`, a 480 ms stall before it) | - | 149 / 150 |
| 5 | 12:28 | **crash** 30 s (crash dump; the stall 399 ms and the scene-state line; the script missed the handler line, its log copy has only the last lines) | - | 166 / 167 |
| 6 | 12:29 | clean | 71 s | 5774 / 5774 |
| 7 | 12:30 | clean | 73 s | 5989 / 5989 |
| 8 | 12:32 | clean | 67 s | 5335 / 5335 |
| 9 | 12:34 | clean | 72 s | 5893 / 5894 |
| 10 | 12:35 | **crash** 32 s (crash dump, a 433 ms stall and the scene-state line) | - | 170 / 171 |

**3 crashes in 11 with Window View's layer off** `[measured 2026-11-22, simulator, n=11]`. So last night's
"very likely Window View" is wrong for the crash: the 8-in-19 against 0-in-12 was luck, or Window View only shifts the
timing. The simulator is not the same runtime as Virtual Desktop, but the dumps below show the same crash on both.
No empty meter today: every start that reached the menu counted all its frames.
Kept: `D:\claude video game stuff\logs\silent-hill-2-remake-vr\2026-11-22-1220-starts-window-view-off-sim\` (the
table as starts.jsonl, the three crash logs and minidumps, the counter script `starts.py`).

The pretend-headset tour was NOT run: the hand-on ran it only if the starts were clean.

## What the crash is (the reader, three passes; inbox files of 2026-11-22)

- All six dumps (11-18, 11-21 15:11, 21:47, 22:33 on the headset, today's starts 4 and 5) crash on the game's render
  thread in the same call chain, calling through one object whose virtual table is gone (memory freed and reused)
  `[measured in the dumps]`. Window View's layer was not loaded in today's two.
- In the 5 dumps with a full log, that object is exactly the address UEVR last logged as "Setting scene state to"
  `[measured]`. The log order (stall, "Inserting", "Setting", the null line) is the same in crashing and clean starts;
  a crash follows the null line by 0-41 ms. It is a race, and the log cannot tell in advance which way it goes.
- No UEVR source is on disk; the reader disassembled the UEVRBackend.dll in use (AFW beta 6). The lines belong to
  UEVR's **Ghosting Fix** (`VR_GhostingFix=true`, our profile's config.txt line 80, with `VR_RenderingMethod=3`): for
  the second eye it takes one of the game's own scene states, keeps the raw pointer and writes it into the view every
  frame, never checking it is still alive; it picks again only when eye 0's state changes and more than 90 frames
  passed. The game owns and frees the state; UEVR takes no reference `[inferred-static, strong]`.
- Smallest changes on our side, in order: (1) `VR_GhostingFix=false` (the clean test; the AFW notes say to keep it on,
  so expect ghosting to check in the headset); (2) inject UEVR later, past the start stall; (3) start with the fix off
  and switch it on live at the main menu; (4) a guard in our plugin that clears a dead state pointer before eye 1
  draws `[hypotheses]`. No upstream fix is known in our research notes (AFW still beta 6, nightly 01143).

## The empty meter (last night), re-read

In all 3 empty runs UEVR's log shows its eye pictures torn down at session start and the rebuild failing ("D3D12:
Failed to create swapchain" 5, 77, 484 times), then "AFR swapchains not created" ~53 times a second: the game never
handed the headset a frame, and the meter counted that correctly `[measured, logs]`. Window View's layer code cannot
make frame calls skip another layer `[inferred-static]`; the link to Window View (3 of 19 on, 0 of 12 off) is weak.
Why the rebuild failed is not known.

## Next

FLAT, a new numbered build: 10 menu starts with `VR_GhostingFix=false`. 0 crashes = cause confirmed. `MODEL: FABLE`.
Then a headset look at what ghosting comes back, and the gentler fixes. The Optimizer's real-headset tour can run
before that (it retries a start crash).

## Afternoon: The Optimizer's tour with the real headset (12:52 to 13:29, Window View's layer off)

Tefa: "headset connected all yours". The full write-up is in the-optimizer `docs/first-run-silent-hill-2-2026-11-22-headset.md`.
- **Performance:** as found, the town start 66.7 a second (slowest 1% 47) and the room after the garage 67.7 (53), the
  graphics card the limit (96-97%). After four live changes (ray-traced reflections off, shadow size 1024, corner
  shadows off, local brightness balance off): both 72.0 on average, 0 late frames, slowest 1% 66, the processor the
  ceiling `[measured 2026-11-22, real headset, n=1 per step]`. The 90% headset picture bought nothing and was put back.
  The four changes are forgotten at the next start; nothing sets them at each start yet (Tefa's call: keep them?).
- **The crash on the real headset with Window View off:** 2 of 9 starts (12:57, 13:14), the same freeze and
  scene-state line. So the Ghosting Fix reading holds on the real runtime too.
- **No pictures to the headset with Window View off:** 2 of 9 starts (13:00, 13:14): "D3D12: Failed to create swapchain"
  right after XR_SESSION_STATE_READY, then "AFR swapchains not created" every frame, the meter at 0 frames, the game
  window stuck flickering on "compiling shaders" (what Tefa saw, as on 2026-11-21 try 3). Window View is cleared of this
  one too `[measured, n=2]`. Cause unknown; the Ghosting Fix test may or may not touch it.
- **The start after a crash failed twice** (the launcher sat 3 minutes, the game never started), the crash reporter
  window up both times.
Kept: `D:\claude video game stuff\logs\silent-hill-2-remake-vr\2026-11-22-1253-optimizer-headset-tour\`.

## 14:04 to 14:37: the test. Ghosting Fix off = 20 of 20 clean starts (build b034, simulator, Fable)

`VR_GhostingFix=false` in the UEVR profile's config.txt (the old file kept beside it as
`config.txt.bak-2026-11-22-before-ghostingfix-off`), everything else b033; numbered b034 ("Start crash" feature). The same
counter, the same conditions as the morning (Window View's layer off, the meter on): **20 menu starts, 0 crashes**,
the menu in 63-71 s, the meter full every time, against 3 of 11 the same morning with the fix on (about 1 in 600 by
luck) `[verified-live 2026-11-22, simulator, n=20]`. With the fix off UEVR's log has no "Inserting new scene state",
no "Setting scene state to" and no "Scene state passed ... is null" line at all, while the ~400 ms start stall is still
there: the swap never runs, so nothing dead is handed to the render thread. **The start crash's cause is UEVR's Ghosting
Fix** (in rendering method 3). Caveat: an f4decomp compile sweep loaded the processor in rounds through the day; it
ran during both the morning's and this count.
Left installed: b034 (Ghosting Fix off). NOT worn: the AFW notes say to keep the fix on, so ghosting may be back; that
is for Tefa's eyes. The package's own `package/profile/config.txt` still says true, so a normal install (the launcher
without `--no-install`) puts the fix back on: the lasting change belongs in the package once Tefa has judged the look.
Not tested: the no-pictures start (real headset only), and whether crashes later in play (level loads, cutscenes) share
this cause. If ghosting is bad: the gentler ways (inject later; the fix switched on live at the main menu; a guard in our
plugin). Kept: logs folder `2026-11-22-1404-starts-ghostingfix-off-sim/`.
The same f4decomp load also means today's Optimizer verdict "the processor is the limit" may be too harsh: rerun with the
sweeps paused before believing it.

## 14:45 to 15:30: Tefa in the headset (b034, then b035), and what the hour alone found; b036

Tefa wore b034 (Ghosting Fix off). His words: "there is flickering shimmer around the little gap in the wall where i have
to squeeze through ... on each side with each eye". Ghosting Fix switched on live in play (`VR_GhostingFix true` through
the connector): "yep gone now" `[reported, Tefa, worn, n=1]`; no crash from the live switch (n=2 the same afternoon).
Also from him: with the fix on, reflections in puddles and windows reset about every 10 s (earlier builds); "the weapon
still flickers a lot in my hand, partly disappearing"; "RT is too costly to run in VR that's a fact", "ray tracing off
for good yes, we build it around rt off"; "can you turn AFW off - it really looks to me that whatever we do, it can't
be used". `VR_AFW_FramewarpMode 0` live: "it made the game all jittery" (the eyes still alternate, nothing fills in).

**b035** = config.txt `VR_RenderingMethod=0` + ray tracing off (`sh2vr_devcvars.txt`: `r.Lumen.HardwareRayTracing 0`,
`r.Lumen.Reflections.HardwareRayTracing 0`), Ghosting Fix off. **My mistake:** I told Tefa this drew both eyes every
frame with no warp. It did not: `warp.cpp` switches the injector to rendering method 3 (the frame warp) 10 s after the
headset starts drawing, by design since 2026-11-13, and the log said so ("rendering method 0 -> 3"). So b035 was the
frame warp as before plus ray tracing off. Tefa on it: "game is running super smooth", "this looks great and
performance is soooo good", "plays so well right now and feels great" (marked IMPORTANT in the builds list), and:
"a lot of flickering going on, like afw was still on" (it was), everything shimmering (flora, roads), "hands are still
leaving that trail", "the weapon is still not like a solid weapon but flickers and parts of it disappear when i move
it", "things close by are still tearing the screen". 72 a second in the armory, ~70 at the town trees, card 76-80%.

**Measured alone, real headset lying still in the armory** (a static view; 2 s of the game window at 30 a second,
lossless; the measure is how much a pixel flips back and forth between frames, mean over the picture; repeat runs
within 0.02-0.04) `[measured 2026-11-22, n=2-3 each]`:

| what was on | flicker | frame |
| --- | --- | --- |
| warp mode 2, Ghosting Fix off (= b035 as played) | 0.37 | 13.9 ms |
| warp mode 0, Ghosting Fix off | 0.27 | 14.5 ms |
| warp mode 0, Ghosting Fix on | 0.16-0.18 | 14.7 ms |
| warp mode 2, Ghosting Fix on | 0.20-0.24 | 14.5 ms |
| rendering method 0 (really both eyes every frame), Ghosting Fix on | 0.41-0.49 on the window (it shows something else in this mode) | 17-20 ms and rising, card 91% |

The flicker sits on every edge in the room. With the Ghosting Fix off it is about twice as strong: both eyes then share
one picture memory (DLSS history), which also fits the hand trails, the weapon that is not solid and near things
tearing `[inferred; the measure agrees]`. Both eyes every frame cannot hold 72 here even with ray tracing off, so the
warp stays.

**b036** = `ghostfix.cpp`: config.txt keeps `VR_GhostingFix=false`, the plugin switches it on once the headset has drawn
20 s (10 s after the warp's own switch), off the game thread, and logs the value before and after
(`sh2vr_noghostfix.txt` in the profile keeps it off). First start: "false -> true", "read back ... is true", one
"Setting scene state" line, no null line, the menu reached, clean quit. The start count is in the next section.

## 15:26 to 16:25: b036 counted, frame rates at three spots, ray tracing held off (b038)

**b036, the start count (simulator, Window View's layer ON this time, the meter on):** 13 of 13 menu starts clean; in
every one the plugin switched the ghosting fix on 20 s after the picture came up ("false -> true", read back true),
one "Setting scene state" line, no null line `[verified-live 2026-11-22, simulator, n=13]`. Plus 7 real-headset starts
into play the same hour (b036 and b038), all clean. Against 3 of 11 (simulator) and 2 of 9 (headset) with the fix on
from the start. The crash zone is the start; a later switch has not crashed in 22 tries today.

**Frame rates, real headset lying still at each save point, the frame meter, 2 x 30 s each** `[measured, n=2 per row]`:

| build | spot | a second | slowest 1% | card | ray tracing read in play |
| --- | --- | --- | --- | --- | --- |
| b036 | town start | 72.0 | 66.4 | 87% | not read |
| b036 | prison armory | 71.9 | 63-64 | 95% | not read |
| b036 | room after the garage | 65.2 | 48-51 | 97% | not read (it was on, by the load) |
| b036, a second start | room after the garage | 72.0 | 66.4 | 74% | 0 |
| b038 | town start | 72.0 | 66.5 | 74% | 0 |
| b038 | prison armory | 71.7 | 58.5 | 95% | 0 |
| b038 | room after the garage | 72.0 | 66.4 | 86% | 0 |

In b036 ray tracing off came from `sh2vr_devcvars.txt` alone (set once at the start) and did not hold in every run: the
game puts its own `Raytracing=On` back (I set `Raytracing=Off` in GameUserSettings.ini with the game closed; the game
wrote On again at its exit; the old file is kept beside it as `.bak-2026-11-22-before-rt-off`). **b038:** `tuning.cpp`
holds `r.Lumen.HardwareRayTracing 0` and `r.Lumen.Reflections.HardwareRayTracing 0` (looked at again every 5 s); read
0 in play at all three spots. b037 was never run: adding the two rows overran the file's 6-entry tries counter (my
slip, caught before a start; the counter is now sized from the table).
Open: the armory stays at 95% of the card with ray tracing off (58 on its slowest 1%), while Tefa's own armory view on
b035 read 73-76% with the ghosting fix off: either the save point's view is heavier, or the ghosting fix (a second
picture memory for DLSS) costs card time there. Not separated yet. The f4decomp compile loops were paused for all of
these. `r.AOGlobalDistanceField 0` and `r.DistanceFieldAO 0` (in tuning.cpp since 2026-11-13) now matter more: with
hardware ray tracing off Lumen traces in software and would use those; the look Tefa called "great" on b035 had them off.
Not done: the weapon that is "not solid" and the hand trails were reported both with the fix off and (the weapon) on;
nothing was measured on them this session; a Ghost Hand / VR Bench run with the hand moving is the next step there.
Kept: logs folder `2026-11-22-1445-wear-and-b036-b038/` (the counts, the frame-rate rows, the scripts, the clips).

## 16:38: Tefa wore b038 and sent a video (not analysed in this session)

The start: clean, the ghosting fix switched on by the plugin. Tefa's video: `D:\Capcut export\SH2 stuff to fix.mp4`
(71.5 s, 2880 x 1440, 60 a second). His list, in the video's order, word for word `[reported, Tefa, worn 2026-11-22, b038]`:
1. "picture jittery/tearing"
2. "James hair shadow visible"
3. "gun and hands shaky and jittery"
4. "things flickering - the wooden posts - and the outside is generally too bright, like missing a layer of lighting,
   also fog is missing a layer that makes it dense"
5. "leaves flickering"
6. "hand shaky and jittery, left is the same, so are weapons in hand"
7. "reflections in puddles dissapear when i look down"
8. "reflections in puddles are constantly - i don't have a good word for it, like pixels are constantly shifting a
   little bit inside the puddles"
9. "flying leaves in the wind are leaving trails underneath"
First thoughts, none checked `[hypothesis]`: 4 (too bright, thin fog) and 7 are what ray tracing off plus
`r.AOGlobalDistanceField 0` / `r.DistanceFieldAO 0` would do (Lumen then has only screen traces: reflections vanish when
their source leaves the screen; no distance-field light or fog shadowing); 8 and 9 are screen-space reflections and the
frame warp / DLSS on thin moving things; 2 is the head's shadow pieces (shadows.cpp); 3 and 6 are the hands' carry
(`[ID SH2-001]`), now with the game at a full 72. The run's injector log is kept in the logs folder
`2026-11-22-1638-tefa-wear-b038/`.
