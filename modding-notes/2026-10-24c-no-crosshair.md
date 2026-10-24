# 2026-10-24 · No crosshair at all

/pd, home PC, no game running. Built and installed; nothing here has been run.

Tefa, 2026-10-24: *"please remove the crosshair, we will not use it in game at all."*

## Why it followed the head `[inferred-static 2026-10-24]`

The game draws its aim crosshair as a HUD widget, `CrosshairContainer_WidgetBP_C`
(`/Game/Game/UI/GameplayHUD/Crosshairs/`, the class jbusfield's reticule finder saved in `data/reticule_parameters.json`).
jbusfield's reticule takes that widget off the screen and redraws it in 3D at the end of the gun, but only when it is
created: `reticule.lua` `createFromWidget` returns at once while the reticule is hidden, before its
`RemoveFromViewport`. Tefa's settings have `"uevr_reticule_hide": true` (installed and in the repo), so the game's own
widget was never taken off the screen and UEVR showed it on the HUD plane in front of the face, which follows the head.
That fits Tefa's sighting; it has not been seen in a log.

## What was built

`dev-archive/native/src/crosshair.cpp` (sh2vr plugin): once a second it lists every live `CrosshairContainer_WidgetBP_C`
and takes any that is in the viewport out of it (`RemoveFromParent`); every frame, after the game's own tick (UEVR's
post-engine tick, before the HUD is painted), it collapses any that the game made visible again. The class path and
the visibility value are in `settings.h`. The 3D reticule stays hidden as before (`uevr_reticule_hide`).
`[compile-verified 2026-10-24]` sh2vr.dll, installed with `build.sh --deploy` (backups in `D:/SH2 builds/`).

## What a run shows

- `crosshair: hidden ...CrosshairContainer_WidgetBP_C... (was in the viewport: yes)` once per widget: found and taken off.
- The heartbeat's `crosshair re-hidden N`: how often the game showed it again. A number that climbs every frame while
  aiming means the game re-shows it each frame and the HUD may still flash it; then hide its child images instead.
- No `crosshair:` line at all with a gun out: the class path is wrong for this build (search the live objects for
  "Crosshair" with the uevr MCP).

Undo: `bash dev-archive/native/tools/build.sh --undo "D:/SH2 builds/_backup-2026-10-24-1124-before-sh2vr-deploy"`
(the build before both of today's changes).
