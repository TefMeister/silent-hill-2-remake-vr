# 2026-11-15 (/pd, Opus, no game run): weapon smoothing, the map at the left back, the shotgun's front-hand fingers, the launcher's upscaler check

Tefa: *"please do all the opus work you can on sh2, pc all yours"*. **The game was not launched; nothing here has been run
in the game.** Four builds in the build history (`builds.py`, project silent-hill-2-remake-vr): b023, b024, b025 installed
in the UEVR profile, each with a deploy backup in `D:/SH2 builds/_backup-2026-11-15-*`. (b022 is a duplicate of b021 snapped
by mistake; labelled so. ⚠️ The build history's numbers run four behind the commits' "b0xx" names up to b021: the history's
b021 is the commits' b025, 2026-11-14 01:49, sha256 43591721.)

## 1. Weapon smoothing, the proper one (b023, `smooth.cpp`, `smooth_math.h`)

Tefa 2026-11-14: *"yeah let's build a proper one"*; melee *"just less to get rid of the hand shaking, but not enough to make
melee feel sluggish"*.

- **Where:** inside the arm solve (`ik.cpp`, `solve_two_bone`), on the RIGHT hand's target (place + turn) just before the
  solve. The held weapon is a child of our arm mesh's `hand_r_bn` (dossier §7), so the arm, the hand and the weapon move as
  one steadier piece; on the two-handed long gun the left hand rides the gun, so it follows too. Skipped while the right
  hand rides the rifle locked in the left hand (`rhbolt.cpp`), where it must stay on the rifle.
- **What:** a One Euro filter with Visceral RE8's numbers (`visceral_re8_hand_smooth.lua`): still cut-off 12 Hz x 0.65^s,
  the cut-off rising with the controller's own speed. Gun s = 4 (2.1 Hz, Village's default), melee s = 1.5 (6.3 Hz),
  bare hands / the healing item / no headset raw. Filtered in the controller's tracking space (its attach parent), so
  walking and turning are not smeared. A jump over 50 cm or 1 rad in one frame (a recentre) is taken as it is.
- **What is held:** the weapon manager's `GetEquippedWeaponName` ("None" = bare hands) with `GetEquippedRangedWeapon`
  (a gun) else melee, looked up five times a second; the filter restarts from the hand as it is at each change.
- **Live tuning:** `sh2vr smooth_gun <0..10>` and `sh2vr smooth_melee <0..10>` lines in `sh2vr_devcvars.txt` (0 = raw).
- **Proof line** every 10 s while smoothing: `smooth: gun, strength 4.0: hand held still N frames, turn per frame raw X
  deg, drawn Y deg`. A working filter shows Y well under X.
- **By numbers** (`dev-archive/native/tests/smooth_test.cpp`, the shipped step, 72 frames a second, made-up tremor of
  0.25 degrees and 0.15 cm a frame) `[verified-numerically 2026-11-15]`:

  | | still-hand turn per frame | still-hand move per frame | 120-degree swing in 0.25 s: worst lag | settles under 1 degree |
  | --- | --- | --- | --- | --- |
  | raw | 0.438 deg | 0.333 cm | - | - |
  | gun (4) | 0.075 (17 %) | 0.067 (20 %) | 9.4 deg | 28 ms |
  | melee (1.5) | 0.147 (34 %) | 0.123 (37 %) | 5.2 deg (under one frame of that swing) | 14 ms |

- **Not established:** how it feels; whether the tracking-space parent is what we think (`GetAttachParent` of UEVR's
  motion-controller component; if it is none, the filter runs in world space and walking will drag the gun a little:
  the proof line still works, the feel would show it). jbusfield's own per-solver `smoothing` (0 in the profile) is
  untouched.

## 2. The map at the left back (b024)

Tefa 2026-11-13: *"map at the back holster - same for SH2 by the way"*; 2026-11-14: *"Map = LEFT back holster"*. The map's
spot moves from the right hip to `MAP_SPOT` = 12 cm left, hip height, 30 cm behind the headset: the mirror of the lower
back's healing spot, as RE2 b208's `BACK_LEFT` mirrors its `BACK_RIGHT`. It is its own spot, apart from the right hand's
weapon table, so a right grip there draws nothing. Everything else of the map (LG toggles it, RG = both hands, LT / RT
pages, Quest X closes) is unchanged. `[compile-verified 2026-11-15]`

**Not carried to SH2 (asked first, per the 11-14 rule):** the 11-14 base layout's sticks (RS up held = jump, RS down held =
crouch instead of dodge, dodge = LS direction + RS click, menu tap / hold).

## 3. The shotgun's front-hand fingers (b025)

The 11-06 screenshots: palm up, fingers spread on the long gun's front. Read statically: without captured game fingers,
`frontgrip::tick` holds the pack's pose `grip_left_weapon_shotgun` every frame, and that pose names two finger shapes
(`left_grip_weapon_shotgun`, `left_trigger_weapon_shotgun`) that the pack's `hands_parameters.json` never defines, so it
posed nothing and the hand kept its open pose `[inferred-static 2026-11-15, the installed file read]`. The rifle's
fore-end pose is complete. Now the shotgun takes the rifle's fore-end grip, and `fingers::pose` logs once whenever a
pose names a missing shape. ⚠️ The pack has 12 such poses (left: board, chainsaw, generic, paper, pipe, pistol, rifle,
shotgun; right: flashlight, rifle off-hand); only the shotgun's front grip is changed. **If the 11-06 screenshots were
the RIFLE, this is not their cause** (its pose is complete): then the F2 capture of the game's own fingers stays the route.

## 4. The launcher checks the upscaler before every start (`dev-archive/launcher/src/upscaler*.{h,cpp}`)

The game rewrites its own `GameUserSettings.ini` now and then (2026-11-10: upscaler off, preset High, motion blur on). Before
each start, with the game closed (also on `--no-install`): `Adv_Supersampling` not 4 (DLSS) -> 4 and `Adv_DLSSQuality` 3
(Quality); DLSS already on -> the player's own level kept; `Adv_MotionBlur` 0 and `IngameMotionBlur=False`. The file is
backed up first, each change said in plain words. Tested on made-up lines and a copy of the real file
(`dev-archive/launcher/tests/upscaler_test.cpp`, 10 of 10), and end to end with `--test-root` on a reset file: put right,
backup kept, a second run left it alone `[verified-numerically 2026-11-15]`. The real file was never written (hash before
and after the same). Not in a release package yet.

## 5. Small things

- `run-sim.ps1` keys and closes the game's OWN window, not the simulator's preview (the /sr drop of 2026-11-11; the
  Darkness found the preview can be `MainWindowHandle`).
- The five inbox notes (quit crashes, the simulator's preview thread, the close message) folded into dossier §3b.
- `bash dev-archive/native/tests/run.sh` builds and runs both test files (the plugin's and the launcher's).

## The next wear (b025 installed), what each outcome means

1. **A gun out, held still, then aimed across a room:** steadier than before and no drag = keep; drags = lower it live
   (`sh2vr smooth_gun 3`); still shaky = raise it (`5`). The log's `smooth:` line gives raw against drawn.
2. **The pipe swung hard:** no lag felt = keep; sluggish = `sh2vr smooth_melee 0.5` or `0`.
3. **LG at the left back:** the map comes out; at the right hip LG now does nothing.
4. **The shotgun in two hands:** the left fingers close round the pump. Still open = look for the `fingers: pose ...`
   warning in the log, then the F2 capture.
