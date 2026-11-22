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
- **Crashes at quit (inbox drained 2026-11-15, from the /gd notes of 11-07 and 11-11 and the /gr and /sr answers).**
  Three kinds on the shelves `[measured from the dumps, 2026-11-07 and 11-11]`: (1) `UEVRBackend.dll+0x1503df`, a
  read of null+0x58 in PLAY, 2026-11-05 x4 and 2026-11-10 x2 (resolve against the exact AFW b6 dll in use: beta 6
  ships two different dlls, nightly- and joeyhodge-based `[reported, release notes]`); (2) two stack-like write
  faults in ntdll on 2026-11-05 evening `[hypothesis: stack overflow]`; (3) every quit since 2026-11-06 is a crash:
  first `0xC0000409`/0xa at an address in no library (after Steam unloaded), from 2026-11-10 13:00 `0xC0000409`/0x7
  (`abort()`) at `openxr_simulator.dll+0x919bd`. The last one is **the simulator's own fault**: since v1.6.0 its
  preview window runs on a static `std::thread` joined only in `xrDestroyInstance`, so a process that quits without
  it aborts `[inferred-static 2026-11-11, /gr, source read; offset not resolved against the binary]`. It costs only
  a 150 MB dump per quit. UEVR and AFW did not change upstream since 2026-08 `[measured 2026-11-07, GitHub API]`.
- **Closing the game under the simulator:** the process owns two top-level windows (the simulator's preview first);
  `MainWindowHandle` may be the preview, and a close sent there closes only the preview
  `[verified-live 2026-11-10 in the-darkness-vr, n=3; not checked in SH2]`. `run-sim.ps1` closes and keys the game's
  own window since 2026-11-15.

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

## 10. What the frame warp does with near, moving things (2026-11-13, /lm; notes 2026-11-13d and 13e)

- Mode 2 (previous frame) lands every pixel by camera and depth alone; an object's own motion is never used for where it lands, so hands and a held weapon are one frame behind on every made-up frame whenever the view turns or they move `[verified-live 2026-11-13, simulator, n=7; inferred-static for the reason]`. No `VR_AFW_*` motion setting changes it. Dead end: better velocity from our arms (it is already right: mode 3's debug view marks them) `[disproved]`.
- Mode 3 (combined) carries marked moving objects, with the outline around them. Mode 1 is worst.
- The warp's distances are metres; its near rules sit at 0.1, 0.5 and 1.0 `[inferred-static; the 0.1 path verified-live]`. Describing the scene larger or smaller to it (our stand-in in the injector's import of `EvaluateFrameWarp`) moves those lines and nothing else; no fix came from it.
- The debug view: Numpad 6 as a real key press; it tints only in modes 1 and 3.
- Ours: `warpscale.cpp` makes a mode 2 call as mode 3 while the view turns or a hand moves fast (b019; the file `sh2vr_warpscale.txt`, "scale degrees speed"). Headset verdict open.

## 11. Weapon smoothing (2026-11-15, /pd; `smooth.cpp`, build b023 in the build history)

- The held weapon is a child of our arm mesh's `hand_r_bn` (§7), so smoothing the right hand's TARGET inside the arm
  solve (`ik.cpp`, before the solve) moves the arm, the hand and the weapon as one piece. Not while the right hand
  rides the rifle locked in the left hand (`rhbolt.cpp`).
- A One Euro filter with Village's numbers (`visceral_re8_hand_smooth.lua`): strength s gives a still cut-off of
  12 Hz x 0.65^s; gun 4 (2.1 Hz), melee 1.5 (6.3 Hz). Filtered in the controller's tracking space (its attach
  parent), so walking and turning are not smeared `[compile-verified 2026-11-15]`.
- By numbers on the shipped step (`tests/smooth_test.cpp`): still-hand shake per frame cut to 17 % (gun) and 34 %
  (melee); a 120-degree swing in 0.25 s trails by 5 degrees at most with melee (under one frame) and settles in
  14 ms `[verified-numerically 2026-11-15]`. Not run in the game.
- What is held: the weapon manager's `GetEquippedWeaponName` ("None" = bare hands) plus `GetEquippedRangedWeapon`
  (a gun) else melee; the healing item = raw. jbusfield's own per-solver `smoothing` (0 in the profile) is left alone.

## 12. The OBS crash at the cutscene switch, and the stills to compare cutscenes against (2026-11-15, /lm reader + simulator run)


For: the live session on Silent Hill 2 (and, for part 1, the Keeper: it is a Lanes fault). Read-only work: nothing
was launched, attached, installed or committed. Files read: the plugin sources (`dev-archive/native/src`), the Lanes
rider and `obs-rec.py` (`_lanes-next-wt`, 0.117.0 = the installed copy), `%LOCALAPPDATA%\BeG0nE\ride.log`,
`%LOCALAPPDATA%\obs-rec-breaks.log`, OBS's crash reports (`%APPDATA%\obs-studio\crashes\`), the OBS Mirror layer's log
(`%LOCALAPPDATA%\XR_APILAYER_NOVENDOR_OBSMirror.log`), the board, FAULTS.md.

### 1. The error sound

### Short answer

