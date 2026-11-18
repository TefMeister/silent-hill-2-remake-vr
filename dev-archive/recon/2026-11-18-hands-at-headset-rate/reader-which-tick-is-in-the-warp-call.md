# AFW: which game tick's view is in the frame-warp call? (2026-11-18, the /lm reader, static read only)

Source read: PureDark/UEVR, branch `AFW`, head `fc8de10` (2026-08-20). Nothing was run. Line numbers are that
commit's. Short names: `FFSR` = `src/mods/vr/FFakeStereoRenderingHook.cpp`, `VR.cpp` / `VR.hpp` = `src/mods/`,
`D3D12` = `src/mods/vr/D3D12Component.cpp`, `OpenXR.cpp` = `src/mods/vr/runtimes/OpenXR.cpp`.
"Tick F" below = the game tick whose view family carries the engine frame number F.

## Answer in one paragraph

In normal running the call's `cameraData[nEye].srcViewToWorldMatrix` IS the view of the same tick whose picture is
being warped. UEVR's `offset` exists exactly to step back to that tick when the game thread has already run two
ticks further. It can only go wrong towards LATER (the view of tick F+2 with the picture of tick F, same eye),
never earlier, and only in the corner cases listed in section 4. Because `drawnpose` finds its record by the
call's own eye position, it always finds the record of the tick the VIEW belongs to; so the record is the
picture's tick whenever UEVR's view is.

## 1. `offset` in `update_camera_data`

- `VR.cpp:2476-2527`. Runs once per frame number (guard `VR.cpp:2478`). Called from `on_present` with
  `m_render_frame_count` (`VR.cpp:2703`), and earlier in the same frame from the DLSS hook when the ghosting fix
  and the object motion vector fix are on (`VR.cpp:122`); the second call for the same number does nothing.
  `[inferred-static]`
- `offset = last_update_matrix_frame_count[nEye] - last_update_camera_data_frame_count`, clamped to 0..2, then
  integer-halved: 0 or 1 stays 0, 2 or more becomes 1 (`VR.cpp:2501-2502`). Index 2 of the ring is never read.
  `[inferred-static]`
- The ring `render_view_inv_matrix[eye][0..2]` is pushed on the game thread at each stereo-offset call:
  `[2]=[1]`, `[1]=[0]`, `[0]=new`, and `last_update_matrix_frame_count[eye] = g_frame_count`
  (`FFSR:5152-5156`). `[0]` = that eye's newest view, `[1]` = that eye's view before it (two ticks older,
  because the eyes alternate). `[inferred-static]`
- The two counters are labelled the same way: during tick F the game thread's `g_frame_count` is F-1 (section 3),
  and during the present of picture F `m_render_frame_count` is still F-1 (it only moves in `on_post_present`,
  `VR.cpp:2790`). So for picture F: `nEye = (F-1) % 2` on both sides (`FFSR:4829`, `VR.cpp:2483`, `D3D12:96`,
  `D3D12:459`), and the difference is 0 when the game thread has not yet written this eye again. `[inferred-static]`
- When it picks the older one: the game thread has already run the stereo offset for tick F+2 (same eye) before
  the present of picture F. Then the count reads F+1, the difference is 2, `offset` is 1, and `[1]` is tick F's
  view: correct. Game thread 0 or 1 ticks ahead: `offset` 0, `[0]` is tick F's view: correct. `[inferred-static]`

## 2. Thread, and how often the hook runs

- The hook body is `FFSR:4780-5164`. It calls game-thread things directly (player controller, pawn
  `set_actor_location`, `GameThreadWorker::execute`: `FFSR:4871`, `5016-5053`), so it is written for the game
  thread. `[inferred-static]` That UE 5 calls it from the viewport draw on the game thread is `[hypothesis]`
  (engine knowledge, not in this source).
- In AFW `is_using_afr()` is true (`VR.hpp:506-511`), the view count hook returns 1 (`FFSR:5482-5493`; 2 only
  for a moment while the ghosting fix learns the second scene state), so: one call per tick, for the ONE eye
  being drawn. `[inferred-static]` for the count of views; that the engine makes exactly one call per view per
  tick is `[hypothesis]`. Calls with view index -1 are ignored (`FFSR:4801-4805`).
- `true_index = g_frame_count % 2` whatever index the engine passed (`FFSR:4828-4829`): it alternates every
  tick. 0 = left. `[inferred-static]`
- The plugins' post callback runs once in that call (`FFSR:5070-5073`), then the matrix is built and pushed
  (`FFSR:5118-5156`). `[inferred-static]`

## 3. The three counters

