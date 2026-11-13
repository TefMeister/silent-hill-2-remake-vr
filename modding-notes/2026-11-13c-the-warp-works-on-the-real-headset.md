# The frame warp works on the real headset when it is switched on AFTER the start; the half-eye ghost is gone with it; what the frame costs (2026-11-13 08:57-09:40, Fable, Quest 3 through Virtual Desktop)

Continues `2026-11-13b-before-the-next-headset-test.md`. Build b014, DLSS Quality, ray tracing on, the prison armory
save. Tefa in the headset 08:57-09:05, then the headset lying connected with nobody in it (frame times only).

## 1. The warp comes up `[verified-live 2026-11-13, n=3 starts: beta 6 twice, beta 5 once]`

Start with `VR_RenderingMethod=0` (both eyes), wait for play, then set `VR_RenderingMethod` to 3 through the injector's
own setting (here: the UEVR connector's `set_vr_setting {"key":"VR_RenderingMethod","value":"3"}`; our
`uevr VR_RenderingMethod 3` line takes the same path). UEVR's log: `Creating AFR left eye swapchain`, right eye, the
two depth ones, `Before/After InitFrameWarp`, no `Failed to create swapchain`, and Virtual Desktop's own log
(`vd-openxr_2026-11-13_0858_...log`) holds no refused call. The crash of 2026-11-12 belongs to starting with 3 already
set; that route is not to be used on Virtual Desktop. One line worth knowing: `[VR] Depth texture is null! Using default
values` at the switch (the depth swapchains are still made).

## 2. Tefa in the headset, beta 6, warp on `[reported 2026-11-13, n=1 wear]`

1. The second picture over the right half of the right eye: **"gone"**. (Each eye is its own 2688-wide picture in this
   mode; the fault of the two-eyes-in-one picture does not exist here. In mode 0 it is still unexplained.)
2. *"feels ok, needs more optimizing for these graphics settings if at all possible before lowering anything"*.
3. Hands and gun: *"they have an outline around them that is noticable, that was not that noticable in beta 5"*.
   Tefa's clip from the beta 5 days (2026-07-25, jbusfield's Lua hands, the street) shows clean hands. PureDark tuned
   the warp for this game then, with Tefa's logs. The warp's core is a closed DLL (`PDAFWPlugin.dll`; the repo holds
   only its header), so "study it" means its options and the open code around it. NOT yet separated: beta 5 against
   beta 6, and jbusfield's hands then against our C++ arms now (both changed since the clip).

## 3. Frame times, real headset, 2688 x 2880 per eye `[measured 2026-11-13, the plugin's heartbeat, 3-4 readings each]`

The connector plugin was in for all of it (its cost is in every figure; not separated today).

| build | both eyes (mode 0) | warp (mode 3) | where the headset was |
| --- | --- | --- | --- |
| beta 6 | 29.5 ms | 17.0-18.9 ms | on Tefa's head |
| beta 6 | 27.3 ms | 18.1 ms | lying on the desk |
| beta 5 | 30.3 ms | 20.3 ms | lying on the desk, same spot (n=1 start each) |

72 a second needs 13.9 ms; 18.1 is 55. The card is at 96-98 %: the frame is the card's.

One change at a time, warp on, beta 6, baseline 17.05 (first four rows) / 18.1 (the rest), each put back after:

