# UEVR 2D screen mode, read from the source (2026-11-12, the /lm reader)

For: the modding lane (cutscenes on a flat screen). Nothing here was run; no game was touched.

**Source used:** `github.com/PureDark/UEVR`, branch `AFW`, commit `fc8de10` (2026-08-20), shallow clone in a scratch
folder. It is the right source: our log's warning line number (`FFakeStereoRenderingHook.cpp:2926`) is exactly that
line in this commit, and its `include/uevr/API.h` is plugin API 2.39 like ours. Short names below:
`FFSR` = `src/mods/vr/FFakeStereoRenderingHook.cpp`, `D3D12` = `src/mods/vr/D3D12Component.cpp`,
`VR.cpp/.hpp` = `src/mods/VR.*`, `Overlay` = `src/mods/vr/OverlayComponent.cpp`, `OpenXR` = `src/mods/vr/runtimes/OpenXR.cpp`.

Tags: **[read]** = read in the source at the line given. **[inferred]** = my conclusion from what was read, not
written anywhere, to be checked live.

## The headline

1. **In 2D mode UEVR draws the scene with a hard-coded 90 degree horizontal view angle.** `FFSR:5243-5275`:
   `float fov = 90.0f; // todo, get from FMinimalViewInfo`, a plain symmetric matrix built from 90 degrees and the
   picture's width/height. The game camera's own angle is never looked at. [read]
   So the too-wide framing is not the headset's angle (the 95.9 we measured comes from `get_ue_projection_matrix`,
   which always hands back the headset's matrix, `PluginLoader.cpp:1288-1291` [read]); it is this fixed 90.
2. **No setting changes that.** Nothing in the config reaches the 2D projection. [read, list in section 2]
3. **"Disable VR" is not a way out**: key only, and the headset would very likely be left on a frozen picture. [section 3]
4. **Our plugin can fix it with one small hook** (section 4): wrap the stereo device's projection function, let UEVR
   run, then overwrite two numbers of the matrix with the cutscene camera's angle. Plus, with the callbacks we
   already have, put the eye position back so both eyes get the same picture (no depth, and the mirror gets one view).

## 1. What 2D screen mode changes

- `is_using_2d_screen()` is just the config toggle `VR_2DScreenMode` (`VR.hpp:687-688`, `:1010`). [read]
- **Head rotation: NOT applied.** `FFSR:4998-5010`: the view rotation is only replaced by the headset's when
  `!is_2d_screen`. The game camera's rotation stays. [read]
- **Head position: NOT applied.** `FFSR:4984-4996`: `head_offset` is subtracted only when `!is_2d_screen`. [read]
- **Eye offset (the distance between the eyes): STILL applied.** Same lines: `*view_d -= eye_separation` runs in
  both modes, with the real eye distance times world scale. So the two eyes are drawn from two spots, and the flat
  screen is a stereo pair (depth inside the screen), not one picture. [read] Probably why a mirror or a wall close
  to the camera looks different per eye. [inferred]
- **Camera offsets still applied**: `VR_CameraForwardOffset / RightOffset / UpOffset` are added before the 2D
  check (`FFSR:4937-4954`). [read]
- **Projection:** fixed 90 degrees horizontal, aspect = `get_hmd_width / get_hmd_height` (`FFSR:5243-5273`). [read]
- **Picture size:** in 2D mode `get_hmd_width/height` return the game window's own size times the OpenXR
  resolution scale (`VR.cpp:2840-2870`), so each eye is drawn at the window's aspect (16:9 for a 16:9 window), not
  the headset's. `adjust_view_rect` still makes a double-wide target, one half per eye (`FFSR:4763-4777`). [read]
- **Stereo stays on:** `is_stereo_enabled` does not look at 2D mode (`FFSR:4643-4719`); two views are still
  rendered with method 0 (`get_desired_number_of_views_hook`, `FFSR:5469-5508`). [read]
