# 2026-10-24 — the fire gate, read from the game's own code (no game running)

/pd, home PC, Fable. Follows `2026-10-24-fire-without-aim.md`, which ended with "read the native
`SHRangedCmbSubcomp::RequestManualFire` at 0x142A90FE0 for the state it checks". Everything below was read from
`SHProto-Win64-Shipping.exe` on disk (image base 0x140000000; the Steam build installed on the home PC on 2026-10-24).
**The game was not launched and nothing here has been run.** Every claim is `[inferred-static 2026-10-24]` unless
tagged otherwise.

## The chain, as the code has it

1. The Blueprint wrapper of `RequestManualFire` (0x1429AB340) unpacks its one Object argument and calls the real
   function at **0x142A90FE0**, which is two instructions: `xor edx, edx; jmp 0x142A90C10`. **The weapon argument is
   thrown away**: RequestManualFire(weapon) is the shared fire request (0x142A90C10) with its byte argument = 0.
2. **0x142A90C10, the fire request** (this = the ranged combat part, dl = kind, 0 for a manual request):
   - weapon = `[[this+0x30]+0x1C8]+0x38` (the equipped ranged weapon, class-checked); none → return.
   - `cmp byte [this+0x118], 0` → **return at once when 0**. This byte is what the UFunction `IsInRangedAim()` returns
     (its wrapper, 0x1429AAC50, is `movzx eax, byte [rcx+0x118]`). **This is the gate the live probe hit** (the probe
     logged `in ranged aim 0` with the gun lowered).
   - a cooldown check on `this+0x198` (helper 0x142AA9B20): while it runs, the request is queued
     (`[this+0x11B] = 1`) instead.
   - `weapon->vtable[0x718](weapon)` must be true: the weapon's **virtual** `IsFireReady`. The UFunction `IsFireReady`
     wrapper (0x142978F40) calls the same slot, so hooking the UFunction (Lukas's pump script, our F11) only changes the
     answer Blueprint callers see; the fire path calls the virtual straight. This is why F11 did nothing
     `[verified-live 2026-10-24, n=1]` for the symptom, now with its cause.
   - then `weapon->vtable[0x750](weapon, 1)`: **the weapon's own fire**. On success: `[this+0x128] = 1`, the cooldown is
     started, the weapon's fire delegates (weapon+0xC08, +0xC20) are bound to the part once, and the combat input mode
     is set to **13** (the live probe saw 12 → 13 for a frame).
3. **The virtual IsFireReady (0x142B17140)**: owner = `[weapon+0x320]`; comp = `[owner+0x660]`; in that component's list
   (`+0x418`, count `+0x420`) the entry whose item (`entry+0x38`) is this weapon gives a float at `entry+0x54`; the answer
   is `float >= [weapon+0x690]`. No entry → false. So "ready" is a per-held-item value (the gun being raised, most
   likely) reaching a threshold the weapon carries. Lowered, the value stays below it.
4. **The weapon's fire (0x142B0FC70, slot 0x750, dl = 1)**: `rounds = [weapon+0xBB4]` (Lukas's offset, confirmed by this
   read); unless a debug byte on an owner component says otherwise, `rounds <= 0` goes to the empty path; then the shot:
   pellets `[weapon+0x750]` clamped 1..6, spread from `[weapon+0x754]` / `[weapon+0x758]`, the direction from the
   weapon's virtuals 0x728 / 0x738, a trace (0x142B23260), the hit handling (0x142B17D30). **Nothing in it reads the aim
   state.**

Also seen: the function that switches the aim ON (0x142A95908 region, the only `mov byte [this+0x118], 1` in the part's
code) first checks the combat input mode `[combat+0x152]` against the list {2, 3, 10, 11, 15, 19}, broadcasts the
delegate at `this+0x130`, sets `[this+0x129] = [this+0x12C] = 1`, sets the byte, and calls `weapon->vtable[0x768](weapon, 1)`.
Not needed for the doors, kept for later.

## How the addresses were found (reusable on any UE5 shipping exe)

