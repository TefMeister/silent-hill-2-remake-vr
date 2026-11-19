# 2026-11-19: the ideas list and the board, checked against the code (the /lm reader's report)

The one read-only reader of the /lm session of 2026-11-19 (Fable; about 7.5 minutes; it launched nothing and edited
nothing). Board line numbers are those of `~/claude-memory/status/silent-hill-2-remake-vr.md` BEFORE the re-cut of the
same day. "code" = judged from source; "board" = judged from a board row only. The last section says what the /lm
session did with each finding.

## 1. Ideas check (`mod-ideas/games/silent-hill-2-remake.md`, the whole page)

Fresh (the five on the numbered list):

1. Cutscenes fade in and out: NOT STARTED. No fade in `cutscreen.cpp` or `realcam.cpp` (code) `[inferred-static 2026-11-19]`.
2. The cutscene screen floats in the fog: NOT STARTED. Nothing about fog in `cutscreen.cpp` (code) `[inferred-static 2026-11-19]`.
3. A weapon examine that plays by itself; weapons empty at first: NOT STARTED. No pickup or examine code (code) `[inferred-static 2026-11-19]`.
4. Hand smoothing for the long guns, each arm switchable: PARTLY. `smooth.cpp` `apply()` is called from `ik.cpp`
   `solve_two_bone` for the RIGHT hand only, every gun (strength 4) and lightly for melee (1.5); live values
   `smooth_gun` / `smooth_melee` in `devcvars.cpp`. Missing: a left-arm filter, per-arm switches, a menu entry (code)
   `[inferred-static 2026-11-19]`.
5. Make AFW perfect, no shimmer round hands: PARTLY. Our own hand pass after the warp exists (`handpaste.cpp`,
   `drawnpose.cpp`, `warpscale.cpp`), carrying hands by the drawn pose (code) `[inferred-static 2026-11-19]`. The
   outline round hands is still open `[reported]`.

Filed / settled:

6. Fewer, stronger enemies; silent radio; Pyramid Head driven off: NOT STARTED (no code, no board row).
7. A mannequin in the first room: NOT STARTED.
8. Weapon accuracy copied from RE2: NOT STARTED.
9. Our own UEVR profile on the AFW build, no outside dependencies: PARTLY. `sh2vr.dll` replaces much of the pack (ik,
   hands, fingers, buttons, holsters, reload), but the pack's Lua still runs (`bridge.cpp`, `scripts/sh2vr_bridge.lua`;
   the pump and the bolt are Lukas's scripts) (code). One package exists `[reported]`.
10. Six places on the body (2026-09-28): replaced by item 13 on the page itself.
11. A poser tool in the headset: NOT STARTED (`main.lua` still has `--hands.enableConfigurationTool()`).
12. The melee weapon holstered on the left hip: DONE as built (`settings.h` `HOLSTER_SPOTS`, `holsters.cpp`) (code);
    worn 2026-10-31 `[reported]`. The parked melee mesh shown on the hip was not looked for.
13. The full VR button and body layout: PARTLY, and since changed by Tefa. Built: `buttons.cpp`, `holsters.cpp`, quick
    turn off by default (code). Differs from the page now: the map at the LEFT BACK, the medicine at the RIGHT BACK
    (b029), the dodge on the right stick click (b026). The flashlight taken from the left shoulder into the hand is
    not built (`settings.h`: "stays on LT for now"). **The ideas page still shows the 2026-10-25 table.**
14. A handgun in the opening room, marked on the map: NOT STARTED.
15. Containers opened by hand (LG, then LT): NOT STARTED.
16. Reload like Visceral RE8: PARTLY. The pistol: `rack.cpp` + `reload.cpp` (code), worn 2026-10-31 `[reported]`. The
    shotgun's and rifle's rounds by hand come from the pack's scripts; the rifle bolt is `rhbolt.cpp`, unworn.
