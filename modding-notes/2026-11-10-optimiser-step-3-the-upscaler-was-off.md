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

## The eye-corner job, step 1: what covering part of the view saves (b005, simulator)

Tefa: "please carry on with the eye corner work". No engine or UEVR setting does it, so the first try is the plainest one:
solid pieces that ride on the headset just in front of the eyes, so the card skips what is behind them.

- **`cornermask` in the plugin (b005, dev):** `sh2vr_mask.txt` in the profile, one piece per line (`x y z pitch yaw roll
  sx sy sz`, cm and degrees from the headset, x forward / y right / z up, the scale of the engine's 100 cm cube
  `/Engine/BasicShapes/Cube`). The pieces hang on a scene component that UEVR moves with the headset (its motion controller
  state, hand 2, permanent), cast no shadow, have no collision. A 10 cm cube asked for at 40 cm ahead and 30 cm left was
  seen there in both eyes, further left in the right eye (the parallax) `[verified-live 2026-11-10, simulator, n=1]`.
- **Which corners a head-fixed piece can cover:** each eye sees much further to its own outer side than to the nose side
  (this headset: 54 degrees out, 40 in). A piece at an eye's OUTER edge is outside the other eye's view, so the two outer
  sides can be covered by one set of pieces. A piece at an eye's inner (nose-side) corner sits in the middle of the other
  eye's view: those corners need a piece only one eye draws, which a plain mesh cannot be `[inferred, geometry]`.
- **The measurement** (gameplay, the game's DLSS on with `r.ScreenPercentage 100` to stay clear of the simulator's 90 Hz
  cap; baseline 25.5 ms; A/B/A, `[measured 2026-11-10, simulator, n=1 each]`):

  | Covered | Frame | |
  | --- | --- | --- |
  | the WHOLE view (one block 10 cm ahead) | 21.0 ms | -17.7 % |
  | the outer 20 % of each eye | 23.5 ms | -8.2 % |
  | the outer 10 % of each eye | 23.7 ms | -7.3 % |

- **What it says.** With everything hidden behind a block the frame is still 82 % of what it was: in this game most of the
  card's time is NOT the world's surfaces (which a piece in front can skip) but work done on every pixel of the picture
  whatever is in it (the upscaler, the anti-aliasing history, the fog, the lighting and post passes) and work done before
  any pixel (shadows, geometry). So covering the real hidden corners with pieces can win a few per cent, not the 10 to 20 %
  a true hidden-area mask gives an engine that skips those pixels in every pass. The outer bands gave more than their share
  of the whole-view saving (7 to 8 % for 10 to 20 % of the pixels against 18 % for all of them): the edges are not cheaper
  than the middle, and big pieces also hide whole objects from the engine's culling `[hypothesis]`.
- **Not done:** the real shape of what the lenses hide (the headset's runtime can say: its visibility mask; the simulator
  is not the Quest), so no final pieces are made and nothing is installed for play. Whether a piece at the very edge is
  ever seen in the headset when the eye turns.
- **The next step is a breakdown of one frame on the card** (which pass costs what), to see what the 82 % is. Nsight's GPU
  trace could not attach to the running game ("Cannot find process": it attaches only to a game it started); the game has
  to be started through it, with UEVR and the simulator, which was not tried today.

## Where the card's time goes (the same day, last part): the bounce lighting is the big one

No trace tool was needed: `dev-archive/tools/optimiser/waterfall.py` switches things off one after another (each stays off)
and reads the frame after every step; `cvar_ab.py` then measures single pieces A/B/A. Gameplay, the armory, DLSS on with
`r.ScreenPercentage 100` (25.2 ms) to stay clear of the simulator's 90 Hz cap. `[measured 2026-11-10, simulator, n=1 each]`

