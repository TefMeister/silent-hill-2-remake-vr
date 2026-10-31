# 2026-10-31 (afternoon): two hands on the map and the long guns, the 21 Sacraments patch (home PC, Opus, flat on the OpenXR Simulator)

Tefa: *"RT is fine and now please move the map to the right hip, LG to take it out, right hand with RG held to two
hand the map. [praydog's original SH2 profile] please use whatever you need for two handing the map and long weapons
from it and we will separately cretit them for this as well as for UEVR. also please have a look at this [21 Sacraments
Patch 0.5.4], but keep it as something we can take out of the mod if needed ... all yours to do your thing"*

## What praydog's profile does (scripts/camera.lua, plugins/sh2r.dll)
- The pack (jbusfield / LukasBlaster7) is built on this same profile: its `sh2r.dll` is byte-identical to the one in
  praydog's zip, and its `camera.lua` is blanked (0 bytes) so the pack's own scripts drive instead `[measured 2026-10-31]`.
- Map: while the game looks at an item (`pawn.Items.ItemExecutive.ItemContext`), praydog puts it at the middle of the
  hands, X from the left hand to the right, Z against the hands' average up, tipped 45 degrees.
- Long guns: after UEVR has placed the gun on the right controller, it is turned about the right hand so it points at the
  left hand, only while aiming (`IsAimingWeapon`). Shotgun and rifle barrels run along the mesh's +Y.

## What we built: `twohand.cpp` (sh2vr.dll 955f567e, backup `D:/SH2 builds/_backup-2026-10-31-1340-before-sh2vr-deploy`)
- Map: left grip at the RIGHT HIP opens it (holsters.cpp); it is held in the LEFT hand (the pack's right-hand hold moved
  to the left controller, its side mirrored); the right grip held as well = both hands, praydog's way. The right grip no
  longer reaches the game as Next Map while the map is held.
- Long guns: the left grip pressed with the drawn left hand along the barrel (12 to 90 cm from the right hand, within
  50 degrees) = two hands; the gun keeps the angle it had at that moment and then turns about the right hand to follow
  the DRAWN left hand (the arm mesh's hand bone; the controller point while the slide code pins the hand); letting go =
  the right hand alone again. Never the aim stance.
- Every write aims at a target (never adds to the last write), in the post-stereo callback, after UEVR's own placement.
- PROVED on the simulator `[verified-live 2026-10-31, simulator, n=1 each]`:
  - shotgun: the barrel axis +Y (the hands' line 16 deg off +Y at the press); the gun followed the left hand to 0.0 deg
    at two different hand places (35 deg off before the call was fixed); picture: the gun's front in the drawn left hand;
  - rifle: the same, +Y, 0.0 deg; picture;
  - map: one hand, upright and readable in the left hand (the first try mirrored the turn too and showed it upside
    down); both hands: the map between the hands, readable, the hands on its edges (pictures).
- Fixes found on the way: `K2_SetWorldTransform` was refused (our `ue::call` fills parameters in order, so the hit-result
  slot took the teleport flag); the map's executive is `Map_ItemInvestigationExe_BP_C` (the class name starts the full
  name). Not tried: a real headset, the shotgun pump and the rifle bolt while two-handed, the hand-held map with the
  right grip turning pages (now off).

## The save menu (buttons.cpp)
The button layer did not count the save point's menu as a menu (its focus check failed), so X (back) and B were both
dead inside it; Escape on the keyboard got out. Now `IsVisible()` counts too (false while closed `[measured]`), and the
layer switched to MENU buttons in it `[verified-live 2026-10-31, n=1]`. X -> B inside it could not be checked: the UEVR
MCP's pad goes to the game around our layer (its B closed the menu straight away).
⚠️ One stray save was written by an A press at the save point at 13:22 (a new slot, "Toluca Prison Armory 1:22 PM");
the old 2024 manual save is untouched. The save spot stands at the save point: never press A after loading.

## The 21 Sacraments patch 0.5.4.1 (a stutter fix, based on lukeman3000's Ishimura Patch)
- What it is: Ultimate ASI Loader (`version.dll` + `version.ini`) and `scripts/silenttraverse.asi` in
  `SHProto/Binaries/Win64`; it changes how the game queues ray-tracing geometry builds (traversal stutter); F8 toggles
  it (rising tone on, falling off); starts on by itself; refuses any other exe build. Our exe matches its hash.
- Installed with its own `Manage.ps1 -Action Install`, which keeps an ownership record (`SilentTraverse-install.json`);
  verified. Logs to `%LOCALAPPDATA%\SilentTraverseSH2\`. With UEVR it loaded and reported `RT_BUDGET state=ON` `[measured]`.
- **To take it out:** game closed, `powershell -ExecutionPolicy Bypass -File "D:\SH2 builds\addons\21-sacraments-0.5.4.1\Manage.ps1" -Action Uninstall -GamePath "C:\Steam\steamapps\common\SILENT HILL 2"`.
  The package and the original zip are kept in `D:\SH2 builds\addons\`. It is NOT in our mod package or on GitHub
  (Tefa has not asked its author yet).
- Our desk-test key F8 (door B fire) moved to F3 so it never toggles the patch.
- Not measured yet: whether it changes frame times with VR (a before/after on the same spot).

## The window
The game keeps `ScreenMode=Windowed` but rewrites the size to 1920x1080 by itself; set back to 1280x720 again (backup
`GameUserSettings.ini.bak-2026-10-31-1328-was-1920`), and `-ResX/-ResY` did not hold either: the window measured
1936x1119 on every launch today. Still windowed, so the session stays visible.

## 14:20 — the Quest 3 face buttons are not the pad's letters (sh2vr.dll bdf3ed72, backup `_backup-2026-10-31-1422-before-sh2vr-deploy`)
Tefa, after the first wear: *"quest 3 B was back instead of X, and they don't line up ... quest 3 X Y B A xbox in same
order B Y X A"*. The log agrees: Quest X ejected the magazine through the pad's B `[verified-live 2026-10-31, headset]`.
UEVR's VR_SwapControllerInputs is false. So the plan's letters are QUEST letters: `settings::QUEST_X` (pad B) = back in
the menus (it is the game's own Cancel, so it passes as it is), `settings::QUEST_B` (pad X) = the pistol's magazine out
(reload.cpp). Both stay hidden from the game in play. buttons.cpp now logs each face press by its Quest letter. Not
worn yet.

## 14:28 — the map is a toggle, the triggers turn its pages, the long gun goes back on let-go (sh2vr.dll d34b3bf0, backup `_backup-2026-10-31-1428-before-sh2vr-deploy`)
Tefa: *"can LG even get the map out from right hip so it stays in left hand and RG can grip it and let go, left hand
always stays on the map and putting it back in the right hip with LG closes it, so does X, and LT and RT turn pages"*.
- Map: LG at the right hip takes it out, LG at the right hip again puts it away; letting go of LG does nothing; Quest X
  (the game's own back) closes it and the mod notices when the map stops showing (0.5 s); RG held = both hands. While it
  is out the grips never reach the game, LT / RT become the game's LB / RB (previous / next map) and never reach it as
  triggers (so RT cannot fire).
- The grips are read from the pad's LB / RB OR UEVR's own VR grip action (`/actions/default/in/Grip`), because the pad's
  LB read "let go" 33 ms after the map opened while Tefa still held it `[measured 2026-10-31, headset]`. The first log line
  says whether UEVR found that action.
- Two hands, let-go: the gun's place in the right hand (GetRelativeTransform) is read when two hands start and put back
  (K2_SetRelativeTransform) when they end; the log line says "put back", "could NOT be put back" or "was never read".
Built and installed; not run yet `[compile-verified 2026-10-31]`.

## 14:40 — second wear read; the map follows the game, the pouch from the headset, the hands follow the long gun (sh2vr.dll 7e65f601, built, NOT installed: the game was still open)
Tefa's second wear: the map works out of the hip and with both hands and LT / RT turn pages, but putting it back "jumps
back to the right hand and then LG and RG flip the pages again". Log: put away at 14:32:27.861, out again 0.4 s later
(a second grip press), so our state and the game's crossed `[measured 2026-10-31, headset]`. Quest B ejects the magazine
`[verified-live]`. The pouch: no hit anywhere at the left hip. Two hands: the let-go puts the gun back `[verified-live]`
("its place in the right hand put back"), but the right hand does not turn with the gun and the left hand slides along
it. The meds buzz but show nothing. RT on an empty gun gives no click.
- Map: a grip at the hip within 1 s of a toggle is ignored; a map on screen that the hand does not hold is taken into the
  left hand, so the grips can never fall through to the game's page buttons.
- Pouch: measured like the holsters (headset tracking space, yaw only) instead of the game's camera.
- Every grip read (rack, reload, holsters, twohand) goes through input::grip, the pad or UEVR's VR grip action (found).
- Two hands: the gun aims at the drawn left hand's starting place carried by the left controller; the drawn left hand
  stays on the gun's line at its starting distance (ik override, off again on let-go); the drawn right hand turns by the
  gun's whole turn (new ik::override_right_turn). Settings TWO_HAND_TURN_RIGHT / TWO_HAND_LOCK_LEFT.
Still to do: the pump and bolt and the fire pace for the long guns, the dry click, the meds shown in the hand, why the
slide reads 40-93 cm from the left hand.

## 14:46–14:55 — third wear (sh2vr.dll d79fcd06, backup `_backup-2026-10-31-1445-before-sh2vr-deploy`), saved here to pick up later
Tefa: *"the right hand now followed the gun - i think - i honestly forgot :D the ammo pouch works for the handgun when
taking out a mag, but the mag looks like the actual ammo box from the game and not the pistol mag. shotgun and rifle
still don't work manual reloads and when empty, ammo pouch does not seem to exist for them. fire rate is still really
fast ... and now holsters don't work ... medicine also didn't work."*
- WORKS `[verified-live 2026-10-31, headset]`: the pistol's new magazine from the pouch at the left hip (three in the log:
  10, 2 and 10 rounds taken from the carried ammo); the flashlight on LT; the right hand turning with the long gun
  (Tefa thinks so, not sure).
- The magazine in the hand is the game's ammo box model, not a pistol magazine (`settings::HAND_MAG_MESH`).
- Holsters: every draw worked until the shotgun ran empty ("0 in the gun of 6" at 14:50:08); from 14:50:05 on the game
  refused every RequestEquipWeapon, even with the hands empty after the shotgun went back `[measured]`. Most likely the
  empty shotgun left the game in its own reload / out-of-ammo state, which blocks weapon changes `[hypothesis]`: look at
  the game's own reload on the long guns first (it plays by itself, the pump / bolt and the rounds-by-hand are not built
  in C++ yet; the pack's shotgun_reload.lua and rifle_reload.lua do not stand down and may need the aim stance).
- Fire rate unchanged: not worked on yet. Medicine: the buzz and the log lines, nothing in the hand, no RT use seen.
