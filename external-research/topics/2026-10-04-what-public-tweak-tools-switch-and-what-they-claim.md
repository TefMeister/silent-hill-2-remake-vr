# What the public Silent Hill 2 tweak tools switch, and what they claim

**Found 2026-10-04 (`/gr`, estate sweep).** Answers the board's `[PD]` row *"performance: study existing tweak
guides and tools for this game (check each one's licence first) … Learn from them; copy nothing."* Nothing here was
run; no code or values were copied. Two big sources were unreadable to the automated fetch (PCGamingWiki and the
Nexus mod pages both returned 403), so they are **unread, not empty**.

## Licences first

- **gmh4589's `sh2r_hide_setting`** ("SH2R ANTIFOG", a C# settings editor, updated 2026-05): **no licence file**.
  Read only, for which Unreal setting names it exposes; nothing taken. Its aim is the opposite of ours: it can
  switch the fog off.
- **Kein's `SHProto`** ("SH2 remake SDK/API", a UE project recreating the game's classes, last pushed 2024-10):
  **no licence file**. Not a tweak tool; noted below as a lead for the native plugin.
- **Ultra+**: already ruled out by its licence (dossier §14). Not revisited.
- Guide sites (DSOGaming, Deltia's Gaming, The Backdash): ordinary articles; facts only.

## What they switch that our §13 list does not have

The ANTIFOG tool writes these into the game's `Engine.ini` `[SystemSettings]` `[reported 2026-10-04]`, which also
says the game honours that section for at least some of them (it is the tool's whole mechanism) `[reported]`:

| Setting | Why it is interesting for VR | Our verdict to test |
| --- | --- | --- |
| `r.Vrs.Enable` | Variable-rate shading: shades parts of the picture at lower detail. In a headset the edges of each eye are blurred by the lens anyway, so this is close to a free win if the game and GPU support it `[hypothesis]` | measure in the headset; look at the edges |
| `r.Shadow.Denoiser` | The shadow denoiser's cost; shadows were our biggest single win (§15) | try lower |
| `r.Lumen.Reflections.BilateralFilter`, `r.Lumen.Reflections.ScreenSpaceReconstruction`, `r.Lumen.Reflections.Temporal` | Lumen reflection clean-up passes; we turned Lumen reflections off entirely in s5, which may look worse than trimming these | try these instead of "off" if Tefa misses the reflections |
| `r.NGX.DLSS.denoisermode` | DLSS's own denoiser mode (ray reconstruction) | only matters with ray tracing on; low priority |
| `r.Fog`, `r.DisableDistortion` / `r.Distortion` | Height fog on/off; heat-haze distortion | keep the fog; distortion off is probably free comfort |

Also seen in guides: `r.FidelityFX.FI.Enabled=1` turns on FSR frame generation via `Engine.ini` `[reported]`.
**Not for VR**: generated frames are not head-tracked.

## What the guides claim, for the indoor problem

- **Volumetric fog is called the biggest single cost**; "Medium" said to keep the look at a much lower price
  `[reported]`. Fits our s3 (+3.4 fps) and Tefa's verdict that our coarser grid made the fog look flat outside: the
  middle setting, not the coarsest grid, is worth trying.
- **Shadows Medium ≈ +10%**, and dynamic shadows in **dark interiors** are singled out as expensive
  `[reported]`. Matches our dark-room problem (§15b): the room's own lights and their shadows.
- **The game uses hardware Lumen on PC** `[reported]`. Our runs had ray tracing off; worth checking that Lumen is
  then on its software path, since the two cost differently.
- **Patch 1.04 added an option to toggle HZB occlusion culling** (stutter on some AMD/Intel GPUs) and fixed sky-map
  stutter `[reported]`. Useful if the headset shows hitches rather than low averages.
- Traversal and shader-compile hitches at doors and narrow halls are a known UE5 trait here `[reported]`: they
  will show in the "slowest 1%" figures walking indoors (51–64 fps, slowest 1% 38–50) regardless of settings.

## Lead for the native plugin

`SHProto` reconstructs the game's classes as a UE project. With no licence it can only be read, but it may name
classes and members our C++ plugin needs (the dossier's §8e object graph came from live reflection instead). Check
it against §8e before the plugin design session.

## Next step

Add `r.Vrs.Enable`, `r.Shadow.Denoiser` and the three Lumen-reflection filters to the §13 table and to the
`[VR CLAUDE]` measuring list; in the dark room, try shadow denoiser and the room-light shadow settings before
lowering resolution again.

## Credits

gmh4589 (sh2r_hide_setting), Kein (SHProto), DSOGaming, Deltia's Gaming, The Backdash.
