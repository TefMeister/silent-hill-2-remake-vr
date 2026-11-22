# Window View's OpenXR layer, read against last night's empty meters and start crashes (2026-11-22, /lm reader)

From: the background reader of the 2026-11-22 /lm session. For: the SH2 notes owner, and the later Window View
Developer job. Read only: nothing was launched, attached, deployed or changed. Sources: `window-view-vr` at d0a0014
(`layer/window_view_layer.cpp`, `layer/controllers.inl`, `keys.inl`, `lasers.inl`), `the-optimizer` meter
(`meter/src/meter_layer.cpp`, `meter_core.h`), the UEVR logs in `D:\claude video game stuff\logs\silent-hill-2-remake-vr\`
(`2026-11-21-23xx-meter-trials`, `-2243-optimizer-headset-meter-empty`, `-2147-optimizer-run`, `-2233-optimizer-headset-asleep`)
and four UE minidumps read offline with cdb.

## 1. The "1 wait, 0 frames" is not frames skipping the meter

- [measured] In all three empty-meter runs (22:43, 23:18-23:20, 00:02-00:04) UEVR's log says `D3D12: Failed to create
  swapchain` for the AFR left-eye picture, straight after it tore down and rebuilt its swapchains on
  `XR_SESSION_STATE_READY` (counts: 5, 77, 484 failures), then `AFR swapchains not created` every desktop frame
  (~53 a second) and `Frame already began, skipping xrWaitFrame call` thousands of times. UEVR waited once, began
  one frame and never handed one in. So the meter's 1 wait / 0 ends was the truth: no VR frames were sent at all.
  The "~57 frames a second" was most likely the desktop rate, not frames reaching the headset `[inferred]`.
- [measured] All 18 runs with a full meter show 3 swapchain creations and 0 failures. The crash runs at 23:12 and 23:53 also
  had 0; the crash at 00:02:17 had 129 (both faults in one run).
- [measured] `uevr-0003-meter0-windowview-alone.txt` and `uevr-0004-meter0.txt` are the same run (same start time), copied twice.
  So there were 3 empty runs, not 4.
- [inferred-static] Window View does not hook xrWaitFrame or xrBeginFrame. It hooks xrEndFrame and passes it on
  (`window_view_layer.cpp:616-631`, lock released at 623 before the call). It hooks xrCreateSwapchain nowhere
  (hook table 637-653). No code path lets a call skip the layer below it. In the empty runs no xrEndFrame ever
  happened, so Window View's per-frame drawing (`build_quads`, 475-559) never ran. Only its instance, session and
  controller hooks had run.
- [hypothesis] Why the runtime refused UEVR's eye swapchain is not explained by anything in Window View's code. The link is
  weak too: 3-4 empty runs out of 19 with it on, against 0 out of 12 with it off (Fisher about 0.12, could be chance).

## 2. The start crash

- [measured] All four kept dumps (11-18 23:40 sim, 11-21 15:11 sim, 21:47 sim, 22:33 headset) crash at the same place:
  `RenderThread 0`, game code `SHProto+0x380a03f`, `call [rax+0xF0]`. That is a call into an object that is already
  gone: the pointer was garbage in three dumps and null in one. The call chain is the same every time, on the
  simulator and on Virtual Desktop alike. Window View's DLL was loaded in all four but was on no thread's stack. Each
  crash came after UEVR's `Scene state passed to FSceneView constructor is null`, a line that good runs also print.
- [measured, from the trials table] Crashes: 8 out of 19 with Window View on, 0 out of 12 with it off (Fisher about 0.01).
  The link is real-looking, but the code shows no direct way for Window View to cause it.
- [hypothesis] Indirect ways it could, all in the code [inferred-static]:
  1. Long stalls inside the game's xrEndFrame, holding `g_mtx`: a GPU fence wait of up to 1 s (`wait_done`, 183-187,
     `kFenceWaitMs` 66), xrWaitSwapchainImage up to 100 ms per picture (339), and up to 8 swapchains made on the first
     frames (216-254). This only happens while the Window View app is running (481); whether it was running is unknown.
     A long hold here could widen a race that UEVR or the game already has.
  2. After a timeout the image stays acquired (341). The next frame acquires a second image, then waits on and releases the
     older one, but copies into the new index (333-345). That breaks the order the OpenXR spec requires, and the GPU
     copy can land on an image the compositor is still using.
  3. Window View's own action set is added to UEVR's bindings, attach and sync (`controllers.inl:147-195`). That changes
     the runtime's input state for every game, even with the app closed (unless settings.txt says `controllers=0`).
  4. It adds up to about 19 quads per frame (8 windows plus lights, arrows, beams, dots). The runtime says 16 layers at most,
     so a busy desk can make xrEndFrame fail (`XR_ERROR_LAYER_LIMIT_EXCEEDED`).
- [inferred-static] The layer has no window or focus code. The app's mouse and keyboard hooks swallow input only while
  MOUSE & KEYBOARD is on (`app/InputRouter.cs:1-9`). Nothing ties a crash to the game window losing focus.

## 3. Chaining compared

- [inferred-static] Meter: takes its next pointers once, after creating the instance (`meter_layer.cpp:211-214`). Its GIPA
  only swaps in its own function (192-193). Clean.
- [inferred-static] Window View: overwrites its global next pointers on every GIPA query (`window_view_layer.cpp:655`),
  with no lock. `fetch_next` fills only empty slots (664), so a second XrInstance in the same process keeps the first
  instance's pointers. Harmless with one instance, as here, but fragile.
- [inferred-static] Side finding: the meter writes its CSV header only when the first rows are flushed
  (`meter_layer.cpp:106-119`). That is why an empty run leaves a 0-byte file.

## Suggested next proof (for /lm, not done)

- With Window View's layer on, run the app CLOSED and then OPEN, about 6 starts each. That separates fault 1 (per-frame
  work) from fault 3 (action hooks).
- Then try `controllers=0` in its settings.txt.
