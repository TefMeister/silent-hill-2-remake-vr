# Reader: the OpenXR Simulator and UEVR frame waits (where the ~8 ms is NOT)

Background reader, 2026-11-05, beside the live SH2 session. Read from source unless tagged `measured`. Sources:
OpenXR-Simulator (webhead2oo9, tag v1.5.1 = master c1fcb17 except two READMEs), praydog/UEVR master 4ee5c6b
(2026-08-29), PureDark/UEVR branch `AFW` (fc8de102, 2026-08-20), elliotttate/UEVRRenderDoc. The installed
`openxr_simulator.dll` hashes identical to the v1.5.1 release DLL, so this source is the code that runs `[verified-live 2026-11-05, n=1 hash]`.

## 1. The simulator (src/runtime.cpp)

**(a) xrWaitFrame** (`xrWaitFrame_runtime`, line 4528) does four things, in order `[inferred-static]`:
1. `frameCondition.wait(lock, []{ return !running || frameLifecycle.CanWait(); })` with `CanWait() { return !waitPending_; }`
   (frame_state.h): it blocks only if the previous xrWaitFrame was never followed by xrBeginFrame. UEVR guards that
   (`frame_synced`), and calls both on the same thread, so this never holds.
2. A message pump `PeekMessageW(&msg, nullptr, ...)`; the window proc has no WM_PAINT handler (DefWindowProcW), cheap.
3. WASD/MCP command checks; the file reads run only when `mcp::g_commandsDue` (a change notification), not per frame.
4. Pacing, quoted: "Pace on a high-resolution waitable timer ... `WaitForSingleObject(pacingTimer, 20)`", then
   `nextTick += periodSec * freq;` and "After a stall, re-base instead of letting the app run uncapped:
   `if (now - nextTick > period) nextTick = now;`". Period is fixed 90 Hz (`periodSec = 1.0 / 90.0`).
   **This is a cap, not a vsync slot.** A caller that arrives late (frame already > 11.1 ms) finds `remainingSec <= 0`,
   breaks out at once and waits 0 ms; it is never held to the next 11.1 ms boundary. The longest possible wait is one
   period, only when the app is faster than 90 Hz. `predictedDisplayTime = now + period`, `shouldRender` always true.
   So the note's "13 ms of work misses the slot and waits for the next" cannot happen on this simulator (it can on a
   real runtime such as Virtual Desktop, which does align to the display) `[inferred-static]`.

**(b) xrEndFrame** (`xrEndFrame_runtime`, line 8101) `[inferred-static]`: brief `frameMutex` lock, validation, a reentrance
flag; per projection layer `presentProjection` -> `blitD3D12ToPreview` records a GPU blit on the simulator's own
`previewQueue12` (eyes scaled into a <=1920x1080 RT, copied to a readback buffer), ordered by fences: app queue
`Signal(crossQueueFence)` -> preview queue `Wait` -> `ExecuteCommandLists` -> `Signal(previewFence)` -> app queue
`Wait(previewFence)`. That last one is GPU-side only ("unlike the CPU wait it costs nothing"). The CPU never waits
on the GPU: `beginPreviewSlot` returns nullptr when all 3 slots (`kPreviewFrames = 3`) are busy ("the caller is the
app's render thread and must never wait for the GPU here"); `consumeCompletedPreviewFrame`: "Nothing here waits".
Then `paintPreviewComposite`: `Map` of an already-finished readback, one `StretchDIBits` to the window (GDI, no DXGI
Present on D3D12), screenshot/burst only on request, then the flicker detectors scan the pixels
(`flicker::ObservePreview/ObserveUi/ObservePaint`), then `WriteFrameStatus` (runtime_status.json). The old design
that blocked the render thread on a fence inside xrEndFrame was removed (comment at line 393-401).
**Measured by the simulator itself**, log of the 18:47 run, 300-frame averages, ~40 windows:
`xrEndFrame timing avg300: total=1.05-1.20 ms projection=0.05 overlay=0.006 detectPaint=1.0-1.1 status=0.02`
`[measured 2026-11-05, simulator log, n~40]`. So xrEndFrame costs ~1.1 ms of the present thread, nearly all of it the
GDI paint plus the flicker detector. Real, but 1 ms, not 8.

**(c) xrBeginFrame** (line 4969): lock, `frameLifecycle.Begin()`, `notify_all`: microseconds. xrAcquire/Wait/Release-
SwapchainImage (4132-4181): a bookkeeping state machine (swapchain_state.h), `Wait()` returns at once, no GPU touch; 3 images per swapchain `[inferred-static]`.

**(d) What the simulator records**: the `xrEndFrame timing avg300` log line (its own CPU cost); the window title / F3
p50/p95 frame time between stereo submissions (`ProjectionTimingTracker`), written to no file; `runtime_status.json`
has a live `frame_count`, but its `target_fps: 90` and `frame_time_ms: 11.1` are hard-coded constants
(mcp_integration.h 495-496), so `get_frame_info`'s frame_time_ms is not a measurement. xrWaitFrame's wait is logged nowhere.

**Verdict: the simulator is not the 8 ms** `[inferred-static + the 1.1 ms measured]`. From the game's side it would
show as UEVR's present thread parked inside openxr_simulator.dll: in `xrWaitFrame_runtime` ->
`WaitForSingleObject(pacingTimer)` / `frameCondition.wait`, or in `xrEndFrame_runtime` -> `StretchDIBits` / the
flicker scan. Sampling that thread's stack a few dozen times would settle it (x64dbg or ghidrust process_stack; a
new /lm job, never from this reader).

