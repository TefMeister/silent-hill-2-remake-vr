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
