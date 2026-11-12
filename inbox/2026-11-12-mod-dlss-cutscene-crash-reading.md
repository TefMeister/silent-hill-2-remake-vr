# The DLSS-on crash at the cutscene's first frame: what the static reading got to (2026-11-12, the /lm reader)

Stopped early on the coordinator's word: the crash was solved live (our own plugin's head-material put-back, a kept
material pointer; not UEVR's DLSS hooks). This is only what was established before the stop. Nothing was run.

Tags: **[read]** = read in a file at hand. **[inferred]** = my conclusion from those, not checked further.

- **The UEVR frame in the stack is only its engine-tick wrapper, not a DLSS or view hook.** The UEVR log of run vr2
  prints `UGameEngine::Tick: 7ff64a547540` with the game loaded at `7ff645a70000`, i.e. game offset `+4ad7540`
  (log lines 9 and 761). [read] The stack frame `+4ad784a` is 0x30a bytes after that, and the frames below the UEVR
  one (`+b61124` and down to `+5cbc362`, next to the program's entry point `+5cbc3d0`) are the game's main loop.
  So `UEVRBackend+0x1685bf` is `engine_tick_hook` (`FFakeStereoRenderingHook.cpp:374` in the AFW source) calling
  the game's own tick. [inferred from those addresses]
- **So the fault was inside the game's own tick on the game thread, with UEVR merely on the way in**; that fits the
  live finding (something our plugin left for the tick to trip over) and does not point at the render thread, DLSS
  or the mirror's capture. [inferred]
- **The three innermost game functions were located but not identified.** From the exe's function table:
  `+4ad784a` lies in a piece starting `+4ad77a2` (ends `+4ad79f8`; the UEVR log, line 702, names the same
  `...77a2` as a reference point, so it is a piece of the tick itself); `+49dabdd` is 0x28 bytes into a function
  `+49dabb5` to `+49db183`; the crash address `+4c60c37` is in a small function `+4c60a6f` to `+4c60c7c`. [read]
  No disassembly or strings were read, so what they are is not known.
- **The installed UEVR is commit `c40eaab` (tag `UEVR_AFW_v1.0-beta.5` + 7), one commit behind the clone used for
  the 2D-mode reading (`fc8de10`).** [read, log lines 2-4 and GitHub's compare] The one commit in between touches
  `VR.hpp` and `FFakeStereoRenderingHook.cpp` ("Removed origin offset"), so line numbers in the earlier inbox file
  may be off by a few lines against the installed build; the `:2926` warning line matched.
- Not done: questions 3 and 4 (UEVR's `hk_NVSDK_NGX_*` hooks, a cvar to keep DLSS off the capture). Not needed now.
