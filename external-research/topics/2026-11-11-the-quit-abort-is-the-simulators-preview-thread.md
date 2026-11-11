# The quit abort inside the OpenXR Simulator is its own preview thread; the UEVR crash still has no upstream match

**Status:** 🆕 new · **Priority:** high — it answers Gold Digger's second crash question of 2026-11-11
(`engine-research/inbox/2026-11-11-gd-sh2-eight-more-crash-dumps.md`, section 2) and re-checks the first.

Read online on 2026-11-11 (GitHub viewer and API, two web searches). Nothing was downloaded; no code was copied.

## (b) `openxr_simulator.dll+0x919bd`, fast-fail code 7 (`abort()`), at quit

**What the simulator's source shows** (elliotttate/OpenXR-Simulator, `src/runtime.cpp` at the tag `v1.7.1`, read in
the GitHub viewer; the file is 530 KB and was read whole, since GitHub's code search does not index files that big):

1. Since v1.6.0 (2026-10-26; "a dedicated preview UI thread" in its release notes, commit 029aa46) the preview window
   runs on its own thread, held in a `std::thread` member of a file-level static object (`g_previewUi`)
   `[inferred-static 2026-11-11, source]`.
2. That thread is joined in exactly one place: `xrDestroyInstance`, which posts the thread a quit message and waits
   for it `[inferred-static 2026-11-11, source]`.
3. The runtime pins its own dll in memory at `xrNegotiateLoaderRuntimeInterface` (so it is never unloaded early), and
   there is no `DllMain` / process-detach handling in the file `[inferred-static 2026-11-11, source]`. So if the
   process ends without `xrDestroyInstance` having run, the static object's destructor runs during process exit with
   the thread still "joinable". In C++ that is a defined fatal case: destroying a joinable `std::thread` calls
   `std::terminate`, which calls `abort()`, which on Windows raises fast-fail code 7 `[inferred-static, the C++
   standard and the Windows CRT]`.
