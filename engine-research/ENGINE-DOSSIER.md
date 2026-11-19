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
