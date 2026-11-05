# 2026-11-05c: the optimiser, step 1 (what eats the frame)

Home PC, Opus, the OpenXR Simulator (90 Hz, 1280 x 1400 per eye), Tefa's Continue save. 21 Sacraments switched off
for the measurements (its loader's LoadPlugins=0, put back afterwards). Frame time = sh2vr's heartbeat (5 s averages,
15 s per reading); the stats overlay draws black in VR and `csvprofile` is stripped from this build.

`[measured 2026-11-05, simulator, n=1-2 each; readings drift about 1 ms over minutes, so only A/B/A counts]`

| Change | Frame | Verdict |
| --- | --- | --- |
| as installed | 19.4-21.2 ms (47-51 fps), graphics card 63-71 % busy | |
| resolution 30 % | -0.5 ms, card 43 % | the card is not the wall |
| shadows off / 1 cascade / draw distance 0.4-0.6 / occlusion queries off | no change beyond the drift | drawing settings are not the wall |
| game paused (`pause`) | no change | the game's own logic is not the wall |
| our plugin (selfprof.cpp, new) | 0.3 ms of each frame (the arms 0.2) | not us |
| **r.VSync 1 -> 0** | **-1.2 to -1.6 ms (n=2)** | **the one real gain; vsync only paces the desktop window** |
| UEVR sync mode 0/1/2, AFW ultra responsive, set live | no change (may only apply at start) | unproven |
| AFW frame warp off (restart) | no change | not the frame warp |

Thread load (Windows, per thread): nothing is maxed. The busiest (an Unreal worker thread, the render thread: shadows
off lowered it 66 -> 59 %) runs ~64 % = ~13 ms of a 20 ms frame; the main (game) thread ~48 %. So about 7 ms of each
frame is waiting. Best explanation `[hypothesis]`: the headset paces frames at 11.1 ms (90 Hz); ~13 ms of work misses
the slot and waits for the next, so the game lands near half rate (49 fps measured by the simulator's own count). At
72 Hz (13.9 ms) the same work might fit. The simulator ignored a 72 Hz setting (target stayed 90), so it needs the real
headset: **Virtual Desktop at 72 Hz, then at 90 Hz, same spot, read the fps.**

Next (step 2): vsync off in VR from sh2vr (add r.VSync 0 to screenfx's list); the 72 Hz headset test; then cut the
render thread (shadows already lower it ~10 %).

## Second pass, 18:40-19:00: work per frame per thread (Windows thread times / the simulator's frame count)

`dev-archive/tools/optimiser/threads2.ps1` (per thread: processor time over 10 s, divided by the frames the simulator
counted). `[measured 2026-11-05, simulator, n=1-2 each]`

- At first: render thread 13.4 ms per frame, game (main) thread 9.0, a third 8.6, at 50 fps.
- Shadows off: render 13.4 -> 11.0. One cascade: -0.6. Small casters skipped (r.Shadow.RadiusThreshold 0.06): -0.6.
  Occlusion queries off: WORSE (14.2). Draw distance 0.5: no change.
- **But the scene drifted:** back at the original setting the render thread read 11.1 and the game thread 5.9 at 54 fps.
  Shadow quality 3 against 5, interleaved twice: 10.25 / 10.5 against 11.2 / 10.3, inside the noise.
- So, settled: no thread does more than ~11 ms per frame, yet a frame takes ~19 ms (53 fps on a 90 Hz pace). The rest is
  waiting on something outside the game's own threads: the VR hand-off (xrWaitFrame / the eye submit), GPU-CPU
  serialisation in the stereo path, or DLSS `[hypothesis]`. None of the ordinary engine settings reach it.
- Kept: r.VSync 0 in screenfx (installed, logged `r.VSync 1 -> 0`); our plugin is 0.3 ms (selfprof).

Next: `MODEL: FABLE` (a wait with no obvious cause): find what each frame waits on: PIX / Nsight timing capture of one
frame (both MCP servers failed to connect today), or UEVR's own frame timing; and the 72 Hz headset test.
