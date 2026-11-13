# The DLSS ghost in the right eye: deep dive (2026-11-13 01:50-02:15, Fable, session alone)

Continues `2026-11-12c-the-real-cutscene-camera-on-the-flat-screen.md`. Everything here ran with DLSS on and ray
tracing on unless a line says otherwise.

Tefa after wearing b013: *"frame rate was solid, not 72 but whatever you have done, it clearly works soooo much
better"*, the eyes' shadows did not differ, *"apart from that flickering it look great"*; the cutscene screen one more
metre back (5.5 m: b014, installed, unworn). The frame warp (AFW) was NOT on in that wear (rendering method 0).

## There are TWO different things, not one

1. **The real headset's ghost** (Quest 3 through Virtual Desktop, one 5376 x 2880 picture, DLSS Quality, rendering
   method 0; no cutscene in that run): a faint second view over the RIGHT HALF of the right eye, most frames. The
   straight edge measured at x = 954 of 1920 in a screen recording of the game window: 49.7 % of the eye's width, the
   eye's exact middle (a 4096-pixel limit would sit at 52.4 %) `[measured 2026-11-13, n=1 run]`. Gone with DLSS off
   `[reported 2026-11-12, n=1]`. Not reproduced on the simulator at 1280 x 1400 per eye at the armory: 0 wrong frames
   in 6.5 min, ray tracing on or off `[measured]`.
2. **The simulator's one-frame wrong pictures after a cutscene**: for one frame the right eye (and the game window)
   shows the whole two-eye picture squeezed. It depends on the RUN, not on the settings: 416 in 4 min in one run after
   the opening cutscene, 10 in another, 1 in a third, same build and settings; in the third, switching DLSS, the
   ray-traced reflections and `VR_GhostingFix` live changed nothing because the baseline itself was clean
   `[measured, n=3 runs]`. So it is a state the run falls into when our flat screen switches back to VR (the switch is
   written from our own thread), the same family as "the right eye wrong while flat" of 2026-11-12. The earlier line
   "ray tracing makes it eight times as frequent" compared two runs: `[disproved as stated]`.

## Read in the injector's source (PureDark/UEVR, AFW branch, fc8de10)

`src/mods/VR.cpp` 28-300: its DLSS hooks pass every DLSS call through unchanged unless the frame warp is on
(`is_using_afw()`): the evaluate hook only notes sizes and texture pointers, the resource-barrier hook returns at
once, the depth hook only works with "never DLSS". Its right-eye-only code (`FFakeStereoRenderingHook.cpp` 3101, 3203)
needs the frame warp or the compatibility modes. So in method 0 the ghost is made by the GAME's DLSS working on UEVR's
two-eyes-in-one-picture, not by the fork's DLSS code `[read in source]`.

Also read: the game window is the injector's "spectator view" (`D3D12Component.cpp` `draw_spectator_view`), drawn from
the scene picture; OBS's window capture of it showed NO ghost on the real headset while a screen-region recording did
`[measured]`: record the screen region (ffmpeg gdigrab with the window's rectangle).

## Ruled out as a test bench

The simulator at its largest size (`OpenXR_ResolutionScale` 1.6: one 4096 x 2240 picture) shows the whole two-eye
picture squeezed into BOTH eyes all the time: the simulator's own limit, not the headset's fault `[measured, n=1]`.
The setting is back at 1.0 (the file restored byte for byte).

## What fits the headset's ghost `[hypothesis]`

The right half of the right eye is exactly where the picture drawn at DLSS's inner size (66.7 %: 1792 px per eye,
3584 for two) passes x = 2688, one eye's full-size width. A pass limited to one eye's FULL size while working at the
INNER size would cut exactly there; behind the cut DLSS gets stale or wrong input, and old frames show as a
see-through second view while the head moves.

## The test that settles it: in the headset, about five minutes

The session switches live with the UEVR connector; Tefa says "gone" or "still there" after each.

1. DLSS at 100 % inner size (`r.ScreenPercentage 100` with DLSS on): gone = the inner-size / full-size cut above; the
   cure is then a size rule.
2. DLSS Performance (50 %): the cut would move to the right eye's left edge (the whole right eye ghosted): the same
   proof from the other side.
3. A still head for ten seconds: gone while still = stale history or motion vectors, not a wrong picture.
4. `VR_GhostingFix` off. 5. `VR_NativeStereoFix` on. 6. `OpenXR_ResolutionScale` 0.76 (a 4096-wide picture; needs a
   restart).

## Housekeeping found on the way

- `dev-archive/tools/optimiser/cvar_ab.py` deletes `sh2vr_devcvars.txt` when it puts values back: the file with the
  DLSS-on note was gone after the ray-tracing A/B; written again.
- `run-sim.ps1` runs do not copy the saves away: `PlayerProfile.sav` differed after the size test; the folder was put
  back from the copy made before Tefa's last wear (the changed one kept in the save-backups folder).
- `odd.py` (counts one-frame wrong pictures in a recording, per time bucket) is in
  `dev-archive/recon/2026-11-12-real-flat-cutscenes/`.