## 2. UEVR (praydog master)

**Where the hooks run.** `D3D12Hook::present_internal` wraps IDXGISwapChain::Present: `m_on_present` ->
`Framework::on_frame_d3d12` -> `m_mods->on_present()` -> `VR::on_present` -> `D3D12Component::on_frame` (copies,
xrBeginFrame if needed, xrEndFrame) -> the real `present_fn(...)` -> `on_post_present` -> `VR::on_post_present`.
All of it on the thread that calls Present, UE5's RHI thread `[inferred-static]`. The game thread never calls
xrWaitFrame; `update_poses` pre-empts `predictedDisplayTime` "before xrWaitFrame has finished on the previous frame"
(OpenXR.cpp 250-258). UEVR master passes the game's own Present interval through (nothing calls
`set_next_present_interval`), which fits the measured r.VSync gain.

**The three stages** (`VR::SynchronizeStage { EARLY=0, LATE=1, VERY_LATE=2 }`, VR.hpp:34; default 2:
`m_sync_mode{ ModCombo::create(generate_name("SynchronizationMode"), s_sync_mode_names, 2) }`) `[inferred-static]`:
- **Early**: `pre_render_viewfamily_renderthread` (render thread) installs `hook_new_rhi_command(cmd_list->root, ...)`;
  when the RHI thread executes that command: `RHIThreadWorker::get().execute(); if (stage == EARLY) {
  synchronize_frame(); ... begin_frame(); }` (FFakeStereoRenderingHook.cpp 2966-2980, same at 2850-2866). So
  xrWaitFrame + xrBeginFrame sit at the start of the frame's RHI work; xrEndFrame at Present.
- **Late**: `VR::on_present`: `if (is_left_eye_frame && get_synchronize_stage() == LATE) runtime->synchronize_frame();`
  (VR.cpp 2131-2133), then in `D3D12Component::on_frame`: `if (!frame_began) begin_frame();` (642-644), the copies,
  `end_frame(quad_layers, ...)` (677). Wait, begin, copy, end back to back inside the Present hook.
- **Very Late**: `VR::on_post_present`: `if (stage == VERY_LATE || !got_first_sync) runtime->synchronize_frame();`
  then `if (openxr && ready && stage > EARLY && !frame_began) begin_frame();` (VR.cpp 2223-2235). The NEXT frame's
  xrWaitFrame + xrBeginFrame run right after Present returns; the next Present only copies and ends.
In every stage xrWaitFrame and xrBeginFrame are on the same RHI/present thread, never the game thread.

