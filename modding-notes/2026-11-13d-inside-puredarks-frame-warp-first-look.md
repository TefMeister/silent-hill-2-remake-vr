# Inside PureDark's frame warp (AFW): a first look, and a lead for the outline around the hands and the gun (2026-11-13 evening, PC1, Opus, no game launched)

**Permission.** PureDark, 2026-11-13 19:49, to Tefa (Discord, Tefa's screenshot): *"the shimmer around hands and weapons
isn't something easily tweaked. You can do a reverse engineering with claude and see if it can tweak it better just for
this game. But it won't really apply to other games. 3rd person and 1st person games have very different issues"*.
Tefa: for Silent Hill 2 only. So: this study is for this game's build only; nothing from inside the plugin is published or
reused elsewhere; the plugin is never redistributed by us (Tefa installs PureDark's own release). Kept in this repo's
notes as findings, never as extracted code. The extracted shaders and their disassembly stay on the Developer PC
(scratch), not in git.

Tefa's complaint (2026-11-13 morning, beta 6, `2026-11-13c-the-warp-works-on-the-real-headset.md` §2): hands and gun
*"have an outline around them that is noticable, that was not that noticable in beta 5"*.

## 1. What the closed part is

`PDAFWPlugin.dll` (beta 6, 565 KB, sha256 5f3cfc38903ac438...; beta 5 775d27515fd1597b...): built by PureDark with MSVC,
DirectX 12 only, three exports, `InitDevice`, `InitFrameWarp`, `EvaluateFrameWarp` (UEVR's open side calls them; its menu
and settings are in PureDark's public UEVR fork). It reads one key (`GetAsyncKeyState`: a debug hotkey). The work is
done by **54 compiled GPU programs (DXIL)** inside it, which keep their resource and setting names `[measured: dxc
-dumpbin]`:

| shaders | what they are |
| --- | --- |
| 0-23 | AMD FidelityFX variable-rate shading / foveation (`vrsAlgorithm`, `foveationRadiiSquared`, `varianceCutoff`): the "Framewarp Shading Rate" option |
| 24-28, 33-40 | blits, a blur (`blurRadius`), an edge fade (`fFadeLeft/Top`), gamma / upper limit, full-screen triangles |
| 29, 31 | motion vectors reprojected from the rendered eye to the other (`_SrcMotionVectorsTex`, `_SrcDepthTex` -> `_DestMotionVectorsTex`) |
| 30 | **object motion correction** with UE's own velocity buffer (`_PrevVelocityTex`, `_InOutVelocityTex`, `_CorrectMVType`, `_ObjectMotionScale`, `_FixUEObjMotionRange`, `_IgnoreMotionThreshold`): "Fix Object Motion Vector" / "Motion Range" |
| 41-42 | the rendered eye forward-projected into the other eye's view (colour, depth, motion vectors, an eye mask) |
| 43-44, 47-48 | hole / occlusion detection (`_OccludedMaskTex`), the UI quad kept out (`_UIQuadPosViewSpace`, `_UIColorAndAlphaTex`) |
| 45-46 | the reprojection proper (packed reprojected data, `_BackupColorTex`, the shading rate) |
| **49** | **filling the holes** (`_OccludedMaskTex` -> `_DestColorTex`) |
| 50-52 | **moving objects** (`_SrcVelocityTex` -> `_MovingObjEyeMaskTex`, with `_IgnoreMotionThreshold`) |

The shared settings block (`matrices`, 2440 bytes): 36 matrices (source / destination / UI eye / camera, current and
previous) then `_UIQuadPosViewSpace, _UIScale, _MotionScale, _RenderSizes, _FoveatedArea, _AspectRatio,
_IgnoreMotionThreshold, _EyeIndex, _SkipUIElements, _UseMotionVectors, _UseEyeMask, _IsPreviousFrame,
_MotionVectorsType, _Debug, _IsFoveated, _IsMoving, _IsRotating, _ShadingRate, _IsUsingUEVelocity`.

UEVR's side (open) has these options (strings of `UEVRBackend.dll`): `AFW_FramewarpMode` (AlternateEyeWarping /
PreviousFrameWarping / CombinedWarping), `GhostingFix`, `AFW_FixObjectMotionVector`, `AFW_FixObjectMotionRange`,
`AFW_IgnoreMotionThreshold`, `AFW_FixMovingObjectBrightnessFlickering`, `AFW_ClearBeforeFramewarp`, `AFW_FramewarpDebug`,
`AFW_UltraResponsive`, `AFW_ShadingRate`, `AFW_EnableSharpening` / `AFW_Sharpness`, `AFW_UseUINT64`. Tefa's config now:
mode 3 (combined), ghosting fix on, object motion fix on with range 3.0, brightness-flicker fix on, ultra responsive on,
`AFW_IgnoreMotionThreshold` not set (its default), debug view never used.

## 2. Beta 5 against beta 6: what changed

`[measured]` 41 of the 54 shaders are byte-identical; the 12 warp shaders 41-52 changed, and nothing else (the motion
vector correction 29-31 is the same). The changes in what they read and write:

- **49, the hole filler:** beta 5 read the motion vectors (`_SrcMotionVectorsTex`, `_DestMotionVectorsTex`); **beta 6 no
  longer does, and instead reads `_OtherEyeLookUp` and `_PreviousFrameLookUp`** and writes a new `_RealDestColorTex`.
  It is also the shader that grew most (10.1 KB -> 12.9 KB).
- **50, 51, 52 (moving objects):** beta 6 dropped the motion vectors from all three.
- 42 (the eye mask) reads fewer inputs.

## 3. The lead for the outline `[hypothesis, inferred-static]`

When one eye is made from the other, the ground just beside a near object's edge was hidden from the rendered eye: a
hole. Beta 5 filled such holes along the motion vectors; **beta 6 fills them by looking the spot up in the other eye's
picture and in the previous frame.** For far things the two are alike. For the hands and the gun, the nearest things in
view, each eye sees a very different background behind the edge (the parallax is largest up close), and the hand itself
moves with the controller between frames: the looked-up colour behind the edge is likely the wrong piece of background,
or a piece of the hand. A thin band of wrong colour that follows the outline is what Tefa describes, and it is new in
beta 6 exactly where this changed. Not proven: the shader's logic was not followed line by line yet, and our arms changed
since the beta 5 clip (jbusfield's Lua hands then, our C++ arms now).

## 4. What would prove or move it (cheapest first)

1. **Tefa's eyes, no build:** beta 5 and beta 6 back to back in the same spot, everything else the same (both are in
   `D:/claude video game stuff/tools/`). Outline only in beta 6 = the hole filler. Then `AFW_FramewarpMode` 1 / 2 / 3 on
   beta 6 (alternate eye only, previous frame only, both): which source draws the band.
2. **The debug view** (`AFW_FramewarpDebug` on) on the simulator: it shows the masks; a hand held in front of the face
   shows whether the band is the occluded mask (holes) or the moving-object mask.
3. `AFW_IgnoreMotionThreshold` and `AFW_FixObjectMotionRange` swept on beta 6 (Tefa's range is 3.0).
4. Only then the deep work: follow shader 49 line by line, and the settings block's values as the plugin fills them
   (`EvaluateFrameWarp`). A game-only change would be ours to make inside our own copy, for this game.

Our side may matter too: our arms are posed after the game's own update, so UE's velocity buffer for them may not match
where they are drawn. The object-motion fix (30) reads exactly that buffer. Worth one look with the debug view.

## 5. The outline reproduced on the simulator, and the three modes compared (same evening, Tefa's headset charging)

Beta 6, DLSS as Tefa has it, the hotel-room save, warp switched on after the start (`VR_RenderingMethod` 3 through the
connector), the simulator's right hand swung left-right in front of the face twice a second (scratch `sweep.py`), 16
full-size captures per mode (960 x 1050 per eye: the simulator window made 2560 x 1400 for the run, put back after),
cropped on the hands, both eyes side by side. Sheets kept in `D:/claude video game stuff/sh2-warp-study/2026-11-13/`.

`[verified-live 2026-11-13, simulator, n=16 captures per mode, one run]`:

| `VR_AFW_FramewarpMode` | what fills the warp's holes | what the moving sleeve and hand show |
| --- | --- | --- |
| 1 (alternate eye only) | the other eye | **worst**: the sleeve torn by a wedge or doubled in about half the frames; ghost fingers |
| 3 (combined: Tefa's setting) | the other eye and the previous frame | white streaks down the sleeve's edge, jagged edges, doubled fingers in about a quarter of the frames, in both eyes in turn |
| 2 (previous frame only) | the previous frame | **cleanest**: edges intact; now and then a soft lighter patch on the floor beside the sleeve |

This fits §3: **the fill from the other eye is what draws the outline**; the previous-frame fill does not. Not yet known:
how mode 2 FEELS in the headset (latency, smoothness, its frame cost: modes 1 and 3 cost about the same, mode 2 never
measured), and beta 5 on the same sweep (its hole filler used motion vectors).

Tools fixed on the way: `dev-archive/recon/2026-10-29-buttons/sh2_front.ps1` now picks the game's own window by its class
(`UnrealWindow`): with a large simulator window "the largest window" was the simulator's and the presses went there. The
connector's key presses (`uevr_input_key enter`) pass the main menu and the "press any button" after the load.
