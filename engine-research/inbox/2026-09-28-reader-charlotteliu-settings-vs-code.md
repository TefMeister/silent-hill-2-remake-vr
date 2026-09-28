# CharlotteLiu's mod: what is settings, what is their own code, and where it disagrees with jbusfield

**From:** background reader, 2026-09-28, home PC, static only. Nothing launched, nothing attached,
nothing copied into our files. **Speaks to:** dossier §8e (the bone chain and tuned numbers), §9 (the
game-animation idea), and the board row "study CharlotteLiu's mod".

**What was read:** the unpacked copy `_study/charlotteliu/` in Tefa's "learn from, do not copy" folder
(archive `SHProto Win64 Shipping 421 3.3 2026-09-23T15-50Z …zip`), compared file by file against
`_study/jbusfield/`. Every "they say X" below means **their files say X**. None of it is checked against
the running game; that is the parallel live check's job.

## ⭐ The headline for the live check: NO bone-name disagreement

Every bone and socket name CharlotteLiu uses is also in jbusfield's files. Extracted all `*_bn` /
`*_socket` names from both trees and diffed them: **zero bone names differ** `[measured 2026-09-28]`
(this measures their text, not the game). The only new names are their own invented attachment ids
(`uevr_attachments_…_accessory_item_socket`) and `DoorLatchSocket`, `magazine_socket`, `no_spawn_socket`.

So on the points dossier §8e lists, the two profiles **agree** `[measured 2026-09-28, their files]`:

| §8e item | CharlotteLiu |
| --- | --- |
| arm chain `clavicle_01 → upperarm → lowerarm → hand_{l,r}_bn` | same, `ik_parameters.json` is **byte-identical** to jbusfield's |
| `spine_05_bn` | same |
| head hidden with `neck_02_bn` | same (`main.lua:550`) |
| `hand_l_socket` / `hand_r_socket` | same (in `hands_parameters.json`) |
| weapons attach to the **bone** `hand_r_bn` | same (`main.lua:764`, `weaponAttachSocket = "hand_r_bn"`) |
| flashlight attaches to `hand_l_bn` | same (`main.lua:765`) — §8e does not list this one |

⚠️ Agreement between two profiles is **not** proof: CharlotteLiu built on jbusfield's config, so a stale
name would be stale in both. It only means there is no second opinion to break a tie.

### Extra names the live check should also look for (not in §8e, used by both profiles)

From the unchanged `ik_parameters.json` and `config_default.json` `[measured 2026-09-28, their files]`:
- **Forearm twist bones:** `r_sleeve_master` (pitch 0.257, roll 0.485), `r_wrist_Offset`,
  `l_wrist_Offset` (pitch 0.273, roll 0.485). Note: **no `_bn` suffix** — these look like coat/rig
  helper bones, so they are the likeliest to differ between game builds `[hypothesis]`.
- `lowerarm_twist_01_{l,r}_bn`, `lowerarm_twist_02_{l,r}_bn`, the `*_01_{l,r}_meta_bn` finger roots,
  and the finger bones `{thumb,index,middle,ring,pinky}_0{1,2,3}_{l,r}_bn`.
- `head_bn` (head bone for the camera), `Root` (root bone name).

### ⭐ A cross-check the live bone dump can do for free

CharlotteLiu's `pawn_config_dev.json` saved the upper-arm pickers as list positions, **57** (right) and
**127** (left); jbusfield's has the default 1. The list is built by walking `GetBoneName(0 … n-1)` in
skeleton order and is 1-based (`uevr_utils.lua:3569`), and their resolved names are `upperarm_r_bn` /
`upperarm_l_bn` (`pawn_parameters.json`). So **their game's skeleton had `upperarm_r_bn` at bone index
56 and `upperarm_l_bn` at 126** `[inferred-static 2026-09-28]`. The existing probe
`dev-archive/probes/sh2_bone_dump.lua` prints `bone <index>` 0-based: if 56 and 126 land on those two
names, both profiles' skeleton knowledge matches our build. If they don't, the skeleton has changed (or
the picker was saved against a different mesh; that is the caveat).

## Tuned numbers: what CharlotteLiu changed

All `[measured 2026-09-28, their files]`. Units are Unreal centimetres and degrees.

- **IK arms: nothing changed.** End-bone offsets, arm mesh offset `(-13.287, 0, -18.1721)` rot
  `(0, -90, 0)`, shoulder-width scale `1.74`, wrist twist `0.35` / max 75°: all identical to §8e.
