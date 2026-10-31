# 2026-10-31 (evening): the third wear's faults, fixed, NOT worn (home PC, Opus, no game run)

sh2vr.dll aded645c, installed; backup `D:/SH2 builds/_backup-2026-10-31-1640-before-sh2vr-deploy`.
Tefa's list from the third wear (`2026-10-31b-...md`, 14:46): the hand magazine is the ammo box; the shotgun and rifle
have no manual reload and no ammo pouch; the fire rate is too fast; the holsters stop working after the shotgun runs
empty; the medicine does nothing. Tefa (this session): "if you need to take another look at lucasblaster7 profile".

## Why the shotgun and rifle reloads were dead `[inferred-static 2026-10-31]`
LukasBlaster7's `shotgun_reload.lua`, `rifle_reload.lua`, `shotgun_pump.lua` and `rifle_bolt.lua` are live (main.lua
requires and inits them; they never stood down) and already do what Tefa asked for: rounds one at a time from the hip,
the pump, the bolt. They read the LEFT GRIP as the pad's LB. Since the button layer of 10-29 the game (and the pack)
never sees the grips as LB: LB is the run toggle. So the scripts never felt a grip (and would have felt one while
running).
- Fix: the plugin sends the real grips to the pack (`bridge::send_grips`, Lua event `sh2vr_grips` "L=1;R=0", on a change
  and once a second); `sh2vr_bridge.lua` keeps them in `SH2VR_GRIP_L` / `SH2VR_GRIP_R`; the four scripts read
  `sh2vrLeftGrip()` (the plugin's grip, or the pad as before when the plugin is silent). `build.sh` now installs the
  four scripts (PATCHED).
- Their hip spot is the pack's (20 cm left, 35 down, 10 ahead of the headset), the same numbers as our pistol pouch.

## Why the long guns fired as fast as the trigger `[inferred-static 2026-10-31]`
The pack's pump / bolt gate answers the game's `IsFireReady`; our lowered fire (door B) calls the weapon's fire
straight and skips it. Fix: `sh2vr_bridge.lua` sends `sh2vr_gate` "gate=1" while `isBlockingFire()` of the pump or the
bolt is true (on every change, and in every `sh2vr_flags`); `fire.cpp` refuses a lowered long-gun shot while it is shut
("trigger refused: the shotgun needs its pump / bolt first"). A floor under it: 0.8 s between shotgun shots, 1.0 s for
the rifle (`FIRE_MIN_INTERVAL_SHOTGUN_S` / `_RIFLE_S`, guesses).

## Why the holsters (and the map) stopped after the shotgun ran empty `[hypothesis]`
Log 14:49:19: shot #34 with 0 rounds; from 14:50:05 every `RequestEquipWeapon` refused, even with empty hands, and the
map "never came up". Likely: our fire on an empty long gun started the game's own reload, and the pack's
`shotgun_reload.lua` blocks the reload montage while the tube is empty (its manual reload), leaving the game mid-reload.
Fix: the trigger on an EMPTY gun (any of the three) never calls the weapon's fire; it plays the game's dry-fire sound
(`Play_Wpn_Pistol_Shot_No_Ammo`, the pack's own) and a light buzz. Same click while the pistol waits for its rack or
magazine. If a draw is still refused, the log now says the game's combat state (input mode, pose, aim, rounds).

## The magazine in the hand
Now the pistol's own model, copied (a poseable mesh) with every bone but `magazine_joint` / `magazine_socket_joint`
shrunk and hidden (Lukas's `showOnlyPistolMagazine`, his eject visual); its magazine bone is driven each tick onto the
old ammo box, which stays as the invisible carrier of the fit. If the copy fails, the box shows as before (log line).

## The medicine
Nothing was ever drawn in the hand (the code only logged). Now the item's own model rides the right controller: found
among the loaded static meshes by name (`HealthDrink_01_B`; `Syringe_03_A` / `Syringe_02_A`, from the pack's
attachment keys). If it is not loaded, one log line lists the look-alikes that are, so the next run knows the path.
Using it is unchanged (RT = the game's healing action, Y). The 10-31 log shows the grip let go 0.2 s after the take,
"goes back unused": nothing was there to see.

## To wear
Shotgun: fire (one shot, then the trigger waits), pump with the left hand on the fore-end, fire again; empty it, hear
the click, take rounds from the left hip with the left grip and let go at the loading port, one by one; then draw the
pistol (the lock-up). Rifle the same with the bolt. Pistol magazine: does it look like a magazine, where does it sit.
Medicine: right grip at the lower back, keep holding, look at the hand.

## 16:57 — the left hand locked onto the long gun (sh2vr.dll 014079e8, backup `_backup-2026-10-31-1657-before-sh2vr-deploy`)
Tefa: *"is there a way to lock the left hand onto a pump handle and rifle front, right now i can move my wrist freely"*.
The two-hand lock pinned the drawn left hand's PLACE on the gun's line but passed the controller's turn, so the wrist
followed the controller. Now (`TWO_HAND_LOCK_TURN`) the hand's turn against the gun is taken at the grab and kept: the
gun's turn times that, every view. On the shotgun the hand slides back along the gun with the controller, up to 10 cm
(`TWO_HAND_PUMP_SLIDE_CM`; the pack's pump counts 8 cm of the controller's own pull), so it rides the pump; on the rifle
it stays at the grab's distance. Not worn. If the grab's angle turns out awkward, a fixed angle per gun (like the
pistol slide's) is the next step.
