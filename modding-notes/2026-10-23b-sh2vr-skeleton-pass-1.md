# 2026-10-23 · sh2vr pass 1: the C++ skeleton runs inside the game

Step 1 of the rewrite plan (`2026-10-23-cpp-rewrite-plan.md`). Fable.

## What was built

`dev-archive/native/`: `sh2vr.dll`, a UEVR C++ plugin built with plain CMake (VS 2022, C++20, static runtime).

| File | Does |
| --- | --- |
| `src/plugin.cpp` | the plugin class: loads, reads the parameter files, heartbeat every 5 s |
| `src/params.cpp/.h` | reads jbusfield's 12 `data/*_parameters.json` from the UEVR profile, re-reads one when it changes on disk |
| `src/json_mini.cpp/.h` | our small JSON reader (from Visceral RE8) |
| `src/settings.h` | the named numbers (heartbeat period, profile folder) |
| `src/common.cpp/.h` | logging, UTF-8 helpers, `%APPDATA%` |
| `include/uevr/` | UEVR's plugin API headers, taken from PureDark's UEVR fork (the AFW build in use), API 2.39.0 |
| `tools/build.sh` | build, check the two entry points, `--deploy` with backup and `--undo`; `PORTED` list for the Lua files each module replaces |

**Not built from praydog's SH2R-UEVR source.** His repo has no licence file (checked 2026-10-23), so copying it
needs his word first; `sh2r.dll` keeps running beside ours as a separate plugin, as it does in jbusfield's pack.
The plan's decision 1 is amended to that.

## Proof it works `[verified-live 2026-10-23, n=1]`

Installed into `%APPDATA%\UnrealVRMod\SHProto-Win64-Shipping\plugins\sh2vr.dll` beside `sh2r.dll`,
`uevr_utils.dll`, `uevr_mcp.dll`; the Lua pack untouched. UEVR AFW b6 accepted it (no version complaint). Log:

```
[sh2vr] loaded: skeleton, API 2.39.0
[sh2vr] params: hands_parameters.json read, 1 top-level keys   ... (12 files)
[sh2vr] params: 12 of 12 files read
[sh2vr] alive: frame 883, 36.1 ms avg, headset on, pawn none, params 12                (title screen)
[sh2vr] alive: frame 5800, 22.4 ms avg, headset on, pawn SHCharacterPlay_BP_C /Game/Game/Maps/Main/Main.main.PersistentLevel.SHCharacterPlay_BP_C_2147475441, params 12   (save loaded)
```

No errors or warnings from the plugin in the whole run. GPU peak 75 °C over the run.

## Next (step 2 of the plan): hands and attachments

Port `hands.lua`, `hands_animation.lua`, `attachments.lua`, `accessories.lua`, `animation.lua` into
`src/hands.cpp` + `src/attachments.cpp`, reading `hands_parameters.json` / `attachments_parameters.json` through
`params::file()`. First read the Lua to list exactly which UEVR calls it makes (UObjectHook attachments, skeletal
mesh components, montages) and check each has a C++ API equivalent; then port, add the two files to `PORTED`, and
a headset check by Tefa decides. `MODEL: FABLE` for the design of how hands and attachments share state; the
line-for-line part is Opus.
