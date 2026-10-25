# 2026-10-25 — the lowered trigger worn; the full button and body layout settled (home PC)

- Firing with the gun lowered (no grip, no aim stance) worn: "Yess the fire without GR works!" `[verified-live 2026-10-25, n=1]`.
- The hand reload ran (slide pulled 13.7 cm, home, magazine out with B), but one press reached two things: the game's own
  B is dodge, and `reload.cpp` reads B without keeping it from the game; the pack's Lua also turns right stick down into B.
  Three layers touch the pad (game, the pack's `main.lua`, our C++); its remap library is off (`remap_disabled: true`);
  which of Lua and C++ runs first is not known `[hypothesis either way]`.
- Tefa settled the full VR button and body layout (mod-ideas `games/silent-hill-2-remake.md`, decisions.md 2026-10-25):
  X back only, Y inventory, B pistol magazine out, left stick click run toggle, LS + RS down dodge, RS up quick turn (off by
  default), LT alone switches the flashlight, map and medicine held by LG with LT to use, holsters on both shoulders and
  hips. It assumes the game's own controls at default: checked, `SaveGames/InputRebinding.sav` holds no rebinding.
- On hold until Wednesday 2026-10-28 (a Fable job, Tefa's word).
- The UEVR injector stays running after the game closes; Discord then shows "playing Silent Hill 2" (it ran four hours
  once). Close it with the game (driving notes, KNOWN.md).
