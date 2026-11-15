# 2026-11-15 (/lm, Opus, OpenXR Simulator, nobody in the headset): the opening cutscene with the warp on; b027 starts

Tefa: *"2 go"* (game sessions on my own). Build installed: b027 of the build history (sh2vr.dll with today's smoothing,
the map at the left back, the dodge on the right stick click, the menu button tap / hold). Launched by the mod's own
launcher, `SH2VR.exe play --sim --no-install` (0.1.1 folder); saves backed up to `D:/SH2 builds/_save-backups/2026-11-15-before-lm-newgame/` first.

## What ran

- **First start crashed** right after the injection (0.1.2 launcher folder): `EXCEPTION_ACCESS_VIOLATION reading
  0xffffffffffffffff` on the game thread, the stack only the game and `UEVRBackend` (no `sh2vr` frame)
  `[measured, the crash report, n=1]`. The second start (0.1.1 launcher folder, same installed plugin) ran.
  **Not a package fault** (the reader, later the same session): both folders inject byte-identical UEVR (AFW beta 6) with
  the same settings; the same stack crashed on 11-12 (twice) and 11-13, always 22-25 s after the start: an occasional
  start-up crash in a hooked game function (dossier §13) `[measured: the hashes and crash folders]`.
- Warning -> main menu -> NEW GAME -> START GAME -> PRESS ANY BUTTON by keys through the UEVR connector, a picture
  before each key (KNOWN.md's route held).
- **The frame warp switched itself on** in play before the cutscene (thousands of warp calls in the log).
- **The opening cutscene ran its whole length with the warp on: 3 min 38 s on the flat screen (5.5 m, the game's real
  camera), no crash, and a clean return to VR** (both eyes alike, the hands drawn, the car-park overlook)
  `[verified-live 2026-11-15, simulator, n=1]`. 18 stall lines in the run, all during the load-in before the cutscene
  (the longest 1.2 s), one of 118 ms at the cutscene's start.
- **One passing frame mid-cutscene** had a right eye 30 % darker with less than half the contrast of the left
  (means 14.3 / 9.9); the next pair, 12 s later, matched exactly (16.6 / 16.5) `[measured, n=1 each]`: a made-up
  frame at a shot change is the likely reading `[hypothesis]`. Tefa's headset clip of 2026-11-13 (torn frames, a tilted
  screen at the switch) was not reproduced here; the simulator may not show what Virtual Desktop does.
- Our plugin's cost: 0.37 ms a frame with today's additions (ik 0.24) `[measured]`. With no weapon at the start the
  smoothing stays raw (as designed); the dodge, the menu button and the map need the VR Bench layer on for the game
  (the connector's simulated pad reaches the game after our plugin, so our button layer never sees it): next run.

## Not established

- The b027 controls in play (dodge, menu tap / hold, the map at the left back, smoothing with a gun): untested.
- Whether the first-start crash belongs to the 0.1.2 package.

## Later: b027's controls on the simulator with VR Bench, and b028

Launched by the 0.1.1 launcher with the VR Bench layer on, CONTINUE, a VR trigger pull past PRESS ANY BUTTON (the save
stands at a save point). Driven by a scratch script that holds both pretend controllers and reads the plugin's log
`[verified-live 2026-11-15, simulator + VR Bench, n=1 each]`:
- **the right stick click = the dodge:** "right stick click -> dodge (the game's B)";
- **the menu button tapped = the inventory:** it opened (the gameplay menu took the focus);
- **the left grip at the LEFT BACK = the map:** out, then put away, "the map is gone from the screen";
- **the menu button HELD 0.6 s did NOT pause:** it opened the inventory like a tap and no "held" line came: the
  button seems to reach the game as a short START however long it is held [hypothesis: UEVR passes it as a pulse].
  So b027 had taken the pause away from the menu button: **b028 switches the tap / hold off** (`MENU_TAP_HOLD_ON`
  false; the menu button = the game's pause again, Y = the inventory as before). Next: read the button's real state
  from UEVR's own action, not from the XInput START.
- **the pistol from the right hip:** the game refused the draw at this save spot (its combat state: no weapon here),
  so the smoothing proof line was not seen; another spot is needed.
