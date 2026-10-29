# 2026-10-29 (morning) — one button layer, built on the game's vanilla pad and proved on the simulator (home PC, Fable)

## The job
Tefa (2026-10-25): *"the first thing to sort out"*: one press reached two things (X racked the slide AND dodged), three
layers touched the pad. Today: *"yes please sh2, and keep the mod ideas for it"*, *"it has to be built on vanilla
controls"*, *"feel free to launch the game with uevr mcp and openxr simulator"*, and *"re8 reload system is the closest
to what i would like to have in sh2"* (filed to mod-ideas, Controls & input).

## The game's own pad, read live (vanilla)
`InputRebinding.sav` holds `None` (no rebinding) `[verified-live 2026-10-29]`, so the live binding list IS the default
table (object of class `/Script/InputRebinding.RebindableInput`, `BindingList.ActionBindings`) `[verified-live 2026-10-29]`:

| pad | actions | pad | actions |
| --- | --- | --- | --- |
| A | Interact, InvestigationFlip, Struggling | LB | Sprint, InvestigationPreviousItem |
| B | Dodge, Cancel, Skip | RB | QuickTurn, InvestigationNextItem |
| X | Reload | LT | RangedAim, InvestigationZoom |
| Y | ActionHealing, InvestigationTranscript | RT | Fire, MeleeAttack |
| right stick click | FlashlightToggle | BACK / START | InventoryToggle / MainMenu |
| d-pad up | MapToggle | d-pad left / down / right | pistol / shotgun / rifle |
| left stick click | nothing | InventoryMapHold | no key |

The pack's own rules (main.lua, inside `if not ui.isRemapDisabled()`): right stick down pressed B (dodge), the left stick
click or a forward double-tap pressed LB (sprint), a backward double-tap pressed RB, LT pressed the right stick click
(flashlight), head gestures. `remap_disabled: true` in remap_config_dev.json is a different switch (the UI's
`remapEnabled` state), so that block was live.

## What was built: `buttons.cpp` (sh2vr.dll 71ffbab5, installed) + the pack stands down
- `plugin.cpp` order: `input::on_state` keeps the REAL controller for the hand modules (reload, rack, fingers), then
  `buttons::on_input` rewrites the state into what the game sees, then `fire::on_input` (the trigger).
- In play: left stick click = run toggle (the game sees LB held); B kept from the game (the magazine, reload.cpp);
  X kept; Y -> BACK (inventory); right stick down -> B (dodge, the left stick steers); right stick up -> RB (quick
  turn, `QUICK_TURN_ON` false by default); LT alone -> right stick click (flashlight), LT never reaches the game (no aim
  stance); the grips never reach LB / RB; right stick Y zeroed (the pack keeps its X turn).
- In a menu: X -> B (back), Y -> BACK, the real B kept out, everything else as the game has it.
- The menu check, every frame on the game thread: no pawn = menu; the gameplay menu widget
  (`/Game/Game/UI/GameplayMenu/GameplayMenu_Widget.GameplayMenu_Widget_C`, inventory / map / memos / examine) answers
  `HasAnyUserFocus()` true while open; the save menu widget the same way; the pause menu widget
  (`/Game/Game/UI/Menu/InGameMenuWidget_BP.InGameMenuWidget_BP_C`) EXISTS only while open (focus false while open,
  "object no longer valid" after B closed it).
- The pack: `SH2VR_PORTED.buttons = true` (build.sh --deploy writes it); main.lua's dodge, sprint, back-tap and LT
  flashlight rules skip when it is set; snap / smooth turn and the head gestures stay the pack's. The edited main.lua
  lives in the UEVR profile folder (backup `D:/claude video game stuff/backups/sh2-main.lua.before-buttons-2026-10-29`).

## Proved on the simulator `[verified-live 2026-10-29, n=1 each]`
- `buttons: PLAY buttons (play)` the moment the pawn appeared; `MENU buttons (the gameplay menu has the focus)` on
  the inventory opening, `PLAY buttons` on its close; `MENU buttons (the pause menu is up)` on START.
- The simulator's left trigger alone (no grip): `buttons: left trigger alone -> flashlight (the game's right stick
  click)`. (The first simulated press only woke UEVR's gamepad spoof; the second was seen.)
- The simulated right-hand A passed through to the game (it started play).
- Not tested (the simulator has no sticks, no B / X / Y, no grips): the run toggle, the dodge, Y -> inventory, X in
  menus, the magazine B. Headset.

## Driving facts (also in control profiles `routes/silent-hill-2/KNOWN.md`)
The MCP inject could not load `openxr_loader.dll` until a copy sat beside the game's exe; the warning screen passed on
the MCP pad's A, not on Enter; the window handle measured 1744x54 right after launch (not sorted out).

## Open
- Wear it: every row of Tefa's table, one by one; then the holsters (the grips) as the next piece.
- The builds repo has no entry for this project yet (`builds.py init`), so this install is stamped by the dll hash only.
- `interaction_control_mode` and the pack's other input modes were not checked with the layer on.
