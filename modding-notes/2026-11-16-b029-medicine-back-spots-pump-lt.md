# 2026-11-16: b029, the no-game fixes from Tefa's b028 wear (Developer PC, Opus)

Built, installed, saved as b029. Not run. Plugin compiled, scripts `luac -p` clean `[compile-verified 2026-11-16]`.

1. **The map came out of the medicine spot (item 3).** The two back spots were 12 cm each side of the middle, 30 cm
   behind, each taking the hand within 18 cm: 24 cm apart, they overlapped in the middle, and spots 30 cm behind move
   about 15 cm sideways for a 30-degree head turn (they follow the head's yaw). Now 22 cm each side (`settings.h`
   HOLSTER_SPOTS lower back, MAP_SPOT). The left / right sign was checked: right = (-f.z, 0, f.x) is +X for a -Z forward,
   correct `[inferred-static]`.
2. **The medicine cycled on presses; holding gave no syringe (item 3).** Now tap = the drink, the same grip kept held
   0.5 s = the syringe, a tap again = put back (`HEAL_PRESS_CYCLES = false`; true = the old cycle). ⚠️ Tefa said LG; the
   medicine spot reads the RIGHT grip only. Asked.
3. **The front hand slid along the pump without LT (item 5).** The plugin now sends LT with the grips (`LT=` in
   `sh2vr_grips`); `shotgun_pump.lua` counts a pull only while LT is held (from where the hand was when LT went down;
   an older plugin = no LT needed), and `twohand.cpp` slides the drawn hand only while LT is held (`PUMP_NEEDS_LT`).

The GPU hang after the second cutscene: frame generation is off in the game's settings; DLSS was created and released
twice at the cutscene exit, 7 s before the hang (board row). Not touched in this build.