17. Switch off what a headset does not need: DONE for the screen effects (`screenfx.cpp`) (code); PARTLY for the frame
    rate (`tuning.cpp`).

## 2. The board's stale rows (the OPEN block was lines 92-189)

**(a) Ticked, struck, closed or history rows inside the block:** 95, 103, 104, 108, 118, 122, 125, 128, 130, 131,
133, 134, 137, 138, 142, 145, 150, 151, 155, 156, 168, 171, 181, 188, 189, and the 📜 "Earlier headers" line 93. 138
is a near word-for-word duplicate of 118. 156 is a standing fact, not work. Lines 158-162 belong to row 157.
**A broken line:** row 174 ended mid-path at `D:\SH2 builds`; its tail sat at line 196, at the left edge, starting
`eleases\...` (a backslash-r in the path had been turned into a line break when it was written). The same at 197 /
198. **Outside the block but still open work** (under the "Done 2026-10-25" heading, so the gate check never saw
them): 207, 208-209, 217-223, 224-227, 228-231 `[PD]`, 232-235 `[FLAT]`, 236 `[VR USER]` (shadows), 237-238 `[FLAT]`;
210-216 (wear the lowered trigger) is done per line 203.

**(b) Features** (each headset row gets one feature's ID; a later row of the same feature supersedes the earlier):
hands and weapons at the headset's rate (96, 99, 101, 107); the long gun curls up (97) and two hands on the long guns
(182, 172, 163, 164, 167, 160); the front hand's fingers (165, 173, 104, 106.3, 94.3); James moving / roomscale (98,
111: row 95 has Tefa's word on b028, "roomscale steps aside, James stays put: yes"); medicine, back spots, map (94,
103, 106.2, 184, 185); the pump with LT (94.3, 104, 159, 177, 178); weapon smoothing (106.1, 108); first person for
traversals (112); cutscenes in and out with the warp (114, 102, 136, 116, 139, 140 and five ticked rows); mirrors
(141); the fog and the draw distance (115, 121); the arms drawn black (117); the outline round hands (126, 122, 120);
the torch's and the gun's shadow (123, 158, 150, 236); performance to 72 (109, 124, 127, 152, 153, 154, 169, 170,
167); the freeze every 4-5 s (113: Tefa's word in row 95, "GONE"); the A button at the main menu (132); the DLSS
ghost (135, 144, 147); getting the warp to start on the real headset (149, 148, 146, 143, 129, all behind the ticked
125); DLSS / graphics-card crashes (100, 102); the rifle bolt (176); holsters (186, 161, 179, 180, 181); the button
layer and the one control layout (187, 189, 188, 110, 105); the pistol reload by hand (177-180, 207, 217-223); finger
poses (175, 224-227); elbows (172, 208-209); the package 0.1.x (174, 166, 155); dangling key tags (119); the 21
Sacraments add-on (183, 170, 168); Tefa's 2026-11-07 wear as one umbrella row (157 with 158-162).

**(c) Gate tags that look wrong:** 97 and 99 (next step needs no headset), 98 (a question to Tefa: `[USER]`), 109,
112, 114, 117, 121, 132, 167 (a build or simulator step comes before any eyes), 166 (half of it is a copy into the
staging repo), 116 (`[FLAT]` but seen only on the real headset); stale because answered by ticked rows: 143, 144, 148,
183 (`[FLAT]`) and 170 (`[VR USER]`).

**(d) Facts above the block that newer rows contradict:** the window ("already windowed 1920x1080 ... not yet
1280x720": it was set and found fullscreen three times since); "motion blur off" (found on again 2026-11-10;
`screenfx.cpp` sets it every launch now); the launch by the Steam link plus the injector by hand (the launcher
`SH2VR.exe` does it since 2026-11-05, and the warp is switched on after the start); the AFW build "in the game
folder" (inside our zip since 11-05); the baseline "about 40 fps" (72 with the warp on b032); "the pistol reload by
hand is BUILT, unworn" and the fire doors "installed, unrun" (both worn 2026-10-25 / 10-31).

## 3. Code shape

Nothing in `dev-archive/native/src/` is over 800 lines (largest: `ik.cpp` 734, `handpaste.cpp` 612, `holsters.cpp`
491, `twohand.cpp` 399). `ik.cpp` crosses next; its move-only split when it does: the rig building (`load_solver`,
`collapse_body`, `finger_poses`, `create`) into `ik_rig.cpp`, the Kismet one-liners into `ik_kismet.h`, the solve
stays. `handpaste.cpp`: the shader text and pipeline set-up into `handpaste_gpu.cpp`, `draw()` stays. Every file over
the limits (10 over 1,500, 17 over 800) is the bundled pack's Lua (jbusfield / LukasBlaster7) under `mod/`, or UEVR's
`API.hpp`: not our hand-written code, and a split would make every pack update a hand merge. Verdict: `keep`.
**One real warning:** the pack's `scripts/main.lua`, which we do edit, has about 193 top-level locals and Lua stops
loading a file at 200: a new edit there goes inside a table or a function, never a new top-level `local`.

## 4. The static question: how can the two-handed gun curl up on its own and stay?

Ruled out by reading `[inferred-static 2026-11-19]`: the right hand's turn is computed from the raw controllers only;
the smoothing is not fed its own output; the drawn-hand record of b030-b032 is picture-only (the only writers of the
gun's place are `twohand.cpp` and `rhbolt.cpp`); the hand's turn is not applied twice.

Found `[hypothesis at the time; the arithmetic shown on a model]`: each view makes two shortest-way turns about the
right hand's grip, starting from the gun as the LAST view left it. Two such turns about different axes do not cancel;
what is left is a small roll about the hands' line, and nothing puts it back. On the reader's model (the arm 4 cm
short of the grip spot): 41 degrees after 1 s, 87 after 5 s, then still; with the hand reaching and the aim circling
3 degrees, about 4 degrees in 10 s. Why new since b029 (its guess): without LT the front hand is now pinned at the full
grip distance, the case where an arm falls short. Fix direction: rebuild the gun's turn each view from a fixed
reference. A second, weaker suspect: with the rifle, LT while two-handed starts the rifle lock (`rhbolt.cpp`), which
trails the gun after the left controller.

## 5. What the /lm session did with it

- **The curl: proven live and cured the same session (b033).** On the simulator the creep showed with the hand
  REACHING and circling (0.6 degrees a second, 11.9 in 20 s; the reader's second case, several times faster than its
  model said); still hands with a reaching arm gave none. The cure is the reader's fix direction: each view starts
  from the gun's own place in the right hand. Measured gone on b033. Note
  `2026-11-19-the-long-gun-that-curled-up.md`. The "arm falls short" case was not run on its own; the cure covers it
  by construction. The rifle-lock suspect was not tested (board row).
- **Board (a):** every listed row moved out of the OPEN block under "Done, moved out of the OPEN block on
  2026-11-19"; the broken lines joined again; the open rows that sat under a Done heading moved back into the block.
- **Board (b):** feature IDs SH2-001 to SH2-031 on the rows (`~/claude-memory/feature-ids.md`). Rows 111 (roomscale
  steps aside) and 113 (the freeze) closed on Tefa's word of 2026-11-16, quoted from row 95.
- **Board (c):** 98 became a `[USER]` question; the `[FLAT]` rows answered by ticked rows (143, 144, 148, 183) were
  closed with the row that answers them. The other re-gate suggestions are listed in one `[PD]` row for the next
  no-game session rather than applied blind; headset rows keep their tag until Tefa's word.
- **Board (d):** a dated correction paragraph added above the block (the old paragraphs are history and stay).
- **Ideas:** nothing settled or dropped (Tefa's call). The page's layout table being older than the build is on the
  `[USER]` row.
- **Code shape:** nothing split, verdict `keep` as above; the `main.lua` locals warning is on the board (`[PD]`).