**What else waits in the Native Stereo (RenderingMethod 0) submit, every stage** (`D3D12Component::on_frame`):
`m_game_tex_commands[idx].wait(INFINITE)` (line 201, ring of 3 by back-buffer index: the spectator copy of 2-3 frames
ago); per `m_openxr.copy(...)` (1668+): `xrAcquireSwapchainImage` -> `xrWaitSwapchainImage` ->
`texture_ctx->commands.wait(INFINITE)` (1751: the fence of the copy last recorded into THIS image, 3 images, so 3
frames old) -> `CopyTextureRegion` of the whole double-wide back buffer -> `execute()` = ExecuteCommandLists +
Signal + SetEventOnCompletion, no wait -> `xrReleaseSwapchainImage`. Copies per frame: DOUBLE_WIDE (549), DEPTH if
depth is on (556), UI / FRAMEWORK_UI when drawn. `Framework::on_frame_d3d12` then `cmd_ctx->wait(INFINITE)` on a
3-ring for ImGui. `wait_for_present()` (VR.hpp:387, `WaitForSingleObject(m_present_finished_event, 11)`) runs only
when the HMD-active state flips (FFSR.cpp:4649), not per frame. Every fence here is 2-3 frames old, so at 63-71 %
GPU load they should return at once `[inferred-static]`. Nothing in this path is a known multi-ms wait.

**SyncedSequentialMethod** (SKIP_TICK / SKIP_DRAW) applies only `if (in_engine_tick && vr->is_using_synchronized_afr()
&& g_frame_count % 2 == 0)` (FFSR.cpp:2520); `is_using_synchronized_afr()` = RenderingMethod SYNCHRONIZED, or
NATIVE_STEREO with extreme-compat mode (VR.hpp:415). With RenderingMethod 0 and extreme-compat off it does nothing.

**Which stage blocks least, and is there a known wait one of them removes?** No stage removes a wait; they only move
xrWaitFrame (before the frame's RHI work / before Present / after Present). On this simulator the choice is moot:
xrWaitFrame returns in 0 ms for a late caller whatever the stage, which matches "sync mode 0/1/2: no change"
`[inferred-static; the note's measurement agrees, n=1]`. On a real runtime the wait is the pacing itself. The one
public fork that measured it (elliotttate/UEVRRenderDoc, built on PureDark AFW, Subnautica 2, 2026-07-09, sync LATE)
reports "`xrWaitFrame` is already near-free on this runtime"; its real cost was `xrEndFrame` 1.45-1.77 ms and
`d3d12_on_frame` 2.33 ms inside the Present hook; moving xrEndFrame to another thread (its opt-in
`VR_OpenXRAsyncEndFrame`) cut d3d12_on_frame to 0.86 ms and lifted 77 -> 80 fps `[reported, their runtime, not ours]`.
UEVR has a built-in "xrWaitFrame took N ms" logger (`profile_calls`, OpenXR.hpp:138-176) but it is a plain `false`
with no setting: a rebuild would be needed, and the installed build is PureDark's.

**AFW.** The UEVR side of PureDark's build IS public (github.com/PureDark/UEVR, branch `AFW`, 50 commits ahead of
praydog master; `Joey-Merged` 2026-10-30). The frame warp itself is not: only `PDAFWPlugin.h` and a dummy .cpp are
in the tree, the real `PDAFWPlugin.dll` is closed. In the public branch "Ultra Responsive" is a toggle
(`is_using_ultra_responsive()`, VR.hpp:115) that UObjectHook reads to skip attach smoothing (UObjectHook.cpp 767,
787); what it does inside the plugin is not public, so not described. The AFW branch keeps the same three sync sites
(`|| is_using_afw()` added, VR.cpp 2710-2712, 2810-2813) and adds no CPU fence waits (8 wait calls in
D3D12Component.cpp, same as master). The installed AFW-beta.6 is newer than the branch read (its log cites
D3D12Component.cpp:1592) `[reported]`.

## 3. So where to look next `[hypothesis]`

Neither the simulator nor UEVR's xrWaitFrame can hold a thread for 8 ms here. Left: (1) UE5's own pipeline
back-pressure (game thread waiting on the previous frame's render/RHI thread; RHI thread blocked inside DXGI Present
by the frame-latency limit, which Present does even at interval 0); (2) UEVR's serial work on the RHI thread inside
the Present hook (ImGui + copies + xrEndFrame, 2-3 ms in the fork's numbers, plus the simulator's 1.1 ms); (3) DLSS.
Per-thread CPU time cannot see a wait; stack samples of the game thread and the present thread can.
