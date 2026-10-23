# 2026-10-23 · Step 2 design: hands and attachments in C++

Fable, 2026-10-23 late. The design everything in step 2 builds on; the line-for-line port that follows is Opus work.
Tefa the same evening: praydog's work may be assumed usable (Tefa will ask him), so his hooks can be folded in later.

## What the Lua does, measured from the scripts

| Lua file | Lines | Job | UEVR doors it uses (through `uevr_utils.lua` / `controllers.lua`) |
| --- | --- | --- | --- |
| `hands.lua` | 1,526 | creates the two hand meshes, attaches them to the controllers, hides/shows them, handles grip/trigger input, applies the per-weapon hand offsets | create component by class, UObjectHook motion-controller attach, relative transform set, FOV fix on materials, input actions |
| `animation.lua` | 1,056 | a PoseableMeshComponent copied from the game's skeletal hand, finger poses by bone rotation, lerped animation between poses | PoseableMesh `SetBoneRotationByName` / `GetBoneRotationByName`, `CopyPoseFromSkeletalComponent`, material copy |
| `hands_animation.lua` | 250 | which pose each hand is in (grip, trigger, holding weapon X) | none directly |
| `attachments.lua` | 2,664 | the weapon in the hand: per-weapon id → location/rotation/scale offsets, left-hand (two-handed) offsets, melee rotation, sights, laser/scope hooks, the config UI for all of it | relative transform set on the weapon mesh, socket names, scope/laser modules |
| `accessories.lua` | 766 | things the off-hand holds on a weapon (flashlight, map) with montage-timed markers | montage state, attach hand to accessory |

Doors checked against the C++ API (`include/uevr/API.hpp`) 2026-10-23: all present: `find_uobject`, `spawn_object`,
`add_component_by_class`, `UObjectHook` motion-controller state with offsets, `process_event` (function calls by
name: `ue.cpp`), property read/write by name, VR poses/actions/joysticks/haptics, pre/post engine tick, XInput.
What the Lua binding did implicitly, filling a function's parameters by reflection, `src/ue.cpp` now does.

## The shared state (the decision)

One record per hand, owned by `hands`, read by everything else:

```
struct Hand {            // src/hands.h
    Side side;                       // LEFT / RIGHT
    API::UObject* component;         // our PoseableMeshComponent (null until created)
    API::UObject* controller_state;  // the UObjectHook motion-controller state it hangs on
    std::string pose;                // the pose name in hands_parameters.json "animations" (grip_left_weapon ...)
    std::string attachment;          // the attachment id in hand ("pistol", "board" ...), "" when empty
    bool hidden, animating_from_mesh, holding;
};
```

- **`hands` owns the hand meshes and their pose.** `attachments` tells it `hold(side, attachment_id)` and `release(side)`;
  `hands` picks the pose from `hands_parameters.json` (`animations.Shared.poses` and `positions`) and drives
  `animation` to lerp the fingers.
- **`attachments` owns the weapon's placement.** Each engine tick it finds the current weapon mesh, looks up its
  offsets (`attachments_parameters.json` `attachmentOffsets[]` by `id` / `child` / `any_child`), and sets the mesh's
  relative transform under the right hand; the left-hand offsets go to `hands::hold(LEFT, ...)`.
- **`animation` is a library, not a module:** bone maths on a PoseableMesh, no state of its own beyond the lerps.
- The Lua's callback web (`registerOnDestroyCallback`, `executeGripAttachmentChanged`, priority-result callbacks)
  becomes direct calls in one direction: attachments → hands → animation. No module calls back up.
- Settings stay in the two JSON files; `params::file("hands_parameters.json")` is the only reader. Numbers that
  were literals in the Lua go into `settings.h`.

## Port order inside step 2, each proven before the next

1. `animation.cpp`: create the poseable copy of a skeletal mesh, read a bone rotation, set it, lerp. Proof: the
   log shows a bone name and a rotation that changes when asked, with the Lua still running.
2. `hands.cpp`: create both hands from `hands_parameters.json` `profiles`, attach to the controllers, hide/show, the
   FOV fix. Proof: Tefa sees the C++ hands with `hands.lua` in `PORTED`; the game's own hands hidden as before.
3. Input to pose: grip/trigger → pose names (`hands_animation.lua`). Proof: fingers curl on grip in the headset.
4. `attachments.cpp`: the weapon offsets, right hand first, then the left-hand offsets, melee rotation, sights.
   Proof: each weapon sits where it did under the Lua (same numbers in the log; Tefa's eye for the rest).
5. `accessories.cpp` last; scope/laser belong with the reticule module later.

## Risks written down now

- UE5 doubles: every FVector/FRotator/FTransform crossing into the game is doubles (`ue.h` sizes are asserted).
- PoseableMesh bone calls take `FName`; `API::FName` constructs from a string (checked).
- `uevr_utils.fixMeshFOV` edits material scalar parameters on dynamic instances; needs `CreateAndSetMaterialInstanceDynamicFromMaterial`
  through `ue::call` with an object return: supported.
- The hands profile may spawn the hand mesh from an asset path (`find_instance_of`); `find_uobject` covers it if
  the asset is loaded, otherwise an asset load is needed (UEVR's Lua had the same limit).

## Correction after the first headset look (2026-10-23 23:55, Tefa in the headset)

Tefa saw **two pairs of hands**: the full IK arms and our new elbow-to-fingers pair; with the game out of focus
(paused) only the elbow pair stayed. The cause: jbusfield's menu setting "Hands Type" (`data/sh2_config.json`
`hands_type`) is **3 = IK Arms** on Tefa's PC (1 None, 2 Forearms, 3 IK Arms, `main.lua` `HandsType`), so
`hands.lua` never made hands in Tefa's game; `ik.lua` makes the arms, and hides them on pause, which ours do not yet.
`[verified-live 2026-10-23, n=1]` for the C++ forearm hands themselves: both made, attached to the controllers,
90 bones set from the profile, nothing refused, they move with the controllers, the gun sits in the right hand.

Fixed (installed at the next game start): `hands.cpp` makes the forearm hands only when `hands_type` is 2, and
takes them down if the menu changes away from it. **The arms Tefa uses are `ik.lua` (2,576 lines) with
`body_yaw.lua` (535), `collision.lua` (662) and `config/ik_config_dev.lua` (1,310 of settings): the core of step 2**,
before attachments. The `Hand` record stays; it holds the IK arms' mesh when `hands_type` is 3.

Tefa's notes from the same look, for the board: weapons must cast shadows under the flashlight (they do not now);
fix the player's shadow if at all possible, with the head showing and the hands looking right. `main.lua` already
says why the shadow is hard: *"These hide the arms but not the shadows and flashlight is still blocked even with
arms hidden"* (the IK-arms mesh-created callback).
