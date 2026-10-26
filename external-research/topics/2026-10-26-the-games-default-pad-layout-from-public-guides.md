# The game's default pad layout, from public control guides

**Date:** 2026-10-26 (`/gr`, FULL pass) · **Status:** 🆕 new · **For:** the button-layer row, which "needs first: the
game's own buttons for inventory, map, back and quick turn".

## What the public guides say (Xbox layout)

`[reported 2026-10-26]`. Two independent control guides (Prima Games, AMK Station) agree on these defaults. ⚠️ Both
sites refused an automated fetch (HTTP 403), so the values below come from the search engine's extracts of those two
pages, not from reading them in full. Treat them as the starting list to confirm live, not as settled.

| Action | Xbox button |
| --- | --- |
| Map | D-pad Up |
| Inventory | View |
| Quick turn | RB |
| Sprint | LB |
| Dodge, and Back in menus | B |
| Heal: tap for a quick heal, hold for the syringe | Y |
| Interact | A |
| Shoot / melee | RT |
| Switch weapons | D-pad Left / Right / Down |
| Flashlight | right stick press (one extract only; weakest row) |

Keyboard: quick turn Q, dodge Space, sprint Left Shift (same extracts).

Already known in-house and consistent with this: B = dodge, X = reload, LB = sprint (board, 2026-10-25). The guides
add the four the board asked for: **inventory = View, map = D-pad Up, back = B, quick turn = RB.**

## Notes that matter for the installer's "controls to default" step

- The game has its own remapping menu for pad and keyboard, plus a choice of button-prompt style (Xbox, PlayStation,
  Nintendo) `[reported]`. The board already found where the player's changes live (`InputRebinding.sav`).
- Steam Input: a player-side fix circulating for wrong prompts on non-Xbox pads is to switch Steam Input off for this
  game, or toggle the in-game layout type `[reported]`. Worth one line in the release readme, since our layer reads the
  pad through XInput and Steam Input sits in front of it.

## Next step

Read the live mapping once through the UEVR MCP (the game's Enhanced Input mapping objects) and tick each row above;
then the button layer can be built against confirmed names. No public dump of this game's input-action asset names was
found (one search) `[reported 2026-10-26]`; that is an automated negative, not proof none exists.

## Sources

- Prima Games, "Silent Hill 2 Remake Complete Controls for PC and Controller" —
  <https://primagames.com/tips/silent-hill-2-remake-complete-controls-for-pc-and-controller>
- AMK Station, "Complete guide to Silent Hill 2 Remake controls for PC and game controller" —
  <https://amkstation.com/complete-guide-to-silent-hill-2-remake-controls-for-pc-and-game-controller/>
- Prima Games, "How to Fix the Silent Hill 2 Remake Controller Issues on PC" —
  <https://primagames.com/tips/how-to-fix-the-silent-hill-2-remake-controller-issues-on-pc>
