# 2026-11-21: The Optimizer's first real run on Silent Hill 2 (simulator, b033)

Session: /lm, Opus, Build PC, a Tester job handed on from the window that built The Optimizer 0.1.0 (Tefa: "ok. so can
we run the optimizer on silent hill 2 pretend headset?", then "yes for all"). Nobody in the headset.
Installed: b033 (`deployed.sh check`: all 5 files match). Launcher: the 0.1.2 release folder's `SH2VR.exe play --sim
--no-install` (it leaves the installed b033 plugin alone; 0.1.1 and 0.1.2 inject byte-identical UEVR).

## What ran

1. The Optimizer built and installed (the meter, the app, `install.ps1`), its tests all passed.
2. A first launch by hand, the meter on, the armory save swapped in: two clocks compared, `god` tried, the connector's
   addresses and the four live settings read. Found the meter locking its own file (below). Closed by `quit`.
3. A second launch by hand doing only what the AI can do (the connector): into play without a VR trigger.
4. Fixes (The Optimizer 0.1.1, being built), tests all passed, reinstalled.
5. **The run:** `runner/run.py silent-hill-2` with the window open (the same thing Start runs). 7 spots in 24 min,
   one start crash on the way (retried by the tour itself), every spot measured, the result written, the game closed by
   its own quit after each spot, the saves put back. The AI's tokens: about 3.46 dollars.
6. Two more launches by hand: the save swap proved broken (control) and then fixed.

## Found `[measured]` unless tagged

- **"Late" under the frame warp:** the meter sees one OpenXR frame per game frame; its frame interval equals the
  plugin's own `alive` average (15.37 vs 15.4 ms; the reader matched 21 windows of 10 s). So the meter's numbers ARE the
  game's frame times; `stat unit` (only on screen) is not needed. FABLE not needed: the two do not disagree.
- **The meter locked its own frame file** while the game ran (`fopen_s` is exclusive): every measure of the tour would
  have failed. Fixed (`_fsopen` with `_SH_DENYWR`); the run then measured every spot.
- **The pack's way into play was partly a guess:** the connector cannot pull a VR trigger (its `vr_input` only reads),
  and the AI had no way to wait. Now: Enter until the main menu widget exists, ContinueGame, wait for the pawn, Enter
  twice. Worked on all 7 of the AI's starts that came up (the 8th crashed first) and 3 of mine.
- **Console `god` does nothing** (no cheat manager). The pack stays without god mode.
- **The pack's crash sign was wrong:** "Scene state passed to FSceneView constructor is null" also appears in starts
  that run on. Now `[Exception Handler]`.
- **The save swap did not move Continue:** it replaced only the newest slot file, and Continue follows the slot last
  used (very likely an armory slot since Tefa's LOAD GAME on 11-19 `[hypothesis]`). So **all 7 "spots" of the run were
  the same armory**. Fixed: the chosen slot is put over every slot; slot 0 then loaded the town start. The result's
  ranking is therefore noise: 7 measures of one room, 59.7-67.2 a second, slowest 1% 42.9-50.4.
- **The verdict:** "the processor is the limit" at every spot (card 55-60% busy), so nothing was changed and the
  settings stayed as they were. On the simulator the town start holds 72 (76-85 a second, card 70-74%); the armory does
  not. Whether the armory is processor-bound in the real headset too is NOT established: the simulator draws
  1280x1400 per eye and runs its own mirror; Tefa saw 72 with the warp on b032 in the headset.
- **Start crash count:** 1 of 12 starts this session (the AI's first start, ~9 s after UEVR loaded, a 404 ms stall,
  then the known line). Kept: the logs folder `2026-11-21-2147-optimizer-run/` (the injector log and the crash folder).

## The next run

Spots are named and the duplicates struck on this PC (spots.json): the town start (slot 0), the prison armory (slot
3), the room after the garage (slot 5). Slot 2 (a live Lying Figure fight) is struck: never unattended. The real test of
the tool is a run with the real headset ("I'll wear the headset", or the headset connected and not worn): only there do
the card and the processor split the way the player sees.

## Not established

- Whether the armory's limit in the headset is the processor or the card.
- The connector plugin's own cost (not measured by this version; the installed copy is the throttled build).
- Whether slot 5 stays calm for a minute (it stands 8 m from the fight).
