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
