# /gr 2026-11-11: the quit abort at `openxr_simulator.dll+0x919bd` matches the simulator's own preview thread

Answers `2026-11-11-gd-sh2-eight-more-crash-dumps.md`, section 2: *"From 13:00 on, the quit crash is inside the
OpenXR Simulator ... `0xC0000409` with code `0x7` (`abort()`) at `openxr_simulator.dll+0x919bd`"*, and its proposed
row *"resolve openxr_simulator.dll+0x919bd against the simulator v1.7.1 release ... and say which call aborts at quit."*

Topic: [`external-research/topics/2026-11-11-the-quit-abort-is-the-simulators-preview-thread.md`](../../external-research/topics/2026-11-11-the-quit-abort-is-the-simulators-preview-thread.md)

**Finding.** Since v1.6.0 the simulator keeps its preview window on a thread held in a file-level static
`std::thread`, joined only inside `xrDestroyInstance`; the dll is pinned and has no process-detach handling, so a
process that exits without `xrDestroyInstance` destroys a joinable thread, which is `std::terminate` → `abort()` →
fast-fail 7; the C runtime is linked into the dll since v1.6.0, which is why the abort sits inside it
`[inferred-static 2026-11-11, source read in the GitHub viewer; offset not resolved against the binary]`. Not fixed on
the simulator's master (2026-11-08). UEVR's `OpenXR::destroy()` (which calls `xrDestroyInstance`) has no caller found
on the quit path `[inferred-static, partial]`.

**Suggested change.** Dossier §3b: one line, "the OpenXR Simulator v1.6.0 or newer aborts at every quit unless
`xrDestroyInstance` ran (its preview thread); not our fault; costs only the dump". Board: the `[PD]` row Gold Digger
proposed can close with that answer; the `[FLAT]` row (a quit with the simulator but no UEVR, then UEVR on the real
runtime) stays as the live proof, predicted: only the simulator run aborts. Options for the dumps: close VR
(`xrDestroyInstance`) before quitting, or use webhead2oo9's v1.5.1 (no preview thread) for test runs; reporting it
upstream is Tefa's call.
