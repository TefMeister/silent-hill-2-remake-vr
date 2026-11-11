# Silent Hill 2 (2024) — engine dossier

Distilled, durable facts about the game's engine and code, for the C++ plugin (`dev-archive/native/`). The dated story is
in `modding-notes/`; this page keeps only what later sessions need to act on. Every claim carries its confidence tag.
Started 2026-10-24 (the repo was restarted on 2026-10-19 without one).

## 1. Identity

- Unreal Engine 5.1 `[reported]`, `SHProto-Win64-Shipping.exe`, image base 0x140000000, `.text` 98.8 MB. RTTI stripped
  (no `.?AV` type descriptors) `[measured 2026-10-24]`.
- Route in: UEVR (PureDark's AFW build), OpenXR via Virtual Desktop. Our plugin `sh2vr.dll` loads beside praydog's
  `sh2r.dll` and jbusfield's Lua pack `[verified-live 2026-10-23, n=1]`.

## 2. Ranged combat: how a shot is requested (the fire gate)

Read from the exe on disk 2026-10-24 (`modding-notes/2026-10-24d-the-fire-gate.md`), all `[inferred-static 2026-10-24]`
and tied to that build; `fire.cpp` checks each against the game's own UFunction answers before using it.

| piece | where | what |
| --- | --- | --- |
| `SHRangedCmbSubcomp::RequestManualFire(weapon)` | 0x142A90FE0 | drops its argument; `= fire request(this, 0)` at 0x142A90C10 |
| aim gate | byte at part **+0x118** | what `IsInRangedAim()` returns (wrapper 0x1429AAC50); 0 ends the request at once |
| cooldown | part +0x198 (helper 0x142AA9B20) | while running, the request is queued (`+0x11B = 1`) |
| weapon ready gate | weapon vtable **slot 0x718** → 0x142B17140 | the VIRTUAL `IsFireReady`: held item's value (`[[owner+0x660]+list]+0x54`) `>=` float at weapon **+0x690**; the UFunction of the same name only wraps this slot |
| the shot | weapon vtable **slot 0x750** → 0x142B0FC70, `(weapon, manual=1)` | rounds at weapon **+0xBB4** (Lukas's offset, confirmed), pellets +0x750, spread +0x754/+0x758, direction from slots 0x728/0x738, trace, hit; **no aim check** |
| after a shot | part | `+0x128 = 1`, cooldown started, weapon delegates +0xC08/+0xC20 bound once, combat input mode → 13 |
| aim on | 0x142A95908 region | allowed in input modes {2, 3, 10, 11, 15, 19} (`[combat+0x152]`), sets `+0x129`, `+0x12C`, the byte, calls weapon slot 0x768(weapon, 1) |
| SHItemWeaponRanged vtable | 0x14678C5C0 | from `StaticClass()` 0x14297EAE0 → `InternalConstructor` 0x14297FBB0 → constructor 0x142B04BA0 |

Live facts behind this: forcing the UFunction `IsFireReady` to true fired nothing lowered `[verified-live 2026-10-24, n=1]`
(the path calls the virtual); `RequestManualFire` fires while aiming and the input mode goes 12 → 13 for a frame
`[verified-live 2026-10-24, n=1]`. **The doors, tried live 16:30 (OpenXR Simulator, flat):** the game's request with
both gates held open fires nothing (a third gate past them, not read); the weapon's own fire at slot 0x750 `(weapon,
true)` fires a real round with the gun lowered, spends it at +0xBB4 before `OnFirePerformed`, ejects the shell
`[verified-live 2026-10-24, n=13]`. The right trigger fires a lowered gun that way (`fire.cpp`). **Event hooks go on
the weapon's OWN Blueprint class**: `WeaponPistol_BP_C` overrides `OnFirePerformed`; a hook on the base class's
UFunction never fires `[verified-live 2026-10-24, n=1]`.
Reload doors (`ammo.cpp`): rounds at +0xBB4 read and written; `WeaponClipSize`; the carried ammo through the pawn's
`Items` component (`GetItemCount(row)`, `RemoveItem(row, n, false)`, row = `WeaponAmmoItem.RowName`); the empty pose
through `Mesh.AnimScriptInstance.SetOutOfAmmo(bool)`; the magazine bones `magazine_joint` / `magazine_socket_joint`;
sockets `slide_joint`, `magazine_joint`, `FX_muzzle` (Lukas's, `[reported]` until worn).

## 3. Reading this exe offline (methods that worked)

- **UFunction name → native wrapper:** the `{char* name, void* exec}` pair table in `.rdata`; find the qword pointer to
  the UTF-8 name string, the next qword is the wrapper. Two tables point at each name; the pair table is the one whose
  neighbours are `.text` addresses near each other. `dev-archive/recon/2026-10-24-fire-gate/sh2_reflect.py`.
- **Class → vtable without RTTI:** the UTF-16 class name is `lea`'d in `StaticClass()`; its `GetPrivateStaticClassBody`
  call passes `InternalConstructor<T>` at `[rsp+0x48]`; that tail-jumps to the constructor, whose first
  `lea rax,[rip+X]; mov [reg], rax` is the vtable. `sh2_slots.py`.
- Reflected property NAMES by offset: not found with the FPropertyParams layout guess; name them live with
  `uevr_inspect_object` instead.
- `flat-to-vr-RE-toolkit/tools/static-disasm.py --x64` reads this exe fine; the ghidrust MCP's function-start search on
  the 98 MB `.text` had not finished after 30 minutes and was not needed.

## 3b. Running it with no headset (2026-10-24)

- UEVR attached with the Virtual Desktop runtime idle: `Failed to initialize Framework on DirectX 12` every frame and
  the PRESS ANY BUTTON screen takes no input. Start the exe with `XR_RUNTIME_JSON` = the OpenXR Simulator's json and
  it runs in VR on the simulator `[verified-live 2026-10-24, n=3]`.
- Started from the exe the game has no Steam user (no Continue, a fresh profile in the SaveGames root); `SteamAppId`
  and `SteamGameId` = 2124490 in the environment fix it `[verified-live 2026-10-24, n=2]`.
- The menu through the UEVR MCP: `MainMenuWidget_BP_C` (`/Engine/Transient...`) `HasSlots()`, `ContinueGame(null, 0)`;
  then Enter at PRESS ANY BUTTON TO PLAY; `SHWeaponManageCmbSubcomp.RequestEquipWeapon("Pistol", true)`.
- The MCP's simulated pad is applied AFTER the plugin's XInput callback: a plugin never sees `uevr_input_gamepad`.
- Pad callbacks run C++ plugins first (file-name order: sh2r, sh2vr, uevr_mcp, uevr_utils), the Lua pack last and
  re-appended after every script reset, so the button layer switches the pack's pad writes OFF rather than undoing
  them `[inferred-static 2026-10-26, /gr, UEVR source]`.
- ⚠️ The connector dll `uevr_mcp.dll` 1.0.0 costs ~7 ms PER FRAME (it rewrites a crash-breadcrumb file ~6 times a
  frame on the game and present threads; the throttle in its source is broken in 1.0.0, fixed on master, unreleased)
  `[measured 2026-11-05, simulator, A/B/A]`. Keep it OUT of the profile except for the run that needs it; by keys the
  game is driven without it (Enter at the warning, Enter on CONTINUE, Enter at PRESS ANY BUTTON TO PLAY).
- The ghidrust connector's debug attach freezes the game for good (threads Suspended, detach does not resume them);
  stopping the connector's process frees it `[verified-live 2026-11-05, n=1]`. Sample waits with
  `dev-archive/tools/optimiser/waitstacks.py` instead.

## 4. Other engine facts (from the live sessions, 2026-10-23/24)

- Combat parts: `SHCharacterPlay_BP_C.Combat.SHRangedCmbSubcomp` found by `FindSubcomponentByClass` from C++ (class as
  an object argument) `[verified-live 2026-10-24, n=1]`. `GetCombatInputMode` reads `[combat+0x152]`
  `[inferred-static 2026-10-24]`.
- Aim movement speed is the weapon's `AimMovementData` (pistol 120; walk ≈ 130, run ≈ 300) `[measured 2026-10-24]`;
  the run input is dropped before `ProcessSprintRequestedChangeEvent` while aiming `[verified-live 2026-10-24, n=1]`.
- The HUD crosshair widget is `CrosshairContainer_WidgetBP_C`; a hidden reticule never removes it, UEVR drew it on
  the head-locked HUD `[inferred-static 2026-10-24]`; removed by `crosshair.cpp`.
- Settings: `Volume_Music` in `PlayerProfile.sav` (GVAS); window in `GameUserSettings.ini` (the game rewrote itself to
  fullscreen once, 2026-10-24).

## 5. Dead ends

- Hooking the UFunction `IsFireReady` to force "ready" (F11): proven not enough `[verified-live 2026-10-24, n=1]`; the
  cause is in §2.
- `SHDebug_Character_Combat_ForceRangedAim()` once: nothing visible `[verified-live 2026-10-24, n=1]`.

## 6. Frame time (the optimiser, 2026-11-05; notes 2026-11-05c and 2026-11-05d)

- On the simulator (90 Hz cap, 1280 x 1400 per eye), standing at the armory save: 19-21 ms with the connector dll in
  the profile, 13.0 ms without it, 11.2 ms (the cap) without it and with UEVR's `VR_SynchronizationMode=0` (Early)
  `[measured 2026-11-05, A/B/A, n=2 per setting]`. Early is now in the profile and in `mod/`.
- `VR_SynchronizationMode` = WHERE UEVR calls xrWaitFrame: 0 Early (the frame's first hooked RHI command), 1 Late (top
  of the Present hook), 2 Very Late (right after Present, for the next frame); all on the present (RHI) thread; the
  game thread never calls it `[inferred-static 2026-11-05, praydog master]`.
- The simulator is a 90 Hz CAP (a late caller waits 0 ms), costs ~1.1 ms per frame inside xrEndFrame, and its
  `frame_time_ms` / `target_fps` status values are constants `[inferred-static 2026-11-05, its source + log]`.
- Drawing settings, the game's logic, our plugin (0.2-0.9 ms, mostly the crosshair re-hide) are not the wall; r.VSync 0
  gains ~1.5 ms (desktop pacing) `[measured 2026-11-05, step 1]`.
- Open: at Very Late the card reads 98 % busy at 13 ms, at Early 52 % at 11.2 ms, same scene `[measured, n=2 each]`;
  the headset's own numbers (Virtual Desktop renders larger than the simulator); the 10-31 halving to ~30 fps in the
  headset from 13:57 (21 Sacraments installed that day) `[measured 2026-10-31, Ghost Hand reports; cause: hypothesis]`.

## 7. Frame order: UEVR's hands, our arm mesh, the held gun (2026-11-06, /pd, Fable; note 2026-11-06b)

- **UEVR moves the motion-controller components and every UObjectHook attachment at the SECOND eye's
  `on_pre_calculate_stereo_view_offset`** (`(view_index + 1) % 2 == 0` -> `tick_attachments`), not at the engine tick.
  At our `on_pre_engine_tick` the controllers hold the previous frame's pose; the fresh one exists from the second
  eye's view step on, where our `on_post_calculate_stereo_view_offset` (`twohand::on_view`, `rhbolt::on_view`) runs
  `[inferred-static 2026-11-06, praydog's UObjectHook.cpp]`.
- **The equipped gun's visible mesh is a child of OUR arm mesh's `hand_r_bn` socket** (the pack's grip-update callback
  with IK arms + `attachments.lua` `K2_AttachTo`; the bridge hands the pack our poseable mesh as `sh2vr_ik_mesh`). A
  `K2_SetWorldTransform` on it becomes a relative transform against that socket `[inferred-static 2026-11-06]`.
- **Our arm mesh is a `UPoseableMeshComponent`: bone writes reach the picture only after the component's own tick**
  (the refresh is `RefreshBoneTransforms`, a virtual, not a UFunction) `[reported; matches a forum report of the same
  one-frame lag]`. A target set at the pre-tick is drawn the same frame; one set at view time is drawn next frame.
- Consequence and cure: a gun turned at view time toward the fresh controller is drawn one frame ahead of a hand
  placed on it (the 11-06 slide). Since 2026-11-06 the gun is drawn where the hands were solved for (`twohand.cpp`:
  aimed at the drawn left hand bone; `rhbolt.cpp`: the trail as of the last view) and only the hands' next targets come
  from the fresh controller. Unrun.
- A true catch-up needs `RefreshBoneTransforms` at view time: find its vtable slot live (`uevr_vtable` on the poseable
  mesh's class) if the drawn-gun cure is not enough.

## 8. Lighting channels and the player's shadow (2026-11-06, live through the connector; note 2026-11-06c)

- **The flashlight's main spot light is on lighting channels 0 and 2; James's body, his flashlight mesh and the
  weapon meshes are on channel 1 only; his fill lights (BackLight, VeryCloseSpotlight) on 1, CloseLight on 0.** The
  game keeps the player out of his own torch by channel. Our poseable arm mesh is on channel 0 (lit and shadowed
  by the torch) `[verified-live 2026-11-06, n=1]`. A weapon lit by the torch in VR needs channel 0 added.
- **A skeletal mesh following another's pose (`SetLeaderPoseComponent`) shares the leader's bone visibility**:
  `IsBoneHiddenByName` on the follower answers for the leader's hidden bones `[verified-live 2026-11-06, n=1]`.
- **The game ships `MI_Invisible`** (`/Game/GBK/MaterialInstances/MI_Invisible`), on three of James's 23 slots; the
  head is slots 0, 1, 6, 8, 9, 10, 11-18, 22 (lacrimal, teeth, eye shadow, face, facial hair, eyes, hair, brows).
  Setting them to it removes the head from the camera with the neck bone shown `[verified-live 2026-11-06, n=1]`.
- `SetSkeletalMesh` does not exist on this build's components; `SetSkeletalMeshAsset` and `SetSkinnedAssetAndUpdate`
  do. `GetMaterialSlotNames` is callable through the connector, not from the Lua binding.
- `F` toggles the torch (desktop picture mean 33.2 on, 26.9 off); `1` takes the pistol out, in VR too.

## 9. The real headset is not the simulator: the warp and DLSS (2026-11-12, /pd; note 2026-11-12)

- Quest 3 through Virtual Desktop, started with `VR_RenderingMethod=3`: the per-eye swapchains (2688 x 2880) are made once, destroyed when the session turns READY, and Virtual Desktop refuses to make them again (17 tries); UEVR draws on regardless and the game dies about 10 s after the inject `[measured 2026-11-12, one log, n=1]`. With 0 (one double-wide picture) the same remake succeeds `[measured, n=1]`. Why it refuses is not known: UEVR does not log the error code `[inferred-static, PureDark/UEVR AFW source]`.
- PureDark's notes say to select the warp in the menu AFTER injecting, with DLSS on and Ghosting Fix ticked `[reported]`. Starting with it on is our own route; it worked on the simulator only.
- Both eyes at once (0) with DLSS Quality at the headset's size shows a see-through second picture; with DLSS off it is gone `[reported 2026-11-12, n=1 each]`. Each eye had its own DLSS instance (two creates in the log), and in this mode UEVR's DLSS hook changes nothing `[inferred-static]`. Suspects, untested: the late pose update against the game's motion vectors (`VR_SynchronizationMode=2`), or a wrong region at that size.
- Our packages carry AFW beta 5 (`Tag: UEVR_AFW_v1.0-beta.5` in every log); beta 6 exists (2026-08-13).
- `sh2vr_devcvars.txt` takes `uevr <Setting> <value>` lines for the injector's own settings since 2026-11-12 `[compile-verified]`.