**Lumen is running.** The earlier "Lumen lighting off: no change" was the wrong switch (`r.DynamicGlobalIlluminationMethod`
read 0 and did nothing; `r.Lumen.DiffuseIndirect.Allow` is the one that acts). One at a time, of 25.2 ms:

| Piece | Cost | Share |
| --- | --- | --- |
| **Lumen bounce light** (`r.Lumen.DiffuseIndirect.Allow 0`) | **6.0 ms** | **24 %** |
| all shadows (`r.ShadowQuality 0`; in the waterfall, after the fog) | 3.8 ms | 15 % |
| the global distance field (`r.AOGlobalDistanceField 0`; Lumen traces through it) | 1.7 ms | 7 % |
| the finishing passes (`ShowFlag.PostProcessing 0`) | 1.7 ms | 7 % |
| volumetric fog | 1.6 ms | 6 % |
| Lumen reflections (`r.Lumen.Reflections.Allow 0`) | 1.0 ms | 4 % |
| ambient occlusion | 0.7 ms | 3 % |
| the upscaler and anti-aliasing themselves, reflections (SSR), see-through surfaces, particles, decals, hair, height fog, the stencil buffer, skin scattering, light shafts, distance-field shadows | within 0.2 ms each | nothing |

About 8.7 ms is left with all of that off: the world's surfaces and geometry (a block over the whole view took 4.5 ms of
it) and what the VR layer, the frame warp and the simulator do per frame.

**How the frame grows with the picture's inner size** (DLSS on): 100 % 25.2 ms, 75 % 18.8, 50 % 14.6, 35 % 13.8. It flattens:
about 12 to 13 ms does not shrink with the size at all (shadows, the fog's grid, Lumen's scene, geometry). That is why even
DLSS Performance does not reach 72 fps here.

**Making the bounce light cheaper without switching it off** (each A/B/A, of 25.2 ms):

| Change | Frame | |
| --- | --- | --- |
| probes half as dense (`r.Lumen.ScreenProbeGather.DownsampleFactor` 16 -> 32) | -1.1 ms (-4.3 %) | the one that pays; a softer bounce light in small corners, to be judged by eye |
| rays per probe 8x8 -> 6x6 (`...TracingOctahedronResolution 6`) | -0.5 ms (-1.9 %) | |
| Lumen's light in the fog volume off (`r.Lumen.TranslucencyVolume.Enable 0`) | -0.6 ms (-2.4 %) | touches how the fog is lit: not offered |
| probes 32 + rays 6x6 + no extra probes at edges, together | -1.1 ms (-4.4 %) | no more than the first alone |
| refreshed half as often (radiosity, surface capture, probes traced per frame), a simpler light format, coarser far probes, smoothing off, bent-normal shading off, coarse-scene tracing only | within 0.2 ms | nothing |
| half-size light gathering (`IntegrateDownsampleFactor`), the surface-lighting update factor, short-range AO | not in this build | |

So the bounce light's cost sits in the part done for every pixel of each eye, and this engine version has no switch to do
that at a smaller size. Nothing found here is both large and invisible.

**Where that leaves Tefa's aim** (max settings, the look untouched): the upscaler back on was the big free one (done).
What is left costs something to the eye, small first: the bounce light's probes at 32 (-4 %), the corner pieces (a few per
cent, unfinished), the fog's grid at 12 (-4 %, Tefa said no to touching the fog), a lower DLSS level. Lumen off (-24 %)
and shadows off (-15 %) would change the game and are not proposals.

