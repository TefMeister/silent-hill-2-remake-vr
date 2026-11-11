# The frame warp's crash at start and the DLSS ghost picture on the real headset: what the logs and the public sources say (2026-11-12, /pd, Developer PC, Fable)

The game was not launched for this note. Everything here is read from three UEVR logs kept from Tefa's wear of
2026-11-12 00:34 to 00:44 (`logs/silent-hill-2-remake-vr/uevr-log_2026-11-12_*.txt`, outside git, on the Developer
PC), from PureDark's public UEVR source (branch `AFW`) and release notes, and from the public source of Virtual
Desktop's OpenXR layer (mbucchia/VirtualDesktop-OpenXR, `main`; the installed copy may be newer or older). One piece of
code was written and compiled, not run.

## What happened in the headset (Tefa, Quest 3 through Virtual Desktop, build v0.1.0-b006)

| start | settings | result |
| --- | --- | --- |
| 00:34 | `VR_RenderingMethod=3` (alternate-eye warp), DLSS Quality | crash about 10 s after the inject `[verified-live 2026-11-12, n=1]` |
| 00:35 | `VR_RenderingMethod=0` (both eyes), DLSS Quality | runs at 19.3 ms; "a seethrough layer of a game on top of what i'm seeing" `[reported 2026-11-12, n=1]` |
| 00:42 | `VR_RenderingMethod=0`, DLSS off | "wasn't there anymore, just a really bad framerate": 53 ms in play `[reported / measured 2026-11-12, n=1]` |

## 1. The crash: Virtual Desktop refuses the per-eye pictures the second time they are asked for

Read from the 00:34 log `[measured 2026-11-12, the log]`:

1. At the inject UEVR makes eight OpenXR swapchains, the two per-eye ones (`AFR left/right eye`, 2688 x 2880) among
   them. All eight succeed.
2. 70 ms later the session turns READY; UEVR destroys all eight (each "Destroyed swapchain N", no error) and makes them
   again. The very first one, `Creating AFR left eye swapchain`, fails: `D3D12: Failed to create swapchain.`
3. It tries again 16 more times over the next six seconds (00:34:23.666 to 00:34:28.642). Every try fails at that same
   first swapchain. 17 failures, no success.
4. `InitFrameWarp` still runs at 00:34:28. From 00:34:30 the log is `Trying to copy to swapchain 4 / 8 / 9 but it
   doesn't exist` and `AFR swapchains not created` (373 lines), and at about 00:34:32 the game dies with an access
   violation (the Unreal crash window).

In the 00:35 log (both eyes in one double-wide picture, 5376 x 2880) the same destroy-and-make-again at READY
succeeds. So the refusal is not "Virtual Desktop cannot remake pictures", it goes with the per-eye layout
`[measured, n=1 each way]`.

What the sources add `[inferred-static 2026-11-12]`:

- UEVR (`D3D12Component.cpp`, `create_swapchain`): the message is printed when `xrCreateSwapchain` returns anything
  but success. **The error code is not logged.** The request is the same structure both times; the only field that
  can differ between the first and the second pass is `sampleCount`, copied from whatever texture UEVR treats as the
  back buffer at that moment (the game's own on the first pass, UEVR's VR render target once it exists).
- Virtual Desktop's layer (`swapchain.cpp`): before it asks Virtual Desktop itself, it can only refuse for an unknown
  format, a cube map, or an invalid session; none fits a request that had just succeeded. A `sampleCount` other than 1
  takes its own slow path and does not fail there. That leaves the call into Virtual Desktop's closed part
  (`ovr_CreateTextureSwapChainDX`). Its destroy is synchronous, so "the old ones were not gone yet" is unlikely.
- **So the cause is not established.** `[hypothesis]`, in the order I would test them: (a) starting with the warp
  already on is simply not a supported route (next section); (b) a wrong `sampleCount` or size on the second pass,
  which the simulator would show; (c) a limit inside Virtual Desktop that eight per-eye-sized pictures hit.
- The crash itself is UEVR going on to draw into pictures it never got. Our plugin is not on that path.

### The author's own instructions say: switch the warp on AFTER injecting

PureDark's release notes, every beta: *"When Injecting, you can't select AFW cuz it's not an option in the injector,
select AFW in the menu after."* and *"Always tick the Ghosting Fix"*, *"Launch the game and turn on DLSS/DLAA"*, *"uses
about 500mb extra VRAM"* `[reported, github.com/PureDark/UEVR releases, read 2026-11-12]`. Beta 4: *"Fixed crashing when
changing resolutions or toggling 2d screen mode"*, so switching at run time is a path he tested.

This project has only ever switched it on from the settings file before the start (2026-11-10, simulator, where it
worked). The headset start of 00:34 was that same route. **The untried route is the supported one:** start with both
eyes (0), then switch to 3 once the picture is up.