| change | frame | note |
| --- | --- | --- |
| `VR_AFW_UltraResponsive` off | 17.1 | nothing |
| `VR_AFW_FramewarpMode` 1 (alternate-eye only) for 3 (combined) | 16.7 | -0.4 |
| `VR_SynchronizationMode` 1 for 2 | 17.05 | nothing |
| `VR_AFW_ShadingRate` 4 (half x half) | 16.6 | -0.5: the warp itself is cheap, its shading rate is no lever here |
| inner size 50 % (DLSS Performance's) for 66.7 % | 14.1 | -3.0: what a lower DLSS level would give (71 a second) |
| ray tracing off (`r.Lumen.HardwareRayTracing 0`) | 15.2 | **-2.9: ray tracing's whole cost with the warp** |
| only the reflections off ray tracing (`r.Lumen.Reflections.HardwareRayTracing 0`) | 16.1 | **-2.0: two thirds of it** |
| only the bounce light off ray tracing (`r.Lumen.ScreenProbeGather.HardwareRayTracing 0`) | 17.9 | -0.2 |
| ray tracing's reach 100 m -> 30 m (`r.RayTracing.Culling.Radius 3000`) | 18.0 | nothing |
| ray-traced reflections at half size (`r.Lumen.Reflections.DownsampleFactor 2`) | 16.6 | **-1.5**, reflections stay ray traced; for Tefa's eye |
| reflections traced on shinier surfaces only (`r.Lumen.Reflections.MaxRoughnessToTrace 0.3` for 0.4) | 16.85 | **-1.2**; for Tefa's eye |

Already lean, read live: lighting mode 0 (surface cache), far field off, no ray-traced shadows, culling 3.
`r.Lumen.ScreenProbeGather.DownsampleFactor` reads 16 live although `user_script.txt` says 13 (not looked into).

## 4. What is still open

- The warp must come on by itself: a step in the plugin that waits for the picture and then makes the switch
  (the settings file must keep 0: a start with 3 crashes).
- The outline around hands and gun: wear beta 5 and beta 6 back to back (beta 5 is in the tools folder as
  `UEVR-nightly-AFW-beta.5`), then the options that shape it (`VR_AFW_FixObjectMotionVector`,
  `...FixObjectMotionRange` 3, `...IgnoreMotionThreshold` 2.5, mode 1 against 3).
- 4.2 ms to find. On the table, to be judged by eye: reflections at half size, the roughness cut, the bounce light's
  probes at 32; the connector's cost with the warp; Virtual Desktop's encoder load.
- Which part of ray tracing cures jbusfield's "lighting differs between the eyes": if it is the bounce light (0.2 ms),
  the reflections (2.0 ms) are a free choice.

## 5. b015: the warp switches itself on (09:29) `[verified-live 2026-11-13, n=1 start, real headset, nobody in it]`

`dev-archive/native/src/warp.cpp` (+ `settings::WARP_AUTO_ON`, `WARP_AFTER_S` 10): config.txt keeps rendering method 0;
once the headset has drawn for 10 s the plugin sets `VR_RenderingMethod` 3 from a thread of its own (never the game
thread: the flat screen's lesson). A file `sh2vr_nowarp.txt` in the profile keeps it off. First run: the switch came at
the warning screen (`warp: the headset has drawn for 10 s, switching the frame warp on (rendering method 0 -> 3)`), no
refused swapchain in either log, then the main menu, the load and play: 18.05 ms in the armory.
NOT tested: a cutscene with the warp on (the flat screen switches `VR_2DScreenMode`, and `realcam` was written against
mode 0); a headset that connects late; Tefa's eyes on any of it.

## 6. Every last drop, warp on: a sweep of 46 settings on the real headset (09:35-10:05, nobody in the headset)

Tool: `dev-archive/tools/optimiser/sweep.py` (new): one setting at a time through the connector, the frame time from the
plugin's heartbeat (two readings after a 6 s settle), and a picture comparison of the game window before and after (the
headset lay still, so the view was the same; two looks with nothing changed differ by 0.8 on its scale = the noise).
Each setting put back after its test. The list and the raw results sit beside it
(`sweep-2026-11-13-warp-on.json`, `...-results.tsv`). Baseline 17.8-18.1 ms, beta 6, b015, DLSS Quality (preset 5, DLL
3.7.0), ray tracing on. `[measured 2026-11-13, n=1 each, one view: the armory]`

| what | frame | picture change (x noise) |
| --- | --- | --- |
| ray-traced reflections at half size (`r.Lumen.Reflections.DownsampleFactor 2`) | -1.4 ms | 0.9 |
| reflections on shinier surfaces only (`r.Lumen.Reflections.MaxRoughnessToTrace 0.3`) | -1.05 | 1.2 |
| fog grid 8 -> 12 | -0.43 | 1.7 (Tefa said no to the fog: not used) |
| bounce light probes 16 -> 32 | -0.4 | 1.2 |
| reflection filter off / 3 reconstruction samples for 5 | -0.33 / -0.28 | 1.3 / 1.0 |
| instanced meshes / skinned meshes out of the ray tracing scene | -0.3 / -0.25 | 0.9 / 0.8 (things missing from reflections: not used) |
| local exposure off | -0.28 | 1.5 |
| the global distance field off / distance-field AO off | -0.27 / -0.25 | 1.2 / 1.2 |
| old-style ambient occlusion levels 0 | -0.23 | 1.9 |
| screen traces OFF for reflections | **+1.07** (slower: the ray tracer then does all of it) | |
| fewer screen-trace steps for reflections (50 -> 20) | +0.67 | |
| within 0.1 ms: grain, tonemapper sharpen, bloom, shadow ray counts and bias, contact shadows, clouds, sky, half-size see-through surfaces, Nanite edge size, HZB occlusion, landscape / foliage / geometry cache in the ray tracing scene, Lumen's light in the fog, mesh distance-field tracing | nothing | |
| already off or at the cheap value: depth of field, lens flares, colour fringing, film grain, motion blur, particles in the ray tracing scene, the reflection radiance cache (on) | | |

Not levers at all: the engine's coarser shading toward the lens edges (`vr.VRS.HMDFixedFoveationLevel` 2 and 4: 18.0,
the injector gives the engine no such picture), the ray tracing scene built in the background (`r.RayTracing.AsyncBuild`),
and **our connector plugin: 18.03 ms without it against 18.05 with it** (the card is the limit now, at 97 %; the 7 ms
of 2026-11-05 was a processor wait). So the connector can stay in.

