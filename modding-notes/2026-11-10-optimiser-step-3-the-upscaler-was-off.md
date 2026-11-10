# 2026-11-10: the optimiser, step 3 (the headset's 30 fps: the upscaler was off)

Home PC, Fable, session alone (Tefa remote: "can you dig into sh2 and try to improve performance?"), the OpenXR Simulator,
Tefa's Continue save (the prison armory), standing still, the connector plugin out, sync Very Late, 21 Sacraments as
installed. Frame time = sh2vr's heartbeat (three 5 s averages after a 6 s settle); every change measured A/B/A.

## The finding

**The game's own upscaler setting had been reset to "off".** In October the settings held `Adv_Supersampling 4` (DLSS) and
`Adv_DLSSQuality 3` (Quality; the engine then runs `r.ScreenPercentage 66.7`), preset Epic, custom. Today's file held
`Adv_Supersampling 0`, `Adv_DLSSQuality 2`, preset High, not custom, motion blur back on: the game rewrote its settings at
some point (it is known to), and from then on it drew every pixel at full size with plain TAA (`r.AntiAliasingMethod 2`,
`r.NGX.DLSS.Enable 0`, `r.ScreenPercentage 100`) `[measured 2026-11-10: the live values; the two settings files compared]`.
The 2026-11-06 headset baseline (33.5 ms, ~30 fps, card 99 %) was taken in that state.

**Put back, the game's own way** (game closed, the file backed up beside itself as
`GameUserSettings.ini.bak-2026-11-10-before-dlss-back-on`): `Adv_Supersampling 4`, `Adv_DLSSQuality 3`,
`AdvQualityIsCustom=True`. After a fresh start the engine reads `r.NGX.DLSS.Enable 1`, `r.ScreenPercentage 66.7`, and the
setting was still there after the run `[verified-live 2026-11-10, simulator, n=1]`.

## Why the simulator said "fine" before and the headset said 30 fps