- **How it reaches the quad** (`D3D12:271-345`): the left half of the scene target is drawn into a "2D Screen
  Texture" of the window's size (`D3D12:1364-1385`), the game's UI is drawn on top; the right half (or the
  native-stereo-fix capture texture) goes into a second one. Then the scene target is cleared to black. These two
  are copied into the OpenXR `UI` and `UI_RIGHT` swapchains (`D3D12:380-398`) and shown as two quads, one per eye
  (`D3D12:840-850`). Same aspect in and out, so stretched to fit, not cropped. [read]
- **The quad's shape** = the UI swapchain's width/height (the window's size, `D3D12:1719-1735`) times `UI_Size`
  metres high, at `UI_Distance`, fixed in the room unless `UI_FollowView` (`Overlay:841-866`). [read] That matches
  our 6.2 x 3.5 m at size 3.5.
- **Frame warp is off in 2D mode** and method 3 falls back to "synchronized" drawing (`VR.hpp:513-523`); switching
  in or out sets a 90-frame settle counter (`VR.cpp:2777-2780`). [read]
- Scene-view compatibility mode has one 2D line: it leaves the view's projection alone in 2D (`FFSR:3156-3164`),
  but the matrix it leaves alone is the 90-degree one from above, so no gain. [read + inferred]

## 2. Is there a setting that keeps the game camera's own angle? No.

Every option that touches the view, and whether it changes the camera in 2D mode:

| Option | Where | In 2D mode |
| --- | --- | --- |
| `VR_HorizontalProjectionOverride`, `VR_VerticalProjectionOverride`, `VR_GrowRectangleForProjectionCropping` | `OpenXR:486-530` | Only reshape the HEADSET matrix; 2D returns before that matrix is used (`FFSR:5275`). No effect. [read] |
| `VR_CameraForwardOffset / Right / Up` | `FFSR:4937-4954` | Yes, still moves the camera. Should be 0 while flat. [read] |
| `VR_WorldScale` | `FFSR:4942`, `:4975-4978` | Only scales the eye distance in 2D (head offset is skipped). [read] |
| `VR_DecoupledPitch` | `FFSR:4931-4935` | Only changes the direction of the offsets above; the rotation is not written in 2D. Matches our "same pictures" test. [read] But with `UI_FollowView=false` it tilts the QUAD if `VR_DecoupledPitchUIAdjust` is on (`Overlay:846-851`). [read] |
| `VR_RoomscaleMovement` | `FFSR:5012-5064` | STILL runs in 2D: moves the pawn by the head's walk. Harmless in a cutscene, but not switched off. [read] |
| `VR_AimMethod`, `VR_AimUsePawnControlRotation`, `VR_AimModifyPlayerControlRotation` | `FFSR:5015-5046`, `:5083-5088` | Not gated by 2D; they act on the pawn / control rotation, not on a cutscene camera. [read; effect on a cutscene inferred: none] |
| `VR_LerpCameraPitch/Yaw/Roll` | `VR.cpp:1795-1830` | Not gated by 2D; smooths the camera rotation before the offset step. Off for us, keep off while flat. [read] |
| `VR_GhostingFix` | `FFSR:3203-3224` | Only swaps scene states under alternating methods; with method 0 nothing. [read] |
| `VR_NativeStereoFix` (+ `SamePass`) | `FFSR:3172-3186`, `D3D12:313-321` | Changes where the right eye's picture comes from; projection the same 90. [read] |
| `VR_RenderingMethod` | `VR.hpp:505-523` | Changes how the two eyes are produced; projection the same 90. Matches "method 3: mirror exactly as wrong". [read] |
| `VR_Compatibility_SceneView`, `VR_Compatibility_SplitScreen` | `FFSR:3094-3168`, `:4874` | See end of section 1: no gain. [read] |
| `VR_ExtremeCompatibilityMode` | `VR.cpp:2849`, `FFSR:5542` | A different, riskier pipeline; the 2D projection branch is the same. [read] |
| `VR_CameraFOVDistanceMultiplier` | declared `VR.hpp:1097` | No use found in the view or projection code. [read] |
| "Disable Projection Matrix Override" (debug tick box) | `VR.cpp:3300`, `VR.hpp:1303` | A tick box that nothing reads. Dead. [read] |
| `VR_EnableCustomZNear / CustomZNear` | `VR.cpp:1921` | Near plane only. [read] |

## 3. The "Disable VR" key

- It is a plain on/off variable `m_disable_vr`, flipped only by the key `VR_DisableVRKey` (`VR.cpp:2363-2365`,
  `VR.hpp:1128-1129`, comment "definitely should not be persistent"). It is not a config value, so
  `set_mod_value` cannot set it. [read]
- A plugin could only do it sideways: set `VR_DisableVRKey` to some key with `set_mod_value` (it is an ordinary
  value in the options list, `VR.hpp:1235`), then post that key-down to the game window, because UEVR's key state
  comes from the window's key messages (`Framework.cpp:967-985`). [read + inferred]
- What it does: `is_hmd_active()` returns false (`VR.hpp:363-366`). From the next frame `is_stereo_enabled`
  returns false (`FFSR:4680-4694`), UEVR's own render target is no longer handed out (`FFSR:5546-5558`), every
  UEVR view hook passes straight through (`FFSR:3043`, `:2431`, `:6208`). So the engine draws plain mono, its own
  camera and angle, into the game window. [read; "the window then shows the true flat game" inferred]
- **The headset:** on OpenXR the present path does not stop (`VR.cpp:2596-2625` only returns early for OpenVR);
  `D3D12::on_frame` keeps copying UEVR's old render target, which nothing draws into any more, and keeps ending
  frames (`D3D12:27-75`, `:826-880`). So the headset would show the last VR frame frozen (re-projected by the
  runtime as the head moves), with a stale UI quad; never the flat game. [inferred from those lines, not seen]
  It also re-creates textures on the way back (`FFSR:4683-4686`). Not a route for cutscenes.

## 4. What the plugin API gives, and the smallest fix

In `API.h` 2.39 (ours and the fork's are the same version) [read]:

- `on_early / on_pre / on_post_calculate_stereo_view_offset`: called in 2D mode too, with the **stereo device
  pointer** as first argument (`PluginLoader.cpp:2115-2155`), the view index, and pointers to the position and
  rotation the engine will use (doubles on UE5). `pre` runs before UEVR's offsets, `post` after
  (`FFSR:4882-4887`, `:5070-5073`).
- `on_pre / on_post_viewport_client_draw`, `on_pre / on_post_slate_draw_window_render_thread`.
- `stereo_hook->get_scene_render_target()` / `get_ui_render_target()` and `frhitexture2d->get_native_resource()`:
  the D3D12 textures UEVR draws the eyes and the UI into. `render_target_pool_hook->get_render_target(name)`.
- `renderer` data: the D3D device, swapchain, command queue.
- **No call for the projection.** `get_ue_projection_matrix` is read-only and is the headset's. No setter, no
  callback around the projection function.

**Proposed fix, two parts.**

A. **Same picture for both eyes, with what we have** (no new hook): in `on_pre_calculate_stereo_view_offset` keep
   the position; in `on_post_...` write it back while our flat screen is on. That removes the eye distance and the
   camera offsets, so both eyes are drawn from the cutscene camera's exact spot: a truly flat picture, and one
   viewpoint for the mirror. [inferred; small, reversible]

B. **The view angle: wrap the projection function.** The engine asks the stereo device for the matrix through its
   table of functions; the entry sits right after the view-offset entry (`FFSR:790`, `:950`), and on the standard
   path UEVR logs it at start: `Found CalculateStereoProjectionMatrix: <index> <address>` (`FFSR:1012`). UEVR hooks
   the engine function itself (inline, `FFSR:1050`), so the table entry is still the engine's address. [read]
   - Take the device pointer from our view-offset callback, replace that one table entry with our function.
   - Ours calls the old entry first (UEVR does all its own work), then, only while our cutscene flat screen is on,
     overwrites two numbers in the result (doubles on UE5): `m[0][0] = 1 / tan(camFOV/2)` and
     `m[1][1] = (width/height) / tan(camFOV/2)`, the same form UEVR itself writes at `FFSR:5246-5258`, with the
     cutscene camera's horizontal angle in place of 90. Nothing else in the matrix changes. [inferred design]
   - Install only if the entry looks as expected (index from UEVR's log line, first byte a jump into UEVR);
     otherwise do nothing and log it.

   What could go wrong [all inferred]:
   - **Which angle.** The camera manager said 26, the mirror component 23.9 / 19.8 at different moments. The angle
     must be read each frame from the same camera the frame is drawn from; settle live which source matches the
     plain game (compare stills).
   - **Aspect and black bars.** The picture is drawn at the window's aspect. A cutscene camera with a fixed film
     aspect gets black bars in the plain game; the stereo path ignores that, so at the same horizontal angle we
     would show extra picture above and below instead of bars. If that shows things not meant to be seen, mask it
     or shape the quad.
   - **The view rectangle** is untouched (still one window-sized half per eye), so nothing to change there.
   - **DLSS / TAA**: the engine adds its own jitter after this matrix and handles camera cuts itself, so the change
     of angle at a cut should behave as in the plain game; the first frames after our switch may smear once.
   - **The quad's shape** stays the window's aspect; right for 16:9 cutscenes.
   - **Timing**: a few frames at the switch where 2D is on and our flag is not (or the reverse) show the wrong
     angle; set our flag before `VR_2DScreenMode=true` and clear it after `false`.
   - **A UEVR update** can move the table entry; the install check above covers it.
   - If the game took the non-standard hook path (UEVR builds its own table, `FFSR:1376-1460`), the log line is
     absent; the table swap still works but the index must be found by hand (connector `uevr_vtable`).

**Not an option: rebuilding UEVR with the one-line fix.** The repo holds only a dummy of PureDark's frame-warp
module (`dependencies/pd-afwmod/dummy/PDAFWPlugin.cpp`), so a build from this source would lose the real frame
warp. [read] The clean long-term fix is to ask PureDark (a GitHub issue) to do the `todo` at `FFSR:5244`.

**Not an option: cropping the 90-degree picture.** A 20-degree shot is the middle 18 % of the 90-degree picture;
far too few pixels. [inferred]

## 5. The warning "the vtable is already hooked, maybe previous frame was not rendered?"

- `FFSR:2896-2935`. Each time the engine starts rendering a set of views that carries UEVR's extension
  (`pre_render_viewfamily_renderthread`, `FFSR:3679`), UEVR borrows the first command of the render command list
  to pass the head pose along, and gives it back when that command runs (`FFSR:2868-2871`). The warning is logged,
  at most once a second, when it is asked to borrow a command that it has already borrowed and that has not run
  yet. Nothing breaks at that point: it just skips the second borrow. [read]
- In this build the borrowing is always queued to the UI drawing thread (the direct path is dead code, the test at
  `FFSR:3814-3818` is always false, as its own comment admits), and runs when the UI is next drawn
  (`FFSR:3746-3806`). So it fires whenever **two or more of those requests pile up between two UI draws**: two
  sets of views in one frame, or a frame with no UI draw. [read + inferred]
- **2D mode itself is not in this code**: no line there looks at it. [read] With method 0 one frame is one set of
  views, so 2D mode alone should not trigger it. [inferred]
- Likely cause in the opening cutscene: a second set of views per frame, i.e. the fake mirror's own capture going
  through the same extension. That would also fit the right-eye fault showing only with a mirror in shot.
  [inferred, not proven] Cheap check: does the line appear with the flat screen forced on in play away from any
  mirror (expect no), and in a mirror cutscene with the flat screen off (expect yes)?
- "Once a second" in our log is the rate limit, not the real rate; it may be every frame. [read]

## Loose ends noticed

- `get_ue_projection_matrix` must not be used to learn what is drawn in 2D mode (section headline). The mirror
  feed of this evening used it (95.9); in 2D the drawn angle was 90.
- The mirror-feed try read doubles correctly (`mirror.cpp` handles `is_double`), so the odd position it logged
  (`266 5382 -338`) is not a float/double mix-up; still unexplained.