**Together** (they overlap, they do not add up):

| set | frame |
| --- | --- |
| as installed | 18.1 ms (55 a second) |
| A: reflections half size + 3 samples + both distance-field switches off | 16.45 |
| B: A + roughness 0.3 | 16.2 |
| C: B + probes 32 | 15.85 (63 a second), picture change 1.0 x noise |
| D: C + reflection filter off + local exposure off | 15.85 (nothing more) |
| for scale: ray tracing off altogether | 15.0 |
| for scale: DLSS Performance's inner size, nothing else changed | 14.1 |

Set C through the plugin's own settings file (six lines in `sh2vr_devcvars.txt`): 16.1-16.2 ms with James's arm in view.
**Nothing found reaches 13.9 ms at DLSS Quality with ray tracing on.** Set C is the most that comes for a picture
change no bigger than the noise in this one view; whether it is invisible in the headset (wet streets, shiny metal,
small dark corners) is Tefa's call. It is NOT installed as a default.

## 7. Tefa's wear 10:10-10:30, b016, DLSS Balanced (real headset)

- **The cheaper set (section 6, set C), on / off / on / off / on, live, Tefa looking:** *"honestly i see no change,
  reflections are still there and the game looks the same, and i tried to catch the line where on is switched off and i
  could not see it"* `[reported 2026-11-13, n=5 flips]`. Where Tefa stood the frame was 13.9 ms (72 a second) either way:
  the 18 ms is the armory's heavy view. **b016 makes the set the mod's default** (`tuning.cpp`, applied once the
  character exists, looked at again every 5 s; `sh2vr_notune.txt` in the profile keeps the game's values).
  ⚠️ One of the six does not take from the plugin: `r.Lumen.Reflections.MaxRoughnessToTrace` logged `0.4 -> 0.4` every
  5 s for a minute (the same call set it from `sh2vr_devcvars.txt` at 10:08); a console command set it at once. OPEN.
- **Beta 5 worn** (same settings): *"the framerate feels worse, there is still a lot of outline around my hands ... the
  shimmer around the hands is more confined though, just around the hand where in beta 6 it really reaches out
  further. i think beta 6 felt smoother"* `[reported, n=1 wear each]`. So the outline is in both; beta 6 stays.
  Tefa points at the warp's debug option (`VR_AFW_FramewarpDebug`, Numpad 6) for the hunt.
- **Warp off** (beta 5, Tefa's head moving): DLSS Quality 24-27 ms, Balanced 23-25 ms, and *"with AFW off i get them
  visual glitches again"* (the right eye's second picture): the warp is the route, mode 0 is not.
- **DLSS Balanced** (Tefa: "we'll go down to DLSS balanced and turn down some graphics settings, but not draw distance,
  i hate when things are popping in and out of existence"): `Adv_DLSSQuality` 3 -> 2 in GameUserSettings.ini (backup
  in the sh2-backups folder), read back `r.ScreenPercentage` 58 after the start.
  Armory, headset on the desk, warp on, set C `[measured, n=1 each]`: Quality 16.2 ms, **Balanced 15.2**, Performance
  14.5. The inner size buys little now: about 14 ms of the frame does not shrink with it.
- **The game's quality groups** (`sg.ShadowQuality` ... one and two steps down): nothing, within 0.15 ms: this game does
  not drive its settings through them (`sweep-2026-11-13-quality-groups-results.tsv`).
- **What the big items cost now** (Balanced, set C, 15.2 ms; `sweep-2026-11-13-big-items-results.tsv`):
  reflections off ray tracing -1.05 (picture change 0.6 x noise in this view); all ray tracing off -1.3 or more (13.9
  is the headset's cap; picture change 11 x noise: the ray-traced LIGHT is what shows); bounce light off the same;
  all shadows off -1.1; fog off -0.5; shadow size 1500 -> 1024 -0.27 (1.0 x noise); reflections at a third / roughness
  0.2: -0.15 each.
