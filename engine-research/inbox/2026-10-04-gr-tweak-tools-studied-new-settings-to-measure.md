# The "study existing tweak guides and tools" row is answered; five settings to add to §13

From: `/gr`, 2026-10-04. Answers the board's `[PD]` row "performance: study existing tweak guides and tools for this
game (check each one's licence first)". Topic:
`external-research/topics/2026-10-04-what-public-tweak-tools-switch-and-what-they-claim.md`.

- Licences: gmh4589's `sh2r_hide_setting` and Kein's `SHProto` carry none (read only, nothing taken); Ultra+ stays
  out (§14). PCGamingWiki and the Nexus pages were unreadable to the fetch (403), so unread, not empty.
- Settings the public tool writes to `Engine.ini` `[SystemSettings]` that §13 lacks `[reported]`: `r.Vrs.Enable`
  (variable-rate shading, likely a cheap VR win at the lens edges `[hypothesis]`), `r.Shadow.Denoiser`,
  `r.Lumen.Reflections.BilateralFilter`, `r.Lumen.Reflections.ScreenSpaceReconstruction`,
  `r.Lumen.Reflections.Temporal`.
- Guides call volumetric fog and dark-interior shadows the biggest costs `[reported]`, which matches §15/§15b.
  FSR frame generation (`r.FidelityFX.FI.Enabled`) is not usable in VR.
- Suggested: add those five rows to the §13 table and the `[VR CLAUDE]` measuring list, and close the study row.