- **Pistol grip re-tuned slightly:** location `(-8.9, -1.6, -5.8)` → `(-9.1, -1.5, -6.0)`, rotation
  `(-180, -25.2, 0)` → `(-179.9, -25.0, 0)`. **Iron pipe:** rotation yaw `0.2` → `-0.8`. Shotgun,
  rifle, chainsaw and the rest unchanged.
- **Ammo held in hand re-tuned completely** (handgun, shotgun, rifle ammo): e.g. handgun ammo
  `(-15.7, -1.4, -3.4)` → `(9.3, 1.5, 1.6)`. The sign flip suggests a different parent, not a nudge
  `[hypothesis]`.
- **One bad saved value:** a new entry `Actor_Handgun_02_A_Rig` has location `(-21736, 37540, 396)`,
  which is a **world** position saved as an offset. Their own comment (`shotgun_reload.lua:998-1000`)
  describes the symptom: with IK arms on, `hand_l_bn` "measured can be very far from the controller
  (spawn ≈ -21600)", so they attach reload props **to the motion controller**, not the hand bone
  `[reported, their comments]`. Worth remembering: their IK hand and their controller did not agree.
- **Hand-pose finger rotation** in `hands_creator_config.json`: pitch/roll/yaw `3.64 / 0.71 / 54.59` →
  `0 / 15.75 / 0`. This file looks like editor state, so low weight `[hypothesis]`.
- **Twelve new hand poses** in `hands_parameters.json`: `pull_lock`, `pull_slide`, `push_wardrobe`,
  `drink_potion`, `inject_syringe`, `ammo_box`, `pistol_mag`, `rifle/shotgun_shell_insert`,
  `closed_hands` (finger angles per pose).
- **UEVR settings (`config.txt`):** Rendering Method `0` (Native Stereo), `VR_NativeStereoFix=true`,
  `VR_EnableCustomZNear=true` with `VR_CustomZNear=0.01`, `UI_FollowView=true`. jbusfield's instead
  carries AFW lines (`VR_AFW_FramewarpMode=3`, `UltraResponsive`, `FixObjectMotionRange=3`,
  `FixObjectMotionVector`, `FixMovingObjectBrightnessFlickering`). Useful as two known-good starting
  points, one per supported rendering mode `[reported]` (known-good to them, not to us).
- **`sh2_config.json`:** ray tracing on, snap turn on (30°), reticule hidden, two-handed weapons on,
  grab laser on.
- **Interaction numbers** (their hand-tuned values, `unlock_door.lua:193-325`, `unlock_item_door.lua`,
  `remote_grab.lua`): grab distance 10-12 cm, latch slide done at 8 cm (15 cm for the `_01_B` and
  `_02_A` models), wardrobe push anchors ±65 cm at 150 cm height with a 40×40 cm grab face, cart bar
  from x=8 to x=79 at y=17, z=16.5; key turn 90° with 3× sensitivity; remote-grab cone 14°,
  30-280 cm, 3 m scan range; recoil caps 35° one hand, 55° two hands.

## SH2 object paths and names they found that jbusfield did not

All `[measured 2026-09-28]` as text in their files; none checked in-game. **These are facts about the
game, free to use as leads once the game confirms them.**

- **Classes:** `/Script/SHProto.SHItem`, `SHItemWeaponRanged`, `SHItemWeaponMelee`, `SHRangedCmbSubcomp`,
  `SHWeaponManageCmbSubcomp`, `SHAnimMontagePlayer`, `SHDoorAttachment`, `SHSlidingDoor` (with
  `SHAkSlidingDoorComponent`), `AnimNotify_UnequipWeapon`, `SHPushable` / `SHPushableAudio`, `SHAk_Lever`.
- **Interaction blueprints:** `/Game/Game/Gameplay/Interactions/PushableObjects/Pushable_Base_ABP.Pushable_Base_ABP_C`,
  `…/PushableObjects/PushNClimbDesiredSpot_BP.PushNClimbDesiredSpot_BP_C` (the cart),
  `…/Others/Switches/InteractionSwitch_Base_BP.InteractionSwitch_Base_BP_C`,
  `…/Others/LightSwitch/PrisonLightSwitch_BP.PrisonLightSwitch_BP_C`,
  `…/Items/ItemBase_BP.ItemBase_BP_C`, and `InteractiveDrawer_Base_ABP_C` (path elided in their file).
  Door locks found by name prefix `DA_SingleDoor_Lock_` / `DA_DoubleDoor_Lock_`; latches by static-mesh
  name `DoorLatchBar` (models `_01_A`, `_01_B`, `_02_A`) and keys-in-doors by `KeyFlat`.
- ⭐ **For Tefa's game-animation idea (§9):** the push animation blueprint `Pushable_Base_ABP_C` and
  the cart's `PushNClimbDesiredSpot_BP_C` are the two objects to watch when triggering a push live.
