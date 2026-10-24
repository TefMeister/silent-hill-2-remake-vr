# 2026-10-24 — firing with the gun lowered: the two doors tried live (OpenXR Simulator, no headset)

/lm, home PC, Fable. Tefa: *"all yours to run the game as much as you need to test it for shooting without RG"*. Follows
`2026-10-24d-the-fire-gate.md` (the gate read offline, two doors on keys). Three launches; the headset was not in use,
the OpenXR Simulator played it (see "How the game was run").

## Result

| door | what it does | lowered pistol, in play |
| --- | --- | --- |
| **A** (F9): the game's own `RequestManualFire` with the aim byte (+0x118) set to 1 and the weapon's ready threshold (+0x690) set to 0 for the call | the game's request path runs | **nothing**: no shot, rounds unchanged `[verified-live 2026-10-24, n=2, flat on the simulator]` |
| **B** (F8): the weapon's own fire, vtable slot 0x750, `(weapon, manual = true)` | the step after the gates | **a real shot every press**: rounds 9 → 8 → 7, the pistol Blueprint's `OnFirePerformed` and `OnShellEjected` ran (MCP hook log), the function returned 1 `[verified-live 2026-10-24, n=3, flat on the simulator]` |

The offsets agreed with the game's own answers on every press (`offset check ... AGREE`, threshold 0.823), so the
layout read from the exe is the running build's.

So the game's request has a third gate past the two that were read (the cooldown branch, or the input-mode 13 step
refusing while lowered: not read further, not needed), and the weapon's own fire has none: **door B is the way**.

Found on the way, and it hid door B's success for a whole run: the plugin counted shots with a hook on
`SHItemWeaponRanged::OnFirePerformed`, but the Blueprint weapon classes OVERRIDE that event, so the pistol's shots never
touched the base class's UFunction. `fire.cpp` now hooks the class of the weapon in hand (`WeaponPistol_BP_C`), as
Lukas's scripts did. The same mistake would blind any hook on a base-class event in this game.

## Pass 2 (installed 16:30): the right trigger fires a lowered gun

`fire.cpp`: in `on_xinput_get_state`, a fresh right-trigger press with a ranged weapon out and `IsInRangedAim()` false
queues a door-B shot (fired on the game thread in the next tick) and the trigger is kept from the game while held
(otherwise the game swaps to the melee weapon on an unaimed trigger, seen 2026-10-24 00:50). In the aim stance the
game keeps its trigger. Our own pace between lowered shots: `FIRE_MIN_INTERVAL_S` (door B skips the game's cooldown).
The finger poses still see the pull (they read the state before fire eats it).

**Pass 2 seen flat (16:55, the trigger build installed):** a pretend pull through the trigger path (F7) fired a shot
with the gun lowered, counted it (the class hook), spent a round and ejected the shell; ten pulls emptied the gun
`[verified-live 2026-10-24, n=1, flat]`. The simulator's pad never reaches the plugin's XInput hook (the MCP's override
is applied after the plugin callbacks), so the real trigger waits for Tefa's controller. Also seen: the OLD script
turned a trigger press into aim + fire (it set LT for the game), which is what fired on the simulated pad; that rule
is off now that the C++ owns the lowered trigger.

## How the game was run with no headset on (keep)

- With UEVR attached and the Virtual Desktop runtime idle (no headset), UEVR logs `Failed to initialize Framework on
  DirectX 12` on every frame and the game's PRESS ANY BUTTON screen takes no input at all. **Fix:** launch the exe with
  `XR_RUNTIME_JSON` pointing at the OpenXR Simulator's json; the simulator plays the headset (hmd on, controllers on).
- Started straight from the exe the game does not know the Steam user, writes a fresh profile into the SaveGames root
  and shows no Continue (HasSlots() false; the menu starts at NEW GAME, a blind Enter opens the new-game settings).
  **Fix:** `SteamAppId=2124490`, `SteamGameId=2124490` in the environment; then HasSlots() is true and the save loads.
- In VR mode the window mirror does not draw the menu text, so the recorded Menu-o-matiC checkpoints do not match.
  The menu's own functions through the UEVR MCP work: the live `MainMenuWidget_BP_C` (search
  `MainMenuWidget_BP_C /Engine`), `ContinueGame(Movie = null, NonSkippableLen = 0)` loads the newest save; then Enter at
  PRESS ANY BUTTON TO PLAY; `SHWeaponManageCmbSubcomp.RequestEquipWeapon(WeaponDataRowName = "Pistol", Instant = true)`
  puts the pistol in the hand. All in `ai-game-control-profiles/routes/silent-hill-2/KNOWN.md`.

## Not established

- Where the bullet goes with the gun lowered: the direction comes from the weapon's own virtuals (0x728 / 0x738), seen
  only flat; the headset says whether it follows the controller.
- Recoil and the body's fire animation: door B skips the request path's montage and input mode 13, so the character
  plays no fire animation (in VR the arms are ours, so this may be right).
- Whether the eaten trigger reaches the Lua pack's remap first (the plugin's callback order against the Lua's).
