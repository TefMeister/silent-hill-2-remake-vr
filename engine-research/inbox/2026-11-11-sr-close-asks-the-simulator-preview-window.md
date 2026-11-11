# /sr 2026-11-11: under the OpenXR Simulator, "the process's main window" may be the simulator's preview

From: `/sr` (estate sweep, part of the 2026-11-11 `/fullscan`). For the modding lane (SH2 tools and launcher).

**What the-darkness-vr found** `[verified-live 2026-11-10, n=3]`
(<https://github.com/TefMeister/the-darkness-vr/blob/main/modding-notes/2026-11-10j-the-polite-close-was-asked-of-the-wrong-window.md>):
under elliotttate's OpenXR Simulator the game's process owns TWO top-level windows, the simulator's preview
("OpenXR Simulator - ...") and the game's own, and the preview comes first. A close message to the preview closes only
the preview; the game plays on. Sent to the game's own window (matched by title), the game quit in 0.7 s with its own
quit in the log.

**Why it may matter here** `[hypothesis, not checked against SH2]`: `dev-archive/tools/optimiser/run-sim.ps1` posts
WM_CLOSE to `$p.MainWindowHandle`. .NET picks that handle by its own rule (a visible, unowned top-level window of the
process), so under the simulator it can be the preview rather than the game. If an SH2 run under the simulator ever
needed the 30 s wait or a forced end after the "polite" close, this is a candidate. Note the separate quit abort
(the simulator's preview thread, `/gr` drop of today) also lands at exit, so the two could look alike in a log.

**Cheap check:** under the simulator, list the process's windows with their titles, and close by the game's own
title (the Darkness script `dev-archive/build-scripts/dk_close.py` does exactly that, and waits until no copy is left).

Library entry: `flat-to-vr-cross-engine-research/docs/runtime-layers/README.md` → "Headset-free testing" → the
simulator traps.
