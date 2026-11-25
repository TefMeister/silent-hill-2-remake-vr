# Gold Digger 2026-11-25: Silent Hill 2 crashes 2026-11-21 to 11-22

From: /gd inside an unattended /fullscan (Build PC). Shelves read: crashes (56), runs (200), hands (135),
performance (543). Almost all repeats.

## New [measured, from the crash dumps' first lines]

- **A crash inside our own frame meter**: `crashes/...SHProto-Win64-Shipping.exe...` of run PC1-2026-11-21_22-01-34:
  `0xC0000409 stack buffer overrun / fail-fast in lanes_tune_meter.dll+0x13BAD` (The Optimizer's OpenXR layer,
  Lanes branch tune-o-matic). n=1. Filed on FAULTS.md as a Lanes fault the same night.
- **An access violation in the C runtime**: run PC1-2026-11-22_19-54-39: `0xC0000005 in ucrtbase.dll+0x43006`,
  writing address 0xF7DBB00FF0, Steam still loaded. Who called it is not in the first lines. n=1.
- The run cards show the game's own crash reporter (`CrashReportClient.exe`) starting at least 9 times between
  2026-11-21 15:51 and 2026-11-22 13:16. Board row proposed if the notes do not already count them: "SH2 crashed about
  9 times on 11-21/22: sort the dumps by where they died (gd.py fold on each) and see which build each one had."

## Nothing new

Performance shelf: 423 "unusual minute" lines across the runs, no warnings; hands and runs shelves: repeats.
