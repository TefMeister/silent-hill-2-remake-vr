# Gold Digger, 2026-11-11: eight more Silent Hill 2 crash dumps (2026-11-10), and the quit crash moved

From: `/gd` inside `/fullscan` on the Keeper PC (PC2), reading the silent-hill-2-remake-vr `crashes` and `runs`
shelves. Read from each dump's own exception record and module list (a small reader, no debugger), the same way as
`2026-11-07-gd-sh2-eleven-crash-dumps.md` (which this adds to; it does not replace it). The reader gives the same
answer as that note for the dump of 2026-11-07 00:24 (`0xC0000409`, code `0xa`, in no loaded module), so the two
notes read alike. Nothing tested live.

All eight runs of 2026-11-10 (12:07 to 15:20, the optimiser day in the simulator) end `crashed`, each with a dump of
145 to 158 MB. In every one Steam's libraries are already in the unloaded list: the crash happens while the game is
shutting down `[measured 2026-11-11, from the dumps]`.

## 1. The UEVR crash of 2026-11-05 is still in

`0xC0000005` read of address `0x58` at **`UEVRBackend.dll+0x1503df`**, the same spot and the same null + 0x58 as the
four dumps of 2026-11-05 `[measured 2026-11-11]`: dumps `2026-11-10_12-07-56` (run of 221 s) and `12-12-29` (271 s).
So the first proposed row of the 11-07 note stands, with an answer to half of it: the build that crashes there was
still loaded on 2026-11-10 at noon. (The 11-07 research note says the two UEVR flavours are different files, so
`+0x1503df` must be resolved against the one in use that day.)

## 2. From 13:00 on, the quit crash is inside the OpenXR Simulator

`0xC0000409` with code **`0x7` (`abort()`)** at **`openxr_simulator.dll+0x919bd`**, in all six later dumps:
`2026-11-10_13-00-03` (run of 2841 s), `13-03-53` (233 s), `13-33-44` (947 s), `13-54-28` (590 s), `14-54-27`
(3142 s), `15-20-41` (1094 s) `[measured 2026-11-11]`. The 11-07 flavour (code `0xa`, an address in no library) does
not appear on 2026-11-10. So the crash at quit now comes from the simulator aborting while the game shuts down
`[hypothesis: the simulator is torn down while UEVR still calls it, or it is asked to end a session twice]`. The
simulator in use on PC1 is the original v1.7.1 (installed record of 2026-11-06), so this may be the simulator's own
fault rather than ours.

Effect as before: costs nothing in play, but every quit writes a 150 MB dump, and settings or logs written at exit
may be cut short.

## Proposed board rows

- `[PD] SH2: resolve openxr_simulator.dll+0x919bd against the simulator v1.7.1 release (its source is public) and
  say which call aborts at quit.` Proof: the function named; then either a fix on our side (end the session before
  quitting) or an issue on the simulator's repo.
- `[FLAT] SH2: one quit with the simulator but without UEVR, one with UEVR but without the simulator (the real
  runtime): which one crashes.`
