# The Optimizer's Silent Hill 2 pack against the real game: the frame meter under the warp, to_play, the connector (2026-11-21, the /lm reader)

From: the static reader of today's /lm session (Opus). I only read files: the-optimizer (meter/src, engine, toolbox,
job, packs/silent-hill-2), this repo's notes and plugin source, KNOWN.md, the installed UEVR-MCP, PureDark/UEVR branch
`AFW` (public, fetched read-only from GitHub), the UEVR profile's config and log, and the meter's CSV of the 21:35 run.
I launched, attached and sent nothing.

## 1. THE MAIN QUESTION: does the warp hide a slow game from the meter? No.

**What the meter times** `[inferred-static]`:
- `meter_layer.cpp:134-139`: the time going into `xrWaitFrame` and the time it returns, plus the runtime's
  `predictedDisplayPeriod`.
- `meter_layer.cpp:141-144`: the time going into `xrEndFrame`, before the call is passed on.
- Each row is built in `meter_core.h:56-64`:
  - `interval_ms` = from one wait's return to the next one's;
  - `wait_ms` = how long the wait blocked;
  - `work_ms` = from the wait's return to this frame's `xrEndFrame`;
  - `period_ms` = the runtime's frame period;
  - `missed` = interval over 1.5 periods.

**How UEVR's warp submits frames** `[inferred-static: PureDark/UEVR AFW, public August source; the installed beta is newer]`:
- With the warp on, UEVR waits on EVERY frame and calls `xrEndFrame` at EVERY Present:
  - `VR.cpp:2708-2713` and `2808-2813`: `(is_left_eye_frame || is_using_afw())`;
  - `D3D12Component.cpp:819-878`: `if (is_right_eye_frame || is_using_afw)` around `end_frame`.
- The warp itself is a call recorded inside that same frame (`EvaluateFrameWarp`, `D3D12Component.cpp:552`). It makes
  up the OTHER EYE of the game's frame. It adds no frames between the game's frames.
- So the meter sees one OpenXR frame per game frame. The 11-18 note says the same: "UEVR hands one in per game frame".

