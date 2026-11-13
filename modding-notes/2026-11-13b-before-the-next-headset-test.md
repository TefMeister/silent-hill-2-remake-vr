# Before the next headset test: the warp build is beta 6 after all, Virtual Desktop keeps its own log, the live switch resets the renderer (2026-11-13 morning, Fable, no game running)

Continues `2026-11-13-the-dlss-ghost-deep-dive.md` and `2026-11-12-the-warp-crash-and-the-dlss-ghost-on-the-real-headset.md`.
Tefa's two jobs, restated this morning: the DLSS picture over exactly half of the right eye, and the frame warp (AFW)
working with DLSS on and ray tracing on. Nothing was launched for this note.

## 1. The installed warp build IS beta 6 `[measured 2026-11-13, sha256 of all six DLLs]`

The earlier line "our packages carry beta 5" came from the tag string inside `UEVRBackend.dll`
(`UEVR_AFW_v1.0-beta.5`). Downloaded both public zips (PureDark/UEVR releases, `UEVR-nightly_AFW_v1.0-beta.5.zip`
and `...beta.6.zip`) and hashed them against the game folder's `UEVR AFW b6\`:

| file | game folder | beta 6 zip | beta 5 zip |
| --- | --- | --- | --- |
| UEVRBackend.dll | d3ff93e7f327016d | d3ff93e7f327016d | f67502f80a6ce62c |
| PDAFWPlugin.dll | 5f3cfc38903ac438 | 5f3cfc38903ac438 | 775d27515fd1597b |

The other four DLLs are the same in both zips. Beta 6's DLL simply still carries the beta 5 tag string (the beta 5
zip's DLL says beta 4). It also holds `AFW_ShadingRate`, the option beta 6 added. So: **the claim "beta 5 is
installed" is `[disproved]`; there is no newer warp build to move to** (beta 6, 2026-08-13, is the newest release).
Not checked: the copy inside the 0.1.2 package (`uevr/`); hash it before the next package.

## 2. Virtual Desktop writes its own OpenXR log, and it names the error code `[measured]`

`%PROGRAMDATA%\Virtual Desktop\OpenXR.log` (VirtualDesktopOpenXR v1.1.0, Streamer 1.34.25, OVR 1.64.0, Quest 3,
recommended 2688x2880). Every refused call is written as `xr<Call> failed with XR_ERROR_...` (seen:
`xrCreateInstance failed with XR_ERROR_EXTENSION_NOT_PRESENT`, UEVR's harmless first try with the Meta metrics
extension). **The file is rewritten at every start**, so the one from the crash of 2026-11-12 00:34 is gone. From now
on it is copied to the logs folder after every headset run. This replaces test E (the wpr trace) as the first way to
get the reason Virtual Desktop refuses the per-eye pictures.

One more reading of the crash log: the first "AFR left eye" request took 75 ms and worked; the refused one came back
within the same millisecond as the last destroy (00:34:22.450). An instant refusal is a rule being applied, not an
allocation failing `[inferred-static]`.

## 3. Switching the warp on after the start does reset the renderer, whoever flips the setting `[read in source, fc8de10]`

`D3D12Component::on_frame` (line 28): `if (m_force_reset || m_last_afr_state != vr->is_using_afr()) setup()`, and
`setup()` -> `on_reset` -> `create_swapchains()`. `is_using_afr()` reads the setting's value directly, so our
`uevr VR_RenderingMethod 3` line in `sh2vr_devcvars.txt` (through `set_mod_value`) takes the same path as the menu's
combo box. The installed `sh2vr.dll` has that switch (`devcvars: uevr` string present). The warp proper only starts 90
frames after the inject and 90 frames after a switch (`afw_since_inject_frame_count`, `afw_switching_skip_frames`);
until then it runs as plain synchronized alternate-eye.

Release-note rules that bind the test: DLSS on in the game first; Ghosting Fix ticked (it is); a DLSS quality or
resolution change with the warp on needs a game restart; the warp check is `Enable Debug Framewarp` (Numpad 6): only
moving things tinted green = working, the whole picture green = not.

## 4. The order for the headset (Tefa in the Quest 3, the session drives; about ten minutes)

1. Start as now (both eyes, DLSS Quality, ray tracing on). Screen-region recording on. Ghost there? (baseline)
2. The one line: warp on, live. Comes up = is the ghost still there in this mode (each eye is drawn on its own
   2688-wide picture, so the two-eyes-in-one-picture fault may simply not exist here), frame rate, hands and gun in
   motion. Refused or crash = Virtual Desktop's log now names the code; copy it at once.
3. Only if the ghost is still wanted in mode 0, or the warp fails: the six switches of the deep-dive note, plus one
   new one: Epic's own upscaler (TSR) at 67 % in place of DLSS. Ghost with TSR too = any upscaling under UEVR's
   two-eye picture; DLSS only = the DLSS plugin's own region.