- `g_frame_count` (`FFSR:67`): game thread, set at the start of the viewport draw hook to the runtime's
  `internal_frame_count` (`FFSR:2473`). That value was written by the PREVIOUS tick's `BeginRenderViewFamily`
  (game thread, the view family's own frame number, `FFSR:3497-3502`), so in tick F it is F-1.
  (`update_hmd_state(..., internal_frame_count + 1)` on the next line bumps the runtime's number,
  `OpenXR.cpp:221`, but after `g_frame_count` was read.) `[inferred-static]`
- `internal_render_frame_count`: the render thread's view-extension hook reads the family's frame number
  (`FFSR:3713`), adds the "frame delay compensation" setting (`FFSR:3733`), hooks an RHI command
  (`FFSR:3912`); when that command runs on the RHI thread the number is stored (`FFSR:2858`,
  `OpenXR.cpp:725-727`). `[inferred-static]`
- `m_frame_count`: set in `on_present` from `internal_render_frame_count` (`VR.cpp:2579`), on the thread that
  presents. `m_render_frame_count`: set in `on_post_present` from `m_frame_count` (`VR.cpp:2790`), same thread.
  `[inferred-static]`
- The warp call itself: `D3D12Component::on_frame`, called from `on_present` (`VR.cpp:2734`); it crops the
  just-drawn back buffer into the eye's colour (`D3D12:525-528`) and passes `&vr->cameraData[nEye]`
  (`D3D12:545`, `552`). So colour = picture F, view = what section 1 chose. `[inferred-static]`

## 4. Conclusion for the plugin's record

Guaranteed? No, but right in normal running. The record found by position is always the record of the tick the
call's view came from. That view is tick F's (the picture's) when the game thread is 0, 1 or 2 ticks ahead of
the present. It is off by exactly +2 ticks (a LATER tick's hands, same eye) in these cases, never by 1, never
earlier:

1. The game thread is 4 or more ticks ahead of the present (this eye written twice since). The count difference
   is then 4, clamped to `offset` 1, which is tick F+2's view. `[inferred-static]` That UE's own frame sync
   keeps the game thread at most about 2 ahead is `[hypothesis]`.
2. A race: the game thread pushes the ring and bumps the count with no lock (`FFSR:5152-5156`; the reader only
   holds `eyes_mtx`, `VR.cpp:2481`). A present landing between the push and the count reads `[0]` = tick F+2's
   view, or a half-written matrix (which would simply not match any record). A window of nanoseconds once per
   tick. `[inferred-static]`
3. More than one stereo-offset call per tick (the brief two-view moment of the ghosting fix; any game code that
   asks for a stereo projection). Each call pushes the ring, so `[1]` stops meaning "two ticks ago". With the
   game thread 2 ahead the call then carries tick F+2's view. `[inferred-static]` for the mechanism,
   `[hypothesis]` that it happens in this game. The plugin's own log tells: "records behind" should sit
   between 0 and 2 and the "head dead still" count should stay near 0 with a moving head.
4. "Frame delay compensation" not 0 (`FFSR:3733`): the render side's numbers shift; an odd value swaps the eye,
   2 makes `offset` always 0. `[inferred-static]`
5. A frame number skipped or repeated (loading, a hitch): the eye parity breaks for a frame. `[hypothesis]`

Not determined: whether the hands in picture F are the hands of tick F. That depends on the arm solve running
earlier in the same tick than the stereo-offset call (the record reads `ik::drawn` at that moment), and on the
engine sending bone changes to the renderer after the view is computed. Not readable from UEVR's source.
`[hypothesis]` that both hold.

## 5. What can make the numbers differ from `(y, z, -x) / (world_to_meters * VR_WorldScale)`

- The formula matches: UE 5 path `cam_pos_d = view_d / double(get_world_to_meters())`, then
  `vec3(y, z, -x)` cast to float (`FFSR:5144-5146`); `get_world_to_meters()` is the float product
  `m_world_to_meters * m_world_scale` (`VR.hpp:551-553`), and `m_world_to_meters` is the same number the
  callback is given (`FFSR:4813`). `drawnpose.cpp:50-54` does the same arithmetic. `[inferred-static]`
- Camera offsets (`FFSR:4937-4954`), head offset and eye separation (`FFSR:4984-4996`), decoupled pitch
  (`FFSR:4932-4935`), room-scale movement (`FFSR:5015-5063`) and snap turn (`FFSR:5067`, `VR.cpp:3925-3956`)
  are all applied BEFORE the plugins' post callback. Room scale and snap turn change the pawn and the control
  rotation, not this call's location. Nothing in UEVR itself touches the location between the callback and the
  matrix (`FFSR:5075-5148`). `[inferred-static]`
- Anything that runs after our callback in the same loop can: UEVR's Lua loader comes after the plugin loader
  (`src/Mods.cpp:28-29`), so a Lua script with its own post-stereo-offset callback, or a native plugin that
  registered after ours, changes the location after we recorded it. `[inferred-static]` Whether any installed
  script does: not checked.
- The world scale: the plugin reads it as text. UEVR prints a float setting with `std::to_string`
  (`src/Mod.hpp:89`), six decimals. A scale that is not exact in six decimals (1.0 is; 1.2345678 is not) comes
  back as a slightly different float, up to about half a millionth off, against a match tolerance of one
  millionth: close to the edge for scales below 1. `[inferred-static]` It is also read once a second, so a
  change of the slider mismatches for up to a second.
- The turn: UEVR casts the rotator to float before the sines (`FFSR:5123-5126`); the plugin uses doubles.
  Inside the plugin's turn tolerance for ordinary angles. `[hypothesis]` (not computed).
- Scene-view compatibility mode calls the same function by hand with 100.0 as world-to-metres
  (`FFSR:3142`) and skips the engine's own call (`FFSR:4874`): the callback still gets the number UEVR uses.
  `[inferred-static]`

## Could not determine

- How many stereo-offset calls this game really makes per tick, and how far ahead its game thread runs.
- Whether the hands are a tick behind the view in the picture (section 4, last paragraph).
- Which field of the call's camera block sits at the offset the plugin reads (the frame warp library's header
  is not in the four files read).
