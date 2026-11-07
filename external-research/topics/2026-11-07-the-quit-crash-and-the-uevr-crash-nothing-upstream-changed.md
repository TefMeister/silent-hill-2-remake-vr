# The quit crash and the UEVR crash of 2026-11-05/06: nothing upstream changed, and no public report matches

**Status:** 🆕 new · **Priority:** high — it narrows Gold Digger's two crash questions of 2026-11-07
(`engine-research/inbox/2026-11-07-gd-sh2-eleven-crash-dumps.md`) to our own side.

Read online on 2026-11-07 (GitHub viewer and API, two web searches). Nothing was downloaded.

## What the upstream projects did since the crashes began

- **praydog's UEVR nightly:** the newest build is still **01143 of 2026-08-30**; praydog/UEVR has no commits since
  then `[measured 2026-11-07, GitHub API]`.
- **PureDark's AFW fork:** the newest release is still **UEVR AFW v1.0-beta.6 of 2026-08-13** `[measured]`. Its `AFW`
  branch has one later commit (2026-08-20, "removed origin offset, it is handled in the AFW plugin internally"), never
  released. A new branch `Joey-Merged` (2026-10-29/30) merges joeyhodge's Phantom Blade Zero fixes and a CVar
  slider retry; no release, nothing that names Silent Hill 2 or shutdown `[measured]`.
- **beta.6 ships in two flavours:** one built on UEVR nightly, one on **joeyhodge's fork** (for UE 5.5 to 5.8 games;
  the release notes say to tick both *Ghosting Fix* and *Bootstrap* with that one) `[reported 2026-11-07, release
  notes]`. The two flavours are different `UEVRBackend.dll` files, so `+0x1503df` means a different place in each
  `[inferred-static]`.
- **joeyhodge/UEVR** (default branch `ue57performance`) is very active (near-daily releases through 2026-11-05), all
  per-game fixes for other titles (Avowed, Stellar Blade, Phantom Blade Zero, UE 5.8); nothing for SH2 or exit
  `[measured]`.
- **jbusfield/SH2_UEVR** last changed 2026-10-19 (licence only), **praydog/SH2R-UEVR** 2024-11-30; neither has a
  crash-on-exit issue `[measured]`.

So UEVR, the AFW build and both SH2 script packs were the same files on 2026-11-04 as on 2026-11-07 `[inferred-static]`:
whatever made every quit crash from 2026-11-06 on came from a change on our side (a plugin build, the launcher, the
parked connector, the stutter patch, a config) or from Windows or driver updates, not from a UEVR update `[hypothesis]`.

## Public reports

- No public report found of SH2 Remake + UEVR crashing at quit with `0xC0000409`, or of `UEVRBackend.dll` crashing
  at a fixed offset reading `0x58` (one web search each; UEVR issue search for "exit" and "0xC0000409")
  `[reported 2026-11-07, negative from n=2 searches]`.
- Old, unrelated: UEVR #16 (2024, Train Sim World: crash on the resolution slider "or on exit"), SH2R-UEVR #5
  (2024, crashes, fixed in praydog's 28c1ec2). Neither matches.
- Not ours but worth knowing: UEVR #452 (2026-11-04) reports SteamVR beta 2.19.1 breaking UEVR's menus and GUI in VR
  on both OpenXR and OpenVR; going back to SteamVR stable cured it `[reported]`. Our wear runs Virtual Desktop's
  OpenXR, so it should only matter if the runtime is ever switched to SteamVR `[inferred-static]`.

## Suggestions for the modding lane

1. Name which beta.6 flavour (`nightly` or `joeyhodge`) is in `UEVR AFW b6\`: the offset only means something
   against that exact dll.
2. The quit crash: a code `0xa` fail-fast at an address in no loaded module, after Steam's libraries unloaded, is
   the usual look of a callback or detour still installed when its module has gone `[hypothesis]`. Since upstream
   did not move, bisect our own 2026-11-05/06 changes (the plugin build of 11-06, the launcher, the 21 Sacraments
   loader) with Gold Digger's proposed [FLAT] run: quit with only our proxy, then with UEVR, then with each plugin.

## Sources

- https://github.com/praydog/UEVR-nightly/releases · https://github.com/praydog/UEVR
- https://github.com/PureDark/UEVR (releases, branches `AFW` and `Joey-Merged`)
- https://github.com/joeyhodge/UEVR
- https://github.com/jbusfield/SH2_UEVR · https://github.com/praydog/SH2R-UEVR (issues)
- https://github.com/praydog/UEVR/issues/452 · https://github.com/praydog/UEVR/issues/16
