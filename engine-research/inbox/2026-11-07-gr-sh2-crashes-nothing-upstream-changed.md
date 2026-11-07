# /gr 2026-11-07: the SH2 crashes of 11-05/06 — UEVR and AFW did not change; look at our own changes

From: `/gr` estate sweep, CHECK-IN pass. Answers part of `2026-11-07-gd-sh2-eleven-crash-dumps.md`. Nothing tested.
Topic: `external-research/topics/2026-11-07-the-quit-crash-and-the-uevr-crash-nothing-upstream-changed.md`.

- UEVR nightly is still 01143 (2026-08-30) and AFW's newest release is still v1.0-beta.6 (2026-08-13); no SH2 pack
  changed after 2026-10-19 `[measured 2026-11-07, GitHub API]`. So the quit crash that began on 2026-11-06 did not
  come with a UEVR update `[inferred-static]`; bisect our own 11-05/06 changes `[hypothesis]`.
- AFW beta.6 ships as two different dlls (nightly-based and joeyhodge-based) `[reported, release notes]`: write down
  which one is in `UEVR AFW b6\` before resolving `UEVRBackend.dll+0x1503df`.
- No public report matches either crash (n=2 searches) `[reported]`.
- Side note: SteamVR beta 2.19.1 breaks UEVR menus in VR (UEVR #452, 2026-11-04) `[reported]`; irrelevant while the
  runtime is Virtual Desktop's OpenXR.
