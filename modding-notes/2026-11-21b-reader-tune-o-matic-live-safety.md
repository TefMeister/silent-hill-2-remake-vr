# Is it safe to change r.ScreenPercentage (and sg.*) live in SH2 with DLSS + the warp? Tune-o-matiC's first real game (2026-11-21, the /lm reader)

From: the static reader of today's /lm session (Opus). Read-only work: logs, notes, source, the profile and the game's
settings files. Nothing was launched, attached, sent to port 8899 or changed outside this file.
For: the /lm session before it runs Tune-o-matiC (`~/github-backups/_lanes-tune-wt/plugins/lanes/tools/tune-o-matic/`)
in the prison armory. Board rows it touches: `[ID SH2-019]`, `[ID SH2-015]`.

## The answer in one line

**NOT SAFE on today's build (b033, the hands' pass on), as far as the evidence goes.** In this game every live change
of `r.ScreenPercentage` tears DLSS down and builds it again: the same event the three DLSS crashes came after
(2026-11-16, and twice on 2026-11-19). It was survived 5 of 5 times with the warp on **before** the hands' pass existed
(b014 and b016). Since the hands' pass, 3 of 6 such rebuilds ended in a crash. The `sg.*` groups are safe to change live
but do nothing for the frame here.

## 1. Does r.ScreenPercentage do anything while DLSS is on? Yes: it IS DLSS's inner size

- The game sets its DLSS level through `r.ScreenPercentage`: Quality reads 66.7 and Balanced reads 58 after a start
  `[verified-live 2026-11-10 and 2026-11-13, n=1 each]`.
- Changing it live changes the frame time:
  - Simulator, DLSS on: 100 % 25.2 ms, 75 % 18.8, 50 % 14.6, 35 % 13.8 `[measured 2026-11-10, simulator, n=1 each]`.
  - Real headset, warp on, b014, 2026-11-13: 66.7 -> 50 took the frame from 16.5 to 14.1 ms, and back to 17.1 when it
    was put back `[measured: uevr-log_2026-11-13_0858_..., heartbeat lines 09:12:27-09:13:07]`.
  - b016 at 10:39: Balanced 15.2 -> Quality 16.3 -> Performance 14.5 -> Balanced 15.2 `[measured:
    uevr-log_2026-11-13_1037_...]`.
- **Every one of those live changes rebuilt DLSS.** UEVR's log shows `hk_NVSDK_NGX_D3D12_CreateFeature` twice, then
  `ReleaseFeature` twice, in the same 50 ms, at 09:12:27, 09:13:00, 10:39:14, 10:39:30 and 10:39:47. Nothing else
  rebuilt DLSS in those sessions except the level load and the quit `[measured, the two logs, n=5]`. That is one DLSS
  instance per eye (the warp draws each eye as its own picture). The two creates took 43 ms between them, so each step
  is probably a hitch of a few frames `[measured: log timestamps; whether the frame waited is inferred]`.