- RTTI is stripped (no `.?AV` strings), so vtables cannot come from type descriptors.
- **Reflection name → wrapper:** every UFunction name is a UTF-8 string in `.rdata`; the FNameNativePtrPair table holds
  `{char* name, void* exec}`. Finding the qword pointer to the name string and reading the next qword gives the wrapper
  (checked: "RequestManualFire" → 0x1429AB340, which the live notes already had). Two tables point at each name; the
  one whose neighbour is in `.text` near the other wrappers is the pair table. Script: `sh2_reflect.py` (scratch).
- **Class → vtable:** the UTF-16 class name is `lea`'d once, in `StaticClass()` (0x14297EAE0 for SHItemWeaponRanged);
  that call hands `GetPrivateStaticClassBody` the class constructor (`InternalConstructor<T>`, stack argument at
  `[rsp+0x48]`); it tail-jumps to the real constructor (0x142B04BA0), whose first `lea rax,[rip+X]; mov [r14], rax`
  stores the vtable: **0x14678C5C0**. Script: `sh2_slots.py` (scratch). Slots read: 0x718 → 0x142B17140, 0x750 →
  0x142B0FC70, 0x768 → 0x142B084E0, 0x700 → 0x142AE3780.
- Property NAMES for these offsets were not found (the FPropertyParams scan for `ArrayDim 1, Offset N` found nothing;
  the layout guess for this engine version is wrong, or the fields are not reflected). Names are not needed for the
  doors; a live `uevr_inspect_object` on the part and the weapon would name them in a minute.

## What was built (installed, UNRUN)

`src/fire.cpp` pass 1, `settings.h` (offsets named, with the note they belong to this build), installed with
`build.sh --deploy` (backup `/d/SH2 builds/_backup-2026-10-24-1548-before-sh2vr-deploy`) `[compile-verified 2026-10-24]`:

- **The offsets prove themselves first.** On either key the plugin compares the game's own `IsInRangedAim()` with the
  byte at +0x118 and the game's `IsFireReady()` with the virtual at slot 0x718, logs both, and opens the doors only on
  AGREE. DISAGREE means the layout read here does not match the running build; nothing is written then.
- **F9, door A:** the aim byte is set to 1 and the weapon's threshold (+0x690) to 0 for the length of one
  `RequestManualFire(weapon)` call, then both are put back. The game's own request runs every step (cooldown,
  delegates, input mode 13).
- **F8, door B:** `weapon->vtable[0x750](weapon, true)`, the weapon's own fire, with the combat part skipped.
- F11 (force the UFunction answer) stays as the proven-useless control.
- Log lines: `fire: offset check: ... -> AGREE` / `DISAGREE`, `fire door A: ...`, `fire door B: ...`, and one second
  later `fire door X: 1.0 s later: N shot(s) performed since`.

## What the next run tells, with the gun LOWERED and rounds in it

| seen | means |
| --- | --- |
| `offset check ... DISAGREE` | the offsets do not fit the running build; nothing was poked; re-read the exe that is actually installed |
| door A: `1 shot(s) performed`, a bang heard | the two gates were the whole wall; the mod can fire lowered through the game's own request |
| door A: 0 shots, door B: 1 shot | the request has a further check past the two (the cooldown path, or input mode 13 refused while lowered); door B is the route |
| both 0 shots, both AGREE | the weapon's fire has a check this read missed (the debug byte path, or the trace / hit handling); read 0x142B0FC70 past 0x142B0FF51 |
| a shot, but the character raises the gun or the aim state sticks | input mode 13 drags the state machine; prefer door B, or restore the mode after |
| the bullet goes somewhere odd | the direction virtuals (0x728 / 0x738) aim from the game's camera or muzzle, not the controller; the aim fix is the next job, not the gate |

## What is NOT established

- That the byte at +0x118 is the only reader of the aim state on the request path: the cooldown branch
  (0x142A90F72) re-checks it, which door A also satisfies; anything inside `0x142A900A0` (called with mode 3 on the
  queued path) was not read.
- What the float at `entry+0x54` is (the raise alpha is a guess from its behaviour), and whether setting the threshold
  to 0 could pass a NaN / negative value. Door B does not depend on it.
- Whether the vtable slot numbers survive a game patch: they belong to this exe. The agree-check is the guard.
- Anything about movement: walking and running while holding the gun up is untouched by this note (door 2 of the
  earlier note).