Not measured: which share of the shadows is the torch (it moves every frame with the body, so its shadow can never be
kept from one frame to the next, which is what Tefa's "shadows that only move with the flashlight beam" comes to).

## The biggest find of the day: the alternate-eye warp was never switched on

Tefa: "carry on please, as much as you can learn about where to get any performance gain".

**What AFW is.** The text inside PureDark's build says it: "Alternate Frame Warping", "No DLSS instance detected, are you
sure you have turned on DLSS in-game?", "Using DX11, AFW only supports DX12, fallback to AFR". Its public source
(PureDark/UEVR, branch `AFW`, `src/mods/VR.hpp`): `enum RenderingMethod { NATIVE_STEREO = 0, SYNCHRONIZED = 1,
ALTERNATING = 2, ALTERNATE_FRAMEWARP = 3 }`, and `is_using_afw()` is true only when the rendering method IS 3 (and 90
frames have passed since the inject, DX12, not the 2D screen). `VR_AFW_FramewarpMode` (None / AlternateEyeWarping /
PreviousFrameWarping / CombinedWarping = 3) only chooses HOW it warps once it is on. `[inferred-static 2026-11-10, from
the source and the dll's strings]`

**This profile has had `VR_RenderingMethod=0` (Native Stereo: both eyes drawn in full, every frame) since its first
commit on 2026-10-23**, in the profile and in `mod/` (`git log -S"VR_RenderingMethod=3"` on the config: nothing). So the
frame warp has never run in this project: every wear drew both eyes, and from the settings reset on, without DLSS too.
`[measured: the git history and the installed file]`

**Switched on** (the profile's config.txt, the old one kept beside it as `config.txt.bak-2026-11-10-before-afw-method`):
`VR_RenderingMethod=3`, with the game's DLSS Quality (which it needs). UEVR's log then shows `Before InitFrameWarp` /
`After InitFrameWarp`. Each game frame hands one picture pair to the headset: the simulator counted 77.0 pictures a
second against 76.3 game frames a second. A still taken while the head swept 25 degrees side to side showed both eyes
whole, no tears or holes; the simulator's one-eye crop now shows one eye (each eye has its own picture, not one
double-wide target). `[verified-live 2026-11-10, simulator, n=1]`

**The numbers** (gameplay, the armory, the simulator at 2048 x 2194 with the inner size matched to the headset's load;
`[measured 2026-11-10, simulator, n=1 each]`; the simulator read 3 % under the headset on the one figure that could be
checked):

| At the headset's load | Native Stereo (as it was) | Alternate-eye warp |
| --- | --- | --- |
| no upscaler (this morning's state) | 32.5 ms, 31 fps | 23.0 ms, 43 fps |
| **DLSS Quality** | 20.4 ms, 49 fps | **14.4 ms, 69 fps** |
| DLSS Quality + the bounce light's probes at 32 | | 14.1 ms, 71 fps |
| DLSS Balanced | 18.1 ms, 55 fps | 13.3 ms, 75 fps |
| DLSS Performance | 16.4 ms, 61 fps | 12.4 ms, 81 fps |

72 fps needs 13.9 ms: within reach for the first time (Quality is half a millisecond short; Balanced clears it).

**What costs what with the warp on** (at the headset's load, DLSS Quality, 14.4 ms; A/B/A): the bounce light 2.6 ms (18 %),
all shadows 2.25 (16 %), the finishing passes 0.9, volumetric fog 0.6, Lumen reflections 0.5, shadow size 1500 -> 1024
0.3, the fog grid 12 0.3, AO 0.25.

**The torch.** Toggled (F): 14.37 -> 13.43 -> 14.40 ms, so the torch is 0.9 ms (6.5 %); with all shadows off the toggle is
worth 0.2 ms, so about 0.7 ms (5 %) is the torch's SHADOW and the other two thirds of the shadow cost (about 1.5 ms)
belong to the room's own lights. `[measured, n=1]`

**Not known, and only the headset can say:** how the warped eye looks around the hands and the gun in motion (the
"shimmer" Tefa has an idea filed about), whether Virtual Desktop takes this mode without a black screen (Early sync gave
one on 11-06; the sync stage is still Very Late), and the real frame rate. To go back: `VR_RenderingMethod=0` in the
profile's config.txt, or copy the kept file over it. `mod/`'s config.txt is NOT changed yet: after Tefa's wear.