- **Animations:** the manual handgun reload is an `AnimSequence`,
  `/Game/Game/Characters/Humans/JamesSunderland/Animation/Combat/Handgun/RecoilAndReload/James_Combat_Recoils_Combat_Handgun_Reload`,
  played in `DefaultSlot`; plus `…/Combat/SyncedAnimations/PH/GrabFront/James_PH_Grab`, and the
  shotgun/rifle reload montages.
- **Item meshes:** `/Game/Game/Art/Items/…` for `HandgunAmmoClosed_01_A`, `ShotgunAmmo_01_B`,
  `ShotgunShell_01_A`, `SniperRifleAmmo_01_A`, `SniperRifle_Shell_01_A`, `HealthDrink_01_B`,
  `Syringe_03_C`; weapon mesh names `Handgun_02_A_Rig`, `shotgun_05_A_rig`,
  `SHItemBase_Sniper_Rifle_01_Skeletal`, `JamesPipe_01_A_Rig`.
- **Sounds (Wwise):** `/Game/WWiseAudio/Events/Characters/James/James_Weapons/{Pistol,Shotgun,Rifle}/…`
  (reload, dry fire, pump, bolt), `…/Enviro/Doors/Play_Key_Unlock`, `…/Item/Play_Item_PickUp`,
  `Play_Sliding_Bolt_Open`, `Play_Event_FoldingStaircase_Lever`.
- **Hard-coded memory offsets** (build-fragile, theirs, **not** to be relied on without re-deriving):
  melee damage-type slot `0x6D8` on the weapon (same as jbusfield), anim-notify `Notify` at `0x10` and
  `MeshContext` at `0x30`, `PREPARE_ANIM_DATA_OFF = 0xC68` on ranged weapons, and reserve ammo via
  `pawn + 0x6C0` with 12-byte entries, which **their own comments mark disabled on 2026-09-22**.

## What is their OWN CODE (reimplement, never copy)

No licence or copyright text anywhere in the archive `[measured 2026-09-28, grep]`, so the default
applies: all of it is theirs.

- **All the Lua.** 87 files, 94,159 lines. Roughly half is jbusfield's shared framework (`scripts/libs/`),
  which CharlotteLiu has **modified in 33 of its 56 files** (e.g. `ik.lua` 2,576 → 2,657 lines, `attachments.lua`
  +5,497 changed lines). The rest is theirs: `unlock_door.lua` (5,487), `rifle_reload.lua` (4,589),
  `shotgun_reload.lua` (3,910), `pistol_reload.lua` (3,368), `unlock_item_door.lua` (3,002),
  `UEVR_05_haptics.lua` (2,684), `remote_grab.lua` (2,045), gestures, recoil, heal, two-handed weapons,
  keyboard/mouse input. Comments are in Chinese, dated 2026-07-29 to 2026-09-22.
- **`SH2.dll`** (their own UEVR plugin, 25 KB, v2.0.0, strings only, not decompiled `[inferred-static]`):
  **replaces** jbusfield's `sh2r.dll`. It does three things: blocks the game's automatic weapon
  put-away (a hook on the unequip path, toggled from Lua), reads/writes reserve ammo by raw offsets, and
  sets clip ammo through the game's `SetClipAmmo`. It does **not** hook melee; their `melee.lua` instead
  fakes the game's melee anim-notify from Lua by writing raw pointers.
- **`bhpatics_bridge_UE.dll`, `TrueGear_bridge_UE.dll`** (haptic vests over a websocket) and
  **`sim_input_bridge_UE.dll`** (sends keyboard input via `SendInput`). Not relevant to our arms path.
- **The ideas are fair to learn from, the code is not.** Two ideas worth keeping in mind, in our own
  words: (1) while two hands hold a long gun, they take the gun off the hand bone and drive it from both
  controllers, so hand and gun do not pull on each other; (2) held props follow the controller, not the
  IK hand, because their IK hand drifted.

## What is SETTINGS / data (free to learn from)

Everything under `data/` (JSON numbers, bone and object names, pose angles, animation lists),
`config.txt` (UEVR settings), the named constants listed above, and the object paths. These are either
facts about the game or tuning values; we re-measure them in our own tools before trusting them.

## Not established

- Nothing was run. No name above has been seen in the live game by this reader.
- `SH2.dll` was read by strings only. The "hook on the unequip path" is inferred from its log strings.
- The 56 / 126 bone-index prediction assumes the picker was saved against `Pawn.Mesh`.
- Most of the 94k lines were not read line by line; this pass extracted names and numbers and read the
  places those came from.
