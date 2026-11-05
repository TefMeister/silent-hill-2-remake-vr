# 2026-11-05d: the optimiser, step 2 (what every frame waited on: our own connector)

Home PC, Fable, `/lm`, the OpenXR Simulator (90 Hz cap, 1280 x 1400 per eye), Tefa's Continue save (the prison
armory), standing still at the load spot. 21 Sacraments off for every run (its loader's `LoadPlugins=0`, put back
afterwards). Frame time = sh2vr's heartbeat (5 s averages, three per reading). One background reader read the
simulator's and UEVR's source meanwhile (its two reports: `dev-archive/recon/2026-11-05-optimiser-step-2/`).

## The cause, with the evidence

**The UEVR MCP connector plugin (`uevr_mcp.dll` 1.0.0, elliotttate/uevr-mcp, our own dev tool, sitting in the
profile's plugins folder since 2026-10-23) rewrote a "crash breadcrumb" file about six times per frame: four times on
the game thread, twice on the present thread, under one shared lock.** That is the ~7 ms every frame waited for.

- Stack samples (`dev-archive/tools/optimiser/waitstacks.py`, new today: suspends one thread for well under a
  millisecond, reads its call stack, resumes it; 1286 samples per thread at the main menu): the game thread was inside
  `CreateFileW` / `CreateDirectoryW` called from `uevr_mcp.dll` in 57 % of samples, the RHI (present) thread inside
  `CreateFileW` from the same dll in 25 % and in that dll's lock in 29 %; the render thread waited on the game thread
  90 % `[measured 2026-11-05, simulator, main menu]`.
- The reader found the code (elliotttate/uevr-mcp `plugin/src/diagnostics.cpp` `write_breadcrumb_locked`:
  `create_directories` + truncating `ofstream` + write + close on every callback enter/exit, from `on_pre_engine_tick`
  (2 scoped callbacks = 4 writes) and `on_present` (2 writes); file `uevr_mcp_last_breadcrumb.json` in UEVR's
  per-game folder). The installed 1.0.0 has a bug that defeats its own 750 ms throttle (`state_changed` is always
  true because the callback name flips on every enter/exit); master commit `8cf1053` (2026-06-14, "Throttle MCP
  breadcrumb diagnostics") fixes it in one line and was never released `[inferred-static 2026-11-05, reader, from
  source]`. No setting turns the breadcrumb off.

## The runs (A/B/A on the connector and on UEVR's sync stage)

`VR_SynchronizationMode` in UEVR is NOT on/off: 0 Early / 1 Late / 2 Very Late, i.e. WHERE in the frame UEVR calls
xrWaitFrame (Early: when the frame's first hooked RHI command runs; Late: at the top of the Present hook; Very Late:
right after Present, for the next frame; all on the present thread) `[inferred-static 2026-11-05, reader, praydog
master]`. Tefa's profile had 2.

| run | sync | uevr_mcp.dll | main menu | gameplay (3 readings) | card busy |
| --- | --- | --- | --- | --- | --- |
| 1 (and all of step 1) | 2 Very Late | in | 19.0 ms | 19-21 ms (step 1) | 63-71 % |
| 2 | 2 Very Late | out | 12.0 ms | 13.0 (13.0/12.8/13.0) | 98 % |
| 3 | 0 Early | out | 11.2 ms | 11.2 (11.2/11.3/11.1) = the 90 Hz cap | 52 % |
| 4 | 2 Very Late | out | 12.0 ms | 13.0 (13.1/13.0/12.8) | 98 % |
| 5 | 0 Early | in | 19.2 ms | 19.7 (19.6/19.8/19.6) | 33 % |

`[measured 2026-11-05, simulator, same spot, n=1 per run; runs 2 and 4 agree to 0.1 ms, so the drift was small today]`

- **The connector costs ~7 ms per frame whichever sync stage** (runs 1 vs 2, 5 vs 3). Taking it out: 19 -> 12-13 ms.
- **Early beats Very Late by ~1.8 ms** with the connector out (13.0 -> 11.2, n=2 each); at Early the game sits on the
  simulator's 90 Hz cap (the RHI thread then waits 40 % of its samples in the simulator's pacing timer), so the true
  frame could be shorter still. Kept: `VR_SynchronizationMode=0` in the profile and in `mod/`'s config.txt.
- Unexplained, reproduced twice: at Very Late the card reads 98 % busy at 13.0 ms frames, at Early 52 % at 11.2 ms.
  Same scene, same spot. Where the extra card work comes from (AFW's warp? the card's clock state? what UEVR copies
  between Present and the next xrWaitFrame?) is open `[measured 2026-11-05, n=2 each; cause: hypothesis]`.
- The simulator itself costs ~1.1 ms per frame (its own log, inside xrEndFrame: the preview paint and the flicker
  scan on the present thread; the `StretchDIBits` 4-5 % seen in the RHI thread's samples) and is a 90 Hz CAP, not a
  vsync slot: a late caller waits 0 ms and is never pushed to the next boundary. So step 1's "misses the 11.1 ms
  slot and rounds down" is wrong on this simulator (it can still happen on Virtual Desktop) `[inferred-static, reader]`.
  Its `frame_time_ms: 11.1` and `target_fps: 90` in runtime_status.json are hard-coded constants, not measurements.

## What it means for the headset

- Every headset wear since 2026-10-23 ran with the connector in the profile: the 10-29 wears' ~50 fps (Ghost Hand
  reports) match the 19 ms the connector produces. The 10-31 runs' further halving to ~30 fps from 13:57 (Gold Digger,
  2026-11-01, now in the recon folder) is a second, separate thing: 21 Sacraments was installed that day; still open.
- **Installed now:** `uevr_mcp.dll` is OUT of the profile (parked in the SH2 builds folder under `_parked-plugins`);
  the profile's sync stage is Early. The launcher's package never contained the connector. A dev session that needs
  the connector copies it in for its run and takes it out again, until the throttled build exists (owed to the dev PC).
- Goal check: 72 fps = 13.9 ms. On the simulator we are at 11.2 ms (capped) with these two changes. The real test is
  still the headset: Virtual Desktop renders at a higher per-eye size than the simulator's 1280 x 1400, and the card
  (98 % busy in runs 2/4) is the next wall there.

## Our own plugin, small

sh2vr's `cost:` line: 0.2-0.4 ms per frame in gameplay, 0.9 ms at the main menu, nearly all of it `crosshair` (the
re-hide of the HUD crosshair each frame: 0.9 at the menu, 0.2-0.4 in play). Not today's problem; a cheap row.

## Tools and lessons

- `waitstacks.py` (new): where threads wait, by sampling. Used instead of PIX / Nsight: PIX's timing capture
  (`pixtool attach <pid> take-new-timing-capture`) exists but its file can only be read in PIX's own window, and Nsight's
  GPU trace shows the card, not a CPU wait. Candidate for the toolkit (owed to the dev PC).
- `run-sim.ps1` (new): one measured run by keys, no connector needed (sync stage, connector in/out, menu and gameplay
  heartbeats, card load, one summary line). Used for runs 4 and 5.
- ⚠️ The ghidrust connector's debug attach froze the game for good (every thread "Suspended"; detach did not resume
  them; NtResumeProcess did nothing; the debug port stayed held by the connector's process, so no second debugger
  could attach). Stopping the connector's process let the game run again without killing it. Never attach it to a
  live game to sample stacks; use `waitstacks.py`. Fault raised for the dev PC.
- By keys, no connector: Enter passes the warning, Enter on CONTINUE (highlighted first), ~95 s load, Enter at PRESS
  ANY BUTTON TO PLAY; the window is found by process (its title is "Silent Hill 2" plus two spaces; FindWindow by that
  title returned nothing, the handle from the process did) `[verified-live 2026-11-05, simulator, n=4]`.

## Not established

- The card's 98 % at Very Late (above). The headset numbers. Whether the throttled connector build is cheap enough to
  stay in (the reader's reading says ~1.3 writes a second instead of ~540: it should be).
- The 72 Hz headset test is still the cheap next check, now with the connector out and Early.

## Credits

elliotttate (uevr-mcp, and the OpenXR Simulator's lineage), praydog (UEVR source read), PureDark (the AFW build;
its UEVR-side source is public on the `AFW` branch, the frame warp itself is not).
