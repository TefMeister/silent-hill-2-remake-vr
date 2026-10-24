# 2026-10-24 — the pistol reload by hand, the RE8 recipe, built in C++ (unworn)

/lm, home PC, Fable, 16:40–17:00. Tefa: *"reload system has to [be] something like we are doing in re8 right now ...
build the reload system in C++ as close to re8 as possible"*. The design is `2026-10-24b-manual-reloads-re8-recipe-design.md`;
this is step 2 and 3 of its order (the pistol slide with the LT step, the pistol magazine), with the doors of step 1
folded in. Installed with the trigger build (`2026-10-24f-...`). **The gestures have not been worn**: the simulator's
controllers stand still and its pad never reaches the plugin, so only the state logic ran flat (below).

## What is built (`dev-archive/native/src/`)

| file | does | from |
| --- | --- | --- |
| `gun.cpp` | the equipped pistol, its mesh, a socket's world pose, the two controllers' world poses, the headset (the player camera), a buzz | new |
| `input.cpp` | the pad as the plugin's XInput hook last saw it: held / pressed (edge per frame) / triggers | new |
| `ammo.cpp` | now writes the rounds (+0xBB4), reads `WeaponClipSize`, counts and takes the carried ammo through the pawn's `Items` component (`GetItemCount` / `RemoveItem` on the weapon's `WeaponAmmoItem` row, `HandgunAmmo` when unnamed), sets the gun's empty pose (`AnimScriptInstance.SetOutOfAmmo`) | Lukas's doors `[reported]` |
| `rack.cpp` | the slide: with the gun empty (or a rack owed) the left grip near `slide_joint` (22 cm) puts the hand on; LT held arms the pull, measured along `slide_joint → FX_muzzle`; 12 cm = fully back (buzz); LT let go at the back, or the grip let go, sends it home and the trigger works again; the left IK hand rides the slide socket (the pack's slide-hand fit, roll 90), moved back with the pull | Visceral RE8 `rack.cpp`, cut to the slide |
| `reload.cpp` | right B: the magazine out (its rounds remembered, the gun reads 0, the magazine bones hidden); left grip at the ammo spot (from the headset: 20 cm left, 35 down, 10 forward, the pack's holster numbers) takes a magazine (the game's `HandgunAmmoClosed_01_A` mesh on the left controller, the pack's fit); within 18 cm of `magazine_joint` it goes in: the old rounds back, topped up from the carried ammo (taken from the inventory); a magazine into an EMPTY gun owes a rack; letting the grip go drops it (its rounds wait) | Visceral RE8 `reload.cpp`, the magazine states only |
| `ghosthand.cpp` | the Ghost Hand BeG0nE recorder: controllers from UEVR's room poses, the game's hands and the slide carried into the room through the two controllers, gestures slide / mag with start / done / fail / cancel; writes `%LOCALAPPDATA%\BeG0nE\ghost-hand\silent-hill-2-remake-vr_<stamp>.ghl` while the headset is live | Visceral RE8 `ghosthand.cpp` |
| `ik.cpp` | `override_left(on, pos, rot)`: the left arm solves to that instead of the controller; `hand_world(side)`: the drawn hand | new door |
| `fire.cpp` | the trigger waits while a rack is owed or the magazine is out; the game's reload button (X) is kept out while a pistol is managed | |
| `main.lua` | the pack's pistol-reload block stands down (`SH2VR_PORTED.reload`), its trigger-to-aim rule too (`.fire`) | PATCHED |

## Seen flat (the simulator playing the headset)

- The ghost-hand recording opens the moment the headset is on and closes with the game `[verified-live 2026-10-24, n=1]`.
- Ten lowered shots emptied the pistol through the trigger path (F7, a pretend pull through the same code): rounds
  10 → 0, each shot counted and its shell ejected; at 0 `rack: the pistol is empty: its slide can be racked`
  `[verified-live 2026-10-24, n=1, flat]`. The trigger with no rounds fires nothing (the weapon's own empty path).
- Nothing of the gestures: no controller moved.

## In the headset (what to look for, in order)

1. Pistol out, LOWERED, walk and run normally; pull the trigger: a shot, no aim stance, the character keeps walking.
   Right grip held: the aim stance as before, the game's own trigger.
2. Empty the gun. Left grip with the left hand on the slide: a buzz, the left hand sits on the slide. Hold LT, pull
   back along the barrel: a second buzz at the back; let LT go: the slide goes home (the game draws it closed).
3. Right B: the magazine drops out (the gun's magazine disappears). Left hand to the left hip, grip: a magazine in the
   hand. Bring it to the gun's well: it goes in (buzz), the ammo count in the HUD may lag until the next shot (Lukas
   saw the same). If the gun was empty, the slide must be racked (step 2) before the trigger works.
4. The log says where each step stands (`rack: ...`, `reload: ...`), and the BeG0nE rider files the ghost-hand log.

## Not established

- Every distance and fit (22 cm grab, 12 cm travel, the hip spot, the magazine on the controller, the hand on the
  slide): the pack's numbers for the pack's hands; ours will need the headset to tune.
- The hand-magazine mesh may not be loaded when first needed (`find_uobject` only finds loaded assets): the log says
  "NOT LOADED" and the gesture works without the picture; then it needs loading through the asset registry as the pack
  does (`AssetRegistryHelpers.LoadAsset_Blocking`).
- Whether the IK override and the Lua pack's attachments fight for the left hand while it is on the slide (the pack's
  accessory "stick" is off while its pistol block stands down; the two-handed grip dock of the pack is not).
- The pump and the bolt (shotgun, rifle) are not started: `lock.cpp` (the prepare-animation block) comes first.