- Not tested: values above 100 (Tune-o-matiC's steps go to 120). NVIDIA's upscaler cannot take an input bigger than its
  output. Above 100 the engine may turn DLSS off, which would release it without building a new one: the pattern of the
  00:20 crash below `[hypothesis]`.

## 2. Why "not safe": every DLSS rebuild we have on record, with the warp on

| when | build | hands' pass | how DLSS was rebuilt | outcome |
| --- | --- | --- | --- | --- |
| 2026-11-13 09:12:27, 09:13:00 | b014 | did not exist | console, inner size 66.7 <-> 50 | survived, 2 of 2 |
| 2026-11-13 10:39:14, :30, :47 | b016 | did not exist | console, the three DLSS levels' inner sizes | survived, 3 of 3 |
| 2026-11-16 00:47:44, 00:48:21, 00:50:23 | b028 | on (warpscale log line) | the game at cutscene switches | survived, 3 |
| 2026-11-16 00:51:07 | b028 | on | the game, the second cutscene's exit | **graphics card hung 7 s later** (`DXGI_ERROR_DEVICE_HUNG`) |
| 2026-11-19 00:20:21 | b032 | on | the game's menu: DLSS to Balanced | **game gone**: released, never rebuilt, the log ends |
| 2026-11-19 00:30:12 | b032 | on | the game's menu again: a 539 ms stall, then rebuilt | **device removed** (`GetDeviceRemovedReason`) |

`[measured: the kept UEVR logs in D:/claude video game stuff/logs/silent-hill-2-remake-vr/; the warpscale.txt 4th
number and the "handpaste: pictures made" lines for the hands' pass]`

- So: 5 of 5 survived before the hands' pass (b020, 2026-11-13 23:31), and 3 of 6 ended in a crash after it.
  The samples are small and mixed up with other changes: the menu route against the console, the warpscale hook that
  stands in the injector's call to the warp (same era), and 2026-11-19 starting with the warp already on. But the hands'
  pass and the warpscale hook are the two things present in every crash and missing from every early survival
  `[inferred from n=11 events; the cause itself is a hypothesis]`.
- One line that looked suspicious is NOT a difference: `FViewport::Draw called on a viewport with a different vtable`
  turns up at the first rebuild of every session, the survived 10:39:14 one included `[measured]`.
- A lead for the SH2-019 hunt (read in source, not tested). `handpaste.cpp` keeps the last frame's colour, depth and
  velocity pictures one frame longer than the injector does (`hold()`, AddRef), and samples them in the next made-up
  picture. It checks that the colour picture's size has not changed, but not the depth's or the velocity's. A DLSS
  rebuild (and a change of inner size) is exactly when the engine swaps those pictures. A held picture can stay alive
  while the memory under it is handed to something new (the engine shares memory between short-lived pictures). That
  could read garbage or hang the card `[hypothesis; inferred-static from handpaste.cpp lines 238-245, 600-619]`.
- **The profile right now:** `sh2vr_warpscale.txt` = `1.0 0 0 1 1.0 0.004 1 0` (4th number 1, so the hands' pass is
  ON). The latest log (2026-11-19 11:39, b033) shows the warpscale hook and `handpaste: pictures made` `[measured
  today]`. So any live `r.ScreenPercentage` step today runs in the condition that crashed.

## 3. sg.* groups: safe live, and they do nothing here

- Swept live, warp on, b016, DLSS Balanced (`sweep-2026-11-13-quality-groups-results.tsv`). Shadow, GI, Reflection,
  Effects, PostProcess, Shading and Foliage were each taken 3 -> 2, then four of them to 1: all within 0.15 ms. The
  picture change was 1.1 to 2.6 times the noise. There was no DLSS rebuild between 10:39:47 and the quit at 11:29, so
  none of the twelve changes rebuilt it, and nothing crashed `[measured 2026-11-13, n=1 each]`.
- The reason is not proven. The game sets its own cvars directly from its GFXSettings options, and the engine lets a
  value set by the game win over one set by a quality group `[hypothesis]`.
- Not measured: `sg.ViewDistanceQuality` and `sg.TextureQuality`. View distance must never be lowered (Tefa: no pop-in)
  whether or not it acts. `sg.ResolutionQuality` is not in the Unreal profile, which is good: it sets
  `r.ScreenPercentage`.

## 4. What Tune-o-matiC as it stands would do in SH2 `[inferred-static: tune.py, tune_knobs.py, tune_common.py, profiles/unreal-uevr.json]`

1. **Wrong start value.** `user_script.txt` has no `r.ScreenPercentage` line, so `start` takes the profile's default,
   step 4 = "100". The truth is 58 (Balanced).
2. **The guard can only make the frame heavier here.** Its range is 70 (floor) to 100 (the made-up top). Every step in
   it is ABOVE the game's own 58. The first "drop" sends 90: DLSS gets more pixels, the frame gets slower, more frames
   are late, and it keeps "dropping" to 70. Each step is a DLSS rebuild.
3. **It may set itself off.** Each step's hitch (two DLSS creates take about 43 ms, about 3 frames at 72 Hz) can count as
   "3 late frames in 2 s on two reads". That triggers the next drop, so it can feed itself `[hypothesis from the
   thresholds]`.
4. **It never raises back up, then pushes 100 when it stops.** A raise needs the card under 90 %, and this game's card
   sits at 96-98 % `[measured 2026-11-13]`. When the guard ends it pushes the top step ("100") live: DLSS at full size
   (about 25 ms on the simulator's matched load), plus one more rebuild.
5. **`undo` pushes 100 live too** (the start value it recorded), even after `tune.py set ... screen_pct 60`. `set`
   keeps the start value it already has.
6. **The file route does not work here.** UEVR runs `user_script.txt` about 5 s after the inject (`[execute_console_script]
   Loading user_script.txt...`). The game then writes its own values back: the file's `ScreenProbeGather.DownsampleFactor
   13` reads 16 in play on both days looked at `[measured: 2026-11-13 note section 3; 2026-11-19 11:40 log]`. So
   `r.ScreenPercentage` or `sg.*` lines written there are most likely overwritten at every start. If one ever did win,
   it would quietly override the DLSS level the game's menu shows `[inferred]`.
7. **It would fight our plugin.** `tuning.cpp` re-sets its six "set C" values every 5 s ("the game writes its own back").
   Tune-o-matiC must not list those six unless `sh2vr_notune.txt` is in the profile `[inferred-static: tuning.cpp]`.
8. The meter is a new OpenXR layer in the chain with UEVR's warp and Virtual Desktop. That has never been tried, and
   what "late" means when the warp makes up every other eye is not settled `[hypothesis]`.

**If Tefa wants the tool tested exactly as it is today:** keep it away from the inner size.
- `tune.py set <game> screen_pct 60` before `guard`: level 0 sits below the floor (1), so the guard never sends it.
- Never run `undo` while the game is running.
- Then only the `sg.*` lines move. That is harmless and does nothing here, so the test checks the tool's working parts,
  not whether it wins frames.
- Or run the SH2-019 simulator test first: `r.ScreenPercentage` steps through the connector, hands' pass on, then off
  (4th number 0).

## 5. Where SH2 keeps its real quality settings (question 2)

| place | what it holds | says what is real? |
| --- | --- | --- |
| `%LOCALAPPDATA%\SilentHill2\Saved\Config\Windows\GameUserSettings.ini`, `[/Script/GameBase.BGameUserSettings]` `GFXSettings=(GsaOptions=(...))` | the game's own options: `Adv_Supersampling` 4 = DLSS; `Adv_DLSSQuality` 2 = Balanced (58 %), 3 = Quality (66.7 %); `Adv_ShadowQuality`, `Adv_TextureQuality`, `Adv_EffectsQuality` 2; `Adv_ShadersQuality` 1; `Adv_ResolutionScalability` 3; `Adv_MotionBlur` 0; `DynamicRes` 0; `FrameGen` 0; `AdvQualityIsCustom=True` | **yes**: the game sets its cvars from this `[measured 2026-11-10 and 13; today's file read]` |
| the same file, `[ScalabilityGroups]` | `sg.*` all 3, `sg.ResolutionQuality=100` | no: live changes did nothing `[measured 2026-11-13]` |
| the same file, the struct fields | `Raytracing=On` (yet `("Raytracing", 0)` in the GsaOptions list); ray tracing IS on live (`r.Lumen.HardwareRayTracing 1`) | the struct field, not the list `[measured: file today, live 2026-11-13]` |
| `Engine.ini` | paths only, no cvar overrides | `[measured today]` |
| UEVR profile `user_script.txt` | 4 lines; the game overwrites the ones it owns | no `[measured]` |
| our plugin: `tuning.cpp` (set C, every 5 s), `sh2vr_devcvars.txt` (dev only) | the six cheaper values; the dev route | yes for those six |
| the running engine (connector `GET /api/console/cvar?name=...`, as `sweep.py` reads it) | the true live value of anything | **the best source while the game runs** `[measured 2026-11-13, sweep.py's "already" rows]` |

- Some values are owned by the game's post-process areas and change as you walk: `MaxRoughnessToTrace` read 0.05, 0.38
  and 0.39 in one minute `[measured: the 2026-11-13 12:55 log]`.
- The game rewrites its settings file by itself. The upscaler was found reset to off on 2026-11-10; the window was
  reset on 10-24 and again since `[measured]`.

**What an SH2 profile should read for its start values:** the live engine value through the connector's cvar read when
the game runs. With the game closed, the GFXSettings `Adv_*` entries through a small mapping table (`Adv_DLSSQuality`
3/2/1 -> 66.7/58/50; the 1 = Performance mapping is `[inferred]`). Never `user_script.txt`.

## 6. A PROPOSAL only: what an SH2 Tune-o-matiC profile would list (question 3)

Rules it follows: Tefa's "invisible savings first"; draw distance never lowered (no pop-in); DLSS on always; fog left
alone (Tefa said no); things missing from reflections not used. Costs are from the armory: Balanced plus set C =
15.2 ms; 72 a second needs 13.9 `[measured 2026-11-13, n=1 each]`.

| setting | steps (cheap -> best) | shows | costs | saves | live? | notes |
| --- | --- | --- | --- | --- | --- | --- |
| DLSS level (`Adv_DLSSQuality` in GFXSettings) | Performance, Balanced, Quality | 2 | 3 | Balanced -> Performance about -0.7 ms; Quality -> Balanced about -1.0 | **restart only, game closed** | an `ask`-style setting the session edits with the game shut; never the menu in play (SH2-019) |
| `r.Lumen.Reflections.HardwareRayTracing` | 0, 1 | 1 | 3 | -1.05 ms, picture 0.6 x noise | yes (swept live twice, no rebuild, no crash) | the best live lever for the guard; watch for reflections that differ between the eyes (board) |
| `r.Shadow.MaxResolution` | 1024, 1500 | 1 | 1 | -0.27, 1.0 x noise | yes | |
| `r.AmbientOcclusionLevels` | 0, -1 | 1-2 | 1 | -0.23 (at Quality), 1.9 x noise | yes | Tefa's eye first |
| `r.LocalExposure` | 0, 1 | 1-2 | 1 | -0.28 at Quality, nothing more on top of set C | yes | probably drop |
| `r.ScreenPercentage` (DLSS inner size) | 50, 54, 58, 62, 66.7; never above the DLSS level's own | 2 | 3 | about -0.35 ms per 4 points near Balanced | **live, but each step rebuilds DLSS** | out of the guard until SH2-019 passes with the hands' pass on |
| `OpenXR_ResolutionScale` (UEVR config) | e.g. 0.8, 0.9, 1.0 | 2-3 | 3 | not measured | restart only | |
| `r.ShadowQuality` | in-between steps unmeasured; 0 = -1.1 ms at 3.3 x noise | 3 | 2 | | yes | last resort, Tefa's eye |
| **left out** | all `sg.*` (nothing here); view distance (pop-in); fog grid (Tefa no); ray-tracing geometry switches (missing reflections); all ray tracing off (-1.3, 11 x noise: the light shows); the six `tuning.cpp` rows (already set; would be fought every 5 s) | | | | | |

What this says about 72 in the armory: set C + RT reflections off + shadows 1024 measured 14.2 ms. The armory is still
0.3 ms short with only near-invisible changes. Where Tefa stood on 11-13 it was already 13.9 `[measured, n=1]`. So the
guard's real job here is one lever, the RT reflections switch, in the heaviest views. The inner size can join it only
once the rebuild is proven safe.

## 7. Worth knowing

- The profile's `config.txt` holds `VR_RenderingMethod=3` now (written 2026-11-19 11:39). The launcher keeps the
  profile's `config.txt` (`KEEP_IF_PRESENT`), so starts begin with the warp already on. The 2026-11-19 logs say
  `warp: the frame warp was already on`. That worked on the real headset twice on 2026-11-19 (Tefa's wears), against
  the 2026-11-12 crash that `warp.h` was written for `[measured]`. The two DLSS-menu crashes were in sessions started
  that way; the 11-16 crash was in one switched on live, so this alone is not the cause `[measured]`.
- `uevr_mcp.dll` (the connector) is in the profile's plugins; it cost nothing with the warp on (18.03 against 18.05 ms)
  `[measured 2026-11-13]`.
