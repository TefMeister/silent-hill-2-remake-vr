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
