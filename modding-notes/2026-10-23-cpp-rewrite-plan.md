# 2026-10-23 · The C++ rewrite: what we port, in which order, and why

Tefa, 2026-10-23: *"we need to get all the stuff that we have right now in silent hill 2, re-written and ported to
C++"*. This is the plan. It is a design, so it carries `MODEL: FABLE`; the straight ports under it are Opus work.

## What "all the stuff we have right now" is, measured

| Piece | Author | Form | Size | Runs every frame? |
| --- | --- | --- | --- | --- |
| `sh2r.dll` (SH2R-UEVR) | praydog | C++ UEVR plugin, MIT, source public | 616 lines | yes (aim trace, melee trace, hit reactions) |
| `camera.lua` | praydog | Lua (folded into jbusfield's pack) | 71 KB | yes (first person camera, items in hand, map) |
| `scripts/` | jbusfield (with Pande4360, gwizdek, markmon, Rusty Gere, lobotomy) | Lua on UEVR's LuaVR, MIT | **52,055 lines**, of which about 7,000 are configuration tables | yes: `on_pre_engine_tick` → hands, IK, attachments, input, interaction, reticule, collision, body yaw, gestures, montages, UI |
| `uevr_utils.dll` | jbusfield | small C++ helper (TArray access) | 351 KB | on demand |
| `rifle_bolt / rifle_reload / shotgun_pump / shotgun_reload.lua` | LukasBlaster7 | Lua, on top of jbusfield's | about 4,000 lines | yes, when that gun is held |
| `data/*.json` | jbusfield | the saved parameters (hands, attachments, IK, reticule, UI) | 500 KB | read at start |

Biggest Lua files: `uevr_utils.lua` 4,326, `main.lua` 4,123, `hands_config.lua` 3,189 (data), `attachments.lua` 2,664,
`ik.lua` 2,576, `remap.lua` 2,516, `input.lua` 1,665, `accessories_config_dev.lua` 1,595 (data), `hands.lua` 1,526.

## What the port will and will not buy, from the 2026-10-23 measurements

- With every Lua script switched off, the armory frame took about **1 ms less** (≈ 20 ms either way at 30 %
  resolution). So a C++ port on its own does **not** reach 72 fps there; the game thread and the RHI thread are the
  wall `[measured 2026-10-23, n=4]`. Say this plainly whenever the rewrite is sold as "faster".
- What C++ does buy: (1) deeper hooks than Lua can reach (vtable and inline hooks, the way praydog's plugin aims
  from the hand), which is where the remaining frame cost might be attacked later; (2) one build to install
  instead of 70 scripts; (3) the shape the other Visceral mods already have (feature per file, named numbers,
  Ghost Hand and BeG0nE wired in); (4) no Lua 200-locals and garbage-collection stalls.

## Decisions

1. ~~Start from praydog's SH2R-UEVR source.~~ **Amended the same evening:** his SH2R-UEVR repo has **no licence
   file** (UEVR itself is MIT, this plugin is not marked), so its source is not copied without his word. The
   skeleton was built fresh against UEVR's API headers (`2026-10-23b-sh2vr-skeleton-pass-1.md`); `sh2r.dll` keeps
   running beside ours, credited, exactly as jbusfield's pack ships it. Tefa, later the same evening: *"just use it"*; Tefa will ask praydog, and their
   aim and melee hooks can be folded in.
2. **Hybrid during the port, like Visceral RE8.** The Lua pack keeps running; each module ported gets a stand-down
   switch in the Lua (a `PORTED` list in the build script removes that script from the profile), so one module at
   a time is swapped and tested in the headset. Never a big-bang swap.
3. **The JSON parameter files are kept as the settings format.** jbusfield's `data/*_parameters.json` are the
   player's saved numbers; the C++ reads the same files (`json_mini`, as RE8 does), so nothing is retyped and the
   Lua config UI keeps working until it is ported last.
4. **Order of porting, by what shapes the feel most and runs most:**
   1. Foundation: plugin skeleton from praydog's tree, `settings.h`, the JSON loader, logging, object lookup
      helpers (`find_uobject`, property by name), the stand-down switch. `MODEL: FABLE` for this step only.
   2. Hands and attachments (`hands.lua`, `hands_animation.lua`, `attachments.lua`, `accessories.lua`,
      `animation.lua`): which hand model is shown, where the weapon sits in the hand, finger poses. The core.
   3. IK and body yaw (`ik.lua`, `body_yaw.lua`, `collision.lua`): the arms and the body under the head.
   4. Input and remap (`input.lua`, `remap.lua`, `controllers.lua`, `gestures.lua`, `keyboard.lua`, `mouse.lua`):
      what each controller button does, decoupled yaw.
   5. Manual reloads (Lukas's four scripts): the same reload / rack state machines Visceral RE7 and RE8 already
      have in C++ (`reload.cpp`, `rack.cpp`, `breech.cpp`), written against Unreal's objects instead of RE Engine's.
      Ghost Hand wired in here (the rule).
   6. Interaction, reticule, laser, line tracer, montages, particles, physics, scope, pawn, flicker fixer.
   7. UI, widgets, the config menu (`configui.lua`, `ui.lua`, `widget.lua`): last, via UEVR's `on_draw_ui`.
   `uevr_utils.lua` (4,326 lines) is not a module: it is the toolbox the others call, and it is ported piece by
   piece as each caller needs it.
5. **Every ported module proves its own effect** (PROTOCOL §11): the Lua off, the C++ on, the same thing seen in
   the headset or the same numbers in the log, before the next module starts.
6. **Code shape from day one:** 800 lines soft, 1,500 hard per file; numbers in `settings.h`; probes in their own
   files and archived when disproved (`code-shape-scan.py`).

## Honest size

Roughly 45,000 lines of logic once the data tables are set aside. Visceral RE8's port runs at about one Lua file
per session. Expect **well over ten sessions**, each ending with a headset check by Tefa, before the Lua pack is
gone. The first two steps (foundation, hands + attachments) are where the next three or four sessions go.

## Open questions for Tefa

- Keep praydog's `camera.lua` as Lua for now (it works and praydog maintains it), or port it in step 2? Suggest
  keep, port last.
- The config menu: UEVR's own ImGui through the C++ API, or keep the Lua menu permanently? Suggest C++, last.
