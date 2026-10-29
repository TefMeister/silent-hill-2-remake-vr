# 2026-10-29 (late morning) — the weapons on the body: drawn by the right grip, back when it lets go (home PC, Fable, simulator)

## The job
Tefa picked it from the list: *"1 please"* (the holsters, the rest of the "full VR button and body layout"). Tefa's rule:
*"right grip: hold a weapon; let go and it goes back to its holster"*.

## What the game offers (read live through the UEVR MCP) `[verified-live 2026-10-29]`
The weapon manager is `Combat`'s `SHWeaponManageCmbSubcomp` (found with `FindSubcomponentByClass`, as the ranged part is):
- `RequestEquipWeapon(WeaponDataRowName, Instant)` -> true: rows `Pistol`, `Shotgun`, `Rifle`, and the melee rows
  (`IronPipe` on this save; `GetLastUsedMeleeWeapon()` names the one the game last had in hand).
- `RequestUnequipWeapon(Instant)` -> true: `GetEquippedWeaponName()` then reads `None`.
- `GetEquippedWeaponName`, `GetEquippedWeapon`, `HasEquippedWeapon`, `GetLastUsedRangedWeapon` ("Shotgun" here).
The pack's Lua has no weapon holsters (its "holster" is the magazine's hip pocket), so this is all ours.

## What was built: `holsters.cpp` (sh2vr.dll, installed)
- Four spots, measured from the HEADSET in its own tracking space (metres, turned with the head's yaw only, never its
  tilt): right hip (20 right, 35 down, 10 forward) = Pistol; right shoulder (15, -10, -10) = Shotgun; left shoulder
  (-15, -10, -10) = Rifle; left hip (-20, -35, 10) = the melee weapon (whatever the game last held). Numbers in
  `settings.h` (`HOLSTER_SPOTS`), to be tuned in the headset.
- Right hand enters a spot: a short buzz. Right grip pressed inside a spot, hands empty: that spot's weapon is equipped
  (instant), "holding". Right grip let go while holding: `RequestUnequipWeapon(true)`, hands empty. At the start of each
  pawn a weapon the game put in the hand with no grip held is put back once ("the hands begin empty"; not exercised yet:
  this save loads with empty hands).
- The grip is the REAL right grip (input.h, before the button layer rewrites the pad), or F6 sent to the game window as
  a pretend grip for desk tests. A status line every 5 s: holding, the spot, the hand's nearest spot in cm, the weapon
  manager's name for what is in hand.
- ⚠️ Why tracking space and not the game's camera: on the simulator the camera manager's `GetCameraLocation` sat about
  1.5 m from the controllers and `GetCameraRotation` read 0,0,0, so a spot built from it (the first version) was never
  where the hand was (nearest 30 cm+). UEVR's own head and grip poses agree with each other. ⚠️ `reload.cpp`'s ammo
  pouch still uses the camera (`gun::hmd`); it worked in the headset on 10-25, so the camera may follow there; on the
  simulator it will not. A `[PD]` row: move the pouch to the same tracking-space maths.

## Proved on the simulator `[verified-live 2026-10-29, n=1 each]`
- `holsters: status ... hand in=right hip (nearest spot 0 cm)` with the simulated right controller at (0.20, -0.35, -0.10)
  from the head; F6 -> `right grip at the right hip: drew the Pistol`, status `equipped=...WeaponPistol_BP_C`; F6 ->
  `right grip let go: the weapon goes back to its spot (right hip)`.
- Hand to (-0.20, -0.35, -0.10): F6 -> `right grip at the left hip: drew the IronPipe`; F6 -> back.
- Not proved: the real grip in the headset, the shoulders' positions (built from the hip numbers), the buzzes, a weapon
  in hand at load, what the game does with the drawn weapon's pose in our hands (the pack's attachments).

## Tools
- `dev-archive/recon/2026-10-29-buttons/sim_ctl.py` moves the simulator's controllers (hand, x, y from HEAD height, z,
  yaw, pitch, trigger, buttonA).
- `dev-archive/recon/2026-10-29-buttons/sh2_front.ps1` brings the REAL game window to the front and clicks into it (the
  process's main window handle is the OpenXR Simulator's own window; the game takes the pad only while in front).

## Open
- Wear it: draw and holster each weapon; tune the spot numbers; then the left-grip items (map, flashlight, syringe,
  drink) and parked weapon meshes at the spots (the melee idea: "shown parked there while a gun is in hand").
- The ammo pouch into tracking space (above).

## Added the same morning: the LEFT grip's items (Tefa: "can you do left hand grip too?")
- Left hip, left grip: the MAP. Pressed = the game's d-pad up (MapToggle) through the button layer's press queue; the
  map stays while the grip is held (the real LB is kept from the game meanwhile, so the map does not flip pages by
  itself); let go = d-pad up again, the map closes. The right grip meanwhile reaches the game as RB = Next Map. While
  the pistol's magazine is out, the left grip at the left hip stays the reload's (the ammo pouch shares the spot).
- Right hip, left grip: the SYRINGE "in the left hand"; bring it within 15 cm of the right hand (the wrist) = the
  game's healing action (Y). Right shoulder, left grip: the DRINK; bring it to the mouth (12 cm ahead, 10 below the
  head) = the same healing action (the game picks the healing item itself: one action for both).
- Left shoulder: the flashlight stays on LT alone (the button layer); a hand-held flashlight with its beam is a later
  piece. F5 to the game window = a pretend left grip.
- Proved on the simulator `[verified-live 2026-10-29, n=1 each]`: left hand at the left hip + F5 -> `the map opens`
  and the game's map screen ("Toluca Prison 1F Map", Previous/Next Map on LB/RB) was on screen; F5 -> `the map
  closes`. Left hand at the right hip + F5 -> `the syringe is in the left hand`; moved to the right hand -> `the
  syringe at the right wrist: used`. The drink not tried.
- Two more facts: (1) the game polls the pad, and so our XInput callback runs, only after UEVR has spoofed a gamepad
  connection, which the first VR CONTROLLER input causes (a simulated trigger pull): before it, `buttons:` counted
  0 polls and a queued press waited. (2) The map is the `MenuSectionMap_Widget_C` section widget, `IsVisible()` only
  while open; the gameplay menu reports no focus for it, so the button layer now checks that widget too (menu mode
  in the map: X = back, the grips = LB / RB).
