# Anti-tamper static audit of the SH2 (2024) shipping executable — clean, injection should work

Author: background reader for the /lm session, 2026-09-19, dev PC `DESKTOP-V8GTSIR`.
Answers the `[PD]` board row **"anti-tamper: find out before building anything."**
Method: **static file analysis only.** The game was never launched, no process was touched, no
debugger attached. Everything below was produced by reading the file on disk.

File audited:
`E:\SteamLibrary\steamapps\common\SILENT HILL 2\SHProto\Binaries\Win64\SHProto-Win64-Shipping.exe`
142,126,592 bytes, FileVersion 1.0.0.5, PE TimeDateStamp 2025-01-21 15:22:49 UTC.
Tools: Python 3.12 + `pefile` 2024.8.26 + `capstone` 5.0.7, plus PowerShell `Get-AuthenticodeSignature`.

---

## The headline

**No anti-tamper, no packer, no protector, no anti-cheat, no DRM wrapper is present in this build.**
`[measured 2026-09-19, pefile 2024.8.26 + capstone 5.0.7]`

**Verdict: injection is likely to work.** The binary is an ordinary, unprotected, unobfuscated MSVC
release build with a complete import table and 769 exported symbols. There is nothing in it that
would resist a DLL being loaded into the process or a function being hooked.
`[inferred-static 2026-09-19]` — inferred, because "likely to work" is a claim about runtime
behaviour and nothing has been run. See **What would actually prove it** below.

---

## ⭐ Bonus finding the board also wanted: the UE version is **5.1**

The version resource carries a second, engine-level block:

```
ProductVersion = ++UE5+Release-5.1-CL-0
InternalName   = SHProto
FileDescription = Noise            <- the internal project name
CompanyName    = Konami Digital Entertainment Co., Ltd.
```

So the shipping build is **Unreal Engine 5.1** `[measured 2026-09-19, version resource]`.
That closes the "note the UE5 minor version if it can be read statically" half of the
*is the game actually installed* `[PD]` row. The game-side application version is separate:
`FileVersion = 1.0.0.5 (21/01/2025)`, `CompanyName = Konami`, `ProductName = SilentHill`.

⚠️ `[hypothesis]`: that UEVR handles 5.1 well. Nothing here tested that; it is only noted because
the board says UEVR behaviour differs per UE version, so the number was worth extracting.

---

## The evidence, item by item

### 1. Sections are stock MSVC; entropy is ordinary

```
name       VA           VSize        RawPtr       RawSize      Entropy    Flags
.text      0x1000       0x5e3e5b7    0x600        0x5e3e600    6.6206     0x60000020
.rodata    0x5e40000    0x7c0        0x5e3ec00    0x800        4.2344     0x60000020
.rdata     0x5e41000    0x1e9c200    0x5e3f400    0x1e9c200    5.2436     0x40000040
.data      0x7cde000    0x69db64     0x7cdb600    0x2c3600     3.9255     0xc0000040
.pdata     0x837c000    0x506994     0x7f9ec00    0x506a00     7.1705     0x40000040
.msvcjmc   0x8883000    0x8          0x84a5600    0x200        0.1161     0xc0000040
_RDATA     0x8884000    0x5d810      0x84a5800    0x5da00      5.7650     0x40000040
.rsrc      0x88e2000    0x2f224      0x8503200    0x2f400      7.2526     0x40000040
.reloc     0x8912000    0x258758     0x8532600    0x258800     5.4662     0x42000040
```

