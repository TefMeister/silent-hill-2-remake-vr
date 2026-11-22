# UEVR's Ghosting Fix hands the second eye a dead scene state (2026-11-22, /lm reader)

From: the background reader. Read only; nothing built, fetched or deployed. Follows
`2026-11-22-mod-start-crash-without-window-view.md`.

## Source

- **No UEVR source on disk**: no `FFakeStereoRenderingHook.cpp` under `~/github-backups` or `D:\`. The 11-12 reading used
  a scratch clone of PureDark/UEVR `AFW` `fc8de10` that is gone.
- So I read the binary in use instead. That is `UEVRBackend.dll` from AFW beta.6 (the dossier records the launcher's
  copy as byte-identical), in `D:\claude video game stuff\tools\UEVR-nightly-AFW-beta.6\`, disassembled with cdb
  (FSceneView constructor hook, `+0x175180-0x175640`).
- The log lines match by the line numbers baked into the code: 3192 Inserting, 3196 null, 3219 Setting. They sit in the
  block the 11-12 note gives for `VR_GhostingFix` (`FFSR:3203-3224`).

## What the code does `[inferred-static, from disassembly]`

- **Allocation: none by UEVR.** "Inserting new scene state" only records the pointer the game passed (the game's own view
  state) in UEVR's hash set. The game allocates it and the game frees it. UEVR takes no reference.
- **No removal from the set** shows anywhere in this function. A freed state stays listed (not checked elsewhere).
- **Null state:** the null line is logged once. A second one-time line may follow (line 3199). Then the swap is skipped
  and the view is left as it is. Nothing is repaired.
- **The swap ("Setting scene state to"):** when the Ghosting Fix is on, the mode is alternating eyes (our
  `VR_RenderingMethod=3` is), and the view is eye 1:
  - UEVR takes the first state in its set that is not eye 0's.
  - It caches that pointer and writes it into the view's init options every frame after.
  - It never checks that the state is still alive. It re-picks only when eye 0's state changes, and only after more than
    90 frames.
- **This fits the dumps:** the object called through was the "Setting" pointer in 5 of 5. The game freed that state
  (around the ~400 ms stall and the null line), and UEVR kept handing it to the render thread.

## Smallest changes on our side, ranked

1. **`VR_GhostingFix=false`** in the profile's `config.txt` (it is `true`, line 80). This removes the swap entirely, so it
   is the cleanest crash test `[inferred-static, strong]`. Cost: the AFW release notes say "always tick the Ghosting Fix"
   `[reported]`. Expect ghosting or warp artefacts in the second eye; that needs a headset check.
2. **Inject later** (launcher `--delay`, past the start's stall and the null line, e.g. at the main menu). UEVR would
   then never record the start-up state that dies `[hypothesis]`. It is free to try.
3. **Ghosting Fix off at start, switched on from our plugin once the menu is up** (a live config change). The cache is
   empty then, but the set may still hold the dead pointer if UEVR saw it `[hypothesis, depends on 2]`.
4. **A plugin guard:** before eye 1 renders, check that the view's state pointer still points at a vtable inside the
   game exe, and null it otherwise. This is intrusive and needs the state offset `[hypothesis]`.

## Upstream

`[reported, external-research/INDEX.md 2026-11-21]` AFW is still beta.6 and UEVR-nightly is still 01143. joeyhodge's
fork has only its exit-teardown change. No note mentions a fix to the scene-state swap.
