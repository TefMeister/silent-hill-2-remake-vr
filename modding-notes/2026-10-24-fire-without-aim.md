# 2026-10-24 — shooting without the aim stance (the RE2 way)

Tefa's requirement (2026-10-24 00:50): *"walking must be normal, must be able to walk and run with the weapon two handed
and shoot without the RG pressed."* The always-aim hold in `main.lua` (LT held while a gun is out) shoots, but the
aim stance slows walking and swallows the run input, so it is not the answer. Live session with Tefa in the headset,
home PC, probe build `sh2vr.dll` (`src/fire.cpp`, F9 probe, F10 force-ready).

## What the game does, measured live

- The ranged combat part is `SHRangedCmbSubcomp` under the pawn's `Combat` component
  (`…SHCharacterPlay_BP_C_<n>.Combat.SHRangedCmbSubcomp_<n>`); `FindSubcomponentByClass` is not callable through the
  MCP (ClassProperty argument), but `uevr_search_objects "Combat.SHRangedCmbSubcomp"` finds the instance, and C++
  `ue::call_ret` with the class as an object argument works. `[verified-live 2026-10-24, n=1]`
- `RequestManualFire(weapon)` on it fires a real round **while aiming** (Tefa: "gun fired once"); the combat input
  mode went 12 → 13 for a frame. `[verified-live 2026-10-24, n=1]`
- **With the gun lowered it does nothing**: `IsFireReady()` on the weapon answers false when not in ranged aim
  (input mode 0, in-ranged-aim 0, fire-ready 0; while aiming all three are 12 / 1 / 1). `[verified-live 2026-10-24, n=2]`
- `IsFireReady` has no backing field on `SHItemWeaponRanged` (the whole class listed): it is computed in native code.
  Lukas's `shotgun_pump.lua` already hooks it and forces its answer through `locals.ReturnValue` for the pump, so a
  forced answer is at least read by someone. Whether the native fire path consults the UFunction answer is the open
  question the F10 force-ready toggle tests. `[hypothesis]`
- Pressing the fire input with no aim swaps to the melee weapon (Tefa saw the iron pipe come out); the pistol came
  back with `SHWeaponManageCmbSubcomp.RequestEquipWeapon("Pistol", Instant=true)`. `[verified-live 2026-10-24, n=1]`
- `SHDebug_Character_Combat_ForceRangedAim()` called once did nothing visible (in-ranged-aim stayed 0). `[verified-live 2026-10-24, n=1]`

## Movement while aiming

- The aim walking speed is the weapon's own `AimMovementData` (pistol: 120 forward/side/back; `WeaponMovementSet`
  unused, `bUseWeaponMovementSet` false). Writing 250 into the live pistol's `AimMovementData` made aim-walking
  "much faster" (Tefa). `[verified-live 2026-10-24, n=1]` The normal unaimed walk and run numbers are still to be
  measured (the movement component's `MaxWalkSpeed` read 108 standing and 120 while aim-walking).
- Clicking run while aiming does nothing: `ProcessSprintRequestedChangeEvent` on the ranged part was never called
  (hook count 0), so the run input is dropped before the gun part hears it — swallowed by the aim input mode, not
  blocked by `IsSprintBlocked` (false). `[verified-live 2026-10-24, n=1]`

## Where this leaves the design

Two doors, to be chosen by the F10 test:
1. **Force "ready" and fire lowered** (the RE2 way proper): if forced `IsFireReady` lets `RequestManualFire` fire a
   lowered gun, the mod never enters the aim stance; walking and running are untouched.
2. **Stay in the aim stance but take its costs away**: aim speed = walk speed per weapon, and run handled by us
   (the aim state's run drop is in `ProcessSprintRequestedChangeEvent`, blockable; the swallowed run input needs
   another route in: `SHDebug_Character_Movement_ForceSprint`, or driving the movement component directly).
