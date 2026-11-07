# Gold Digger, 2026-11-07: eleven Silent Hill 2 crash dumps of 2026-11-05 to 11-07 that the notes do not mention

From: `/gd` inside `/fullscan` on the home PC (PC1), reading the silent-hill-2-remake-vr `crashes` and `runs` shelves.
Read from the dumps' own exception record and module lists (a small reader, no debugger). Nothing tested live.

Of 32 run cards on the shelf, 24 end `crashed`, each with a dump; the modding notes of 11-05 and 11-06 name none of them.
The dumps fall into three groups `[measured 2026-11-07, from the dumps]`:

## 1. A crash inside UEVR: four times at the same spot (2026-11-05 afternoon)

`0xC0000005` (read of a bad address) at **`UEVRBackend.dll+0x1503df`**, reading address `0x58` (three times) or `0x78`
(once): a null pointer with a field offset. Dumps `2026-11-05_16-16-20`, `16-21-36`, `16-28-14`, `19-26-31`
(items named `..._SHProto-Win64-Shipping.exe.<pid>.dmp` on the crashes shelf). Same day as
`2026-11-05-uevr-inside-our-mod-and-the-launcher.md` and the optimiser steps, so it may belong to one of those builds;
which UEVR build was in at 16:16 is the first question.

## 2. Two write faults in Windows' own code (2026-11-05 evening)

`0xC0000005` WRITE at `ntdll.dll+0xc3ab6` / `+0xc3a99`, to `0x722aa00f18` and `0x3379000ff0`: addresses just under a
page boundary, the usual look of a thread running out of stack `[hypothesis]` (deep recursion, or a very large local).
Dumps `2026-11-05_19-22-28`, `19-30-35`.

## 3. Since 2026-11-06: a "fail-fast" crash at quit, every run

`0xC0000409` with code `0xa` (Control Flow Guard: a call to a place that is not a valid function `[inferred-static]`,
the Windows fast-fail code list) in dumps `2026-11-06_00-57-27`, `01-07-27`, `21-08-52`, `2026-11-07_00-24-08`, and
code `0x7` (`abort()`, in `ucrtbase.dll+0xa527e`) in `2026-11-06_20-48-18`. In all four `0xa` dumps the crash address
ends in `...0828f` and lies in no loaded library, and Steam's libraries (`steamclient64.dll`, `tier0_s64.dll`,
`vstdlib_s64.dll`) are already in the UNLOADED list: so it happens while the game is shutting down, after Steam let go,
most likely a call into a library (ours, UEVR's, or a plugin) that is already gone `[hypothesis]`.
The run cards show these runs lasted 70 s to 13 min, then `exit_code 0xC0000409`.

Effect: every quit since 2026-11-06 ends as a crash. It costs nothing in play, but it writes a 100 to 160 MB dump each
time, and anything saved at exit (settings, logs) may be cut short.

## Proposed board rows

- `[PD] SH2: which UEVR build crashed at UEVRBackend.dll+0x1503df on 2026-11-05 (null + 0x58), and is it still in?`
  Proof: the address resolved against that build's symbols, or the build named as gone.
- `[FLAT] SH2: one launch and a clean quit with only our proxy (no UEVR), then with UEVR: which one makes the quit crash
  (0xC0000409 / 0xa)?` Proof: a run card that ends `closed` with exit code 0.

Side note for the tool: the dumps are large; `gd.py digest` on the crashes shelf prints only raw memory strings.
The exception record and the module list (what this note used) would be the useful digest for a dump.