Every name is one MSVC emits. There is **no** `.vmp0/.vmp1` (VMProtect), no `.themida`/`.winlice`
(Themida/WinLicense), no `.bind` (Steam's DRM wrapper), no unnamed high-entropy blob section, and no
section marked writable+executable. `.text` averages 6.62 bits/byte, which is normal compiled x86-64.
`[measured 2026-09-19]`

I also scanned `.text` in 64 KB windows: **1,508 windows, mean 6.353, and only 3 windows above 7.2
bits.** Those three (file offsets 0x830600–0x850600, ~150 KB of 98 MB) were opened by hand and are
**OpenSSL**: the region literally contains the string
`Montgomery Multiplication with scatter/gather for x86_64, CRYPTOGAMS by <appro@openssl.org>`
followed by embedded key/constant tables. UE5 bundles OpenSSL, so this is expected. **It is not a
virtualised code region.** `[measured 2026-09-19]`

### 2. The entry point is the four-instruction MSVC CRT stub

```
0x145cbc3d0: sub  rsp, 0x28
0x145cbc3d4: call 0x145cc5c4c
0x145cbc3d9: add  rsp, 0x28
0x145cbc3dd: jmp  0x145cbc25c
```

That is textbook `mainCRTStartup`. A packed or protected binary has its entry point redirected into
a stub in a separate section that unpacks the real image first. This one does not.
`[measured 2026-09-19, capstone]`

### 3. The two TLS callbacks are the standard CRT pair, not anti-debug

TLS callbacks are the classic place to hide an anti-debug check, because they run before the entry
point and before a debugger can break. Both were disassembled:

- `0x145cbb354` — checks `edx == 2` (DLL_THREAD_ATTACH), then walks a callback array between two
  `lea rip`-relative bounds. This is MSVC's `__dyn_tls_init`.
- `0x145cbb4ec` — checks `edx == 3 || edx == 0` (thread/process detach), then runs a destructor list
  off `gs:[0x58]` TLS storage. This is MSVC's `__dyn_tls_dtor`.

Neither reads the PEB `BeingDebugged` flag, calls `NtQueryInformationProcess`, or does anything
timing-related. `[measured 2026-09-19, capstone]`

### 4. Control Flow Guard is instrumented but **not enabled**

Load config `GuardFlags = 0x100` = `IMAGE_GUARD_CF_INSTRUMENTED` only. The
`IMAGE_GUARD_CF_FUNCTION_TABLE_PRESENT` bit (0x400) is **absent**, and `DllCharacteristics = 0x8160`
does **not** carry `IMAGE_DLLCHARACTERISTICS_GUARD_CF` (0x4000). `[measured 2026-09-19]`

Plain words: the compiler left CFG's scaffolding in, but Windows will not enforce it at load time.
That is the friendly case for a mod — there is no guard table to update when we redirect a function
pointer or install a detour. `[inferred-static 2026-09-19]`

`DllCharacteristics 0x8160` decodes to HIGH_ENTROPY_VA + DYNAMIC_BASE + NX_COMPAT +
TERMINAL_SERVER_AWARE. ASLR is on, which is ordinary and only means addresses must be resolved from
the module base at runtime, never hard-coded.

### 5. The import table is enormous and completely normal for UE5

**1,015 named imports across 46 DLLs**, plus 17 delay-loaded DLLs. `[measured 2026-09-19]`

This is the single strongest signal. Packers and protectors collapse the import table to a handful
of entries (typically `LoadLibrary` + `GetProcAddress` + a few) and rebuild it at runtime. 1,015
imports is an unpacked binary, full stop.

Statically imported, abbreviated: `KERNEL32 (261)`, `USER32 (141)`, `MSVCP140 (128)`, `WS2_32 (38)`,
`GDI32 (32)`, `ADVAPI32 (28)`, `VCRUNTIME140 (27)`, `ole32 (16)`, `SETUPAPI (15)`, `CRYPT32 (15)`,
`tbb (14)`, `IMM32 (12)`, `HID (11)`, `OPENGL32 (6)`, `DSOUND (6)`, `WINMM (5)`, `SHELL32 (5)`,
`OLEAUT32 (5)`, `UIAutomationCore (5)`, `IPHLPAPI (4)`, `dwmapi (4)`, `VERSION (3)`,
**`dxgi (2: CreateDXGIFactory, CreateDXGIFactory1)`**, `d3d9 (2: D3DPERF_Begin/EndEvent)`,
`d3d11 (1: D3D11CreateDevice)`, `XINPUT1_3 (2, by ordinal)`, `WINTRUST (1)`, `bcrypt (1)`,
`UxTheme (1)`, `CFGMGR32 (2)`, `WINHTTP (2)`, and the usual `api-ms-win-crt-*` set.

Delay-loaded: `EOSSDK-Win64-Shipping (249)`, `ViconDataStreamSDK_CPP (34)`, `libvorbis_64 (23)`,
`GFSDK_Aftermath_Lib.x64 (16)`, `dbghelp (16)`, `steam_api64 (14)`, `MFPlat (13)`, `Galaxy64 (12)`,
`libxess (9)`, `libvorbisfile_64 (9)`, `OpenImageDenoise (9)`, `MF (8)`, `libogg_64 (6)`,
**`d3d12 (4)`**, `SHLWAPI (4)`, `MFReadWrite (1)`, `XAudio2_9Redist (1)`.

Of interest to us: **`dxgi.dll` is statically imported and `d3d12.dll` is delay-imported** — the
renderer surface UEVR hooks is reachable the ordinary way. `[inferred-static 2026-09-19]`

### 6. The anti-debug-shaped imports are all Unreal's own crash and diagnostics code

The complete list of anything that could be read as anti-debug, with the count in the import table:

| API | Present | The ordinary Unreal reason |
| --- | --- | --- |
| `IsDebuggerPresent` | 1 | `FWindowsPlatformMisc::IsDebuggerPresent()` — gates `UE_DEBUG_BREAK` |
| `DebugBreak` | 1 | the other half of `UE_DEBUG_BREAK` |
| `OutputDebugStringA/W` | 2 | `FMsg::Logf` output device |
| `SetUnhandledExceptionFilter` | 1 | Unreal's crash reporter |
| `AddVectoredExceptionHandler` | 1 | Unreal's crash reporter |
| `CreateToolhelp32Snapshot`, `Process32*` | 3 | `FWindowsPlatformProcess` process enumeration |
| `OpenProcess` | 2 | same |
| `GetThreadContext` | 1 | callstack walking in the crash handler |
| `VirtualProtect`, `VirtualAlloc` | 2 | the allocator |
| `LoadLibraryA/W`, `GetProcAddress`, `FreeLibrary` | 5 | plugin and delay-load machinery |
| `QueryPerformanceCounter` | 1 | the clock |

**Absent entirely:** `CheckRemoteDebuggerPresent`, `NtQueryInformationProcess`,
`NtSetInformationThread`, `DebugActiveProcess`, `WaitForDebugEvent`, `ContinueDebugEvent`,
`CreateRemoteThread`, `WriteProcessMemory`, `ReadProcessMemory`, `SetThreadContext`,
`NtQuerySystemInformation`. `[measured 2026-09-19]`

That absence matters more than the presence. A binary that means to resist debugging reaches for the
`Nt*` routines, and none of them are here — not by name, not by delay import.

### 7. A byte-level string sweep of the whole 142 MB found no protector at all

Searched the entire file for 40+ markers. **Zero hits** for: `Denuvo`, `VMProtect`, `vmp0`, `vmp1`,
`Themida`, `WinLicense`, `SecuROM`, `SafeDisc`, `Arxan`, `GuardIT`, `EasyAntiCheat`, `BattlEye`,
`BEService`, `BEClient`, `Enigma`, `Obsidium`, `ASProtect`, `Armadillo`, `nProtect`, `GameGuard`,
`XignCode`, `Hyperion`, `SteamStub`, `.bind`, `ScyllaHide`, `x64dbg`, `OllyDbg`, `Cheat Engine`,
`ProcessHacker`, `ThreadHideFromDebugger`, `ProcessDebugPort`, `ProcessDebugObjectHandle`,
`DbgUiRemoteBreakin`. `[measured 2026-09-19]`

Two hits needed checking and both are innocent:
- `AntiCheat` at 0x609d811 — it is the string `bAntiCheatProtected`, sitting in a run of stock Unreal
  `FOnlineSessionSettings` property names (`NumPublicConnections`, `OwningUserId`, `bUsesStats`,
  `bIsDedicated`, `BuildUniqueId`). It is a **session-settings flag name in Unreal's online
  subsystem**, not an anti-cheat. `[measured 2026-09-19]`
- `SteamAPI_RestartAppIfNecessary` in the Steam delay-import name table — the ordinary Steam
  ownership check that relaunches the exe through the client. It is not an integrity check and does
  not interfere with injection. `[inferred-static 2026-09-19]`

### 8. Nothing else in the install carries protection either

- A full recursive search of `E:\SteamLibrary\steamapps\common\SILENT HILL 2\` for anything named
  `*easyanticheat*`, `*EAC*`, `*battleye*`, `*BE*Service*`, `*denuvo*` returned **nothing**.
  `[measured 2026-09-19]`
- The whole install holds **62 DLLs**, and `SHProto\Binaries\Win64\` holds only six of them, all
  ordinary middleware: `amd_fidelityfx_dx12.dll`, `boost_thread-vc142-mt-x64-1_70.dll`,
  `OpenImageDenoise.dll`, `tbb.dll`, `tbb12.dll`, `D3D12\D3D12Core.dll`. `[measured 2026-09-19]`
- The root `SHProto.exe` (329,216 bytes, TimeDateStamp 2024-04-19) is the stock UE bootstrap shim:
  84 imports across 5 DLLs, **no TLS directory at all**, clean sections, same CFG-instrumented-only
  load config. It does nothing but launch the real executable. `[measured 2026-09-19]`

### 9. Neither executable is Authenticode-signed

`Get-AuthenticodeSignature` returns `Status: NotSigned` for both `SHProto-Win64-Shipping.exe` and
`SHProto.exe`. `[measured 2026-09-19, PowerShell]`

Two consequences: there is no publisher signature for a loaded DLL to invalidate, and it confirms
independently that the file was not wrapped after linking — a wrapper would usually re-sign.

### 10. `.noes.dat` is an inert joke file the game never reads

16 bytes: a UTF-8 BOM followed by `¯\_(ツ)_/¯` (`ef bb bf c2 af 5c 5f 28 e3 83 84 29 5f 2f c2 af`).

It arrived with the Steam install — its creation and modification timestamps (2026-09-19 10:56:15)
sit inside the same install window as `SHProto-Win64-Shipping.exe` (10:56:08), so it is a **depot
file, not something a previous mod left behind**. `[measured 2026-09-19]`

I searched both executables for the substrings `noes`, `Noes` and `noes.dat` in **both ASCII and
UTF-16LE**. **Not one hit in either file.** The game does not open it, does not name it, and cannot
be checking it. `[measured 2026-09-19]`

`[hypothesis]` that it is leftover developer humour from Bloober's build tooling — a hidden
dot-prefixed file is the shape of a build-system placeholder. **Nothing here establishes that**, and
it does not matter either way: it has no bearing on injection.

---

## What this does and does not let us claim

**Claimed, and measured:** no packer, no protector, no anti-cheat, no anti-debug API surface, no DRM
wrapper section, a normal import table, a normal entry point, standard CRT TLS callbacks, CFG not
enforced, and no signature. `[measured 2026-09-19]`

**Inferred, not proven:** that injection will therefore succeed. Static analysis can prove the
*absence* of the well-known protections; it cannot prove the absence of a bespoke, unnamed check
buried in 98 MB of `.text` that, say, walks its own module list at startup and refuses to run with an
unexpected DLL loaded. **Nothing suggests one is there** and 142 MB of UE5 with a plain CRT entry
point is exactly what an unprotected game looks like — but the honest tag on "injection works" is
`[hypothesis]` until a DLL is actually in the process. `[hypothesis]`

**Deliberately NOT claimed:** anything about whether UEVR specifically attaches, renders stereo, or
survives. That is a different question from anti-tamper and this audit says nothing about it.

## ⚠️ One thing this audit could not rule out, and it is worth naming

Silent Hill 2 (2024) is **publicly reported** to have shipped with Denuvo. This build, dated
2025-01-21, has none — which is consistent with it having been removed in a patch, but I did not
verify the history and am not claiming it. What matters for us is the file in front of us:
**this build is clean.** `[measured 2026-09-19]`

The live risk that survives is therefore a **future Steam update re-adding protection**, not the
current state. Worth a note in the dossier so a later session that suddenly cannot inject checks the
build version before assuming its own code broke.

## What would actually prove it

The cheapest possible live test, and it needs no headset — it is the `[FLAT]` row already on the
board, step two of the standing three-step first-launch rule:

1. Drop the lanes plugin's shared logging proxy next to the executable.
2. Launch, reach the main menu.
3. Read its log.

If the log shows the proxy loaded and the game reached the menu, injection works and this whole
question closes as `[verified-live]`. If the game refuses to start, or starts and then exits, the
proxy's log says how far it got — which is exactly the diagnostic a bespoke check would trip.

A second, independent confirmation costs nothing extra: UEVR attaching at all is itself proof that a
foreign DLL survives in the process.

## Tag summary for the dossier

| Claim | Tag |
| --- | --- |
| No packer/protector/anti-cheat/DRM-wrapper in build 1.0.0.5 | `[measured 2026-09-19]` |
| Engine is Unreal Engine 5.1 | `[measured 2026-09-19]` |
| CFG instrumented but not enforced | `[measured 2026-09-19]` |
| Executables are unsigned | `[measured 2026-09-19]` |
| `.noes.dat` is never referenced by either executable | `[measured 2026-09-19]` |
| Injection will work | `[hypothesis]` — needs the `[FLAT]` proxy launch |