**Measured today: the 21:35 run on the simulator** (`%LOCALAPPDATA%\TheOptimizer\meter\2026-11-21_21-35-52_SHProto-Win64-Shipping.frametime.csv`
against the UEVR log of the same run) `[measured, n=1 run, simulator]`:
- **Warp calls against meter frames.** In 21 windows of 10 s, the plugin's warp calls (`handpaste: N made-up pictures
  redrawn, M left`, which is one per game frame) match the meter's frames within 0-4. For example: 661/660, 634/634,
  648/648, 901/900.
- **Game ticks against meter frames.** In 5-s windows, the plugin's game ticks (`alive: frame N, X ms avg`) equal the
  meter's frames. The tick average equals the meter's mean interval within 0.05 ms in play, for example 15.4 / 15.41
  and 18.2 / 18.24.
- **So the meter's `interval_ms` IS the game's frame time under the warp.** A slow game shows as long intervals.

**Where "late" is still misleading** (`read.py:22, 59-61`: late = over 1.5 x 1000/72 = 20.8 ms):
- **Simulator** `[measured]`. It is a 90 Hz cap that never snaps a late frame to the next slot (dossier section 6).
  In play today the game ran at a mean of 15.4 ms (about 65 a second), yet only 0-8 frames per 10 s were over
  20.8 ms. That is 0-1.3 % "late", which is under the 1 % hold limit in most windows.
  - The verdict still fails it correctly. `decide.py:22` also needs the slowest 1 % at 0.97 x 72, and p99 here is
    19-21 ms (about 50 a second) `[inferred from the measured rows]`.
  - On the simulator the "late %" number in the result line therefore under-reports badly. Late against 1.0 x target
    (with a few % tolerance), or the mean rate, is the honest number there.
- **Real headset through Virtual Desktop** `[inferred-static from OpenXR's xrWaitFrame pacing; not yet measured with
  the meter]`. A real runtime paces the wait to the display. A 15 ms frame should then be shown at the next slot
  (27.8 ms at 72 Hz) and count as late, so the 1.5 factor fits there. Virtual Desktop's own spacewarp, if it is on,
  happens below the meter: the meter still shows the game's own rate.
- **The live file judges against the headset's period, not 72** (`meter_core.h:63`, 1.5 x 11.1 = 16.7 ms on the
  simulator). Today's `live.json` showed `missed` 40 of 124 in play.
  - `settle_wait` (`tools.py`) waits for `missed == 0` for 5 s. With the game under about 60 a second on the
    simulator that never happens, so every settle runs the full 60 s (`settings.py:47`) `[inferred from the measured
    live.json]`.

**`work_ms` is not the game's own work here** `[measured + inferred-static]`:
- The profile's `config.txt` has `VR_SynchronizationMode=2` (Very Late) today. KNOWN.md line 121 and dossier
  section 6 still say Early (0), so they are out of date.
- In Very Late, UEVR calls `xrWaitFrame` right after Present (`VR.cpp:2810-2813`) and `xrEndFrame` inside the NEXT
  Present hook. `work_ms` is therefore the present thread's whole cycle.
- In play today: work 13.9 ms against an interval of 15.4, with the wait at 0.02 ms. The remaining ~1.5 ms is the
  simulator's `xrEndFrame` (~1.1 ms) plus DXGI Present.
- It says nothing about the graphics card or the game thread. So the "room" fallback in `decide.py:28` would be wrong
  here. It is only used without nvidia-smi, which this PC has.

**When the headset really gets half as many frames** (worth knowing; the meter is still right about what the headset
gets) `[inferred-static: VR.hpp:513-523, VR.cpp:92]`:
- The warp is off and UEVR falls back to its two-frames-per-picture mode:
  - for 90 frames after the warp comes on;
  - for 90 frames after any change of the depth picture's size (every DLSS rebuild);
  - for the whole time the flat screen is up (`VR_2DScreenMode`: our cutscenes).
- Never measure across a load, a cutscene or a DLSS change.

**The right "late" signal for this game:**
- Keep the meter's `interval_ms`. It is measured to be the game's own frame time under the warp.
- Judge with the mean rate and the slowest 1 % (already in `decide.py`), plus late against 1.0 x target on the
  simulator.
- Cross-check with the plugin's 5-s heartbeat (`plugin.cpp:118-131`, `alive: ... X ms avg`). It is free, and the AI
  can read it through `uevr_get_log` (`/api/log`) `[route name only, not tried]`.
- `stat unit` is not needed for "late". It only prints on screen, so a tool cannot read it without a picture
  `[inferred]`. It would only help split game / draw / graphics card, and nvidia-smi already does the card-or-processor
  split.

**A meter fault seen on the way** `[measured n=1 + inferred-static]`:
- Two reads of the 21:35 CSV failed with "being used by another process" while that run was still writing it. After the
  game had closed, the same file read fine.
- `meter_layer.cpp:91` opens it with `fopen_s`, which Microsoft documents as not shareable.
- `read.measure_window` reads the CSV while the game is still running (`read.py:213-216`, called from
  `look_and_measure`). Nothing catches the error there, so the measurement may fail on every spot.
- Fix: `_fsopen(..., _SH_DENYWR)`, or `CreateFile` with `FILE_SHARE_READ`. Check it once in a live run first.

## 2. to_play: the toolbox translates nothing. The AI must do each step with UEVR-MCP tools

The steps are handed through as they are (`tools.py:139-140, 187-188`). No code anywhere reads `connector_call` or
`vr_trigger` `[inferred-static, grep]`.

- **Waiting for the window.** No connector tool waits for it. Poll the toolbox's `game_state`, then `uevr_is_ready` /
  `uevr_get_status`.
- **Enter at the warning.** Use `uevr_input_key` with key `enter`, tap (the same route as `toplay.sh:16`). If that
  fails, use `uevr_input_gamepad` buttons `{"a": true}` for 0.3 s (KNOWN 56-57). Pad A is fine before the load only.
- **ContinueGame.**
  - Find the menu: `uevr_search_objects` with query `MainMenuWidget_BP_C /Engine/Transient` (KNOWN 58). The
    `/Engine/Transient` part skips the class default; the widget only exists once the warning has passed.
  - Call it: `uevr_invoke_method(address, "ContinueGame", args='{"Movie": null, "NonSkippableLen": 0}')`.
  - Not `uevr_call_method`: that one is for getters with no parameters. It does suit `HasSlots`.
  - **The pack says `"object": "main menu"`, which is a description, not a search string.** Put the search string
    in the pack.
- **Waiting until loaded.** `uevr_get_player` until the pawn's class is not None (as in `toplay.sh:13`), then a few
  seconds more (KNOWN 137).
- **vr_trigger: no UEVR-MCP tool can do it** `[inferred-static: tool schemas]`.
  - `uevr_vr_input` only READS the controller state.
  - `uevr_input_gamepad`'s `rightTrigger` drives the game's pad, not a VR controller. It is untested at this prompt
    `[hypothesis]`.
  - Pad A must never be pressed after the load: it opens the save menu (KNOWN 83-85).
  - Enter and pad B did not pass the prompt on 11-19 (KNOWN 259). The pack's "Enter works there too" holds only
    sometimes (it worked 11-13 and 11-15).
  - What works: the simulator's `controller_pose_command.json` with the trigger set (`recon/2026-10-29-buttons/sim_ctl.py`,
    used by `toplay.sh:28`), or VR Bench, or a person.
  - job.md gives the AI only the toolbox and the connector, so **the AI cannot do this step**. It needs a toolbox tool
    (a trigger pull written to the simulator's controller file, the way `look.py` writes the head file).
- **job.md section 4 says "Never use the connector to ... load a save".** That contradicts the ContinueGame step,
  which loads the save. Reword it so that to_play steps are allowed.

## 3. The connector's HTTP addresses: correct

- The port is 8899 (UEVR-MCP README line 83).
- `/api/console/cvar` and `/api/console/command` are in the installed `uevr_mcp.dll`'s strings `[measured]`.
- The profile's copy is byte-identical to the installed one `[measured, md5]`.
- The request shapes match `sweep.py`, which was used live on 2026-11-13 `[measured then]`:
  - GET `cvar?name=` answers `{"float": ...}`;
  - POST `{"command": "..."}`.
- `knobs.py:137, 336` and `game.py:83, 97` use exactly these. Reading `r.ScreenPercentage` for the health check
  changes nothing.

## 4. Quit, crash signs, god mode

- **Quit.** `game.py:130-150` POSTs `quit` to `/api/console/command`. That is the same as `uevr_exec_command quit`, which
  closed the game cleanly in about 10 s every time it was used (KNOWN 202, 232, 278).
  - One weak spot `[hypothesis]`: if the HTTP reply is lost because the game is already closing, `console()` returns
    False. `quit_game` then says "could not be sent ... still running" without waiting (`game.py:139-140`). It should
    wait anyway.
  - The pack's "the injector window is closed after": nothing in the toolbox does that. With SH2VR.exe no window was
    left (KNOWN 232, 278).
- **The crash sign is wrong** `[measured, n=1]`.
  - `Scene state passed to FSceneView constructor is null` appears in a run that did NOT crash: today 21:41:37, and
    the game was still alive and ticking 30 s later (current `log.txt`).
  - `crash_seen` (`game.py:189-202`) would therefore call a clean quit a crash.
  - Use `[Exception Handler]` / `Exception address` (the real crash of 15:11 today), or a CrashReportClient process.
- **God mode.**
  - `null` makes the tour skip the step (`tour.py:112-114, 230`), which is fine. A route, once found, must be
    `{"console": "..."}` (`tools.py:269-280`).
  - No god route is known for this game. A shipping build usually has no cheat manager, so `god` will likely do
    nothing `[hypothesis]`.
  - No tool notices a death. The AI would have to see it.

## 5. Other things in pack.json against KNOWN.md

- **Saves among enemies.**
  - `spots_find` takes EVERY `SaveGameData_*.sav`. KNOWN 241-244: a Continue save among enemies ends an unattended
    run within a minute, and there is no god mode.
  - Limit the spots to known calm slots. The prison armory, slot 3 (note 2026-11-21), is calm and has every weapon.
- **The connector's cost.**
  - KNOWN 119 measured 7 ms per frame and said to keep the plugin out of the profile.
  - 2026-11-13 measured no cost with the warp on (18.03 against 18.05 ms).
  - The installed DLL sits beside a kept `uevr_mcp.dll.v1.0.0-before-throttle`, so the 7 ms was probably the build
    before the throttle `[hypothesis]`.
  - The pack needs the connector for to_play, the quit and every live setting. Every result therefore includes its
    cost, and `connector_cost` records none. Say that in the result.
- **VR Bench's leftover hand file.** Neither `preflight` nor `launch_game` removes `%LOCALAPPDATA%\VR-Bench\input.json`.
  `toplay.sh:7` does, because a leftover one blocks PRESS ANY BUTTON TO PLAY (KNOWN 255-258). `look.py` also leaves its
  `head_pose_command.json` in the simulator's folder after the look-around `[inferred-static]`.
- **The simulator is not the headset's size.** Today's log: `handpaste: pictures made, 1280 x 1400` per eye, against the
  pack's 2688 x 2880 `[measured]`. Simulator frame times rank the spots. They are not the headset's numbers.
- **Stale notes** `[measured config.txt today]`:
  - KNOWN 196-199 says to keep `VR_RenderingMethod` 0 in config.txt; it holds 3 (the launcher keeps it).
  - KNOWN 121 says Early sync; it is 2.