The headset (Quest 3, Virtual Desktop) asks for **2688 x 2880 per eye** (UEVR's log of the 11-07 wear: one 5376 x 2880
target). Steps 1 and 2 measured at the simulator's 1280 x 1400: less than a quarter of the pixels. The simulator cannot
make a picture wider than 4096 (`OpenXR system max width: 4096`): asked for the headset's size it creates no swapchain,
UEVR never begins its session, and the game runs FLAT with nothing saying so except `headset off` in the plugin's
heartbeat (two readings today were that, and were thrown away). The largest it can do is **2048 x 2194 per eye** (58 % of
the headset's pixels); the headset's pixel load is then matched by raising `r.ScreenPercentage` (131 % = the headset's
100 %).

**The match is good:** "headset pixels, as now" read 32.5 ms in the simulator; the headset itself read 33.5 ms on 11-06
`[measured, n=1 each]`. So the estimates below should be within a few per cent.

## The measurements (gameplay, card 98 % busy throughout)

Estimated headset frame time (simulator at 2048 x 2194, the internal size matched to the headset's):

| Setting | Frame | Rate | Against now |
| --- | --- | --- | --- |
| as it was (no upscaler, 100 %) | 32.5 ms | 31 fps | |
| **DLSS Quality (66.7 %)** | **20.4 ms** | **49 fps** | -37 % |
| DLSS Quality + fog grid 12 | 19.6 ms | 51 fps | -40 % |
| DLSS Balanced (58 %) | 18.1 ms | 55 fps | -44 % |
| DLSS Performance (50 %) | 16.4 ms | 61 fps | -50 % |
| DLSS Performance + fog grid 12 + AO off + shadows 1024 | 15.5 ms | 64 fps | -52 % |
| DLSS Ultra Performance (33 %) | 13.5 ms | 74 fps | -58 % |

`[measured 2026-11-10, simulator, A/B/A, n=1 each; the DLSS rows were set by console variables at the matching internal
size; the first row agrees with the headset]`. 72 fps needs 13.9 ms: only Ultra Performance reaches it. The frame warp's
half rate (36 fps real) needs 27.8 ms: the state before missed it (33.5), every DLSS level clears it with room.

One setting at a time, at the simulator's own size (baseline 22.8 ms in gameplay; `[measured, A/B/A, n=1 each]`):

| Change | Frame | |
| --- | --- | --- |
| volumetric fog off | -1.7 ms (-7.6 %) | not wanted: it is the game's look |
| fog grid 8 -> 12 pixels (`r.VolumetricFog.GridPixelSize`) | -0.9 ms (-3.9 %) | 16 gives no more than 12 |
| ambient occlusion off | -0.5 ms (-2.2 %) | |
| shadow size 1500 -> 1024 | -0.3 ms (-1.4 %) | |
| Lumen lighting, Lumen reflections, virtual shadow maps | 0 | already off in this build's settings |
| reflections (SSR) quality or off, distance-field AO, contact shadows, bloom, hair strands, skin scattering, the light volume, the post-process / effects / shadow groups 3 -> 2 | within +/-0.1 ms | nothing |

So in this scene the frame is almost entirely "how many pixels are shaded". Plain TAA at 66.7 % costs 0.5 ms less than
DLSS at 66.7 % (13.2 against 13.8 at the prompt screen): DLSS did engage.

## Tools made

- **`devcvars` in the plugin (build b004):** if `sh2vr_devcvars.txt` is in the profile, its `name value` lines are applied
  through the engine's console manager whenever the file changes, each logged before -> after. No file, nothing happens.
  It needs no connector, so the 7 ms the connector costs is not in the readings.
- **`dev-archive/tools/optimiser/cvar_ab.py`:** A/B/A on any settings, reading the old values back from the plugin's log to
  restore them. One line per test.
- `run-sim.ps1`: presses the last Enter twice (the first came too early today: a whole batch was measured on the "press any
  button to play" screen; the world is drawn behind it, the numbers reacted, but they were redone in gameplay), and says
  `VR ON` or `VR OFF: THE READING IS THE FLAT GAME`.

## Also found

- `uevr_mcp.dll` (a build dated 2026-11-06) was back in the profile's plugins folder; it is parked again. Who put it back is
  not known; if it is the throttled build its cost was not measured today.
- A screenshot through the simulator shows both eyes in the "left eye" crop for this game (UEVR's one double-wide target).

## Not done

- Nothing was worn. How DLSS looks around the hands with the frame warp (the shimmer row) is for the headset.
- The fog grid and the other small ones are NOT applied: a look change for 4 %; Tefa's call.
- The launcher does not yet set or check the upscaler; the game can reset it again.
- Whether a level below Quality still looks right in the headset.

## Later the same day: what is left that a player cannot see (Tefa's aim)

Tefa: *"my idea was always to get the game running as well as possible with max settings and really try everything
imaginable without sacrificing how the game looks. fog is important to look dense and right ... apart from the fog, what
can we do, anything in the background, have shadows not real time but still maybe and only move with flashlight beam,
anything you can think of, before lowering quality settings?"*

Measured with the game's own DLSS Quality on (14.1 ms at the simulator's 2048 x 2194), A/B/A, `[measured 2026-11-10, n=1 each]`:

| Idea | Result | What it means |
| --- | --- | --- |
| the background (other programs on the card) | the card is 1 % busy with no game running | nothing to win on this PC; Virtual Desktop's encoder while streaming was not measured |
| shadows not redrawn (Tefa's idea) | ALL shadows off saves 1.4 ms (10 %); the engine already keeps the shadows of things that did not move (`r.Shadow.CacheWholeSceneShadows 1`) | the ceiling is 10 % and part of it is already taken; a torch that moves must redraw its own shadow |
| the corners the lenses hide (`vr.HiddenAreaMask`) | the setting is on, switching it off changes nothing | it is NOT working: UEVR gives the engine no mask, so the hidden corners are drawn in full. The one real "nobody can see it" saving left (usually 10 to 20 % of the pixels), and it needs code in the VR layer, not a setting |
| coarser shading at the edges (`vr.VRS.HMDFixedFoveationLevel` 2, 3) | no change | not reaching the card through UEVR's path |
| fog's inner detail at the same density (history supersample 4 -> 1; depth slices 148 -> 96) | 0 %; -1.7 % | nothing worth touching the fog for |
| GPU occlusion (HZB), a full depth pre-pass, the torch's shadow texels, small shadow casters | within +/-0.1 ms | nothing |

So with the upscaler back on, this scene's frame is the pixel count and little else. What is left without touching the
look, in order: (1) the hidden-corner mask, done by us in the VR layer; (2) the upscaler level, which is a look question
only Tefa's eyes can settle in the headset (DLSS Quality often looks sharper than the plain 100 % it replaces); (3) the
size Virtual Desktop asks for (2688 x 2880 per eye is more than the headset's panel shows 1:1), also a look question.
