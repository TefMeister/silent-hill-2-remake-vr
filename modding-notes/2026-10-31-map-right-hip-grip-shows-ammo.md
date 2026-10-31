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

## Later the same morning: the map to the right shoulder, the left hip for ammo only (sh2vr.dll 3d65d3bd)
Tefa: *"ammo pouch needs to be on the left hip, can the map go to right shoulder with LG activating it? ... for shotgun
and rifle ammo, they get inserted one by one, not when the mag is out."* The syringe at the left hip would have taken
the left grip from the pack's shotgun and rifle round grabs, which the pistol-only `mag_out()` check never covered.
- The map: left grip at the RIGHT SHOULDER (held open, as before).
- The drink and the syringe are ONE healing item at the RIGHT HIP: grab it, bring it to the mouth or to the right
  wrist, either uses it. Both were already the same press (the game's Y, ActionHealing; the game picks the item).
- The left hip: nothing of holsters.cpp; the ammo only.
- Backup `D:/SH2 builds/_backup-2026-10-31-1131-before-sh2vr-deploy`.

## Then: the healing item the settled way (sh2vr.dll ef77c753)
Tefa: *"i thought that LG press takes the drink out, LG long press takes the syringe out and LT uses it?"* Checked:
that is the settled layout (2026-10-25); the 10-29 build had used a mouth / wrist gesture instead, which could fire
by itself with the right hand resting near the hip. Now: left grip pressed at the right hip = the drink in the hand;
held 2 s (`HEAL_SYRINGE_HOLD_S`, a buzz) = the syringe; LT while holding = used (the game's Y); let go = back unused.
LT with the left grip held is never the flashlight (buttons.cpp takes LT ALONE only). The mouth helper is gone.
⚠️ Open: Y is the game's one healing action and the game picks the item it uses, so "drink" or "syringe" is ours
only in the log until the game's own use-this-item call is found (live, UEVR-MCP) `[hypothesis]`.
Backup `D:/SH2 builds/_backup-2026-10-31-1144-before-sh2vr-deploy`.

## Then: half a second, and the static leads (sh2vr.dll, backup 1148)
Tefa: *"please change the timing to turn it into syrenge in 0.5 seconds, 2 seconds feels like forever."*
`HEAL_SYRINGE_HOLD_S` = 0.5. Static leads for the flat jobs, read from the exe's strings `[inferred-static 2026-10-31]`:
the game's own HUD option `AmmoWidgetVisibility` (enum `EAmmoWidgetVisibility`, beside InteractionIconVisibility /
TutorialsVisibility; not stored in GameUserSettings.ini or the saves, so at its default) is the first thing to try
for the ammo counter while RG is held; `USHItemHealingUseExecutive` / `USHItemUseExecutive` / `SHHealingItemHandle`
are the item-use classes to look at for a real drink-or-syringe choice.

## Then: the healing item to the lower back, in the RIGHT hand (sh2vr.dll 888f1a23)
Tefa: *"i think the game normally puts the health drink and the syringe in the players right hand instead of the
weapon. have you already made a game changing change to hold it in the left hand with the weapon out? if not, can we
instead move the health drink and syringe to the lower back and little to the right like re8 right now and sub weapon
in re2?"* Checked: nothing of ours changes how the game heals; our code only presses the game's own healing button
(Y) and the game plays its own animation with whichever hand it uses. "Left hand" was only which grip started it.
Now: a fifth spot, `lower back` (12 cm right, 35 down, 30 behind the head: RE2's BACK spot), row `HEAL_ROW` (no
weapon). RIGHT grip pressed there = the drink; held 0.5 s = the syringe (buzz); RT = used (Y); let go = back unused.
RT is kept from the game and never fires while the item is in hand, and until RT is let go after using it
(`fire::on_input` asks `holsters::heal_in_hand()`). RT's edge is read on the game thread. The right grip holding the
item never counts as "show the ammo". The left grip now has only the map. Unused `USE_AT_WRIST_CM` / mouth numbers
removed. Not run yet `[compile-verified 2026-10-31]`. Backup `D:/SH2 builds/_backup-2026-10-31-1253-before-sh2vr-deploy`.
⚠️ RT (the holding hand's own trigger) was my pick; Tefa said LT when the item was in the left hand.
