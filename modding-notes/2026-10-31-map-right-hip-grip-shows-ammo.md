# 2026-10-31: the map at the right hip, the gun stays in the hand, the right grip shows the ammo (home PC, no game run)

Tefa: *"please move the map to right hip and the feature to hold on to a weapon, can you remove that safely, or is it
cross wired with everything now? the idea is to show ammo counter and optionally the crosshair while RG is held,
without it triggering the AIM stance"*

## Found first: the pack's button stand-down had been lost since 2026-10-29 10:28
The 2026-10-29 button-layer note says main.lua's dodge / sprint / back-tap / LT-flashlight rules skip when
`SH2VR_PORTED.buttons` is set. That edit was made in the UEVR profile folder only. `build.sh --deploy` copies the
repo's `mod/.../scripts/main.lua` over the profile's (it is in PATCHED), so the next deploy (10:28) put the unedited
file back. The installed main.lua was byte-identical to the before-buttons backup `[measured 2026-10-31]`. So every
later simulator run (the holsters, the left grip items) had the pack's own pad rules live beside buttons.cpp. The
edited copy survived in `D:/SH2 builds/_backup-2026-10-29-0955-before-sh2vr-deploy/files/scripts/main.lua`; its
four hunks are now in the repo copy, so deploys carry them. Rule: edit pack files in `mod/`, never in the profile.

Which runs first on a pad poll, the pack's Lua or our C++, is still not measured. The 10-29 simulator proof (LT alone
reached buttons.cpp as the flashlight while the pack's aim block wrote LT = 0) points at the C++ running first
`[inferred-static]`.

## Changed (sh2vr.dll c8f02050, installed with the patched main.lua; backup `_backup-2026-10-31-1121-before-sh2vr-deploy`)
- `settings.h`: the map at the RIGHT hip, the syringe at the LEFT hip (it gives way to the ammo pouch while a
  magazine is out, the check the map had before). The drink stays at the right shoulder.
- `holsters.cpp`: letting go of the right grip no longer puts the weapon back. The grip pressed in a spot: that
  spot's weapon in hand goes back; anything else draws the spot's weapon (a swap). This was self-contained in
  holsters.cpp; `holding()` has no other users, so nothing else was wired to it.
- `holsters.cpp`: the right grip HELD away from every spot with a gun out, in play = "show the ammo"
  (`showing_ammo()`); for now it logs the rounds in the gun, clip size and carried ammo. `SHOW_CROSSHAIR_ON_GRIP`
  (off) is the setting for the optional crosshair; neither on-screen part is built.
- `main.lua` (repo copy): with the buttons ported, the aim block never asks for the aim stance (no grip aim, no
  always_aim); only the pump / bolt blocks still keep the trigger from the game; the pack's own 180 turn on RB skips.
  `luac -p` passes.

## Open
- The on-screen ammo counter: find the piece of the game's HUD that shows the ammo (it shows in the aim stance) and
  show it while `showing_ammo()`, or draw our own. Needs the game running (UEVR-MCP object search), Claude alone.
- The optional crosshair: crosshair.cpp takes the game's crosshair out of the viewport for good; showing it would
  mean collapsing instead, and it points along the head, not the gun. Decide with Tefa after the counter.
- Wear: the map at the right hip, draw / put back / swap, the grip held = ammo line in the log, no aim stance.
