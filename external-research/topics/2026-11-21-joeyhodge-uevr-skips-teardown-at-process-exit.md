# joeyhodge's UEVR fork stops tearing UEVR down when the process exits (2026-11-19, not in a release yet); nothing else upstream changed

**Status:** 🆕 new · **Priority:** medium. Bears on the dossier's "Crashes at quit" entry (§3b), kind (3): every quit
since 2026-11-06 ends in a crash, first `0xC0000409` / 0xa at an address in no library after Steam unloaded. The
`/gr` pass of 2026-11-07 found no upstream change and no public report that matched.

## What is public

Read online on 2026-11-21 from the repository viewer and API. Nothing was downloaded, nothing copied.
All `[reported 2026-11-21]` unless tagged otherwise.

- **joeyhodge/UEVR, default branch `ue57performance`, commit 51ef1b0 (2026-11-19, "Don't tear down the framework while
  the process is exiting"; merged as 1f227d2 the same day):** when the UEVR library is told the process is ending
  (the detach notice that comes from `ExitProcess`, as opposed to an explicit unload), it now lets go of its one global
  framework object without destroying it. Before, the C runtime's static destructors destroyed it at that moment,
  which ran down the VR component and the Direct3D 12 component and released graphics resources, after the game's
  other threads had already been killed by the exit. In the case they name (TEKKEN 8 Demo on an Intel graphics driver)
  the release went into the driver, which waited on a dead worker thread forever: the window was gone but the process
  stayed alive about 15 minutes. An explicit unload still does the full clean-up. One file, eight lines.
- It is **not in a release yet**: the fork's newest release is `111726DecadeofUnreal` (2026-11-16). The 13 Nov
  release added opt-in OpenXR wait-only pacing (noted on 11-14).
- **Unchanged since the pass of 2026-11-14** `[measured 2026-11-21]`: PureDark's AFW is still v1.0-beta.6
  (2026-08-13); praydog's UEVR-nightly is still 01143 (2026-08-30); praydog/SH2R-UEVR's last push is 2024-11-30;
  jbusfield/SH2_UEVR's last push is 2026-10-19 (the licence). Neither SH2 pack is archived.
- One web search on the DLSS-change and device-hung crashes (`[ID SH2-019]`) found nothing new for this game: the
  close matches are older Steam threads and other games' frame-generation (`sl.dlssg`) device-hung reports.

## What it means for us

- Our build runs UEVR as an injected library in a game that ends through `ExitProcess`, so the same destructor chain
  runs at our quit `[inferred-static: the mechanism is generic to the framework's global object, not to one game]`.
  On a different driver (ours is NVIDIA, with DLSS and the AFW plugin's resources alive) a release against killed
  threads could show up as a fault rather than a hang: a plausible source for the "address in no library after Steam
  unloaded" quit crashes `[hypothesis]`. The later `abort()` kind is the OpenXR Simulator's own thread (topic of
  2026-11-11) and stays separate.
- Cheap test, with no change to anyone's files: read one of the `0xC0000409` / 0xa dumps' stacks for the UEVR
  library's process-detach handler and the framework's destructors. If they are there, the cure is the same shape in
  our own code path (our launcher or plugin can stop the teardown, or ask UEVR to shut down before the game exits),
  or waiting for the fork's next release `[hypothesis]`.
- AFW beta.6 has a joeyhodge-based flavour, but it was built in August, long before this change `[reported, release
  notes of beta.6]`; it does not carry it.

## Sources

- joeyhodge, UEVR fork, commit 51ef1b0 (by arhan-mopic): https://github.com/joeyhodge/UEVR/commit/51ef1b0
- joeyhodge, UEVR fork, merge 1f227d2 and releases: https://github.com/joeyhodge/UEVR/commit/1f227d2 and https://github.com/joeyhodge/UEVR/releases
- PureDark, UEVR AFW releases (checked, no change): https://github.com/PureDark/UEVR/releases
- praydog, UEVR-nightly releases (checked, no change): https://github.com/praydog/UEVR-nightly/releases
- praydog, SH2R-UEVR (checked, no change): https://github.com/praydog/SH2R-UEVR
- jbusfield, SH2_UEVR (checked, no change): https://github.com/jbusfield/SH2_UEVR
