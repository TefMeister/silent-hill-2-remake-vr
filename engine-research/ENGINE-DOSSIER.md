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