Also: the build in our packages logs `Tag: UEVR_AFW_v1.0-beta.5`; **beta 6 (2026-08-13) is newer**: "Catching up with
upstream (nightly 1139 and latest joeyhodge), thus fixed the memory leak with some new UE titles" `[reported]`. The
/gr note of 2026-11-07 speaks of a `UEVR AFW b6` folder, but the 0.1.2 package's `uevr/UEVRBackend.dll` carries the
beta 5 tag `[measured 2026-11-12, strings]`.

### Built for it: a live switch for the injector's own settings

`dev-archive/native/src/devcvars.cpp`: a line `uevr <Setting> <value>` in `sh2vr_devcvars.txt` now sets one of the
injector's own settings through its plugin interface (`set_mod_value`), logged `devcvars: uevr VR_RenderingMethod 0 ->
3`. So the warp can be switched on after the start by writing one line into a file, with no headset menu and no
connector. Builds clean `[compile-verified 2026-11-12]` (pd root, `dev-archive/native/build/Release/sh2vr.dll`, sha256
0f9b5d1848064275...). **Not installed, not run.** Whether the injector acts on a rendering-method change made this way
the same as on its menu (it must remake the swapchains) is not known until it is tried `[hypothesis]`. (The header's
`set_mod_value` template only compiles for numbers; the call goes to the function pointer directly.)

## 2. The ghost picture with DLSS on, both eyes

What the 00:35 log shows `[measured]`: two DLSS instances are created at 00:36:21 (`hk_NVSDK_NGX_D3D12_CreateFeature
FeatureID 1`, twice) and released at the quit; three scene states are known to UEVR. So each eye has its own DLSS
instance and its own history: **the ghost is not two eyes sharing one history** `[inferred-static from the log]`.

What the source shows `[inferred-static]`:

- In this mode (0) UEVR's DLSS hook only reads the pass's inputs and hands the call on unchanged (`VR.cpp`,
  `hk_NVSDK_NGX_D3D12_EvaluateFeature`: every write is inside `is_using_afw()`). So the injector's warp code is not
  what paints the ghost.
- UEVR's "Ghosting Fix" (on in the profile) only does anything for the alternating modes (`is_using_afr()`), not for
  both-eyes-at-once.

So what is left, none of it proven `[hypothesis]`:

1. **The head moves between the game computing its motion vectors and UEVR's late pose update** (`VR_SynchronizationMode
   = 2`, Very Late, in the profile since 2026-11-06). DLSS trusts motion vectors and keeps history that TAA would throw
   away, so a mismatch shows as a see-through copy of the last frames; TAA with the same mismatch looks clean. This
   fits "clean without DLSS", and it fits the simulator never showing it (its head stands still unless told to move).
2. DLSS at the headset's size being fed a wrong region (the straight upright edges in the screenshots look like a
   rectangle boundary, not like a trail).
3. The author's position: DLSS is there to be used WITH the warp, not with both eyes at once. If the warp works, this
   mode stops mattering.

## The tests this leaves (each one command; no headset needed for the first three)

| # | where | do | it means |
| --- | --- | --- | --- |
| A | simulator | per-eye size set to 2688 x 2880 (fits: the 4096 limit is for the double-wide picture), start with `VR_RenderingMethod=3` | the simulator's log prints every `xrCreateSwapchain called: ... sampleCount=...`: a second-pass request that differs from the first = UEVR's bug, and the way round is the switch-after-start route or beta 6. Identical requests = it is Virtual Desktop's refusal. |
| B | simulator | start with 0 and DLSS on, then `uevr VR_RenderingMethod 3` in the settings file | does the injector take a rendering-method change from a plugin: `After InitFrameWarp` in its log and per-eye swapchains in the simulator's. If not, the switch has to come from its menu. |
| C | simulator | mode 0, DLSS on, the head swept (pose sweep), stills; then the same with `uevr VR_SynchronizationMode 0` / `1` | ghost only while the head moves and gone at 0 or 1 = lead 1. Ghost with a still head = lead 2. No ghost at all on the simulator = it needs the real size or the real runtime. |
| D | real headset, Tefa | B's route: start with 0, then the one line | the warp either comes up (the 69-a-second estimate can finally be worn) or the log names the same refusal after a supported switch. |
| E | real headset | a trace of Virtual Desktop's layer (`VirtualDesktopOpenXR.wprp` sits in the Streamer's OpenXR folder; `wpr -start <that file> -filemode`, run, `wpr -stop out.etl`) | the exact error code of the refused call. Only if A and D leave it open. |

## Not established

- Why Virtual Desktop refuses the second request. No error code exists in any log we have.
- Whether the simulator at the real per-eye size reproduces either fault.
- Whether beta 6 behaves differently. Its notes do not mention swapchains or Virtual Desktop.
- That the ghost needs head movement: nobody has looked for it with a still head.
