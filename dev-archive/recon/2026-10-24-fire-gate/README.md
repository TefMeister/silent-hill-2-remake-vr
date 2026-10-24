# 2026-10-24 — offline lookups for the fire gate

The four scripts that read `SHProto-Win64-Shipping.exe` on disk for `modding-notes/2026-10-24d-the-fire-gate.md`
(no game running; capstone + pefile; the disassembly itself came from `flat-to-vr-RE-toolkit/tools/static-disasm.py`).
Our scripts only: no game bytes, no listings are kept here.

| script | what it answers |
| --- | --- |
| `sh2_reflect.py` | UFunction name → Blueprint wrapper address (the `{name, exec}` pair table in `.rdata`); every byte write to `[reg+0x118]`; RTTI check |
| `sh2_vtable.py <lo> <hi> <Class>...` | the `+0x118` writers inside one code range; where a UTF-16 class name is `lea`'d (its `StaticClass()`) |
| `sh2_slots.py <InternalConstructor VA>` | constructor → vtable → the virtual slots the fire path calls, each disassembled to a file |
| `sh2_propnames.py <offset>...` | an attempt to name reflected properties by offset; found nothing for this engine version (layout guess wrong) |

Results in use: SHItemWeaponRanged vtable 0x14678C5C0; slot 0x718 = IsFireReady 0x142B17140; slot 0x750 = fire
0x142B0FC70; the ranged combat part's aim byte at +0x118; the weapon's fire-ready threshold at +0x690. All
`[inferred-static 2026-10-24]`, this exe only.