The beep is OBS's own crash box ("OBS has crashed!"). OBS is only running because Lanes' rider records every VR
session. **What crashes it at a cutscene is NOT the two riders**: it is the recording of the headset view. Our flat
cutscene screen (`cutscreen.cpp` sets UEVR's `VR_2DScreenMode`) makes UEVR rebuild its OpenXR pictures at the game
window's size; the headset view that OBS records changes size; OBS 33.0.1 dies about two seconds later. Lanes' guard
then closes the box, kills OBS and starts a fresh recording (probably the Discord bar flash). So: the mod only sets
it off; the fault (and the fix) is Lanes'.

### What the files show

- **Which log starts the recording.** Our only BeG0nE recorder is Ghost Hand (`ghosthand.cpp`), writing
  `%LOCALAPPDATA%\BeG0nE\ghost-hand\silent-hill-2-remake-vr_<stamp>.ghl`. It opens when UEVR's `is_hmd_active()` turns
  true and closes when it turns false; it does not look at cutscenes `[read in source]`. One `.ghl` per game run,
  opened at the headset session's start (main menu), with one `VR SESSION` and one `RECORDING` line per run in
  ride.log `[measured: every SH2 run 11-12 to 11-15]`. So no recorder log starts growing at a cutscene; the rider
  starts OBS once per run, not per cutscene.
- **The two-riders cause (FAULTS row, the Keeper's).** Real, and fixed: ride.log shows every line twice and a `CLASH
  GUARD ... second recording does not start` one second after each start until 11-15; since `14:14:57 rider up (pid
  3348, ..._lanes-next-wt\...)` every line is single and a refusal reason is logged (`not recording: could not tell
  which program is logging`, 17:47:05) `[measured]`. The Startup entry runs the rider from `_lanes-next-wt`, whose
  `rider.py` is byte-identical to the installed 0.117.0 copy and holds the `Local\BeG0nE-rider` mutex (d47dabd is
  an ancestor of 0.117.0) `[measured]`. That fix covers the crashes right after a session start (crash reports
  22:50:03 on 11-13, 01:15:44 on 11-14: Source Record starting its NVENC encoder, 2-6 s after a start).
- **The cutscene crashes are a second, separate cause, NOT fixed by d47dabd.** OBS's crash reports (it keeps the last
  ten) and the OBS Mirror log, lined up by the second `[measured]`:

  | the game rebuilds its OpenXR pictures (Mirror log) | OBS crash report |
  | --- | --- |
  | 11-13 13:06:14 to window size (flat screen on) | 13:06:16 |
  | 13:07:10 flat on | 13:07:12 |
  | 13:45:55 back to the headset size (flat off) | 13:45:57 |
  | 13:58:13 flat on | 13:58:15 |
  | 14:06:03 flat on | 14:06:05 |
  | 14:08:07 flat on | 14:08:08 |
  | 11-14 02:03:05 flat on | 02:03:07 |
  | 02:03:18 flat off | 02:03:20 |

  "Flat on" = the eye picture swapchain replaced by a 1920x1080 one (the burst `1920x1080, 1920x1080, 4x4`); in 2D
  mode UEVR sizes the eye pictures from the game window (`inbox/2026-11-12-mod-uevr-2d-mode-source-reading.md`,
  `VR.cpp:2840-2870`). The earlier breaks (11-13 12:39, 12:51, 12:55, 12:57, 12:58, crash reports already rotated
  out) each follow a flat on/off rebuild by 11-16 s, the guard's polling plus its restart `[measured]`. Changes of
  size between the two headset sizes (the warp switch, 5376x2880 <-> 2688x2880; 13:49-13:56, four times) did NOT
  crash `[measured, n=4]`.
- **Only with the headset view as the second file.** Every crashing recording was `+ OpenXR headset view (OBS
  Mirror)`. Simulator runs with full opening cutscenes, recorded `+ OpenXR Simulator preview` (11-12 22:31-22:37,
  22:52-22:58, 22:59-23:05), have no break at all `[measured, n=3+]`. Tefa's 23:18 headset wear of the same build
  broke twice (23:18:52, 23:20:16).
- **The crash itself** (all eight above): `end_data_capture_thread -> remove_connection -> video_output_disconnect2 ->
  pthread_mutex_unlock` on a null mutex, while the UI thread runs a queued task in `obs_output_release /
  obs_output_destroy` (`OBSApp::Exec`) `[measured, crash reports]`. That is an output being torn down; the only
  output besides the main recording is the Source Record filter on the mirror source. `[hypothesis]` Source Record
  restarts its output when its source changes size, and OBS 33.0.1 crashes in that teardown.

### Ours or Lanes'?

Lanes' `[inferred from the measured table]`. The mod's switch is legitimate and must stay; the crash needs OBS + the
Source Record filter on the OBS Mirror source, which only Lanes sets up (`obs-rec.py` `point_second(...,
kind=MIRROR_KIND)`). Fix belongs on the Keeper (Lanes work), e.g. record the headset view without Source Record,
pause the second file around a size change, or a newer OBS. The FAULTS row (2026-11-13) is half right: its cause is
real but explains the start-of-session crashes, not the cutscene ones. The board row "[FLAT] The Windows error sound
at the cutscene's start and end" is this, and "two Windows error sounds" at the main menu (b008) may be the
start-of-session pair.

### What a live check should look for

1. On the simulator (today's live run records `+ OpenXR Simulator preview`, ride.log 17:49:01): **prediction: no
   beep and no new row** at the cutscene's start or end `[hypothesis from the table]`.
2. In the headset (the second file is then the OBS Mirror): at each `cutscreen: cutscene began, flat screen going
   on` / `ended ... off` line in `%APPDATA%\UnrealVRMod\SHProto-Win64-Shipping\log.txt`:
   - the Mirror log shows `Creating swapchain with dimensions=1920x1080` (twice) the same second;
   - a new `%APPDATA%\obs-studio\crashes\Crash <date> <time>.txt` 1-2 s later, with `video_output_disconnect2` and
     `obs_output_release` in it;
   - a new row in `%LOCALAPPDATA%\obs-rec-breaks.log` (`OBS showed "OBS has crashed!"  restarted`) 5-20 s later;
   - ride.log stays single-lined (one rider) and logs nothing at that moment (the guard writes only to the breaks
     log).
   All four = confirmed. A beep with no crash report = something else (look at the window list for a message box).
3. Cheap separation without the headset: `sh2vr_flat.txt` in the profile forces the flat screen on in play
   (`cutscreen.cpp dev_forced`); creating and deleting it switches 2D mode on and off on demand. Only useful when
   the second file is the OBS Mirror, i.e. with the headset (or with no simulator preview window up).

### 2. The opening cutscene stills: "the warp on, compared against b014's stills"

### Where the reference pictures are

- **b014's stills = `dev-archive/recon/2026-11-12-real-flat-cutscenes/runs/dlss-dive/`**: route stills `r0`-`r3`
  and `t000.jpg`-`t231.jpg` (one a second from the key press that starts the opening), taken 11-13 01:55:20-01:59:11,
  DLSS on, ray tracing on, rendering method 0 (no warp). b014 was built 01:50 (builds INDEX.md) and the deep-dive
  session ran 01:50-02:15, so this is the b014 run `[inferred: by time; no log names the build]`. Not committed
  (`.gitignore`: `runs/*/`); local to this PC only. No sheet was made for it.
- **The committed reference, b008 (same camera code, DLSS on)**: `runs/sheet-realcam2-dlsson-0-120.jpg` and
  `-123-249.jpg` (every 3 s), and the plain game without any mod: `runs/sheet-plain1-0-120.jpg` / `-123-249.jpg`.
  Spot check: b014's `t039` (over the shoulder into the mirror) and `t093` (the car park) match the b008 sheet's +39
  and +93 `[measured, looked at]`.
- The plain timeline (note `modding-notes/2026-11-12c-...`): +11 two pairs of hands in the mirror frame, +21 lips
  close-up, +30 the hand rubbed, +39 over the shoulder into the mirror, +60 the wide bathroom, +69 James's face, +78
  black with two lines of text (to about +88), +90 out of the door, +93 the car park, +123 the wide overlook, +156 at
  the railing, +195 behind him at the view; play from about +205. +3 to +9 are black.

### How to run it

All from `dev-archive/recon/2026-11-12-real-flat-cutscenes/`. The game must be CLOSED first (the script refuses
otherwise: "a game is already running: stop").

1. `python cutcompare.py warp-b0NN vr 250`
   - `vr` starts `D:\SH2 builds\releases\Silent Hill 2 VR 0.1.1\SH2VR.exe play --sim --no-install`: the OpenXR
     Simulator, with whatever plugin is INSTALLED (today b027; the warp switches itself on 10 s after the headset
     starts drawing, `warp.cpp`, unless `sh2vr_nowarp.txt` is in the profile). `plain` = the game alone, `-windowed`.
   - It copies the save folder to `D:\SH2 builds\_save-backups\2026-11-12c-real-flat-<label>` first and puts it back
     after, checked by hash ("saves put back, same as before: True").
   - Route, by itself with a picture check before each key: waits 50 s, the warning screen (`r0`), Enter to the main
     menu (`r1`), down + Enter, four downs to START GAME (`r2`), Enter, 32 s, `r3` "press any button", Enter. Then one
     still a second for `250` s (`t000`...`t249`), 640 px wide, from the game window (PrintWindow).
   - Takes about 6 minutes in all. It closes the game the game's own way (WM_CLOSE), kills a CrashReportClient if
     one is up, then puts the saves back.
   - Options: `--keep-open` (leaves the game up and the saves NOT put back; close later with `python closeup.py
     2026-11-12c-real-flat-<label>`); `--dll <path>` puts a test build in for this run only (writes beside the UEVR
     profile: the live session's call, not the reader's).
   - Pictures land in `runs/<label>/`; it prints `RUN <label> vr: stills N, game up True/False, cutscene-began line at
     +X s`, then the last 40 `cutscreen:` and `realcam:` lines of the injector's log.
2. Sheets: `python sheet.py warp-b0NN 0 120 3 7` and `python sheet.py warp-b0NN 123 249 3 7` -> `runs/sheet-warp-b0NN-0-120.jpg`
   etc. For b014 the same with `dlss-dive` (`0 120 3 7`, `123 231 3 7`); compare against those and against the
   committed `sheet-realcam2-dlsson-*` (same columns: 7 a row, every 3 s).
3. `waitcut.py <s>` is not needed for the comparison; it returns once the newest `cutscene began` line is `<s>`
   seconds old (exit 1 "no cutscene line" after 400 s): for timing a by-hand check (an OBS frame, a connector read)
   at a fixed second into the cutscene. `odd.py <video>` counts one-frame wrong pictures in the BeG0nE recording
   (`D:\claude video game stuff\recordings\silent-hill-2-remake-vr\`), per 20 s.

### Pass / fail

- **Pass:** `game up True` and the run reaches the end; `cutscene-began line` at roughly +9 to +12 s (the hands at
  +11/12); in the log, `warp: ... switching the frame warp on (rendering method 0 -> 3)` BEFORE `cutscene began`, then
  `realcam: angle ... the projection's first number 1.000 -> <n>` lines; every still matches b014 / b008 at the same
  second within a second or two: the close-ups stay close (lips +21, hand +30), the mirror shot at +39 shows James in
  the mirror like the reference, the car park +93 the same framing; black only at +3..+9 and +78..+88; at the end
  `cutscene ended, flat screen going off` and the play view back.
- **Fail, and what each means:** wide views from behind where the reference has a close-up = the real camera angle
  is not applied with the warp on (realcam was written against method 0); a squeezed two-eye picture or a
  see-through double in a still = the right-eye fault family; black or frozen stills outside the known black seconds
  = the flat screen does not draw with the warp; `game up False` / a CrashReportClient = a crash at the switch
  (note the second against the log); the timeline shifted by many seconds = a stall or a lost key (look at `r0`-`r3`).
- Caveat `[hypothesis]`: the window stills are UEVR's spectator view; with the warp on, what the window shows may
  differ from the headset. UEVR's source says the warp is off in 2D mode and method 3 falls back to synchronized
  drawing while flat (`VR.hpp:513-523`), so the flat part itself should look like b014; the risk is the two switches.
- Side check for part 1: this run is on the simulator, so by the table above it should NOT beep. A beep here would
  undo the "OBS Mirror only" reading.

- **Live, 2026-11-15:** the opening cutscene with the warp on ran 3 min 38 s on the flat screen and switched back to VR cleanly; no OBS break at either switch while the simulator preview was recorded `[verified-live 2026-11-15, simulator, n=1]` (`modding-notes/2026-11-15b-the-opening-cutscene-with-the-warp-on.md`).

## 13. An occasional crash 22-25 s after the start (2026-11-15, /lm reader, files only)


For: the live session. Files only; nothing launched, attached or changed. Read: both release folders under
`D:\SH2 builds\releases\`, the launcher source (`dev-archive/launcher/src`), the tools copy
`D:\claude video game stuff\tools\UEVR-nightly-AFW-beta.6\`, every crash folder in
`%LOCALAPPDATA%\SilentHill2\Saved\Crashes\` (CrashContext.runtime-xml), today's injector log, the pack's Lua in the
installed profile, `%LOCALAPPDATA%\BeG0nE\ride.log`.

### Verdict

**Not a 0.1.2 package fault. A recurring early-start crash that both folders can hit** `[inferred from the measured
points below]`. The two starts injected the very same UEVR, with the same launcher and the same inject settings. On a
`--no-install` run, the only files that differ between the folders are never used.

### 1. The two folders `[measured, sha256 first 16 hex]`

- `uevr\`: all six DLLs byte-identical in 0.1.1 and 0.1.2, and identical to the public **AFW beta 6** zip's copies
  (the tools folder `UEVR-nightly-AFW-beta.6`):
  UEVRBackend `d3ff93e7f327016d`, PDAFWPlugin `5f3cfc38903ac438`, LuaVR `feb1db09852a61f5`, UEVRPluginNullifier
  `ad940ec9587db5ce`, openvr_api `7e091996282fa9df`, openxr_loader `3bb4eb190024f92e`. So "the 0.1.2 package's copy
  is not hashed yet" (board) is now done: it is beta 6.
- Version strings in UEVRBackend.dll (both): commit `c40eaab8043dcd4b3209c77c50f7fadcd49093e8`, tag string
  `UEVR_AFW_v1.0-beta.5` (beta 6's DLL still carries the beta 5 tag, note 2026-11-13b). Today's injector log says
  the same commit.
- `SH2VR.exe`: identical (`a73c4374f4e2dbf2`, 444,928 bytes).
- **Inject settings:** compiled into the launcher (`settings.h`): `INJECT_DELAY_MS` 8000 after the window shows,
  nullifier off and OpenXR unless `--nullify` / `--openvr` / `--delay` are passed. No settings file sits in either
  folder. Same exe, same command line, so the settings are the same.
- **What differs:** `package/profile/plugins/sh2vr.dll` (0.1.1 `00d3cd1f...` 677,376 B; 0.1.2 `517fad1c...`
  759,296 B), `package/profile/config.txt` (line endings only: CRLF against LF, same 118 lines), README.txt,
  FILES.txt. **With `--no-install` the launcher installs nothing** (`main.cpp play()`: `install_all` only
  `if (!pid && !no_install)`). UEVR is injected from `<launcher folder>\uevr` (identical), and the game ran on the
  installed profile in both starts. So nothing that differs was in play.

### 2. The crash and its siblings `[measured, CrashContext.runtime-xml]`

17:47 (`UECC-Windows-9AC0E9904B610FDAB45CA38DF45E8842_0000`), 25 s after start, game thread, reading
0xffffffffffffffff:

```
SHProto +4f1af02 <- +4c52806 <- +4b281cd <- UEVRBackend +14cac3 <- UEVRBackend +169811 <- SHProto +507f5a1
<- +4ad30f1 <- +4ad7b43 (inside UGameEngine::Tick, +4ad7540) <- UEVRBackend +1685bf (its engine-tick hook)
<- +b61124 ... main loop
```

The same signature was seen three times before, all 22-25 s after start:

| crash | after | reading | path above the three game frames |
| --- | --- | --- | --- |
| 11-12 21:33 | 22 s | 0x0 | UEVR +14cac3 <- +169811 <- S +507f5a1 <- UEVR +14d418/+17f2dd/+152f03/+23895/+1684f3 (UEVR's own tick work) |
| 11-12 21:36 | 22 s | 0x0 | the same |
| 11-13 20:23 | 24 s | 0xffff... | identical to today's (game tick) |
| **11-15 17:47** | 25 s | 0xffff... | identical to 11-13 20:23 |

So it is **n=4 across three days and several builds** (b007-era, b0xx on 11-13, today's b027), and it has nothing
to do with which release folder starts the game. ride.log for today: `17:46:54 COLLECTING`, `17:47:05 VR SESSION`
(Ghost Hand opened, i.e. the headset session was up), `17:47:10 CrashReportClient`: about 5 s after VR came up.

### 3. What the offsets say `[inferred / hypothesis]`

- `UEVRBackend +1685bf` is UEVR's `engine_tick_hook` (read 2026-11-12, `inbox/2026-11-12-mod-dlss-cutscene-crash-reading.md`).
- `S +507f5a1` is reached both from the game's tick and from UEVR's own code (21:33 variant). The next frames are a
  UEVR detour (`+169811`) that calls back into the game (`+14cac3` -> `S +4b281cd`). That is the shape of **a
  UFunction whose native pointer UEVR replaced (`hook_ptr`), called through UObject::ProcessEvent**, with UEVR
  calling the original `[hypothesis: +507f5a1 in ProcessEvent, +169811 the hook_ptr dispatcher; not disassembled]`.
- UFunction hooks installed at load (before ~25 s), in both starts alike: the pack's `main.lua` marks two Blueprint
  functions as native and hooks them (`LegacyCameraShake.BlueprintUpdateCameraShake` and `ReceivePlayShake`, flag
  0x400, "courtesy of Praydog"); `uevr_utils.lua` hooks `PlayerController.ClientRestart`; jbusfield's `sh2r.dll`
  hooks `MeleeWeaponEnvTrace` (log: "Hooked MeleeWeaponEnvTrace"). Our `fire.cpp` hooks (IsFireReady,
  OnFirePerformed) come only once the pawn exists (17:50 in today's good run), so they are unlikely at 25 s.
  `[hypothesis]` A BP function forced "native" and called while its owner is half-built (a camera shake at the
  menu or the headset start) fits a read of 0xffff... / 0x0 inside the script VM. Not proven.

### 4. To settle it, when wanted

- Open `UEMinidump.dmp` of the 17:47 crash (1.1 MB, same folder) in a debugger with the game exe: name the UFunction
  in the ProcessEvent frame (its `Function` argument). If it is one of the camera-shake functions, the cure is the
  pack's (jbusfield), or a guard on our side.
- Count it: this crash at 22-25 s, against starts in that period (ride.log COLLECTING lines). If it is rare, the
  launcher could simply retry; the start is cheap.
- Nothing points at 0.1.2. No reason to hold back the 0.1.2 folder, and no reason to re-pack it.

## 14. The frame warp's views, and which hands went into which picture (2026-11-18, /pd, files and the fork's public source)

Full story: `modding-notes/2026-11-18-hands-at-the-headsets-rate-the-carry-by-the-drawn-hand.md`.

- The view the warp's call carries for the drawn eye is built in UEVR's open side (PureDark's fork, AFW branch,
  `FFakeStereoRenderingHook.cpp`) right after the plugins' `on_post_calculate_stereo_view_offset`, from the very
  location and rotation that callback sees: `position = (y, z, -x) / (world_to_meters * VR_WorldScale)` (the division
  in doubles, then floats), `view-to-world = translate(position) * yawPitchRoll(-yaw, pitch, -roll)`; `VR.cpp`
  `update_camera_data` copies it into `cameraData[eye].srcViewToWorldMatrix`, the other eye's into `dest`
  `[inferred-static 2026-11-18]`. So a UE world point `p` is `(p.y, p.z, -p.x) / scale` in the warp's world, and that
  reading agrees with Unreal's own rotation matrix within 0.04 mm over 20,000 random views
  `[verified-numerically 2026-11-18, tests/carry_test.cpp]`.
- The eye's place in a call is therefore a key the game thread can compute too: `drawnpose.cpp` matches each warp
  call to the tick that posed it that way. NOT yet seen live (the counts are in the log's `drawnpose:` line).
- The views are floats: 3 km from the map's origin a carried hand point is 0.6 mm (3 px) off `[verified-numerically]`.
- **Dead end, do not repeat:** carrying the made-up frame's hands by the raw controller's move read at the warp's
  call (b020-b029). It cannot follow a smoothed gun (11 to 30 px apart on quicker moves, flipping every other frame)
  and a read one frame off in time misses as much `[verified-numerically 2026-11-18, tests/handcarry_test.cpp]`.
- Hands at the HEADSET's rate while the game runs slower needs pictures handed to the headset between the game's
  frames, from a loop of our own in UEVR's open side: the size of an own frame warp. Not started (parked row).

### 14b. Seen live 2026-11-18 (simulator): the installed injector shifts the origin

- In play the views in the warp's call are the game's own minus a constant, different per load (310.995, 1.020,
  92.265 m in one save; 409.005, -181.453, 543.561 m in the armory; none in the main menu) `[verified-live 2026-11-18,
  n=2 saves]`. The August source of the fork does not do this: the installed beta 6 is newer than the public AFW branch.
  Anything of ours that goes into the warp's world must take the same shift off (`drawnpose.cpp` learns it).
- With the shift learned, every call's view matches exactly one of our per-tick records while the head moves, always
  2 records behind the newest `[verified-live 2026-11-18, n=3 starts]`. The reader's reading of the fork (inbox file of
  2026-11-18, folded in here): one stereo-offset call a tick, only for the eye being drawn; the call's view is the
  picture's own tick while the game thread is 0 to 2 ticks ahead of the present, else the view 2 ticks later
  `[inferred-static]`; plugins' callbacks run before the Lua loader's, so a script could still move the view after ours.
- NOT settled: whether the hands in the picture are that same tick's (the simulator's 160 px frames cannot tell
  "same" from "one later"; "one earlier" measured worst).

## 15. A turn that builds up: two shortest-way turns a view on the held gun (2026-11-19, /lm, simulator; note 2026-11-19)

- The two-handed long gun is a child of our arm mesh's `hand_r_bn`; `twohand::on_view` writes its world place at every
  view. Until b032 it read the gun where the last view had left it and made two shortest-way turns about the right
  hand's grip (aim at the left hand; meet the drawn left hand). Aimed at targets, so the direction never drifted, but
  the pair leaves a small turn about the hands' line each view and nothing took it out: **0.6 degrees a second with the
  left hand circling 6 cm, 11.9 degrees in 20 s, without end** `[verified-live 2026-11-19, simulator, n=1]`. Tefa in the
  headset: the gun "curving upwards ... stays in that position".
- **Rule for any per-view write on something the hand holds: start from a fixed place (its saved place in the hand),
  never from the last write.** b033 does (`TWO_HAND_FROM_ITS_PLACE`): the same circling, no creep in 30 s
  `[verified-live 2026-11-19, simulator, n=1]`. "Aims at a target" is not the same as "does not build up": a
  shortest-way turn fixes two of the three angles and leaves the third to history.
- Dead end avoided: Tefa's wear log already showed the barrel within 0.1 to 2.8 degrees of the left hand throughout, so
  the smoothing and the drawn-hand record were not the cause; the board's planned test (smoothing off) would not have
  separated it.
- Measured for the first time, not explained: while two-handed and circling, the gun sits 15 to 16 degrees (mean) out
  of its place in the drawn right hand: the hand takes only part of the turn `[measured 2026-11-19, n=1]`.
- The hand carry for the warp's made-up pictures (section 14): a refused carry repeats the last good one for up to 3
  pictures since b033 (was: the raw controller's move at once); refusals are counted by reason in the log.
- DLSS Balanced from the start runs with the warp on (simulator, 3 runs); the close Tefa hit is the live change.
- The occasional start crash (section 13): 2 of 5 starts on 2026-11-19; the injector's last line both times was `Scene
  state passed to FSceneView constructor is null`.
- 2026-11-21: the same start crash again (1 of 1 VR start, simulator, b033, about 10 s after UEVR's entry, the same
  last line), with Lanes' new frame meter loaded beside every other layer; the meter is very likely not the cause
  (note `modding-notes/2026-11-21-tune-o-matic-first-run.md`) `[inferred, n=1]`.

## 16. Settings changed live, and DLSS (2026-11-21, /lm reader, files only; note 2026-11-21b)

- With DLSS on, `r.ScreenPercentage` is DLSS's inner size (Balanced 58, Quality 66.7 after a start)
  `[verified-live 2026-11-10 and 2026-11-13, n=1 each]`, and **every live change of it tears DLSS down and builds it
  again** (two creates, two releases in 50 ms) `[measured, n=5]`. Lowering it does make the frame cheaper (66.7 -> 50:
  16.5 -> 14.1 ms, real headset, warp on, 2026-11-13).
- DLSS rebuilds with the warp on: 5 of 5 survived before the hands' pass (b014/b016); 3 of 6 crashed since it (b028,
  b032 twice) `[measured]`; the hands' pass keeping depth and velocity a frame longer without a size check is the lead
  for `[ID SH2-019]` `[hypothesis, read in handpaste.cpp, untested]`.
- So: **no live change of the DLSS size or level while the hands' pass is on.** `sg.*` groups are safe live and do
  almost nothing here (within 0.15 ms, twelve changes); ray-traced reflections off (`r.Lumen.Reflections.HardwareRayTracing 0`,
  -1.05 ms) changed live twice without a crash.
- The real settings: `GameUserSettings.ini` `GFXSettings` (`Adv_Supersampling` 4 = DLSS, `Adv_DLSSQuality` 2 =
  Balanced, 3 = Quality, the other `Adv_*`). `[ScalabilityGroups]` (all 3) has no measured effect; the game writes its
  own values back over `user_script.txt`'s after UEVR runs it `[measured 2026-11-13 and 2026-11-19]`.

## 17. Plugin traps to check ours against (2026-11-21, from /sr; phunkaeg's VR Modding Playbook ch. 18) `[reported]`

An object lookup that misses scans the whole object array (~105 ms; a periodic miss = a regular hitch); UEVR places the
hands by the view's yaw only; Blueprint-only functions never reach UEVR's function hook; VR controller input reaches
only the lowest polled XInput slot; additive UI writes no alpha (vanishes in the headset) and an undepth-tested
reticle shimmers under DLSS; a newer MSVC runtime than the game's can make the first `std::mutex` lock throw. None is
known to be wrong in ours; the first is the cheap check for any regular hitch Tefa reports.
<https://github.com/phunkaeg/vr-modding-playbook/blob/main/docs/18-beyond-the-native-injector.md>

## 18. The Optimizer's first run: frame times under the warp, the way into play, the saves (2026-11-21, /lm; note 2026-11-21c)

- **Under the frame warp, one OpenXR frame per game frame.** An OpenXR frame meter's frame interval is the game's own
  frame time: it matched the plugin's `alive: ... ms avg` within 0.1 ms (15.4 ms in the armory) and the warp-call count
  within 0-4 per 10 s `[measured 2026-11-21, n=2 runs; the reader's 21 windows]`. UEVR waits and ends a frame on every
  Present; the warp makes up the other eye inside that frame, it adds none `[inferred-static, the AFW fork's public
  source]`. So "late" is judged on the meter's interval (mean rate and the slowest 1%); `stat unit` is not needed.
  The simulator never pushes a late frame to the next slot, so a "late %" against 1.5 frames under-reports there; the
  slowest-1% rule still catches it. Half-rate stretches to avoid measuring across: 90 frames after the warp comes on,
  90 after any DLSS rebuild, and the flat cutscene mode `[inferred-static]`.
- **Correction to section 6:** the profile's `VR_SynchronizationMode` is 2 (Very Late) and `VR_RenderingMethod` 3 now
  `[measured 2026-11-21, config.txt]`, not Early (0). In mode 2 the meter's "work" column is the present thread's whole
  cycle, not the game's work.
- **Simulator numbers, b033, DLSS Balanced, connector in:** the armory 60-67 a second, slowest 1% at 43-50, the card
  55-60% busy (7 measures of the same room: the spread is the noise) `[measured 2026-11-21, n=7]`; the town start
  (outdoors, fog) 76-85 a second, the card 70-74% busy `[measured, n=1, 20 s]`. The armory is the hard spot and it is not
  the card that limits it on the simulator.
- **Into play with the connector alone:** Enter until `MainMenuWidget_BP_C /Engine/Transient` exists, ContinueGame on
  it, wait for `SHCharacterPlay_BP_C`, Enter at PRESS ANY BUTTON TO PLAY (twice: the first is often early). No VR
  trigger needed `[verified-live 2026-11-21, n=10 starts]`. The connector cannot pull a VR trigger.
- **No god mode:** console `god` does nothing (no CheatManager; `EnableCheats` makes none) `[verified-live, n=1]`.
- **The "Scene state passed to FSceneView constructor is null" line is not a crash sign** on its own: written in a start
  that ran on. `[Exception Handler]` is.
- **Continue follows the slot last used, not the newest file:** with the newest file replaced by slot 0, then slot 1,
  Continue still loaded the prison armory; with slot 0 put over every slot, it loaded the town start
  `[verified-live 2026-11-21, n=1 each way]`.
- **The save slots on the Build PC** (the reader, from the files' strings): 0 and 1 the town start (one spot), 2 the first
  Lying Figure fight (live enemy saved: never unattended), 3, 4 and 6 the prison armory (one room), 5 the room after the
  garage, 8 m from slot 2's fight. Detail kept in `engine-research/2026-11-21-reader-save-slot-labels.md`.
- **The start crash: UEVR's Ghosting Fix keeps a scene state the game frees** (2026-11-22 12:4x, /lm, simulator): with
  Window View's layer OFF the crash still came 3 of 11, so the entry below is `[disproved]` for the crash. All six dumps
  crash on the render thread through the object UEVR last logged as "Setting scene state to" (5 of 5 with a full log)
  `[measured]`; UEVR's `VR_GhostingFix` borrows the game's scene state for eye 1 and never checks it is alive
  `[inferred-static, the beta 6 dll]`. **Tested 14:04-14:37: `VR_GhostingFix=false` (b034) = 20 of 20 clean simulator starts,
  no scene-state lines in the log `[verified-live 2026-11-22, simulator, n=20]`.** The package's profile still ships true. Note `modding-notes/2026-11-22-start-crash-is-not-window-view.md`.
- `[disproved 2026-11-22 for the crash]` **The start crash: very likely Window View's OpenXR layer** (2026-11-22, /lm, real headset, 31 menu starts with the
  other OpenXR layers switched off one by one): with Window View's layer on, 8 start crashes in 19 and 4 starts where the
  frame calls skipped the layers below it after the first frame; with it off, 12 of 12 clean `[measured, n small]`.
  Tefa: a crash may have come when the game window was clicked out of focus (maybe a coincidence). Section 13's crash
  reading still stands; this is what most likely sets it off. Table: the-optimizer `docs/first-run-silent-hill-2-2026-11-21.md`.

## 19. The warp's mode 2 and near things: the 1 m rule (2026-11-22, /lm reader, files only)

Live verdict the same day: Tefa's "dome" was mode 2 itself; mode 3 cures it for his eyes `[reported, Tefa, worn, n=1]`. Mode 3 is what b041 and later set.

The reader's note, as written (folded from `inbox/2026-11-22-pd-warp-mode-2-and-game-driven-camera-motion.md`):

From: the background reader of the 2026-11-22 live session. Nothing launched. Read: notes 2026-11-13c/d/e, the
profile's `config.txt`, the b038 wear log, PureDark's PUBLIC fork (branch AFW, commit fc8de10, 2026-08-20, fetched
fresh), and the warp's GPU programs in the installed beta 6 `PDAFWPlugin.dll` (PureDark's permission of 2026-11-13,
this game only; findings in words only, nothing extracted is kept in git).

The video measure this answers: with the view moved by the game (stick turn, walking sideways in front of a door) the
world steps on every other headset frame (36 a second); when the head turns, every frame moves.

### 1. Measured in the b038 log

- 716-720 warp calls every 10 s in play = 72 a second, one made-up eye per call. So each eye is real 36 times and
  made-up 36 times a second: "36 steps a second" is exactly "the made-up pictures do not move the world"
  `[measured: the `drawnpose:` lines, 16:40-16:55]`.
- The DRAWN eye's view in every warp call matched one of our per-tick records of the game's own view, with one fixed
  shift per load: 0 unmatched in every 10 s window of play (33-35 only in the window around each load)
  `[measured]`. So the views the injector hands the warp DO contain the stick turn and the walking, tick by tick. The
  made-up eye's target view is not something our log sees.
- The picture was 1.0 to 2.0 records behind the game thread, changing through the run (simulator, 2026-11-18: always
  2) `[measured]`. Harmless for the matching; noted because it is new.

### 2. Which code decides `[inferred-static]`

**The open side (public August source; the installed beta 6 is newer, so this is "as far as the public source goes"):**

- The made-up eye's target view is built in the SAME callback and from the SAME game camera place and turn as the
  drawn eye's (`FFakeStereoRenderingHook.cpp`, `view_inv_matrix_other`). Nothing there leaves the stick or the walking
  out.
- `VR_AFW_UltraResponsive` only switches off the smoothing of things attached to the controllers; the fork's own menu
  text says so ("This basically just disables lerping in UObjectHook Config tab"). It does not touch the warp.
- `VR_GhostingFix` decides two things in the warp's call: the kind of motion vectors the warp is told it gets
  ("this eye's last frame" when on, "the other eye's last frame" when off), and, together with
  `VR_AFW_FixObjectMotionVector` and `VR_AFW_FixMovingObjectBrightnessFlickering`, whether the game's velocity picture
  is handed over at all (`D3D12Component.cpp`, the lines around `EvaluateFrameWarp`).
- `VR_AFW_FramewarpMode`: 0 none, 1 other eye, 2 previous frame, 3 both. `VR_AFW_FixObjectMotionRange` and
  `VR_AFW_IgnoreMotionThreshold` (default 2.5, not in our config) are passed straight through.

**The closed side, mode 2 (the four programs that place pixels; the same branch in each):**

- A pixel of the eye's previous real picture is put back into the world with that eye's PREVIOUS view and projected
  with its view NOW. Both turn and place are in those views, so stick turn and walking are carried, for every pixel
  **further than 1 m**.
- **Nearer than 1 m the previous view is first moved to the eye's place now: only the turn is applied, every
  sideways or forward move of the camera is dropped.** This confirms the 2026-11-13e reading, and adds the condition:
  it applies to every near pixel when the warp's "eye mask" is off, and only to near pixels the warp has marked as
  MOVING when the mask is on. Near pixels marked still then get the full treatment.
- A pixel is marked moving when its motion vector differs from the motion the camera alone would give (worked out
  from the warp's own views) by more than `IgnoreMotionThreshold` / 2160 of the picture; nearer than 0.5 m the bar
  rises toward 10 / 2160.
- The warp's "previous view" is kept inside the closed file (the call only carries the views of now). Whether the
  mask is in use is also set there. Neither can be read from the open side.
- Two settings named `_IsMoving` and `_IsRotating` exist in the warp's settings block; none of the placing programs
  reads them.

### 3. Is "the world holds on made-up frames" expected for mode 2?

- **Walking sideways close to a door or a wall: YES, by design, if it is nearer than 1 m** (and marked moving, or
  the mask is off). The made-up picture turns it with the view but does not slide it. `[inferred-static]`
- **A stick turn: NO.** By the program a turn is carried on every path, near and far, and on 2026-11-13 the simulator
  showed the room moving evenly on every frame of a stick turn in mode 2 (`[verified-live 2026-11-13, simulator,
  n=7]`, beta 6, before our hands' pass existed). If far things also stand still on made-up frames during a stick
  turn in the b038 video, something outside what was read is wrong: the warp's kept "previous view", or the marking
  (a whole view wrongly marked moving), or a difference between the real headset and the simulator.
- Head turns looking smooth proves little: the headset's own last-moment turn correction smooths head turns on any
  frame, made-up or not `[hypothesis]`.
- What differs from 2026-11-13: the ghosting fix is now switched on 20 s after the start instead of from the start
  (b036), ray tracing is off, the game holds a full 72, and our hands' pass runs. Tefa on b035 (fix OFF, otherwise
  the same): "running super smooth"; on b038 (fix ON): "picture jittery/tearing" `[reported]`. That points at the
  ghosting fix's side of the call before anything else `[hypothesis]`.
- Our own pass does not touch anything further than 1 m (`handpaste.cpp`: every test starts with "nearer than the
  near line") `[inferred-static]`.

### 4. What the 46-setting sweep found about it

Nothing: it could not. The sweep of 2026-11-13 (note 13c section 6) had the headset lying still in one view and
measured frame time and a still-picture difference; all 46 settings were the game's own drawing settings. The warp's
settings were only timed (same note, section 3): Ultra Responsive off: no change in frame time; mode 1 for 3: -0.4 ms;
shading rate: -0.5 ms. No test has ever looked at how the world moves on made-up frames under stick or walking on the
real headset. `[measured, from the note and the sweep's own list]`

### 5. Cheapest live A/B (lines in `sh2vr_devcvars.txt`, 20 s each, same corridor, a steady stick turn, then a sideways walk)

First, from the video, with no game: **is the thing that stands still nearer or further than about 1 m?** Far things
standing still in a stick turn = the unexpected case. Only near things, only when walking = the 1 m rule.

1. `uevr VR_AFW_IgnoreMotionThreshold 100` (now 2.5). Raises the "moving" bar forty times. World moves on every frame
   now = the world was being marked as moving under game-driven motion. Put back to 2.5.
2. `uevr VR_GhostingFix false` (b035's state). World moves on every frame = the fix's motion-vector side. ⚠️ With the
   fix off no velocity picture reaches our hands' pass, and it then treats EVERYTHING nearer than 1 m as hand
   (`handpaste.cpp` `hand()`: no velocity picture = closeness alone): near walls and tables get carried with the hands
   and tear. That is very likely b035's "things close by are still tearing" `[inferred-static]`. For a clean look
   also set the 4th number of `sh2vr_warpscale.txt` to 0 for this step.
3. `uevr VR_AFW_FixMovingObjectBrightnessFlickering false`, then `uevr VR_AFW_FixObjectMotionVector false`: each takes
   the velocity picture away from the warp (same caution as step 2).
4. `sh2vr_warpscale.txt` 4th number 0 (`1.0 0 0 0 1.0 0.004 1 0`): our pass off. Expected: no change to the far
   world. A change = ours.
5. `uevr VR_AFW_FramewarpMode 1` for 20 s: the made-up eye is then built from the other eye's fresh picture, so the
   world must move on every frame. If it does there and not in 2, the fault is in the previous-frame path alone.
6. Mode 3 with the debug tint (Numpad 6 as a real key): green = marked moving. The whole world going green in a
   stick turn or a sideways walk, and not in a head turn, is the marking fault seen directly.

Each is undone by writing the old value back; none needs a restart.

### 6. Side finding for the start-crash notes

The "Inserting new scene state" / "Setting scene state to" lines are in the public fork
(`FFakeStereoRenderingHook.cpp`, about lines 3190-3220), so that code can be read as source. The "Setting" line is a
log-ONCE line: "one Setting line in the log" does not mean one swap. `[inferred-static, public August source]`

### Tags

- 72 warp calls a second, every drawn view matched `[measured, b038 log]`.
- Mode 2 drops the camera's move for near (under 1 m) pixels, keeps the turn, carries both further out
  `[inferred-static, beta 6's own programs]`.
- Ultra Responsive is not involved `[inferred-static, public source]`.
- Why far things would stand still in a stick turn `[not established]`; candidates `[hypothesis]`.

## 20. The hand's hop between real and made-up frames (2026-11-22, /lm reader, files only)

Written for mode 2. In mode 3 (b041 on) the hands are a different matter: section 22 and the note 2026-11-22c.

The reader's note, as written (folded from `inbox/2026-11-22-pd-hand-hop-between-real-and-made-up-frames.md`):

From: the background reader of the 2026-11-22 live session. Nothing launched. Read: `handpaste.cpp`, `drawnpose.cpp`,
`carry_math.h`, `smooth.cpp`, `smooth_math.h`, `warpscale.cpp`, `ik.cpp`, `ghosthand.cpp`, note 2026-11-19, and the
b038 wear log (`2026-11-22-1638-tefa-wear-b038/uevr-log.txt`, 16:38-16:55, 101 status windows of 10 s).
The file in force: `sh2vr_warpscale.txt` = `1.0 0 0 1 1.0 0.004 1 0` = scale 1, no mode switching, the hands' pass
on, near line 1 m, "moving against the room" 0.004, **7th = 1** (carry by the hand the game drew; a refused carry
repeats the last good one), **8th = 0** (the hands of the view's own record). The log's start lines confirm all of it
`[measured]`.

The measure this answers: the gun hand hops back and forth against the wall by about 6 px of a 1440-wide picture
(about 12 at full size) on alternate frames while moving; Ghost Hand: "the game's hand jumped 563 times while the
controller was calm; left hand wanders 3.2 cm".

### 1. How a made-up picture gets its hand (as the code stands)

Call N draws eye E for real and makes up eye X. The hand pixels of X's last real picture (tick N-1) are carried by
the hand's own rigid move from the record of tick N-1 to the record of tick N, between X's real view then and X's
target view now. A whole step, never half; both records hold the hand AFTER smoothing (`ik.cpp`: the record is
written after `smooth::apply`). So in one eye the hand should read: real N-1, carried to N, real N+1.

### 2. The ways it can disagree, with the log's verdict

| # | way | what the log says |
| --- | --- | --- |
| 1 | **The hand is right and the WALL is one tick old.** The carry puts the hand where it is at tick N; if the warp leaves the wall where it was at N-1 (the 1 m rule for near things, or the fault of the other inbox file), the hand hops against the wall by one tick of wall motion on every made-up frame, only while James moves. | Not something the log can see. Fits "while moving". Size by arithmetic: a sideways walk of 1.3 m a second, a wall 1.5 to 2.5 m away = 11 to 19 px a tick at full size. **Leading explanation** `[hypothesis]`. |
| 2 | A carry refused (no record, turned or moved too far, over 5 cm from the raw controller's) = a one-picture jump. | In play 0 to 16 refusals in about 1,440 carries per 10 s, typically under 5, and every one kept the last good carry (none fell back to the raw controller) `[measured]`. Far too rare for a steady alternate-frame hop. **Ruled out as the steady hop.** |
| 3 | No record for the picture, or the wrong one. | Every view matched in every window of play; unmatched only around the five loads (33-35 each) `[measured]`. **Ruled out.** |
| 4 | The carry is by the raw controller, not the drawn hand (7th number 0, or a silent fallback). | 1,420-1,440 of 1,440 carries a window "by the drawn hand" `[measured]`. The 7th number is in force. **Ruled out.** |
| 5 | The record is not the pose that was really drawn (so the "right" pair is the wrong pair). | Not seen by the log. The log's yardstick: the raw controller's carry differed from the drawn hand's by 1 to 7 mm on average per picture (0.5 mm in the calmest windows, up to the 50 mm cap) `[measured]`. 2 to 5 mm at arm's length is 6 to 15 px at full size: the same size as the hop. So IF the picture really follows the raw controller and not the record, this is the hop. Decided by one live flip (section 4, step 2). `[hypothesis]` |
| 6 | Smoothing on one end and not the other. | Not in the code: both records are after smoothing, the left hand is never smoothed at either end `[inferred-static]`. The `smooth:` lines (35 of them, strength 4, "turn per frame raw / drawn") read 0.5 to 1.4: no evidence either way, because "held still" there means under 20 degrees a second, which includes slow steady moves a filter passes unchanged `[measured, not diagnostic]`. |
| 7 | The record's tick is one off the picture's (the 8th number's subject). | Harmless at a steady speed: a constant offset cancels in the step between two records. It only shows under a change of speed, about 2 px for hand tremor or for James starting to walk (arithmetic). Cannot make 12 px. **Not the hop** `[inferred-static]`. The 8th number stays a test switch only. |
| 8 | A sign or convention error in the carry. | Would give an error that grows with walking speed and with every hand move, and the raw-against-drawn yardstick would sit far above 1-7 mm. `tests/carry_test.cpp` agrees with Unreal's own matrices within 0.04 mm. **Unlikely** `[inferred-static]`. |
| 9 | Half-step against full-step. | No half step exists in the code `[inferred-static]`. |
| 10 | The pixel is not recognised as hand in that picture, so the warp's own (one tick old, turn-only) hand shows. A hand pixel counts only when its motion over two ticks differs from a still thing's by more than 0.004 of the picture (about 5 px). | By definition this can only cost up to that bar: about 3 px a tick at full size. A shimmer at slow moves, not 12 px `[inferred-static]`. |
| 11 | The warp's leftover copy of the hand is not where our clean-up looks. `handpaste.cpp` `source_of()` assumes the warp lands EVERY near pixel by the turn alone. The warp's programs (read today, see the other file) do that only for near pixels it marked as moving; near pixels it marked still are landed with the camera's move as well. While walking, a hand the warp calls "still" is then left somewhere else (tens of px away) and not painted over. | Not in the log. Would look like a second faint hand or gun on alternate frames ("trails", "not solid"), not a hop of the one hand. The tint (4th number 2) shows it: a second gun with no red on it. `[hypothesis]` |
| 12 | The search for the source pixel stops within 0.002 of the picture (about 5 px) instead of exactly. | A few px of wobble at depth edges at worst `[inferred-static]`. |
| 13 | A long gun held in two hands turns about the grip toward the left hand at view time; the record holds the right HAND, not the gun (note 2026-11-19: the gun sits 15-16 degrees out of the drawn right hand while two-handed). The gun's pixels are carried by a blend of the two hands' moves. | Tefa carried the shotgun in this run (the log's weapon lines, 16:40 and 16:44). Affects the gun's far end, not a hand against a wall `[inferred-static]`. |

### 3. The Ghost Hand line is not about made-up frames

`ghosthand.cpp` writes, once a game tick, the real controllers (room) and the game's hand BONES (world, turned into
the room). It never sees a made-up picture. So "the game's hand jumped 563 times while the controller was calm" is
about the hands as the game itself draws them. Three things in the code produce such jumps with nothing wrong
`[inferred-static]`:

- the right hand is smoothed at strength 4 while a gun is held: it keeps moving after the controller has stopped;
- the left hand's bone is put on the gun's front grip while two-handed (`frontgrip.cpp`), away from its controller;
- the world-to-room turn is only refreshed while the hands are more than 15 cm apart, and it pairs the controller
  components (the pose of the previous view) with the headset runtime's pose of now: a stick turn or a walk with the
  hands together makes the game hand appear to jump in the room.

"Left hand wanders 3.2 cm" fits the second. The reader could not open the 16:38 report through the sanctioned tool
(the run name was not found; "latest" is a later, 114 s session with 4 jumps) and did not read the raw recording
(locked). To make the count mean something: `begone.py ask ghost-hand <that run> ...` for which hand jumps, and
whether the jumps sit in stretches where James walks or turns by stick.

### 4. What settles it, cheapest first

1. **From the video, no game:** measure the hand against the picture's own edge instead of against the wall, in a
   stretch where James walks with the head fairly still. Steady in the picture while the wall steps = way 1 (it is
   the world, the other inbox file). Hopping in the picture = the carry (ways 5, 11).
2. **Live, 20 s each, the 7th number:** `... 1 0` (now), `... 0 0` (raw controller's carry), `... 2 0` (b032's way).
   The hop smaller with 0 = the record is not what is drawn (way 5). Larger with 0 = the record is right.
3. **Live, the 4th number 2 (tint) while walking:** green = carried hand, red = painted-over leftover, magenta = no
   background found. A second gun or hand with no tint on it = way 11.
4. **Live, the 4th number 0 for 20 s:** our pass off. The hand then shows the warp's own one-tick-old hand on made-up
   frames; if the hop against the wall is the SAME size, our carry was adding nothing there and the wall is the one
   that moves wrongly.

### Tags

- Counts of carries, refusals, matches and the raw-against-drawn difference `[measured, b038 log, 16:40-16:55]`.
- Everything about what the code does `[inferred-static]`.
- Which way is the real one `[not established]`; way 1 first, then 5 and 11 `[hypothesis]`.

## 21. James's hair shadow was our shadow body in the head torch's light (2026-11-22, /lm reader, files only)

Acted on in b040 (the shadow copy off the torch's channel); the setting read back on the simulator, the look not yet judged by Tefa.

The reader's note, as written (folded from `inbox/2026-11-22-pd-hair-shadow-is-our-shadow-body-in-the-head-torch.md`):

From: the background reader of the 2026-11-22 live session. Nothing was launched; source, notes and the b038 wear log
(`2026-11-22-1638-tefa-wear-b038/uevr-log.txt` in the SH2 logs folder) only.
Tefa, b038, worn: "James hair shadow visible" (video: thin strands fanning from the top of the view on a prison
wall in the torch light, plus a small head-shaped blob).

### What shadows.cpp really does (read in `dev-archive/native/src/shadows.cpp`)

| piece | what is done to it | by what |
| --- | --- | --- |
| the body `CharacterMesh0`: 14 head material slots (0 1 6 8 9 11-18 22: lacrimal, teeth, eye shadow, face, facial hair, EIGHT hair slots, eyebrows) | set to the game's `MI_Invisible` | `SetMaterial` |
| the body's eyeballs (slot 10 keeps its material) | hidden by bone (`eye_l`, `eye_r`, `eye_Lt`, `eye_Rt`) | `HideBoneByName` |
| the body's own shadow | OFF in play | `SetCastShadow(false)` |
| **the shadow body** (a second skeletal mesh we add, following the body's pose) | hidden from the camera, **casts a shadow anyway** (`bCastHiddenShadow`), **holds the head's 14 real materials, hair included**, on lighting channels **0 and 1** | `make_copy()` |

So the head pieces are not "shadow off". The design is the opposite: the visible body casts nothing, and a hidden
full copy WITH the head and the hair casts the whole shadow. `[inferred-static, the source]`

The hair is eight material slots of the body mesh itself (`MI_JamesSunderland_Hair_LookDev_test` x8), not a separate
component `[measured: the log's slot list, lines 5249-5272]`. No groom or separate hair mesh is named anywhere in our
notes; nobody has listed the pawn's components for one `[not established]`.

### Why the strands show `[inferred-static; every link below is read from a file, the chain itself was not seen live]`

1. The torch's main light is on lighting channels 0 and 2; the game keeps James on channel 1 only, so his own torch
   never lights or shadows him (dossier section 8, `[verified-live 2026-11-06, n=1]`).
2. Our shadow body is put on channels **0 and 1** (`shadows.cpp`, `make_copy()`, the `SetLightingChannels(true, true,
   false)` line). The log confirms it both times a pawn was made: "casts hidden shadow 1, channels 110", and "the shadow
   body holds 14 of 14 head materials" `[measured: lines 5273-5274 and 6220-6221]`.
3. The pack's light location is "Head" (`light_type` 1 in the profile's `data/sh2_config.json`): the torch's light is
   attached to the headset with no offset (`main.lua` `attachLightToController`, hand 2). Note 2026-11-13c section 8 says
   the same: "the pack's Head light sits at the eyes".
4. So a light on channel 0 sits inside a hidden head whose hair is on channel 0 and casts shadows. The fringe hangs
   in front of the light: long strand shadows from the top of the view, and a piece of the head as a blob. With
   roomscale off the head model does not follow a lean, so leaning back or to the side puts more hair in front of
   the light.

Ruled out by the log `[measured]`: the game undoing our work in play. Body fixes are logged for the first three and
then every 50th; the first pawn (16:39:37 to 16:46:04) logged exactly two, both at its start, so nothing was put back
for six and a half minutes. The second pawn logged two at its start and a third only at the 16:54:59 cutscene (20
things: the planned switch to "head shown"). The body's own hair slots are invisible and its shadow is off.
Not ruled out: a hair piece we do not know of (see above); why this was not reported before b038 (the shadow body has
held the head's materials since the black-eyes fix of 2026-11-12) `[not established]`.

### The smallest fix

`dev-archive/native/src/shadows.cpp`, `make_copy()`: the shadow body on channel 1 only, exactly where the game keeps
the body it stands in for.

```
-    ue::call(c, L"SetLightingChannels", {true, true, false});
+    ue::call(c, L"SetLightingChannels", {settings::SHADOWS_COPY_ON_TORCH, true, false});   // 2026-11-22: off the torch's channel (the head torch sits inside the head: hair shadows on the wall)
```

plus `constexpr bool SHADOWS_COPY_ON_TORCH = false;` beside the other `SHADOWS_*` lines in `settings.h`.
Not compiled by the reader.

What it changes: the torch no longer draws any shadow of James's body or head (as in the flat game, which never
did). Every light that shadowed the game's own body still shadows the copy, now with the head: that was the point of
the copy. The weapon (channel 0 added by `weapon_channel()`) and our arms (channel 0) still shadow in the torch.
Not checked: which channels the room lights use; a room light on channel 0 only would lose the player's shadow, as
it does in the flat game.

If Tefa later carries the torch in a hand (`light_type` 2 or 3) and wants his body's shadow from it: channel 0 on the
copy only while the light is not on the head. Not needed for the fault.

### How it proves itself

- **In the log:** the existing line prints the copy's channels as read back from the game: it must say
  `channels 010` where b038 says `channels 110`. Cheap addition: in `body_fix()`, read the copy's channels each second
  and count a put-right if channel 0 is back.
- **Before any build, live through the connector (reversible, one call):** `SetLightingChannels(false, true, false)` on
  the component the log names ("the shadow body made on the pawn: ... (SkeletalMeshComponent ...)"), torch on, facing a
  near wall in a dark room: the strands and the blob go; calling it with `true, true, false` brings them back. That is
  the measure, act, re-measure for this change, and it also tests the whole chain above.
- **Logging gap worth closing while there:** body fixes 4 to 49 are silent, so the return from a cutscene (head
  hidden again) cannot be read in the log.

### Tags

- The copy is on channels 0 and 1 and holds the hair `[measured, b038 log]`.
- The torch light sits at the headset `[inferred-static: profile setting + main.lua]`.
- That this is what draws the strands `[inferred-static]`, to be confirmed by the one live call above.

## 22. What the warp's mode 3 ("both") does, its edge band, its debug colours, its options (2026-11-22, /lm reader, files only)

Live the same evening (note 2026-11-22c) `[measured, simulator, full-size pictures]`: with a hand moving, mode 3's made-up pictures show striped and speckled hands, a pale edge line and black holes beside them; none of FixObjectMotionVector, FixMovingObjectBrightnessFlickering, UltraResponsive, FixObjectMotionRange 1 / 8 changes that; our own hand pass run after the mode 3 call (b042 on) covers the hand and cleans the band and the holes. The reader's rim patch (staging) was NOT used: b042's clean-up takes the picture from beside the pasted hand instead.

The reader's note, as written (folded from `inbox/2026-11-22-pd-warp-mode-3-what-it-does-and-the-rim.md`):

From: the background reader of the 2026-11-22 evening session. Nothing launched, nothing attached. Read: the installed
beta 6 `PDAFWPlugin.dll` (sha256 5f3cfc38903ac438, the tools copy; PureDark's permission of 2026-11-13, this game only;
findings in words, nothing extracted is kept in git), its 12 warp programs (41-52) and the motion-vector program (30)
followed line by line, the native code that orders them (`EvaluateFrameWarp` -> the worker at 0x18001cfc0, the settings
writer at 0x18001a680, the pipeline table at 0x18001bae0), PureDark's public fork (branch AFW: `PDAFWPlugin.h`,
`D3D12Component.cpp`, `VR.cpp`), our `handpaste.cpp` / `warpscale.cpp`, the b040 log of 19:33, one sheet of Tefa's
debug clip (`2026-11-22-1933-tefa-dome-switches/video/sheet_18.jpg`).

Tags: what was read from code is `[inferred-static]`; what a log or a file shows is `[measured]`; the rest says so.

### 1. The answer first

- **The yellow rim is the warp's "edge band" class, not a hole and not hand** `[inferred-static]`. In mode 3 the warp
  throws away every pixel of this eye's last real picture that it calls "moving" (the old hand), which leaves a
  hand-shaped hole in the previous-frame picture. Around every such hole it then distrusts the still scenery beside
  it: a band about **1/390 of the picture's width (6-7 px at 2688) on the left and on the right of the hole, never
  above or below**, is taken from the OTHER eye's warped picture instead (where that eye sees it), or from this eye's
  old picture NOT moved at all (where it does not). Different eye, different fog and light, point-sampled: a fine line
  of slightly wrong picture just outside the hands' and sleeves' edge. That is what the debug view paints yellow.
- **The width is a fixed number inside the program (`RenderSizes.x * 0.0025641`). No setting changes it**
  `[inferred-static]`. `VR_AFW_FixObjectMotionRange` is NOT a pixel radius (section 4).
- **Lever most likely to remove it: our own pass, after the mode 3 call, putting this eye's own last picture back
  into that band** (moved by the camera, by depth). Written and compile-checked in staging (section 6); not run.
  `[hypothesis]` until seen in the headset.
- **Cheapest proof, no build, 20 s:** mode 3, our pass off, `uevr VR_AFW_IgnoreMotionThreshold 100`. Nothing is then
  called "moving", no hand-shaped hole, no band. Expected: the rim goes, and the hands go back to mode 2's one-frame
  lag (doubling while walking or turning). Rim gone = this reading is right. Put back to 2.5 after: the same number
  also feeds the motion vectors DLSS gets (section 4), so it is a test, not a setting to keep.
- **The speckle inside the green hands** (visible in the sheet: the hands' skin is grainy, the real frames between are
  smooth) `[measured, one sheet]` is most likely the "Fix Moving Object Brightness Flickering" path: each hand pixel's
  colour is looked up, one point per pixel with no filtering, in this eye's last real picture at a place worked out
  from the game's velocity picture `[inferred-static for the mechanism; hypothesis that it is the speckle]`. The live
  session's `bright_off` clips test exactly this.

### 2. Which programs run, per mode `[inferred-static]`

The call's `UseUINT64` byte is 0 in our log (word 16 of the call = `9f345d100`: byte 0, shading rate 9 = 1x1)
`[measured, b040 log]`, so the plain programs run, not the 64-bit ones (47/46/45 are the same logic).

| mode | programs, in order | reads |
| --- | --- | --- |
| 1 other eye | 52 -> 51 -> 44 | the drawn eye's colour and depth. No motion vectors, no velocity, no masks. |
| 2 previous frame | 52 -> 51 -> 44 | the warp's own kept copy of this eye's last real picture and depth ("Backup"). `UseEyeMask` 0, `UseMotionVectors` 0: no motion data at all, and every pixel nearer than 1 m gets the turn only. This is why no motion setting ever changed mode 2. |
| 3 both | **42**, then previous frame: 52 -> 51 -> 44 -> **48**, then other eye: 52 -> **50** (51 when no velocity picture is handed over) -> 44 -> **48**, then **49** | everything: both pictures, both depths, the game's motion vectors (42), the game's velocity picture (50), the kept copy (50, 49). |

What each does:
- **42, who is moving:** per pixel of the drawn eye, the game's motion vector minus what the camera alone would give.
  More than `IgnoreMotionThreshold / 2160` of the picture = "moving" (mask 10), else "still" (1). Nearer than 0.5 m
  the bar rises toward 10 / 2160 (about 13 px at 2880) at zero distance, so a hand close to the face needs a faster
  move to count, and parts of one hand can fall on both sides of the bar.
- **52 / 51, the throw:** every source pixel is thrown forward to where it lands in the made-up view; the nearest one
  wins; point to point, no filtering. In the previous-frame pass of mode 3 the mask is ON: **pixels marked moving in
  this eye's last real picture are not thrown at all**, and still pixels get the full camera move with depth at every
  distance. The 1 m "turn only" rule is dead in mode 3. That is why Tefa's "dome" is gone in mode 3.
- **44:** fills one-pixel cracks from the nearest neighbour.
- **48, the edge marks:** in the previous-frame result, a covered pixel with a hole 6 and 7 px to one side gets mark
  1, to the other side mark 2; in the other-eye result the same gives mark 5. Left and right only.
- **50, the moving things in the other-eye pass:** a moving pixel is PLACED by the other eye's fresh depth (so the
  hand is where it is now), marked 10, and COLOURED from this eye's kept last picture at the spot the velocity picture
  points back to (one point, unfiltered). Without the velocity picture (51): coloured straight from the other eye.
- **49, the merge**, per pixel, first rule that fits (previous-frame result = P, other-eye result = O):
  1. mark 10 (moving): take O (colour and depth). Nearer than 0.1 m: keep P.
  2. hole in P, and O has something nearer or valid: mark 5 -> this eye's old picture, unmoved; else -> O.
  3. mark 1 and O covers it -> O. Mark 2 -> O, or the old picture unmoved when O has nothing.
  4. O nearer than 3 m where P is more than 10 m further -> O.
  5. otherwise P.

### 3. The debug view's colours `[inferred-static, program 49; the tints are added to the picture]`

| colour | pixel class |
| --- | --- |
| **green** | marked moving (10): placed by the other eye's depth, coloured as 50 / 51 says |
| magenta | moving and nearer than 0.1 m: the previous-frame picture kept (`[verified-live 2026-11-13]` in the notes) |
| **yellow** | the edge band: mark 1 taken from the other eye; or mark 5 / mark 2 with nothing in the other eye, taken from this eye's old picture unmoved |
| white-ish (all lighter) | mark 2 (the band's other side) taken from the other eye |
| red | a hole in the previous-frame picture filled from the other eye (the pink bar at the picture's edge and the red lines top right in the sheet's last rows) |
| blue | near thing in the other eye over far background |
| cyan | only with a foveated area set |

So: green hands = "the warp knows they move" (our arms' velocity is fine, as on 2026-11-13). Thin yellow rim = the
band and the fallbacks of rules 2-3. From the code alone the band (other eye's colour) and the fallback (old hand
sliver, unmoved) cannot be told apart; in the sheet the yellow sits mostly along one side of each hand and sleeve,
which fits either `[measured, coarse]`. The small green dashes floating between the hands in the sheet are stray
background pixels over the motion bar.

With "Debug Framewarp" on, **Ctrl + Numpad-minus** steps through 6 raw pages instead of the picture (motion vectors,
the moving mask, depth, the drawn picture, the kept copy, the other-eye result) `[inferred-static]`. The dark,
speckled frames of the clip may simply be one of those pages or the untinted look of rule 1; not settled
`[hypothesis]`: ask Tefa whether he pressed anything.

### 4. What each option really changes `[inferred-static]`

| option | where | what | the rim |
| --- | --- | --- | --- |
| `VR_AFW_IgnoreMotionThreshold` (2.5) | program 42 (mode 3 only) and program 30 | the bar for "moving", in 1/2160 of the picture; raised near the face | higher = fewer things moving = fewer holes and bands, but those things lag a frame. 100 = nothing moves (the proof test). It also changes program 30, see next row. |
| `VR_AFW_FixObjectMotionVector` | open side, at the DLSS hook, only with the ghosting fix on | runs program 30: rewrites the game's motion vectors for moving things so they point two ticks back in the SAME eye (each eye's DLSS runs at half rate). The rewritten vectors go to DLSS and to program 42. Also one of three conditions for handing the velocity picture to the warp. | indirect only |
| `VR_AFW_FixObjectMotionRange` (3.0) | program 30 | a DISTANCE IN METRES from the eye: nearer than this, the "is it moving" bar of program 30 is 2.5 times finer (threshold / 5400 instead of / 2160). Not a pixel radius, nothing is widened around anything. The header's own comment: "for first person view this is preferred to be set to 0.5". | none directly |
| `VR_AFW_FixMovingObjectBrightnessFlickering` | the call (with ghosting fix and the option above) | hands the velocity picture over: program 50 instead of 51, so moving things are coloured from this eye's old picture instead of the other eye's. Mode 3 only. | none on the band; it is the suspected speckle. ⚠️ Off = no velocity picture reaches OUR pass, which then treats everything nearer than 1 m as hand. |
| `VR_AFW_UltraResponsive` | open side | switches off the smoothing of things attached to controllers. Never reaches the warp. | none |
| "Clear Before Framewarp" | the worker | clears the output picture before the passes. Pixels no rule writes show black instead of this eye's old picture. | a diagnostic: lines that turn black were "nobody wrote here" |
| "Framewarp Shading Rate" | mode 3 only | above 1x1 the passes run on a smaller picture and 49 looks the colours up at full size | band and masks get blockier, not narrower |
| sharpening | open side, before the call | sharpens the real picture (it uses the other eye's buffer as scratch; harmless, the warp reads its own kept copy) | none directly; off in the profile |

### 5. A mode 3 call, for our pass

- **Same layout, same textures** `[inferred-static, public header and call site; the log's call words agree]`. One
  struct for every mode: list 0, in-buffers 8, out-buffer 16 (filled by the warp), mode 60, eye 64, camera 72, clear 80,
  threshold 84, motion-vector type 92, debug 96, velocity 120, UINT64 128, shading rate 132. Mode arrives as 3.
- **The output is the same picture** as in mode 2: the other eye's frame buffer colour; the open side sends that to
  the headset whatever the mode `[inferred-static]`.
- **The warp's "kept copy" is the same picture as our `g_keep`**: the made-up eye's last real one.
- **No reason the pass cannot run** `[inferred-static]`. The live session has already switched it in (9th number,
  `c.mode`, `cr.w`). Two things differ from mode 2 and are worth knowing:
  1. Where our pass expects "the warp's leftover copy" (the turn alone), mode 3 has the hole of the dropped old hand
     instead (filled by rule 2). Painting it over is harmless and replaces any old-hand sliver. But a hand OUR test
     calls hand and the WARP calls still (a slow hand: our bar is about 5 px over two ticks, theirs about 3 px, more
     near the face) is left by the warp at its old place moved with the full camera move, not the turn alone; while
     the head moves sideways our paint-over then looks a few px beside it `[hypothesis]`.
  2. The warp's own mode 3 hand is already at the right place (other eye's fresh depth). What ours adds is the
     picture: this eye's own last real hand, carried rigidly and filtered, instead of point lookups.
- **Which picture to take the hands from:** this eye's previous real picture, as now. The other eye's is newer but
  seen from 6 cm to the side: at arm's length that is a shift of the order of 100-200 px with whole flanks of the hand
  this eye sees and that eye does not. PureDark's own program 50 makes the same choice.

### 6. The proposal, concrete (staging, not applied)

`staging/silent-hill-2-remake-vr/2026-11-22-warp-mode-3-hands/`: `rim.patch` (162 lines, against the live session's
working files as they stood at about 21:30: handpaste.cpp d148a65b, handpaste.h f4f5f425, settings.h 9c55693b,
warpscale.cpp 6f301317; `git apply --check` passes), `src/` (the four files patched), `handpaste_with_rim.hlsl`,
`check.bat`. `[compile-verified 2026-11-22]`: the pixel program with fxc ps_5_0 (1369 instruction slots, was 894),
both .cpp with cl /W4 /Zs against the project's headers, no warning. NOT run.

What it adds on top of the live session's mode 3 switch:
- `settings.h`: `HANDPASTE_RIM` (0 = off). `warpscale.cpp` `read_file()`: a 10th number in `sh2vr_warpscale.txt` =
  the band's half width as a share of the picture (try 0.0034; the warp's is 0.0026). `handpaste.h`: `set_rim()`.
- `handpaste.cpp`, the numbers: 12 more floats `rm0..rm2` (a still point in front of this eye then -> now: the
  made-up view times the last view turned back), and `fr.w` = the rim width (0 unless the call is mode 3). The array
  grows 112 -> 124 floats, inside the 512-byte slot (a static_assert guards it).
- `handpaste.cpp`, the program: `rim(uv)` = one of 8 points to the left and right within the width is the old hand's
  place; `still_src(uv)` = where that still spot was in this eye's last real picture (4 steps, by depth, the sky
  guarded); and in `ps`, after the paint-over block: `else if (cr.w > 0.5 && fr.w > 0 && rim(uv))` -> this eye's own
  last picture from `still_src`, unless that spot is hand. Tint: blue.
- Order of steps (the "which runs last" line): the warp writes its picture, then our pass; the band is decided inside
  the warp (program 49) and ours runs after it on the same list, so ours has the last word on those pixels.
- Not covered: bands beside holes that are not the hands (any edge while the head moves); up to 12 extra depth reads
  per pixel over the whole picture when the rim is on (cost not measured).

### 7. Live checks, cheapest first (20 s each, mode 3)

1. `uevr VR_AFW_IgnoreMotionThreshold 100`, our pass off: rim gone and hands lagging = section 1 confirmed. Back to 2.5.
2. "Clear Before Framewarp" on: any line that turns black is "nobody wrote here".
3. Debug view, hand held still then moved sideways: yellow only left and right of the hand, never on a top or bottom
   edge = the band of program 48.
4. `uevr VR_AFW_FixMovingObjectBrightnessFlickering false` with our pass OFF (4th number 0): speckle inside the hands
   gone = program 50's lookups (the live session's `bright_off`).
5. The staging patch, tint on (4th number 2), 10th number 0.0034: blue where the yellow was; then tint off for Tefa.

### Not established

- Whether the rim Tefa sees is mostly the other-eye band or the unmoved old-picture fallback.
- What the dark, speckled frames of the clip are.
- Whether the descriptor blocks are laid out as assumed (colour, depth, motion, mask in a row); the reading of which
  texture is which rests on it and on the offsets matching the public `TextureDesc`.
- The crash 12 minutes into mode 3: nothing in this reading points at a cause.

## 23. The drawn eye is shown from the engine's scene render target, not from the warp call's texture (2026-11-22, /lm reader, files only)

Proven live in b044 `[verified-live 2026-11-22, simulator, n=1; real headset: it runs, the look not judged]`: a pass that writes into `StereoHook::get_scene_render_target()`'s left, eye-sized part from our stand-in reaches the drawn eye in every frame (test view of deband.cpp). The scene render target read 2560 x 1400 on the simulator and 5376 x 2880 on the real headset, format 90 (B8G8R8A8 typeless).

The reader's note, as written (folded from `inbox/2026-11-22-pd-reaching-the-drawn-eye-it-is-the-scene-render-target.md`):

From: the background reader of the 2026-11-22 evening session. Nothing launched. Read: PureDark's public fork, branch
AFW (the August source, commit fc8de10: `src/mods/vr/D3D12Component.cpp`, `src/mods/VR.cpp`, `src/Mods.cpp`,
`src/mods/pluginloader/FFakeStereoRenderingFunctions.cpp`, `FFakeStereoRenderingHook.cpp`), the plugin API headers in
`dev-archive/native/include/uevr/`, the profile's `config.txt`.
⚠️ The installed beta 6 injector is newer than the public source (dossier 14b). Line numbers are the public file's;
the order below agrees with what the live test showed (pass (a) reaches the headset, pass (b) does not).

### 1. The order of one frame in rendering method 3 `[inferred-static, D3D12Component.cpp `on_frame`]`

Everything happens inside `D3D12Component::on_frame`, called from `VR::on_present` (VR.cpp 2734), on the thread that
presents.

1. **Line 54:** `backbuffer` = the engine's scene render target: `m_fake_stereo_hook->get_render_target_manager()->get_render_target()->get_native_resource()`.
   This is the texture the game really drew the eye into: double wide, **the eye of this frame always in the LEFT half**
   (both branches below copy `left = 0 .. width/2`). The injector forces it to 8-bit BGRA (section 3).
2. **Line 511:** `cmdList = d3d12Renderer->BeginCommandList(index)`: the warp file's own command list.
3. **Line 528:** `Crop(cmdList, eyeFrameBuffer.color, backbuffer, left half)`: a COPY of the drawn eye into the warp's
   per-eye buffer (line 525-526 with sharpening on: crop into the other eye's buffer, sharpen into this one).
   **This copy is the texture at offset 8 of the call. It is only the warp's input. Nothing ever shows it.**
4. **Line 552:** `EvaluateFrameWarp(params)`: our stand-in, the real call, our passes. The list is still open.
5. **Line 571:** `d3d12Renderer->EndCommandList(index)`: the list is closed and run (it must be: our hand pass, recorded
   on it, reaches the headset through the copy of step 7).
6. **Line 596 (left-eye frame) / 688 (right-eye frame):** `m_openxr.copy(AFR_LEFT_EYE or AFR_RIGHT_EYE, backbuffer.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, &src_box)`:
   **the drawn eye goes to its OpenXR swapchain straight from the scene render target**, left half.
7. **Line 603 / 733:** `m_openxr.copy(the other eye's swapchain, otherEyeFrameBuffer.color.pTexture, ALL_SHADER_RESOURCE, &src_box)`:
   the made-up eye, from the warp's buffer (and its depth from the warp's depth).
8. **Line 866:** `xrEndFrame`. One OpenXR frame per game frame, both eyes in it.

`OpenXR::copy` (line 1886) is a plain `CopyTextureRegion` on the swapchain picture's own small command list, run at
once on the same queue: no resample, no format change. So what the live test saw is exactly the code: (b) changed a
copy nobody shows.

### 2. The cheapest reliable way to change the drawn eye

**Where:** the place we already stand in, after the real call, on the same open list (step 4). It is before step 5
and 6 in the same function, so it is ahead of the swapchain copy on the queue: the same ordering that already carries
pass (a). "Which step runs last": the drawn eye's swapchain copy is step 6; ours is step 4; nothing writes the scene
render target in between.

**What to write to:** the scene render target itself. The plugin API hands it out, and it is literally the same
getter as line 54 `[inferred-static, FFakeStereoRenderingFunctions.cpp]`:
`uevr::API::StereoHook::get_scene_render_target()` -> `->get_native_resource()` = `ID3D12Resource*`
(API.hpp about lines 1187 and 1686). Only true while `VR_ExtremeCompatibilityMode` is false; it is false in the
profile `[measured, config.txt line 78]`.

**How, least risk:**
- Read from the call's own colour texture (`c.color`: the same pixels, a 1:1 crop, sharpening is off in the profile),
  whose state we already know and handle; draw the deband pass into our own target; then
  `CopyTextureRegion(scene render target, 0, 0, 0, our target, box 0..eyeW x 0..eyeH)`.
- The scene render target then needs one barrier pair only: `RENDER_TARGET -> COPY_DEST -> RENDER_TARGET`.
  `RENDER_TARGET` is the state the injector itself names for it in step 6 (and the API header says the same of its
  framework target). ⚠️ The injector is not consistent about it: for the crop of step 3 it tells the warp file the
  same texture is in `PRESENT`. No debug layer runs, so both pass; we should copy the injector's step-6 claim, since
  that is the transition recorded right after ours.
- Checks before touching it, every call: pointer not null; its `GetDesc()` width is 2x (or 1x) the eye picture's
  width and the height equal; format in the 8-bit BGRA family (copy needs the same family as our target). Get the
  pointer fresh every call (the engine replaces the target at a size change or a load), on this thread, as line 54 does.
- The warp's own kept copy of this picture was taken inside the real call, before us: the made-up pictures stay built
  from the untouched picture and get the pass once on their way out, as now. Every shown picture gets it exactly once.

**What does NOT work** `[inferred-static]`:
- `on_present`: the mod order is FrameworkConfig, VR, UObjectHook, PluginLoader (Mods.cpp 14-29), and `Mods::on_present`
  runs them in that order: the plugins' `on_present` comes after `VR::on_present`, that is after `xrEndFrame`. Too late.
- `on_post_render_vr_framework_dx12`: that is the injector's own menu picture, not an eye.
- Another import slot: the crop and the swapchain copy are not imports. The crop is a call through the warp file's
  renderer object (the public header's `D3D12RendererAPI`, slot 27 `Crop` by the header's order); standing in that
  would put a pass BEFORE the warp sees the picture. Possible, more invasive, and the made-up pictures would then be
  warped grain. Second choice only.

### 3. Where the dither goes: what the injector does, and a guess about the engine

- **No resample and no conversion in the injector for the drawn eye**: scene render target -> `CopyTextureRegion` ->
  swapchain (`B8G8R8A8_UNORM_SRGB`, line 1661). For the warp's input: a 1:1 crop. `[inferred-static]`
- **The injector forces the game to 8 bits** `[inferred-static]`: at hook time it finds `r.DefaultBackBufferPixelFormat`
  and writes 0 into it (FFakeStereoRenderingHook.cpp about 1307-1314, comment "8bit RGBA, which is what VR headsets
  support"), and where there is no such setting it forces the render target's format to `PF_B8G8R8A8` (about 6481,
  6831, 7014, 7037). Flat, this game most likely shows 10 bits.
- **Guess, not checked against the engine's source or a capture** `[hypothesis]`: the tonemapper's "grain
  quantization" is sized for the format the tonemapper itself writes. If it is not the last pass in this game (passes
  after it, or an in-between target of 10 bits), the dither is a quarter of an 8-bit step and the last copy into the
  8-bit target rounds it away: clean one-level contours, exactly what the parity view shows. What would settle it
  without the game: an existing PIX / Nsight capture of one frame (the pass list after the tonemapper and the formats).
- **A fix at the source, if that is the cause** `[hypothesis]`: a very small film grain (the engine adds it inside the
  tonemapper, before the rounding; an amplitude of about one 8-bit step survives the 8-bit copy) set by our plugin on
  the camera's post-process settings. It would be in every real picture and ride along into the made-up ones, no
  pass at all. It is a screen effect by name, but at one level it is invisible as grain. Needs the game running to
  try: `r.FilmGrain 1` plus the intensity.
- Not ours, worth knowing `[hypothesis]`: Virtual Desktop's video stream is a second place where dark fog can band
  (its 10-bit codec option). The parity view on the simulator proves the steps are already in the picture, so this
  would only add to it.

### Tags

- The order of the frame, which texture each eye is copied from, the getter being the same object `[inferred-static,
  public August source]`.
- (a) reaches the headset, (b) does not `[measured, the coordinator's live test, b042]`: agrees with the order.
- Why the dither is lost, and the film-grain fix `[hypothesis]`.
