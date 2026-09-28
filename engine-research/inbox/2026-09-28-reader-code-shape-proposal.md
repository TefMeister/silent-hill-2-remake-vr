# Proposed code shape, before the first native line is written

**From:** background reader, 2026-09-28, static. **Speaks to:** the board row "decide the repo's code
shape", and the RESUME row "what the native plugin owns and what Lua fills in". A proposal for the main
session and Tefa to accept or change; nothing here is built. All `[hypothesis]` until a first build
proves the layout works.

## The one rule the layout serves

**The per-frame arm and hand path lives only in C++.** Lua never runs on it. Lua handles things that
happen now and then: level loaded, weapon changed, settings changed, a menu opened. That is what keeps
us out of the trap §8e describes (tens of script calls per arm per frame).

Per-frame work runs once per **game tick**, never once per eye, so Native Stereo and AFW agree
(modding notes 2026-09-26). In the plugin that means doing arm work in the engine-tick callback, never
in the per-eye view callback `[inferred-static]` (from UEVR's plugin API design; confirm on first build).

## Folder layout (inside `dev-archive/`, where live source lives)

```
dev-archive/
  plugin/                      C++ UEVR plugin, one DLL
    CMakeLists.txt
    src/
      plugin_main.cpp          entry point + wiring of callbacks only (aim < 200 lines)
      settings.hpp / .cpp      THE settings table (see below) + loading it from JSON
      game_names.hpp           every SH2 name in one place: bones, sockets, object paths, class names
      game/pawn.cpp/.hpp       find James, his mesh, weapon, flashlight; cache; drop on level change
      arms/ik.cpp/.hpp         two-bone solve (calls the engine's solver or our own maths)
      arms/arm_driver.cpp/.hpp per tick: controllers in, solve, bones out
      arms/handover.cpp/.hpp   Tefa's idea: game animation drives the arms, then blend back
      body/hide.cpp/.hpp       head and body hiding
      bridge/lua_bridge.cpp    the small event channel to and from Lua
      util/log.cpp, util/math.hpp
    probes/                    one file per probe, built only with -DSH2VR_PROBES=ON
    tests/                     maths tests that run without the game
  lua/                         the UEVR profile scripts: glue only
    main.lua                   loads modules, forwards events (aim < 300 lines)
    glue/*.lua                 one small file per concern (menu, config UI, events)
  profile/                     what gets installed: UEVR config.txt, data/settings.json
  probes/                      Lua probes (sh2_bone_dump.lua already lives here)
  archive/                     disproved probes and retired files, with a README line each
  tools/                       build, package, code-shape check
```

Install into the game's profile folder is a separate step the main session runs, never automatic.

## One settings table for named numbers

- **`settings.hpp` holds every number**, grouped: `arms`, `hands`, `body`, `handover`, `comfort`,
  `weapons`, `debug`. Each entry has a name, a default, its unit (cm, degrees, seconds) and a one-word
  source: `tuned` (we set it by feel), `measured` (from the game), or `derived`.
- **`profile/data/settings.json` overrides the defaults** at load, so tuning in the headset needs no
  rebuild. Lua reads the same file, so there is never a second copy of a number.
- **Names are data too:** `game_names.hpp` holds every bone, socket and object path as a named
  constant. When the live check finds a name wrong, it changes in one line.
- **No bare numbers anywhere else.** `0`, `1` and `-1` as plain arithmetic are fine; anything that is a
  distance, angle, time, threshold or index gets a name. The lanes code-shape scan checks it.
- **No memory offsets planned.** Reach the game by reflection (names), not raw addresses. If one is ever
  needed, it goes in the table under the Steam build ID it was measured on, and the plugin refuses to
  use it on any other build.

## Probes in their own files

- C++ probes: one file each in `plugin/probes/`, compiled only when `SH2VR_PROBES` is on, so a release
  build cannot contain one. Each registers itself with one line; nothing in `src/` calls a probe.
- Lua probes: one file each in `probes/`, loaded by hand, never by `main.lua`.
- The session that answers a probe's question moves it to `archive/` with a one-line README entry.

## File limits

- **800 lines soft, 1,500 hard**, for every hand-written file, C++ and Lua alike. Past soft, split
  before adding. Past hard, splitting is the job.
- Target much smaller: most files above should stay under 400. The entry files (`plugin_main.cpp`,
  `main.lua`) stay under 200-300, because they only wire things together.
- Lua files also keep well under Lua's 200-local limit, which a past project hit exactly.
- Splits are move-only, on a branch, tagged `pre-split-YYYY-MM-DD` first, proven to change nothing.

## Why this shape (the cautionary tale, measured)

jbusfield's profile is 49,718 lines of Lua and CharlotteLiu's is 94,159, with single files of 2,576 to
5,487 lines `[measured 2026-09-28, wc -l]`. CharlotteLiu's own two-handed-weapon file says it moved its
pure maths into a separate library file with "zero SDK, zero IO, zero game meaning" — the same split
proposed here, arrived at late `[reported, their comment]`. Starting with it costs nothing.

## A companion file

`staging/silent-hill-2-remake-vr/2026-09-28-names-to-verify.json` (private staging repo, uncommitted)
lists every bone, socket and object name from both community profiles in one machine-readable list, so
the live check can tick them off and the verified list can seed `game_names.hpp`.
