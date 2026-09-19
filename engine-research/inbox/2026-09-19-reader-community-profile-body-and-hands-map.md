# How the existing community UEVR profile makes the body and hands work — a map for a native rewrite

Author: background reader for the /lm session, 2026-09-19, dev PC `DESKTOP-V8GTSIR`.
Method: **static reading of the extracted profile only.** The game was never launched; nothing here
was observed running. Every claim about behaviour is therefore `[inferred-static]` at best — it is
what the code says it does, not what it was seen doing.

Subject: the extracted `shproto` UEVR profile at
`C:\Users\Tefa\AppData\Local\Temp\...\scratchpad\sh2\shproto\` — 89 files: 86 Lua scripts, 18 JSON
data files, 2 native DLLs.

This is a **map, not a plan.** It deliberately proposes nothing.

---

## ⚠️ First, two corrections to the working assumptions

**1. The profile is 49,718 lines of Lua, not ~6,200.** `[measured 2026-09-19, wc -l]`
The five files named in the brief (`ik.lua` 2,576 · `hands.lua` 1,526 · `hands_animation.lua` 250 ·
`pawn.lua` 500 · `main.lua` 1,345) total 6,197 — but they sit on top of `uevr_utils.lua` (4,326),
`hands_config.lua` (3,189), `attachments.lua` (2,664), `remap.lua` (2,516), `input.lua` (1,665) and
about seventy more. Scale matters for judging what a rewrite is replacing.

**2. Most of that is a general-purpose framework, not Silent Hill knowledge.** `[inferred-static 2026-09-19]`
The `scripts/libs/` tree contains live references to `/Script/AtomicHeart.AHGameplayStatics`,
`/Script/Indiana.HUDWidget`, `/Script/GunfireRuntime.RangedWeapon`, `/Script/Phoenix.PhoenixCameraStackManager`
and `/Script/GameBase.*` — classes from **other games entirely**. The libs are a shared community
VR-hands framework that has been carried between UEVR projects; only `main.lua`, `melee.lua`,
`camera.lua`, the `data/*.json` and `plugins/sh2r.dll` are Silent Hill 2.

**This split is the single most useful thing on this page.** The framework is the part with the
least value to copy (it is generic, indirected, and mostly exists to work around Lua). The
`data/*.json` and `main.lua` object paths are the part that took someone weeks to find.

---

## 1. The outline: confirmed, with three corrections

The stated outline was: build a `PoseableMeshComponent` from the game's skeletal mesh, copy the pose
onto it each frame with `CopyPoseFromSkeletalComponent`, then overwrite bones with
`SetBoneTransformByName` to drive arm IK.

**That is right in shape and wrong in three details.**

### ✅ Confirmed: it builds a PoseableMeshComponent from the game's skeletal mesh

`scripts/libs/uevr_utils.lua:3397` — `createPoseableMeshFromSkeletalMesh()`:

```lua
poseableComponent = M.create_component_of_class("Class /Script/Engine.PoseableMeshComponent", ...)
poseableComponent.SkeletalMesh = skeletalMeshComponent.SkeletalMesh
if poseableComponent.SetMasterPoseComponent ~= nil then
    poseableComponent:SetMasterPoseComponent(skeletalMeshComponent, true)
    poseableComponent:SetMasterPoseComponent(nil, false)
elseif poseableComponent.SetLeaderPoseComponent ~= nil then
    poseableComponent:SetLeaderPoseComponent(skeletalMeshComponent, true)
    poseableComponent:SetLeaderPoseComponent(nil, false)
end
poseableComponent:CopyPoseFromSkeletalComponent(skeletalMeshComponent)
M.copyMaterials(skeletalMeshComponent, poseableComponent, showDebug)
```

Note the leader-pose pair set-then-clear: it is used purely to **force one bone-transform update**,
then immediately detached so the component becomes free-standing. `[inferred-static 2026-09-19]`

### ❌ Correction 1: `CopyPoseFromSkeletalComponent` is NOT the per-frame path — it is the *fallback*

There are exactly three call sites `[measured 2026-09-19, grep]`:

| File:line | When it runs |
| --- | --- |
| `uevr_utils.lua:3426` | once, at component creation |
| `ik.lua:677` (`Rig:animateFromMesh`) | **only while a montage is playing** |
| `hands.lua:657` (`updateAnimationFromMesh_Native`) | the separate "Forearms" hand mode |

The tick body is `ik.lua:407–457`, and it is an **either/or**:

```lua
local isLeftAnimating  = select(1, executeIsAnimatingFromMeshCallback(Handed.Left))
local isRightAnimating = select(1, executeIsAnimatingFromMeshCallback(Handed.Right))
local didAnimate = false
if (isLeftAnimating or isRightAnimating) then
    didAnimate = self:animateFromMesh()      -- <- CopyPoseFromSkeletalComponent
end
if didAnimate == false then
    if self.wasAnimating then
        self:setInitialTransform(); self.wasAnimating = false
    end
    self:rebuildOrderedSolversIfNeeded()
    for _, solverEntry in ipairs(self.orderedSolvers or {}) do
        ... self:solveTwoBone(activeParams) ...
    end
end
```

So in ordinary play the pose is **not** copied from the game each frame. The poseable mesh keeps
whatever bone transforms it had, and the IK solver rewrites the arm bones on top. The game's
animation only takes over during montages (reloads, melee swings, scripted actions), and coming out
of one, `setInitialTransform()` re-stamps a saved bone list to undo it. `[inferred-static 2026-09-19]`

### ❌ Correction 2: the per-frame bone writes are `SetBoneRotationByName`, not `SetBoneTransformByName`

`SetBoneTransformByName` appears **3 times** in the whole hands/IK path, and only in
`Rig:setInitialTransform` — the restore-after-montage path. The solver writes rotations:
`SetBoneRotationByName` × 16, `SetBoneScaleByName` × 8, `SetBoneLocationByName` × 2.
`[measured 2026-09-19, grep over ik/hands/animation/hands_animation/controllers/pawn]`

### ❌ Correction 3: the IK is not hand-rolled — it calls Unreal's own solver

`ik.lua:1574`:

```lua
UKismetAnimationLibrary:K2_TwoBoneIK(
    shoulderWS, jointWS, endWS, jointTargetWS, effectorWS,
    outJointWS, outEndWS, allowStretch, startStretchRatio, maxStretchScale)
```

`UKismetAnimationLibrary` is fetched once at rig creation from
`"Class /Script/AnimGraphRuntime.KismetAnimationLibrary"`, and **IK is disabled outright if it is
missing** (`ik.lua:387`). Everything around that call is setup and cleanup: build the effector from
the controller, invent a pole vector, then convert the two solved *positions* back into bone
*rotations*, because `K2_TwoBoneIK` returns points and `SetBoneRotationByName` wants angles.
`[inferred-static 2026-09-19]`

---

## 2. The engine interface — every class, function and property the body/hands path reaches for

This is the list a native plugin would be calling instead. Grouped by purpose.
`[measured 2026-09-19, grep over ik.lua, hands.lua, hands_animation.lua, animation.lua, controllers.lua, pawn.lua, uevr_utils.lua, main.lua]`

### 2a. Classes resolved by path

| Class path | Role |
| --- | --- |
| `Class /Script/Engine.PoseableMeshComponent` | the mesh we own and write bones on |
| `Class /Script/Engine.SkeletalMeshComponent` | the game's mesh we copy from |
| `Class /Script/Engine.SceneComponent` | HMD proxy component |
| `Class /Script/HeadMountedDisplay.MotionControllerComponent` | the hands' parents |
| `Class /Script/AnimGraphRuntime.KismetAnimationLibrary` | **the IK solver** |
| `Class /Script/Engine.KismetMathLibrary` | all transform maths |
| `Class /Script/Engine.KismetSystemLibrary` | overlap queries, traces |
| `Class /Script/Engine.StaticMeshComponent`, `.StaticMesh` | debug visualisers, attachments |
| `Class /Script/Engine.SphereComponent`, `.CapsuleComponent`, `.BoxComponent` | hand collision |
| `Class /Script/Engine.PrimitiveComponent` | overlap filter class |
| `Class /Script/Engine.Material`, `.MaterialInterface`, `.MaterialInstanceConstant` | material copy onto the poseable mesh |

### 2b. Mesh creation and attachment

| Call | Notes |
| --- | --- |
| `AActor::AddComponentByClass(class, manualAttachment, relativeTransform, deferredFinish)` | the creation route; falls back to UEVR's `api:add_component_by_class` when the UFunction is absent |
| `FinishAddComponent` | deferred-finish path |
| `UPoseableMeshComponent::SkeletalMesh` (property, direct write) | assigns the asset |
| `SetSkeletalMesh` / `SetSkeletalMeshAsset` | tried in that order (UE version difference) |
| `SetMasterPoseComponent` / `SetLeaderPoseComponent` | set-then-clear, to force one update |
| `CopyPoseFromSkeletalComponent(src)` | pose snapshot |
| `USceneComponent::K2_AttachTo(parent, socketFName, attachType, weld)` | **the universal attach call** (10 files) |
| `AttachToComponent` | used once, in `scope.lua` |
| `DetachFromParent(bMaintainWorldPosition, bCallModify)` | |
| `K2_DestroyComponent`, `K2_DestroyActor` | teardown |
| `SpawnObject`, `spawn_actor` | owner actors for the controllers |

### 2c. Bone reading and writing (the core interface)

Every one of these is on `UPoseableMeshComponent`, and every one takes an **FName**, so a native
implementation lives or dies on caching FNames rather than rebuilding them per frame.

| Read | Write |
| --- | --- |
| `GetBoneTransformByName(FName, EBoneSpaces)` | `SetBoneTransformByName(FName, FTransform, EBoneSpaces)` |
| `GetBoneRotationByName(FName, EBoneSpaces)` | `SetBoneRotationByName(FName, FRotator, EBoneSpaces)` |
| `GetBoneLocationByName(FName, EBoneSpaces)` | `SetBoneLocationByName(FName, FVector, EBoneSpaces)` |
| `GetBoneName(int32 index)` | `SetBoneScaleByName(FName, FVector, EBoneSpaces)` |
| `GetParentBone(FName)` | `HideBoneByName(FName, EPhysBodyOp)` |
| `GetNumBones()` | `UnHideBoneByName(FName)` |
| `BoneIsChildOf(child FName, parent FName)` | |
| `TransformToBoneSpace` / `TransformFromBoneSpace` | |

`EBoneSpaces`: `0 = WorldSpace`, `1 = ComponentSpace` in this code (`ik.lua` uses the
`EBoneSpaces.ComponentSpace` / `.WorldSpace` constants; `animation.lua` passes a literal `boneSpace = 0`).
⚠️ Worth re-deriving from the SDK rather than trusting this line — the two files are not obviously
consistent and I did not confirm the enum values against the engine. `[hypothesis]`

### 2d. Transform maths — `UKismetMathLibrary`, called by reflection

`MakeTransform`, `BreakTransform`, `ComposeTransforms`, `InvertTransform`, `TransformRotation`,
`TransformLocation`, `TransformDirection`, `InverseTransformDirection`, `InverseTransformRotation`,
`ComposeRotators`, `Quat_Rotator`, `Subtract_VectorVector`, `Add_VectorVector`,
`Multiply_VectorFloat`, `LessLess_VectorRotator`, `VSize`, `FClamp`, `RadiansToDegrees`,
`Conv_TransformToString`, `Conv_StringToName`.

⚠️ **This whole group is pure overhead for a native plugin.** It is ordinary vector and quaternion
arithmetic being routed through Unreal's reflection system one call at a time because Lua has no
maths types. `ComposeTransforms`/`InvertTransform` alone are called ~20× per frame per arm.
One comment records what that costs them, at `ik.lua:1511`:

> *"`InverseTransformRotation(compToWorld, controllerRotWS)` amplifies the 0.036° of real controller
> movement into 0.220° by inheriting compToWorld's per-tick rotational noise. The noise is entirely
> in that one conversion."*

They then add NLERP smoothing to hide it (`ik.lua:1596–1618`).
`[inferred-static 2026-09-19]` — the numbers are theirs, unverified here.

### 2e. The IK solver itself

`UKismetAnimationLibrary::K2_TwoBoneIK(RootPos, JointPos, EndPos, JointTarget, Effector, OutJointPos,
OutEndPos, bAllowStretching, StartStretchRatio, MaxStretchScale)` — one call per arm per frame.

Around it, per arm per frame:
`K2_GetComponentToWorld` (explicitly **not** cached — `ik.lua:1480` says caching it makes the hand
drift when the pawn rotates), `GetBoneLocationByName` × 3, plus `GetRightVector` / `GetUpVector` /
`GetForwardVector` for the pole.

### 2f. Motion controllers

`UMotionControllerComponent` created with `AddComponentByClass`, then:
- `MotionSource` (FName property) set to the hand source string;
- `Hand` (enum property) set when present;
- `SetCollisionEnabled(0, false)`;
- `K2_GetComponentLocation()` / `K2_GetComponentRotation()` read each frame as the IK target.

The HMD is different: a plain `USceneComponent` registered with UEVR's own hook —
`UEVR_UObjectHook.get_or_add_motion_controller_state(c)` → `set_hand(2)` → `set_permanent(true)`.
`[inferred-static 2026-09-19]`

### 2g. Rendering and visibility

`SetVisibility(bool, bPropagate)`, `SetHiddenInGame(bool, bPropagate)`,
`SetRenderInMainPass(bool)`, `SetRenderInDepthPass(bool)`, `bCastDynamicShadow` (property),
`BoundsScale` (property), `SetCollisionEnabled`, `SetCollisionResponseToAllChannels`.

### 2h. Animation state

`IsAnyMontagePlaying`, `IsPlaying`, `GetAnimationMode`, `GetSocketTransform`, `GetSocketLocation`,
`GetSocketRotation`, `GetAllSocketNames`.

---

## 3. The genuinely valuable part: Silent Hill 2's own objects

This is the game-specific knowledge that cost them time. Everything in this section is
`[inferred-static 2026-09-19]` — read out of their config and code, never confirmed against a
running game.

### 3a. There is no first-person mesh. Everything hangs off `Pawn.Mesh`

`data/ik_parameters.json` and `data/pawn_parameters.json` both name one mesh, three times over:

```json
"mesh": "Pawn.Mesh",  "armsMeshName": "Pawn.Mesh",
"bodyMeshName": "Pawn.Mesh",  "armsAnimationMeshName": "Pawn.Mesh"
```

That is the standard `ACharacter::Mesh` — James Sunderland's **third-person body**. The arms you see
in VR are poseable copies of the full body mesh with everything but the arms scaled away.

### 3b. The skeleton — bone names, verbatim

**Arm chain (the IK bones), from `data/ik_parameters.json`:**

| Role | Left | Right |
| --- | --- | --- |
| clavicle (pole reference / animation root) | `clavicle_01_l_bn` | `clavicle_01_r_bn` |
| upper arm (IK start) | `upperarm_l_bn` | `upperarm_r_bn` |
| forearm (IK joint) | `lowerarm_l_bn` | `lowerarm_r_bn` |
| hand (IK end) | `hand_l_bn` | `hand_r_bn` |
| twist correction | `l_wrist_Offset` | `r_sleeve_master`, `r_wrist_Offset` |

Spine bone used for the body-hiding trick: **`spine_05_bn`**.
Head bone hidden in VR: **`neck_02_bn`** (`main.lua:419`, via `HideBoneByName`).

**Full hand/arm bone list, from `data/hands_parameters.json` → `profiles.Main.Arms.InitialTransform`**
(45 left, 46 right — the complete set they stamp transforms onto):

```
hand_?_bn
index_01_?_meta_bn  index_01_?_bn  index_02_?_bn  index_03_?_bn  index_?_be
middle_01_?_meta_bn middle_01_?_bn middle_02_?_bn middle_03_?_bn middle_?_be
ring_01_?_meta_bn   ring_01_?_bn   ring_02_?_bn   ring_03_?_bn   ring_?_be
pinky_01_?_meta_bn  pinky_01_?_bn  pinky_02_?_bn  pinky_03_?_bn  pinky_?_be
thumb_01_?_bn       thumb_02_?_bn  thumb_03_?_bn  thumb_?_be
lowerarm_twist_01_?_bn_Offset  lowerarm_twist_02_?_bn  lowerarm_twist_02_?_bn_Offset
?_wrist_Offset  ?_sleeve_master  ?_a_sleeve ?_b_sleeve ?_c_sleeve ?_d_sleeve
?_jacket_sleeve  ?_jacket_sleeve_{B,BL,BR,F,FL,FR,L,R}
```
(`?` = `l`/`r`. The right side additionally carries `rl_b_sleeve` and `rl_d_sleeve`. The `_be`
suffix bones are the finger-tip end bones.)

⚠️ **The jacket sleeve bones matter.** James wears a coat, so a dozen cloth bones per arm ride the
skeleton. The profile stamps explicit transforms on every one of them, which is why its
initial-transform table is so large.

### 3c. Sockets

| Socket | On | Used for |
| --- | --- | --- |
| `hand_l_socket`, `hand_r_socket` | the character skeleton | hand alignment reference |
| `MuzzleSocket` | weapon meshes | muzzle direction, used to drive grip rotation (`attachments.lua:1872`) |
| `hand_r_bn` | — | **weapons attach to the right hand *bone*, not a socket** (`main.lua:602`) |

### 3d. The game's object graph — the paths they had to discover

These are the lines a native plugin would reimplement, and finding them is the expensive part:

```lua
-- the equipped weapon, four levels deep through the anim instance:
pawn.Mesh.AnimScriptInstance.WeaponManageCmbSubcomp.EquippedWeapon
      -> .Mesh, .RootComponent, .RegisteredFirePoint.RelativeRotation, .AutoAimMaxRange

-- the torch, by scanning the equipment list for its class:
pawn.Items.EquipmentActors[]  -> is_a("Class /Script/SHProto.SHFlashlight")
      -> .Mesh, .LightMain, .Lightshaft

-- items picked up and examined:
pawn.Items.ItemExecutive.ItemContext.Mesh

-- state:
pawn:GetGameplayInputMode()            -- 1 == cutscene-ish
pawn.Movement.PushableComponent        -- non-nil while pushing something
pawn.CameraOverlapHandler              -- the thing that hides the player near the camera
pawn.View:OverrideControlRotation(FRotator, pawn)   -- how they steer the camera
pawn.RootComponent.CapsuleHalfHeight   -- used to place the arms mesh vertically
```

Weapon identification is by **name prefix** on `get_full_name()`: `WeaponPistol`, `WeaponShotgun`,
`WeaponRifle` (`main.lua:531–539`).

### 3e. SH2-specific UClasses they found

`/Script/SHProto.SHFlashlight` · `SHCharacterStatics` (has `IsCharacterInCutscene(pawn)`) ·
`SHItemWeaponMelee` · `SHMeleeBaseDamage` · `SHAnimCombatSubcomp` ·
`AnimNotify_MeleeAttackCheck` · `AnimNotify_ModifyCombatInputMode` ·
`ScriptStruct /Script/SHProto.SHBlendData` · `ScriptStruct /Script/SHProto.SHCameraDataStruct`.

### 3f. The tuned numbers, from `data/ik_parameters.json` and `data/sh2_config.json`

| Setting | Value |
| --- | --- |
| arms mesh offset | `(-13.287, 0, -18.1721)` location, `(0, -90, 0)` rotation |
| shoulder width scale | `1.74` |
| right hand: end-bone offset / rotation | `(-5.725, 7.3769, 0.4778)` / `(8, 140, -180)` |
| left hand: end-bone offset / rotation | `(-7.7141, -10.0053, 4.0928)` / `(-22, 50, 0)` |
| wrist twist influence / max | `0.35` / `75°` |
| shipped hand mode | `hands_type: 3` = **IKArms** (full two-bone arms, not floating forearms) |
| root offset | `(0, 0, -10)` |
| snap turn | `30°`, on |

Those rotation values — `(8, 140, -180)` for the right hand — are hand-tuned calibration between
controller space and this skeleton's bone axes. **They are not derivable; they were dialled in.**
They are the most directly reusable numbers on this page. `[inferred-static 2026-09-19]`

### 3g. Finger poses are stored as explicit per-bone rotators

`data/hands_parameters.json` → `animations.Shared.positions` holds, for each named pose, an `"on"`
and `"off"` rotator per finger bone, which `hands_animation.lua` blends between.

Poses defined: `open`, `grip`, `trigger`, `thumb` — each × left/right × ten grip variants
(`pistol`, `shotgun`, `rifle`, `rifle_offhand`, `pipe`, `board`, `chainsaw`, `flashlight`, `paper`,
`generic`). That is ~10 hand-authored grips per hand, and the reason the file is 146 KB.

⚠️ **`chainsaw` is in the list.** That is a Silent Hill 2 bonus weapon, so the grip set is
complete-game work, not a first pass.

---

## 4. The two native DLLs

Both export exactly `uevr_plugin_initialize` and `uevr_plugin_required_version` — they are UEVR
plugins, not injected hooks of their own. `[measured 2026-09-19, pefile]`

### 4a. `sh2r.dll` — 1,127,424 bytes, built 2024-11-13, MSVC 14.41

**The one-function-hook claim is confirmed, and it is even narrower than that.**
`[measured 2026-09-19, string extraction]`

Its complete meaningful string set:

```
SHPlugin::on_initialize()
SHPlugin::hook_melee_trace_check()
MeleeWeaponEnvTrace
Failed to find MeleeWeaponEnvTrace
Failed to find MeleeWeaponEnvTrace start
MeleeWeaponEnvTrace: 0x%p
Failed to hook MeleeWeaponEnvTrace
Hooked MeleeWeaponEnvTrace
SHPlugin::on_melee_trace_check_internal(0x%p, %f, %f, %f, 0x%p, 0x%p, %d)
Hit actor: %s
Hit component: %s
OnMeleeHitLeg
Environment
OnMeleeTraceSuccess
```

**What else it touches:** nothing. Its import table is **107 entries, all `KERNEL32.dll`**, and all
of them CRT support (heap, locale, TLS, console, SRW locks). No Detours, no MinHook DLL, no D3D, no
Unreal. Whatever trampoline it uses is statically linked. `[measured 2026-09-19]`

**How it finds its target** — a build path leaked into the binary:
`D:\a\SH2R-UEVR\SH2R-UEVR\build\_deps\kananlib-src\src\Scan.cpp`
So: project name **SH2R-UEVR**, built on GitHub Actions (`D:\a\` is the Actions runner), using
**kananlib** (praydog's scanner). The kananlib helpers it carries are
`utility::find_function_from_string_ref`, `find_function_start`, `find_function_start_with_call`,
`find_function_entry`, `resolve_displacement`, `resolve_instruction`.
⚠️ **So the hook target is located by scanning for a string reference — which means a game patch that
moves or renames that string breaks it.** `[inferred-static 2026-09-19]`

**What it does with the hook:** it classifies what the melee trace hit and raises a UEVR Lua event.
The Lua side (`melee.lua:110–160`) receives it via `uevr.sdk.callbacks.on_lua_event`:

- `OnMeleeTraceSuccess` with payload `"Enemy"` → advance a 3-step combo counter, 0.5 s cooldown,
  fire right-controller haptics.
- `OnMeleeTraceSuccess` with `"Glass"` → 0.5 s cooldown.
- `OnMeleeTraceSuccess` with anything else (`"Environment"`) → 0.033 s cooldown.
- `OnMeleeHitLeg` → look up `PistolDamage_C` and **`write_qword` it into the weapon's damage-type
  field** at a fixed offset, to enable kneecapping.

That last one is a raw memory write from Lua at a hard-coded struct offset
(`MELEE_WEAPON_DAMAGE_TYPE_OFFSET`) — the most build-fragile thing in the whole profile.
`[inferred-static 2026-09-19]`

**Verdict: it is unrelated to hands, body or IK.** It exists purely because swinging a real arm
needs to know, natively and immediately, that a swing connected.

### 4b. `uevr_utils.dll` — 351,232 bytes, built 2026-07-09, MSVC 14.44

**The "call any UFunction from Lua with parsed parameters" description is confirmed, and I can say
exactly why it exists and what its transport is.** `[measured 2026-09-19]`

Its imports are **55 entries, every one CRT** (`VCRUNTIME140`, `api-ms-win-crt-*`, 16 `KERNEL32`).
It touches no Unreal export and no Windows subsystem — it works entirely through UEVR's plugin API.

Its strings show a complete Unreal property marshaller:

```
[uevr_utils] Entered parseFunctionProperties
[uevr_utils] Function Property: %ws | Native Byte Offset: 0x%X (%u)
[uevr_utils] Property Class: %ws | Flag Hex: 0x%llX | Flags: %s
[uevr_utils] Property Flags Analysis -> Gives Back Data: %s | Is Return Value: %s
[Layout Match] C++ Target: %ws | Frame Size: %zu bytes
[uevr_utils] Parsing StructProperty / Parsing ArrayProperty / Nested ArrayProperty detected
[uevr_utils] Array of {float,double,int32,int64,uint16,uint32,uint64,bool,FName,EnumProperty,StructProperty} explicitly bound to dest.
```

Property classes handled: `ByteProperty`, `BoolProperty`, `FloatProperty`, `DoubleProperty`,
`Int32Property`, `Int64Property`, `UInt16/32/64Property`, `NameProperty`, `EnumProperty`,
`ObjectProperty`, `StructProperty`, `ArrayProperty` (including nested).

**The transport is JSON over UEVR's custom-event channel** — `scripts/libs/core/plugin.lua:295`:

```lua
local data = { debug = M.showDebug,
               caller_object = callerObject:get_address(),
               function_name  = functionName,
               params         = argsArray }
local callID = "ExecuteFunction_" .. guid()
uevr.api:dispatch_custom_event(callID, json.dump_string(data))
```

and the reply comes back through `on_lua_event` as another JSON string, matched by that GUID.

**What else it touches:** it also implements `getProperty` / `writePropertyValue` — read and write
any UProperty by name on any object address — with the same JSON round-trip.

**Why it exists, in its own words** (`plugin.lua:3`):

> *"This module allows you call any function in the game's SDK without limitations such as the
> TArray issue that may exist in native lua calls."*

**And it is barely used.** Three live call sites in the entire 49 k lines `[measured 2026-09-19, grep]`:

| Call site | Function |
| --- | --- |
| `collision.lua:405` | `KismetSystemLibrary::SphereOverlapComponents` |
| `collision.lua:453` | `KismetSystemLibrary::ComponentOverlapComponents` |
| `uevr_utils.lua:2503` | `GetAllSocketNames` |

All three are UFunctions with `TArray` in or out. **That is the whole reason this DLL exists.**

---

## 5. Where the Lua is fighting the engine

Everything in this section is `[inferred-static 2026-09-19]` — read from the code and its own
comments, never profiled here. **No performance number below was measured by me.**

### 5.1 ⭐ The big one: making a hand out of a whole body, every frame

A `PoseableMeshComponent` carries the *entire* James skeleton. To make it look like a forearm at the
end of a controller, `animation.transformBoneToRoot` (`animation.lua:162`) collapses everything else.

The thorough version walks the whole skeleton:

```lua
local count = poseableComponent:GetNumBones()
for index = 1, count do
    local childFName = poseableComponent:GetBoneName(index)
    if not poseableComponent:BoneIsChildOf(childFName, parentFName) then
        M.setBoneSpaceLocalTransform(poseableComponent, childFName, localTransform, boneSpace, rootTransform)
    end
end
```

That is, per bone: `GetBoneName`, `BoneIsChildOf`, and then `setBoneSpaceLocalTransform` —
which is itself `GetBoneTransformByName` + `GetParentBone` + `ComposeTransforms` +
`InvertTransform` + `SetBoneTransformByName`. **Six or more reflection calls per bone, over a full
UE5 character skeleton with a coat.**

They know. There is a `parentPathOnly` mode that only walks the chain from the target bone up to a
named ancestor, with the comment at `animation.lua:161`:

> *"Set parentPathOnly to true to only affect the bones in the parent path of the target bone. For
> example if you are calling this function on the tick and need the best performance"*

And the shipped SH2 config uses it: `OptimizeAnimations: true`, `Name: lowerarm_l_bn`,
`OptimizeAnimationsRootBone: clavicle_01_l_bn` — so it only collapses forearm → clavicle. The
"correct" version is too slow to run per frame and was traded away.

And it has to run per frame at all only because `CopyPoseFromSkeletalComponent` overwrites
everything: `hands.lua:628` says so —

> *"When an animation is applied, every bone in the entire skeleton is moved, so we need to reapply
> the transform from wrist to root"*

**A native plugin building a hand-only mesh, or writing the skeleton's bone array directly rather
than through per-bone UFunctions, would not have this problem at all.**

### 5.2 The whole `KismetMathLibrary` layer is pure tax

Per arm per frame the solver makes ~30 reflection calls to do vector and rotator arithmetic that
native code does in registers. A native plugin does this maths inline. This is the single largest
difference in principle between the two approaches.

### 5.3 Precision lost in a conversion, then papered over with smoothing

`ik.lua:1511` (quoted in full in §2d): one component-space conversion turns 0.036° of real controller
movement into 0.220° of noise. Their answer is `nlerpSmoothDir` on the arm directions (20% old) and
the pole (40% old), plus an effector-offset smoother. **That is latency being spent to hide
numerical error.** Whether native double-precision maths removes the cause is untested — but it is
worth knowing the smoothing exists for a reason, and is not a comfort feature.

### 5.4 `BoundsScale = 16.0`, knowingly

`hands.lua:527`:

> *"fixes flickering but > 1 causes a perfomance hit with dynamic shadows according to unreal doc —
> a better way to do this should be found"*

The hands are attached to a controller far from the mesh's own bounds origin, so Unreal culls them.
Their fix is a 16× bounds inflation on every hand component, plus `bCastDynamicShadow = false`, plus
a whole separate `flicker_fixer.lua`. **They say themselves it is the wrong fix.**

### 5.5 Hiding the body by scaling bones to 0.001

`ik.lua:618` (`updateBonesVisibility`) — root bone to `(0.001, 0.001, 0.001)`, then `spine_05_bn` to
`(0.001, 0.001, 1.74)` so shoulder width survives, then both clavicles back to `(1,1,1)`. The
comment admits the sequencing is superstition (`ik.lua:650`):

> *"keeping the bones in the same numbered order as the original seems to keep the transforms being
> applied in the correct order but I dont know if that is always the case. Applying them out of
> order results in a destroyed mesh"*

Meanwhile the *head* is hidden a completely different way — `HideBoneByName("neck_02_bn")` —
and the pawn's own meshes a third way, through four independent switches
(`SetVisibility`, `SetHiddenInGame`, `SetRenderInMainPass`, `SetRenderInDepthPass`) all exposed as
config because none of them alone works everywhere.

### 5.6 A 100 ms polling loop instead of an event

`pawn.lua:420` — `uevrUtils.setInterval(100, ...)` re-checks four hide flags ten times a second
forever, because there is no callback for "the arms should now be hidden". Cheap, but it means the
transition is up to 100 ms late.

### 5.7 A JSON round-trip to call three functions

§4b. Serialise a table to JSON, dispatch a custom event, parse it in C++, reflectively build a
parameter frame, call, serialise the result back, parse it in Lua — all to reach
`SphereOverlapComponents`. A native plugin calls it directly.

### 5.8 Defensive duck-typing everywhere

`if component.SetSkeletalMesh ~= nil then ... elseif component.SetSkeletalMeshAsset ~= nil then`,
`SetMasterPoseComponent` vs `SetLeaderPoseComponent`, `pcall` around every bone operation,
`uevrUtils.getValid(obj, {"path","through","fields"})`. This is the framework being portable across
UE versions and games. **Against one known build — UE 5.1, SHProto 1.0.0.5 — every one of these
branches is a constant.**

### 5.9 Per-frame work the game does not need

`main.lua`'s `on_pre_engine_tick` re-tests `CameraOverlapHandler:IsComponentTickEnabled()` and
re-nulls `OwnerCharacterPlay` **every single frame**, and re-reads
`SHCharacterStatics::IsCharacterInCutscene(pawn)` every frame. Both are state that changes rarely.

---

## 6. What I did NOT establish

Stated plainly, so none of this gets planned around:

- **Nothing was observed running.** No frame time, no bone count, no proof any of it works.
- **The bone names come from their config, not from the game's skeleton.** If their config is stale
  the names are wrong, and nothing here would show it.
- **`EBoneSpaces` values** (§2c) — read inconsistently across two files and not confirmed.
- **I did not decompile either DLL**, only read its strings, imports and exports. `sh2r.dll`'s
  hooking method and `uevr_utils.dll`'s frame-building are inferred from string evidence.
- **Whether the arms are a poseable copy of the body because that is the only way**, or because it
  was the first way that worked, is unknown. There is no first-person mesh in this game, but that
  does not prove a hand-only mesh could not be built some other way.
- **Nothing about licence or provenance.** This is a map of how it works, for study. Whether any of
  it may be reused, and on what terms, is a separate question nobody has asked here.

---

## 7. Tag summary

| Claim | Tag |
| --- | --- |
| Profile is 49,718 lines of Lua across 86 files | `[measured 2026-09-19, wc]` |
| The libs are a cross-game framework (Atomic Heart, Indiana, Phoenix classes present) | `[measured 2026-09-19, grep]` |
| `CopyPoseFromSkeletalComponent` runs only during montages, not per frame | `[inferred-static 2026-09-19]` |
| IK is `UKismetAnimationLibrary::K2_TwoBoneIK`, one call per arm per frame | `[inferred-static 2026-09-19]` |
| Per-frame bone writes are `SetBoneRotationByName`, not `SetBoneTransformByName` | `[measured 2026-09-19, grep counts]` |
| Arms and body are both `Pawn.Mesh`; no first-person mesh | `[inferred-static 2026-09-19]` |
| The listed bone, socket and class names | `[inferred-static 2026-09-19]` — from their config, not the game |
| `sh2r.dll` hooks exactly `MeleeWeaponEnvTrace`, found by kananlib string-ref scan | `[inferred-static 2026-09-19]` — strings + imports, not decompiled |
| `sh2r.dll` imports nothing but KERNEL32 CRT | `[measured 2026-09-19, pefile]` |
| `uevr_utils.dll` is a JSON-over-custom-event UFunction/UProperty bridge | `[inferred-static 2026-09-19]` |
| `uevr_utils.dll` has 3 live call sites, all TArray functions | `[measured 2026-09-19, grep]` |
| Every performance characterisation in §5 | `[inferred-static 2026-09-19]` — nothing profiled |
