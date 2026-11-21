# 2026-11-21: Tune-o-matiC's first run on Silent Hill 2 (simulator, the prison armory save) -- the start crash, and why the tool is not safe here as built

Session: /lm, Opus, Build PC, Tefa at the PC (paused the run, then "still need you to not run the game").
Tool: Tune-o-matiC (Lanes, branch `tune-o-matic`, `plugins/lanes/tools/tune-o-matic/`), built the same day on Tefa's ask
("find the most optimal setup for performance and graphics, so that the graphics never go lower than 72fps"). Tefa
wanted it tested in game as built, then shaped: *"i really want to test it in game first and then we shape it"*.

## What ran

1. `tune.py meter on` for `SHProto-Win64-Shipping.exe` (the meter: an OpenXR layer that writes each frame's time),
   `tune.py start sh2 --profile unreal-uevr` (state only; nothing written to the game).
2. First launch from `dev-archive/launcher/build/Release/SH2VR.exe play --sim --no-install`: **wrong copy**: that folder
   has no `uevr\`, the launcher said `UEVR file missing ... openxr_loader.dll` and the game came up flat with nothing
   injected. Closed by its window (the game's own close). Also launched before swapping the save. Both are mine.
3. `saveswap.py start 3` (the armory), then the release folder's launcher
   (`D:\SH2 builds\releases\Silent Hill 2 VR 0.1.2\SH2VR.exe play --sim --no-install`): UEVR loaded.
4. Tefa: "pause please" (nothing pressed in the game after that). The game then **crashed by itself, about 10 s after
   UEVR's entry** (15:11:10 -> 15:11:20.989).
5. After: the crash dialog closed, `saveswap.py end` (12 files the same by hash), `tune.py meter off` (unregistered).
   Kept: `D:\claude video game stuff\logs\silent-hill-2-remake-vr\2026-11-21-1511-start-crash-tune-meter\` (the UEVR
   log, the crash folder with its minidump, the meter's CSV, the launcher's output).

## The crash `[measured, n=1]`

- The injector's last lines: `sh2vr stall: 423 ms frame`, then `Scene state passed to FSceneView constructor is null`,
  then `[Exception Handler] Exception address is null`. CrashContext: `EXCEPTION_ACCESS_VIOLATION 0x0`, the game
  thread through `UEVRBackend +1685bf` (its engine-tick hook) into `SHProto +4ad7c00` and `+2d33138`.
- That is the board's start crash of 2026-11-18/19 ("about 20 s in ... `Scene state passed to FSceneView constructor
  is null`": 1 of 5 and 2 of 5 starts). Count now: **today 1 of 1 VR start.** Not the dossier's section 13 signature
  (that one goes through ProcessEvent and reads 0xffff...).
- **Our frame meter was loaded** (its DLL name is in the minidump), beside every other layer registered on this PC
  (xrFPS, Window View, Hand Freeze, the OBS mirror). The same crash happened 3 times in 10 starts before the meter
  existed, and no frame of ours is in the crash trace, so **the meter is very likely not the cause** `[inferred, n=1]`.
  A second start with the meter off would settle it if the count ever suggests otherwise.

## What the meter recorded `[measured, n=1]`

91 frames in 9.4 s of loading (the simulator paced at 90 a second; loading frames, so nothing to judge). It caught
the 429 ms frame at the moment `sh2vr` logged its own "stall: 423 ms frame": two separate clocks agree, the first
evidence that the meter measures a UEVR game correctly. Its live file showed the last two seconds as it ran.

## Why Tune-o-matiC as built is not safe on this game (the reader, files only; full text kept beside this note)

`2026-11-21b-reader-tune-o-matic-live-safety.md`. In short:

- **Every live change of `r.ScreenPercentage` here rebuilds DLSS** (two creates and two releases in 50 ms) `[measured,
  n=5, the kept logs of 2026-11-13]`. With DLSS on, that cvar is DLSS's inner size: Balanced reads 58, Quality 66.7.
- **DLSS rebuilds with the warp on:** 5 of 5 survived before the hands' pass existed (b014/b016); since the hands'
  pass (b020 on), 3 of 6 ended in a crash (b028 11-16, b032 11-19 twice) `[measured; the hands' pass as the cause is a
  hypothesis]`. The hands' pass is on in the profile now (`sh2vr_warpscale.txt`, 4th number 1).
- So the **live guard as built would trigger the crash family on its first step**, and it would step the wrong way:
  it believes the size is 100 (it reads only `user_script.txt`), and its steps 70..100 are all ABOVE the real 58.
- The `sg.*` groups are safe live here (twelve live changes 2026-11-13, none rebuilt DLSS) but move the frame by at
  most 0.15 ms. The real settings live in `GameUserSettings.ini`'s `GFXSettings` (`Adv_*`), and the game rewrites
  `user_script.txt`'s values after UEVR runs it.
- What would help here: an SH2 profile (DLSS level restart-only, ray-traced reflections off -1.05 ms and already
  changed live twice without a crash, shadow size 1024 -0.27 ms; never view distance), start values read from the
  live engine through the connector, and a guard lever that is not the DLSS size until `[ID SH2-019]` is settled.

Tefa asked for the tool to be tested before it is changed; nothing in it was changed. These go to the Lanes fault
list as the tool's first real-game findings.

## Not established

- Whether the meter can ever cause trouble in this game (one start, crashed by the known start crash).
- Any frame rate in the armory today: the run never reached play.
