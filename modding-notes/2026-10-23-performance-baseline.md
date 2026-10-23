# 2026-10-23 · Performance baseline in the prison armory

**Goal set by Tefa:** at least 72 frames a second in the prison armory (Tefa's only save, all weapons, one of
the heaviest spots in the game).

## Setup

- Home PC: RTX 5080, Intel i5-14600K. Quest 3 over Virtual Desktop, OpenXR.
- UEVR: PureDark's AFW build (UEVR-nightly AFW v1.0-beta.6), frame warp mode 3.
- Headset render size reported by UEVR: **2688 × 2880 per eye** `[measured 2026-10-23]`.
- Game: Epic, DLSS Quality (`r.ScreenPercentage` 66.7), motion blur off, ray tracing off, frame generation off.
- Already lean: `r.DynamicGlobalIlluminationMethod 0` (no Lumen GI), `r.ReflectionMethod 2` (screen-space),
  `r.Shadow.Virtual.Enable 0` `[measured 2026-10-23]`.
- Profile: jbusfield's SH2_UEVR (later runs: LukasBlaster7's version of it).
- Same spot and same head pose for every run (headset on the desk, not worn).
- Numbers are UE `stat unit`, read 3–4 times a few seconds apart.

## Results `[measured 2026-10-23, n=3–4 readings per row]`

| Run | Frame | Game | Draw | GPU | RHI | ≈ FPS |
| --- | --- | --- | --- | --- | --- | --- |
| A. As installed (no VynnGfx) | 24.5–24.9 | 13.5–14.6 | 24.5–24.9 | 22.3–23.2 | 15.7–16.9 | **40** |
| B. + VynnGfx Optimized Tweaks | 24.2–24.4 | 13.6–15.2 | 24.3 | 21.9–22.2 | 15.3–16.7 | **41** |
| C. B at 50 % resolution | 20.9–21.9 | 19.4–20.2 | 6.9–9.2 | 14.9–15.8 | 15.2–18.7 | 47 |
| D. B at 30 % resolution | 20.2–22.4 | 18.7–19.9 | 8.2–8.5 | 10.1–10.3 | 14.9–19.9 | 48 |
| E. B, VR scripts OFF, 66.7 % | 20.2–22.6 | 13.8–20.4 | 6.8–13.8 | 18.7–19.2 | 17.9–20.1 | 47 |
| F. B, VR scripts OFF, 30 % | 19.1–21.0 | 15.8–19.1 | 6.8–7.4 | 8.7–8.8 | 17.5–18.8 | 50 |
| G. F with `r.GTSyncType 0` | 19.7–21.6 | 17.8–19.3 | 6.5–7.3 | 8.7–8.9 | 16.8–18.3 | 49 |

Runs E–G use the game's own camera (no first-person scripts), so the view is not identical to A–D.

## What it means

1. **VynnGfx's tweaks help a little:** about 0.4 ms, roughly 40 → 41 fps here `[measured 2026-10-23, n=4]`.
2. **The processor is the wall, not the graphics card.** Even at 30 % resolution, with the graphics card at
   about 9–10 ms, a frame still takes about 20 ms (≈ 50 fps). The game thread (≈ 18–20 ms) and the RHI thread
   (≈ 17–19 ms) are both above the 13.9 ms that 72 fps needs `[measured 2026-10-23, n=4]`.
3. **jbusfield's and LukasBlaster7's Lua scripts are not the main cost:** switching them off lowered the
   floor by only about 1 ms `[measured 2026-10-23, n=4; view differs, so treat as rough]`. A C++ rewrite will
   help, but on its own it will not reach 72 here.
4. At normal resolution the graphics card is also near the limit (≈ 22 ms). So 72 needs work on both sides.
5. `r.GTSyncType` (VynnGfx sets 1) made no measurable difference `[measured 2026-10-23, n=4]`.
6. This build strips `stat game` and the other detailed stat groups; only `stat unit` works.

## Temperatures

GPU logged every 2 s for the whole session (440 samples): **peak 73 °C**, peak 309 W. Safe; an RTX 5080 slows
itself down around 88 °C. The CPU temperature could not be read: LibreHardwareMonitor runs as administrator and
its web server is off. `[measured 2026-10-23]`

## Ideas for next time `[hypothesis]`

- Measure the same spot **flat, without UEVR**, to see how much of the game-thread and RHI cost is UEVR
  rendering two views versus the game itself.
- Lower the per-eye render size (2688 × 2880 is very high; Virtual Desktop's setting decides it).
- CPU-side game settings: view distance, foliage, effects, shadow quality, and `r.ViewDistanceScale`.
- Check the CPU is not throttling: needs CPU temperature (turn on LibreHardwareMonitor's web server).
