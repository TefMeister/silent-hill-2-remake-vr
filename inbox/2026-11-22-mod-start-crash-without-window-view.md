# The start crash without Window View: UEVR's extra scene state (2026-11-22, /lm reader)

From: the background reader. Read only. Sources: today's dumps `start-04-crash` and `start-05-fail`
(`%LOCALAPPDATA%\TheOptimizer\silent-hill-2\starts-2026-11-22\`), the four older dumps and 28 UEVR start logs in
`D:\claude video game stuff\logs\silent-hill-2-remake-vr\`, all read with cdb. Follows
`2026-11-22-mod-window-view-layer-reading.md`. Dossier section 13 covers a different crash on the game thread.

## 1. Same crash? Yes, both of them `[measured]`

- **start-05:** identical to the other four. RenderThread 0, `SHProto+0x380a03f` `call [rax+0xF0]`, the same call chain,
  and the slot read gives a garbage pointer.
- **start-04:** the same thread and the same chain from frame 5 down, but a different call site
  (`SHProto+0x3553894`, `call [rax+0x58]`), and it called address 0.
- **Window View's layer is not loaded in either dump.**

## 2. What is at 0x380a03f `[measured in the dump, inferred-static]`

- It is a small function at `SHProto+0x380a020` with two inputs (`this`, `arg`).
- If `arg+0x18` holds an object, it calls that object's virtual slot 0xF0 (`obj, this`) and returns the result when not null.
- Otherwise it returns `this+0x37B8`.
- **The object called through is UEVR's own inserted scene state, in 5 of 5 dumps that have a log:**
  - 15:11: `1f7cd45aac0`
  - 21:47: `1635a64d560`
  - 22:33: `1d3629c0040`
  - 11-18: `181cc625580`
  - start-04: `225ca870040`
  - Each is exactly the address in the last `Inserting new scene state` / `Setting scene state to` line before the crash.
    start-04's different call site goes through the same object.
- In every dump that object's first word (its vtable pointer) no longer points into the game exe. In 21:47, for example,
  it is a heap address. So the memory was freed and reused while the render thread still held it: a use-after-free.
- `[hypothesis]` What frees it: the engine drops or rebuilds the view states during the ~400 ms stall. UEVR then logs
  `Scene state passed to FSceneView constructor is null`. UEVR's inserted state is either released with them or was made
  in memory the engine owns, but the render thread's view still points at it. Not proven; the next step is to read
  UEVR's scene-state insert code (FFakeStereoRenderingHook.cpp ~3048-3220).

## 3. Does the log ordering separate crashes from clean starts? No `[measured]`

- Every start, clean or crashing, logs the same order:
  1. first poses
  2. a ~4.3 s stall
  3. "called before view extensions"
  4. three `Inserting`, the vtable warning, `Setting`, the vtable warning
  5. a 390-480 ms stall
  6. the null line
- Time from `Setting` to the null line: crashes 446-2043 ms, clean starts 21-1927 ms. The ranges overlap.
- The vtable warning appears 1-2 times in both kinds.
- A further `Inserting` after the null line appears in both kinds.
- The only mark: a crash always comes 0-41 ms after the null line (where the log goes that far). A clean start just carries
  on. So the null line is the moment of the race, but it does not predict which way it goes.
- So nothing UEVR writes tells the two apart beforehand. Window View (and whatever else adds work around that frame) can
  change the timing, but the fault sits between UEVR and the engine.
