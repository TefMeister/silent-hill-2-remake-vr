# 2026-11-25: SH2-032, the start crash after a run: config.txt put back to safe values as the plugin loads (b047)

/pd, Build PC, Opus. **The game was not launched, and nothing here has been run inside the game.**

## The fault (board row SH2-032)

After a run, UEVR's `config.txt` held `VR_GhostingFix=true` and `VR_RenderingMethod=3`, the two values our plugin switches
on a few seconds into the headset's drawing (`ghostfix.cpp` after 20 s, `warp.cpp` after 10 s). A start with either one
crashes (the ghosting fix: the start crash of 2026-11-22; method 3: Virtual Desktop refuses the per-eye pictures and
the game dies about 10 s after the inject). Put back by hand twice on 2026-11-22 `[measured, n=2]`
(`2026-11-22b-the-nine-point-video-read.md`, 20:50).

## When UEVR writes config.txt, and when it reads it (praydog/UEVR master, read 2026-11-25) `[inferred-static]`

- **Writes** (`Framework::save_config`): once just after its start-up (`first_frame_initialize`, after the plugins are
  initialised), and every time its menu opens or closes (`set_draw_ui` with saving on: the menu key, the window's close
  button, L3+R3). Not found: a save at quit, a periodic save, a save when a plugin calls `set_mod_value`. So "saved at
  exit" on 11-22 was most likely the menu toggled during the run, or the start-up save after the plugin's switches,
  whichever came last; either way the file ends up with the live values.
- **Reads** (`Mods::reload_config`): in `Mods::on_initialize_d3d_thread`, once BEFORE the mods' D3D start (the plugins'
  `uevr_plugin_initialize` is inside it) and once AFTER. So a value a plugin sets in memory at its initialise is read
  back from the file straight after: a memory-only fix would not hold.
- **Plugin DLLs are loaded** (`LoadLibrary`, no initialise) in the Framework constructor (`PluginLoader::early_init`),
  BEFORE the first read.
- Not read: `deferred_save_config`'s body (Framework.hpp), `IModValue::set`, DllMain, the last third of VR.cpp; the
  AFW fork we run (PureDark's beta.6) may differ from master in details.

## The fix (b047 = b046 + this)

1. **The plugin corrects the FILE while UEVR loads it** (`src/startsafe.cpp`): a static initialiser of `sh2vr.dll`
   (runs inside `LoadLibrary`, before UEVR first reads `config.txt`) puts `VR_GhostingFix=false` and
   `VR_RenderingMethod=0` back (settings.h `START_SAFE`), kernel32 calls only (the loader lock is held), written through
   a temp file and a replace, line ends kept, a missing key not added. `on_initialize` logs what it did
   (`startsafe: config.txt had ... put back ...` or `already safe`) and the live values.
   **Proved offline** `[measured 2026-11-25, n=1]`: the built DLL loaded into a plain Python process with APPDATA
   pointed at a test folder whose config.txt held both crash values: after the load the file read `false` / `0`, the
   other lines and the CRLF ends unchanged, no temp file left.
2. **Our launcher writes the same two values** with the runtime line before every inject (`launcher/src/inject.cpp`,
   `config_lines.h`), and logs each change. For the private packages; Tefa starts with UEVR's own injector, so (1) is
   the one that matters day to day.
3. **The repo's shipped config.txt** (`mod/UnrealVRMod/.../config.txt`, which `pack.sh` puts in a package and the
   launcher places when a profile has none) still said `VR_GhostingFix=true`: now `false`.
4. **The ghosting fix's off-switch file was never found**: its path read `L"\sh2vr_noghostfix.txt"` ("\s" is not a
   backslash), unlike the warp's. Fixed.

Tests: `tests/startsafe_test.cpp` (6) and `launcher/tests/config_lines_test.cpp` (6), all passing with the other three
(`bash dev-archive/native/tests/run.sh`). sh2vr.dll sha256 `2b0f847d...7a53`, installed with `build.sh --deploy`
(backup `D:\SH2 builds\_backup-2026-11-25-1836-before-sh2vr-deploy`), saved as `Frame warp/v0.1.0-b047`.

## What is NOT established

- That UEVR's AFW build reads config.txt after loading plugin DLLs exactly as master does `[inferred-static]`. The log
  will tell: on a start after a run that saved the switches, `startsafe: config.txt had VR_GhostingFix=true ... put
  back` followed by `VR_GhostingFix is false now` and no crash = it works. `is true now` = UEVR read the file before the
  DLL loaded (then the fix must move: an earlier hook, or the injector's side).
- What reset the values on the 13 + 7 clean starts of 11-22: most likely those runs never opened UEVR's menu and ended
  before the switches were saved, or were started by scripts that wrote the file `[hypothesis]`.

## The one check (needs the game, any start)

Start the game the usual way after a run in which UEVR's menu was opened. Read `log.txt` for the `startsafe:` lines.
Expected: "put back ... before UEVR read it", both values safe "now", the start goes through, the ghosting fix and the
warp switch on later as before.
