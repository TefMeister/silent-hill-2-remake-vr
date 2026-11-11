# The crosshair's cost found and removed, the 0.1.2 package, and the button-layer row closed (2026-11-11 evening)

Developer PC, Claude on Fable, OpenXR Simulator, nobody in the headset, Tefa away. sh2vr.dll 517fad1c installed (backup
`D:/SH2 builds/_backup-2026-11-11-2020-before-sh2vr-deploy`). **Nothing here is worn.**

## 1. The crosshair's cost: it was the search, not the hide

The board said "sh2vr's own cost is almost all the crosshair re-hide: hide it once on change, not every frame". Two
things were found `[measured 2026-11-11, simulator, n=1 run each]`:

- The cost line's `crosshair` figure times `crosshair::find()`, not `hide()`. At the main menu there is no gameplay HUD,
  so the crosshair's widget class is not loaded; `ue::find_class` does not remember a miss, so once a second it searched
  every object's name for it: `crosshair 0.23-0.35` ms a frame on average, which is about 20 ms once a second, a hitch.
  Now `find()` returns at once while there is no character (`get_local_pawn(0) == nullptr`).
  Main menu, the plugin's whole cost: **0.29-0.42 ms a frame before, 0.05-0.06 after.**
- `hide()` asked every widget for its visibility through the game's own function call every engine tick. It now reads
  the widget's visibility byte from memory (the offset of UWidget's `Visibility` property, looked up once) and calls
  the game only on the frame the game has shown the widget again; with no such property it falls back to asking. The log
  says which: "the visibility is read from the widget's own memory (no call a frame)" `[verified-live 2026-11-11]`.
  `hide()` runs outside the timed sections, so its saving does not show in the cost line; in play the line reads
  0.32-0.35 ms, of which the arm solve is 0.22 and the crosshair under 0.01.

The crosshair was still hidden in play (two widgets found and hidden, re-hidden 0 times).

## 2. The package: 0.1.2

`pack.sh 0.1.2` from `mod/` (whose `config.txt` has had `VR_SynchronizationMode=2`, Very Late, since 2026-11-06): the
0.1.0 and 0.1.1 zips carry Early, which gives a black headset on Virtual Desktop (`XR_ERROR_SWAPCHAIN_RECT_INVALID`).
`D:/SH2 builds/releases/Silent-Hill-2-VR-0.1.2.zip`, 117 files, 7.4 MB, sha256 8bc62176585da612..., with today's
sh2vr.dll. Smoke test `[verified-live 2026-11-11, simulator, n=1]`: `SH2VR.exe play --sim` from the 0.1.2 folder installed
it, the plugin's heartbeat read "headset on", the installed config reads 2, no `SWAPCHAIN_RECT_INVALID` in the log.
NOT tested: the real headset through Virtual Desktop, which is where the black picture was. Do not hand out 0.1.0 or
0.1.1.

## 3. "One press did two things": the row was stale

The 2026-10-25 row ("X did this and dodged at the same time", "the first thing to sort out") was answered on
2026-10-29 by `buttons.cpp`, the one button layer, and worn on 2026-10-31 (run toggle, right-stick dodge, all four
weapon spots worked). Read again today `[inferred-static 2026-11-11]`: in play the game never sees the pad's B or X from
the face buttons (`set(state, XINPUT_GAMEPAD_B, false)`, `..._X, false`); B reaches it only as the right stick's dodge;
Quest B is read by the hand reload from the real controller. The pack's own button rules stand down
(`SH2VR_PORTED.buttons`). Not re-proved live today (the simulator run was driven by keys); the row is closed on the code
and the 10-31 wear. If one press still does two things in the headset, that is a new fault: say which button.
