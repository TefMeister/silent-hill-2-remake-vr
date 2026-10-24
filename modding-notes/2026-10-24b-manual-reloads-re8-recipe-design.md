# 2026-10-24 · Manual reloads in C++, the RE8 recipe: the design

/pd, home PC, no game running. Nothing here has been run. Opus.

Tefa, 2026-10-24 00:40: *"racking, pumping and rifle bolt all have to follow the same recipe as RE8 - LG puts the hand
on the weapon part, LT makes it rackable"*. This note maps Visceral RE8's reload modules onto what LukasBlaster7's
scripts already proved about Silent Hill 2's guns, and sets the order of the port. It is written before any code
because the four Lua files and the pistol block in `main.lua` (about 4,000 lines together) are tied to each other
and to jbusfield's accessories and attachments; a blind port of all of it in one pass would be guesswork.

## 1. The recipe (Visceral RE8, `visceral-re8-vr/dev-archive/native/src/rack.h`, `reload.h`, `breech.h`)

| Gesture | RE8 | Lukas's SH2 scripts today |
| --- | --- | --- |
| Slide / bolt | left grip with the left hand on the gun line puts the hand ON the part; **LT makes the pull live**; pull back along the gun; letting go of the left grip once fully back sends it forward | left grip near the bone + pull back; no LT step |
| Pump | left grip + LT on the barrel line arms one cycle; pull back; let go of LT, the handle returns | left grip near `bone_forearm` + pull back (~85 %) |
| Magazine | right B drops it (its rounds stay with it); left grip in the hip ammo spot puts one in the left hand; bring it to the gun; an empty gun then needs a rack | hip pocket + left grip, refill on release near the magazine |
| Shells / rounds | one per grab from the ammo spot, brought to the loading port | the same (`shotgun_reload.lua`, `rifle_reload.lua`) |
| The left hand | held where it took hold, relative to the gun hand, and slides back with the pull: rack rewrites the left IK target right before the left-arm solve | jbusfield's accessories "stick" the offhand to a socket |

So the change for Tefa is the LT step on slide, pump and bolt, and the hand following the part. The magazine and shell
flows are already the same shape.

## 2. The game's doors, per gun (all from Lukas's scripts, which say "MCP-confirmed") `[reported]`

| | Pistol `WeaponPistol_BP_C` | Shotgun `WeaponShotgun_Alt_D_BP_C` | Rifle `WeaponRifle_BP_C` |
| --- | --- | --- | --- |
| Rounds in the gun | int at weapon `+0xBB4` (`HasAnyAmmoInClip` reads it); the `WeaponsAmmoCounts` map mirrors it but writing the map alone does not flip `HasAnyAmmo` | `+0xBB4` | `+0xBB4`, clip size 4 |
| Ammo carried | `HandgunAmmo` | `ShotgunAmmo` | `RifleAmmo` (`WeaponAmmoItem.RowName`) |
| Moving part | `slide_joint`, grab within 22, travel 12 | `bone_forearm`, grab within 13.3, travel 8 | `bone_lock`, grab within 16, travel 10 |
| Loading spot | `magazine_joint` / `magazine_socket_joint`, within 18 | near `bone_forearm`, within 16 | `FX_Chamber`, within 18 |
| Refill | `AnimNotify_ReloadFillWeaponClip` (Lukas pulses X); empty via `SetOutOfAmmo` | +1 at `+0xBB4`, spend 1 ammo; the game's full-tube `RangedReload` blocked when empty | +1 at `+0xBB4`, spend 1 |
| After a shot | — | strip the two soft references at `+0xC38` / `+0xC68` (48 bytes) and block `OnRangedSingleActionPrepareEnd`, so `IsFireReady` stays false and the pump does not cycle by itself; the pump forces `IsFireReady` true through its return value | the same chain: `OnFirePerformed` → `OnShellEjected` → `UpperBodyAnim_WeaponPrepare` → `AnimNotify_SingleActionPrepareReady` → `OnRangedSingleActionPrepareEnd` |
| Hand-held part | `HandgunAmmoClosed_01_A` | `ShotgunShell_01_A` | `SniperRifle_Shell_01_A` |

Distances are in game units (centimetres). Every offset above belongs to this game build; if the game updates they move.

## 3. The C++ modules (the RE8 shape, one feature per file)

| File | Takes over | Does |
| --- | --- | --- |
| `ammo.cpp` | the `+0xBB4` writes in all five places | read / write the rounds in the gun, spend from the carried ammo, through one checked door (offset in `settings.h`, refused with one log line when the weapon class is not one of the three) |
| `lock.cpp` | the shared PrepareEnd / IsFireReady hooks in `shotgun_pump.lua` | after a shot on the pump / bolt guns: strip and later restore the soft references, block the prepare end, answer `IsFireReady`; the same UFunction pre / post hook pattern `fire.cpp` already uses |
| `rack.cpp` | `shotgun_pump.lua`, `rifle_bolt.lua`, the pistol slide in `main.lua` | the RE8 gesture: LG docks the hand on the part, LT makes it live, travel along the gun from the part's socket and the muzzle; writes the left IK target before the solve (needs a small hook in `ik.cpp`: a "left target override" the rack fills) |
| `reload.cpp` | `shotgun_reload.lua`, `rifle_reload.lua`, the pistol magazine in `main.lua` | B drops the magazine, the hip ammo spot, the item in the left hand, the loading spot, the "needs a rack" flag |
| `ghosthand.cpp` | new | the Ghost Hand BeG0nE recorder (the rule for any mod with hand gestures), moved over from Visceral RE8's 123-line file: rack / pump / bolt / mag / shell start, done, fail, cancel |
| input | the `registerOnPreInputGetStateCallback` blocks | `on_xinput_get_state` in the plugin: eat RT while the gun is locked, read LG / LT / B; the UEVR C++ API has the same callback the Lua uses `[inferred-static 2026-10-24, Plugin.hpp]` |

Each Lua module stands down when the build script lists it in `PORTED`, as hands and IK did: Lukas's four files and
the pistol block get a one-line `if SH2VR_PORTED and SH2VR_PORTED.<name> then return end` at their entry.

## 4. Order (one piece per headset test, never a big-bang swap)

1. `ammo.cpp` + `lock.cpp` + `ghosthand.cpp`: no gesture yet, the Lua still racks; proves the doors from C++
   (log: rounds in the gun before / after each shot, the lock taking and letting go).
2. The **pistol slide** in `rack.cpp` with the LT step (the gun Tefa uses most), the Lua slide stands down.
3. The pistol magazine in `reload.cpp`.
4. The shotgun pump, then its shells.
5. The rifle bolt, then its rounds.

## 5. Not established (each needs the game)

- Every offset and name in §2 is Lukas's, on the build he had `[reported]`; the first C++ run must log the rounds it
  reads at `+0xBB4` next to the game's own `HasAnyAmmoInClip` and refuse to write if the two disagree.
- Whether eating RT in `on_xinput_get_state` is enough on the C++ side, or whether the Lua pack's remap (priority 5 / 10
  in `main.lua`) still sees the raw press first `[hypothesis]`.
- Where the IK left-hand target can be overridden in `ik.cpp` without a frame of lag `[hypothesis]`: RE8 writes it
  right before its own solve, and our `ik.cpp` solves in the same pre-engine tick, so the order inside that tick is
  what matters.