4. v1.6.0 also linked the C runtime statically (`/MT`, commit 8d7782e, "to avoid MSVCP140 version clash with host
   process"), so `abort()` lives inside `openxr_simulator.dll` itself; that is why the dumps place the abort at
   `openxr_simulator.dll+0x919bd` rather than in `ucrtbase.dll` `[inferred-static]`.

That matches every fact in the 2026-11-11 Gold Digger note: code 7, inside the simulator's own dll, after Steam's
libraries unloaded (process exit), on every quit with the simulator `[inferred-static: a mechanism match, the
offset itself not resolved against the binary]`.

**Does UEVR call `xrDestroyInstance` at quit?** UEVR's `OpenXR::destroy()` (praydog/UEVR, `src/mods/vr/runtimes/
OpenXR.cpp`) ends the session and destroys the instance, but only when a frame was ever submitted
(`ever_submitted`). I did not find a caller of it on the quit path: `src/mods/VR.cpp` (read whole, 120 KB) never calls
it, and UEVR's `src/Main.cpp` handles only process attach `[inferred-static 2026-11-11, partial: code search plus two
whole-file reads; a caller elsewhere is not ruled out]`. If UEVR never destroys the instance when the game quits,
every quit on simulator v1.6.0 or newer would abort, which is what the dumps show `[hypothesis]`.

**Not fixed upstream yet.** The simulator's default branch (`master`, last change 2026-11-08) has the same structure:
the thread is still joined only in `xrDestroyInstance` `[measured 2026-11-11, source]`. The 2026-11-08 commit
4b2a1ce ("Release every D3D11 device child at xrDestroySession") fixes a different exit crash (a D3D11 device freed
from the dll's static object at process exit, an access violation in the NVIDIA driver) and is D3D11-only; this game is
D3D12 `[measured 2026-11-11, commit message]`. No issue on the repo mentions an exit abort (8 issues in all, newest
2026-10-21) `[measured]`.

**The fork:** webhead2oo9/OpenXR-Simulator's newest release, v1.5.1 of 2026-08-25, predates the preview thread: its
`runtime.cpp` (486 KB, read whole; `xrDestroyInstance_runtime` found in it as the positive control) has no
`std::thread` at all `[inferred-static 2026-11-11]`. So the fork should not abort at quit this way `[hypothesis,
untested]`, at the cost of the newer features (frame timing, 90/120 Hz setting) that v1.6.0 to v1.7.1 added.

The same source makes the earlier quit crash (2026-11-06/07, code `0xa`, an address in no loaded library) no
clearer; this note does not explain it.

## Also on the simulator's `master`, not in any release yet

- **The 4096 picture cap is lifted for Direct3D** (commit 692a66d, 2026-10-29): D3D11 and D3D12 sessions now get the
  hardware limit of 16384 in the system properties, the view configuration's largest size and the swapchain check;
  OpenGL and Vulkan keep 4096 `[measured 2026-11-11, commit message]`. That is the board's `[PD]` row "the OpenXR
  Simulator cannot make a picture wider than 4096": a build from `master` should let UEVR start VR at the headset's
  own 5376-wide size `[hypothesis, untested]`. v1.7.1 does not have it.
- OpenXR 1.1 instances and the version string (db35307, 2026-10-29), grip poses at the vendor offset (48982e4).

## (a) `UEVRBackend.dll+0x1503df` (null + 0x58), still seen 2026-11-10

Nothing upstream moved since the 2026-11-07 check, so there is still no newer build to try and no public fix:

- praydog's UEVR: no commit after 2026-08-30; UEVR-nightly still 01143 (2026-08-30) `[measured 2026-11-11, API]`.
- PureDark's AFW fork: newest release still v1.0-beta.6 (2026-08-13); branches `AFW` and `master` have no commit
  since 2026-10-25; `Joey-Merged` last changed 2026-10-30 (Phantom Blade Zero fixes, CVar slider retry); the fork has
  no issues filed since 2026-10-15 `[measured 2026-11-11, API]`.
- praydog/UEVR issues touched since 2026-11-05: only #429 (double-precision view math detection, opened 2026-08-17)
  `[measured]`. One web search for the dll name with this game found nothing `[reported, negative from n=1]`.
- joeyhodge's fork kept releasing (2026-11-08 "The Deep End of Native OpenXR": experimental texture-array submit and
  an asynchronous xrWaitFrame option, D3D12 only; 2026-11-11 UE 5.9 work); nothing for this game, exit or a null read
  at 0x58 `[measured 2026-11-11, release notes and commit subjects]`.
- jbusfield/SH2_UEVR (last push 2026-10-19) and praydog/SH2R-UEVR (2024-11-30) unchanged, no new issues.

So the 11-07 advice stands: name which beta.6 flavour is installed, resolve the offset against that file, and
bisect on our side.

## DLSS resets and UE5 VR performance

One search for this game rewriting its own upscaler setting found no public report `[reported, negative from n=1]`.
Nothing new on UE5 VR performance for this game.

## Suggestions for the modding lane

1. Treat the quit abort as the simulator's, not ours: it costs nothing in play, only the dump. Three ways out, cheapest
   first: (i) make sure `xrDestroyInstance` runs before the game exits (the quit path through UEVR, or our plugin
   closing VR on quit) and see whether the dump stops; (ii) for test runs only, the fork's v1.5.1 (no preview thread);
   (iii) report it on elliotttate/OpenXR-Simulator with the dump's facts (Tefa's call; the fix shape is the one the
   simulator already uses for its flicker detector's worker, a thread that is let go rather than joined at exit).
2. The FLAT row "one quit with the simulator but without UEVR, one with UEVR but without the simulator" still separates
   the two: the prediction from this note is that only the simulator run aborts.

## Sources

- https://github.com/elliotttate/OpenXR-Simulator (tag v1.7.1 `src/runtime.cpp`; `master`; commits 029aa46, 8d7782e,
  4b2a1ce, 692a66d, db35307; releases v1.6.0, v1.7.1; issues)
- https://github.com/webhead2oo9/OpenXR-Simulator (release v1.5.1, `src/runtime.cpp`)
- https://github.com/praydog/UEVR (`src/mods/vr/runtimes/OpenXR.cpp`, `src/mods/VR.cpp`, `src/Main.cpp`; issues) ·
  https://github.com/praydog/UEVR-nightly/releases
- https://github.com/PureDark/UEVR · https://github.com/joeyhodge/UEVR/releases
- https://github.com/jbusfield/SH2_UEVR · https://github.com/praydog/SH2R-UEVR
