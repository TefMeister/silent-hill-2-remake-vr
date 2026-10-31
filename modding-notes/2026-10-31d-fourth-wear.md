# 2026-10-31 (evening): the fourth wear, Tefa in the headset (home PC, Opus)

sh2vr.dll 014079e8, injected 19:00 at the main menu (the 18:52 try had crashed on the loading screen, UEVR's own render
hook; see 2026-10-31c). Log kept: `D:/claude video game stuff/logs/sh2/2026-10-31-1900-fourth-wear-log.txt`.

## What Tefa said (19:2x), word for word
> all the hud elements have to be removed from map screen. [Image #2] map needs to stay held between left thumb and
> fingers at the back of the map, locked like this, same for right hand if it is docked on the right side of the map.
> map still doesn't go away when LG is used again in right hip holster, but it does stay more straight and like it has
> to in the left hand. manual reloads are improving a lot - now i can insert shells into the shotgun and that looks
> good, they just need to go in automatically when they reach the weapon, we also have to get the last part to play as
> the game's own animation if doable, pushing the shell in. i can do the racking motion now, but do it from anywhere
> even the tip of thegun, grip it with LG and then i'll have the back and forth motion possible without the LT required
> to be held, without it moving the pump handle, the game still automatically racks the shotgun after a shot. rifle
> ammo from the pouch comes out as a spent case, not a bullet and there is no way to rack the bolt, even dock it or
> anything, like this part doesn't exist. pistol issues - ammo from the pouch is invisible, but works. when weapon is
> emty and needs racking then after inserting the mag hand jumps on the slide automatically and needs LT to rack. it
> has to stay docked on the mag bottom even when in the pistol. RG at the back now brings out the medicine, but this is
> supposed to holster the gun and have the medicine out without rg being held, then RT to use it. right now i had both
> pistol and the syrenge in the same hand.

## What the log says `[measured 2026-10-31, log, n=1]`
- Map: LG at the right hip again logs "the map is put away", and ~1 s later "the map is on screen but not in the hand:
  taken into the left hand": our own catch-up takes the still-closing map back into the hand. Three times (19:03:49,
  19:03:50, 19:04:28).
- Shotgun: no "trigger refused" line all wear and `gate=0` throughout: the pack's pump never reported its gate shut, so
  the game's own pump animation ran after every shot.
- Pistol magazine invisible: at each take, "call refused: K2_SetRelativeLocationAndRotation: an argument does not match
  its parameter" and "MarkRenderTransformDirty: no such function": the copy's bone drive fails.
- Pistol rack: after a magazine into an empty gun, "slide NEEDED", then the rack is held with LT (our rule).
- Medicine: the models are found and shown (HealthDrink_01_B; Syringe_03_A), but only while RG is held, and the gun
  stays in the same hand.

## Fixed after the wear, sh2vr.dll d3aacc7 (backup `D:/SH2 builds/_backup-2026-10-31-1924-before-sh2vr-deploy`), NOT worn
- `K2_SetRelativeLocationAndRotation` was always refused: its 4th parameter is the out HitResult, we passed a bool. Now
  (location, rotation, sweep) only, in reload.cpp (the magazine's fit), holsters.cpp (the medicine's fit), hands.cpp.
  `[compile-verified 2026-10-31]`. The magazine copy now rides the left controller (its bounds were where its owner
  actor stands), re-asserts the magazine bone's scale and updates its bounds each tick: the likely cause of "invisible"
  `[hypothesis]`.
- Medicine: RG at the lower back puts the gun away and takes the drink out, no grip hold; RG there again = syringe, a
  third time = back; a weapon spot puts it back; RT uses it.
- Map: put away with the game's back (pad B, Cancel), pressed again every 1 s (5 at most) while it still shows;
  nothing takes it back into the hand meanwhile.
- Pistol: after an insert the left hand stays on the magazine (its place against the seat at the insert) until LG
  lets go; the slide only takes a FRESH left grip press, and the grab alone holds it (no LT): pull back, push forward
  or let go = home (`RACK_NEEDS_LT` false, `RACK_HOME_AT` 0.3).

## Still to do (need the game running)
- Map screen: find and hide every HUD widget showing with the map (live widget list).
- Map in the hand: thumb in front, fingers behind, the map locked to that grip; the same for the right hand when it
  holds the right edge (Tefa's picture: I could not see it in this session).
- Shotgun pump: the pack's lock after a shot never happened (gate 0 all wear): check live whether its OnFirePerformed
  hook runs for our lowered fire (state via `shotgunPump.dumpDiag`), and why the game's own pump animation still plays.
  The grab only at the pump (not anywhere on the gun); the pump handle must move with the hand.
- Shotgun shells: go in when they reach the loading port (no release needed); the last push as the game's own
  animation if possible.
- Rifle: the round from the pouch is `SniperRifle_Shell_01_A` (the spent case, rifle_reload.lua `SHELL_MESH`): find
  the live round mesh. The bolt: no grab at all; check live whether `rifle_manual_bolt` is on and its lock happens.

## Ghost Hand (BeG0nE) on this wear
19:20:36 report: mag 3 of 3 finished, slide 2 of 2; the game's hand jumped 43 times while the controller was calm (lead: our left-hand overrides switching on and off: two-hand grab, rack, now the magazine hold); a controller jumped 196 times (tracking). Read with `begone.py report ghost-hand latest` when the hand jumps come up.
