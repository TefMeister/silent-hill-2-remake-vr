# UEVR runs the C++ plugins' pad callbacks BEFORE the Lua scripts'

**Date:** 2026-10-26 (`/gr`, FULL pass) · **Status:** 🆕 new · **For:** the board's top `[PD]` row, "THE BUTTON LAYER":
*"Which runs first, Lua or C++, is not known `[hypothesis either way]`: measure it before building."*

## The answer, from UEVR's public source

`[inferred-static 2026-10-26]`, read in GitHub's own viewer from praydog's UEVR (master, last push 2026-08-30) and
checked line for line against PureDark's fork (the AFW build this project runs): the same lines are present in both.

1. **One list, called in order.** Every `on_xinput_get_state` callback, from a C++ plugin or from Lua, goes into one
   list inside UEVR's plugin loader, appended at registration, and each pad poll walks that list front to back,
   handing every callback the same `XINPUT_STATE` to edit. So whoever registered first edits first, and whoever
   registered last has the final word on what the game sees.
2. **C++ plugins register first.** The plugin loader calls each DLL's `uevr_plugin_initialize` during start-up, and
   the stock C++ wrapper (`uevr/Plugin.hpp`, which `sh2vr.dll` uses) registers its pad callback inside that call.
3. **Lua registers later, and again on every "Reset scripts".** The Lua side does not exist until UEVR's first frame
   (the Lua loader's first `reset_scripts`). Each reset removes the Lua side's callbacks and adds them again, so they
   go back on the END of the list every time.
4. **C++ plugins among themselves run in file-name order.** The loader keeps plugins in a sorted map keyed by the file
   name, so with the profile as installed for development (the published package carries only `sh2r` and
   `uevr_utils`; `sh2vr` and the MCP plugin are added on the test PC) the order is `sh2r` → `sh2vr` → `uevr_mcp` →
   `uevr_utils`. Check the folder's actual file names before relying on this.

**So, in this game, every pad poll runs:** `sh2r.dll` → **`sh2vr.dll` (ours)** → the UEVR MCP plugin → `uevr_utils.dll`
→ **the Lua pack (`main.lua` and friends) last.**

Two cross-checks that this is the right reading:

- praydog's `sh2r.dll`, at its public source (SH2R-UEVR, last push 2024-11-30), never touches the pad state, so it
  adds nothing to the layer problem `[inferred-static 2026-10-26]`.
- The dossier's §3b already records, from a live run, that *"the MCP's simulated pad is applied AFTER the plugin's
  XInput callback: a plugin never sees `uevr_input_gamepad`"* `[verified-live 2026-10-24]`. `uevr_mcp` sorts after
  `sh2vr`, which is exactly that.

## What it means for the button layer

- **Our C++ sees the real controller first**, before any Lua edit. That is the right seat for "one layer, ours, reads
  the real controller".
- **But anything `main.lua` writes lands after us**, so the C++ can neither see nor undo it: the pack's "right stick
  down presses B" and "stick press becomes LB" are added on top of whatever we decided, every poll. That matches the
  reported clash shape (one press reaching two things).
- Therefore the clean design is to **switch the pack's own pad writes off** (its remap library is already off; the
  stick-to-B and stick-to-LB writes in `main.lua` are the ones left) and do every rewrite in C++. A C++ plugin cannot
  out-wait Lua by order alone, because a "Reset scripts" puts Lua back at the end.
- `[hypothesis]` If a C++ last word is ever needed: UEVR's plugin functions include `remove_callback`, which the Lua
  side itself uses; a plugin could remove and re-add its own pad callback after Lua's first reset to move to the end,
  but it would have to repeat that after every reset. Not recommended; turning the Lua writes off is simpler.

## How to confirm it live (cheap)

Log one line per poll for a few frames from both sides, each with a running counter and the button word it received
and the one it leaves: the C++ line should appear first in the same poll, and the Lua line should show the C++
plugin's edits already applied. One flat run, no headset needed.

## Sources

- praydog, **UEVR**: plugin loader (`src/mods/PluginLoader.cpp/.hpp`), Lua loader (`src/mods/LuaLoader.cpp`), Lua
  context (`lua-api/lib/src/ScriptContext.cpp`), C++ wrapper (`include/uevr/Plugin.hpp`) —
  <https://github.com/praydog/UEVR>
- PureDark, **UEVR (AFW fork)**, same files — <https://github.com/PureDark/UEVR>
- praydog, **SH2R-UEVR** (`src/Plugin.cpp`) — <https://github.com/praydog/SH2R-UEVR>

No code was copied; the mechanism is described in our own words.
