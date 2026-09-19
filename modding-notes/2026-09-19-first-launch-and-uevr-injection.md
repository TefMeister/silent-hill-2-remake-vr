# First launch, and UEVR is already in it

**2026-09-19, `/lm`, dev PC `DESKTOP-V8GTSIR`.** The game was launched, driven and left running by
Claude; Tefa was at work throughout. Evidence:
`dev-archive/recon/2026-09-19-first-launch-and-uevr-injection/`.

This is the game's first live session on this account, so it followed the standing three steps:
**runs as shipped → runs with our file → playable in a 1280×720 window.** All three passed, and the
third turned out to matter more than usual.

## 1. It crashed the first time, and the crash named its own cause

`LowLevelFatalError` in `RenderingThread.cpp:1273` —
**"GameThread timed out waiting for RenderThread after 120.00 secs"** `[verified-live 2026-09-19, n=1]`.

That is a **timeout, not a renderer fault**: the render thread was still working when the game thread
gave up. The defaults it was working through were **1920×1080 fullscreen, every scalability group at
3, and `Raytracing=On`** — on an **i7-3770S (2012, 4 cores) with a GTX 1660 SUPER**, read out of the
crash context, which also gave the engine version for free.

**⭐ Engine: Unreal Engine 5.1.1** (`5.1.1-0+++UE5+Release-5.1`) `[measured 2026-09-19]`. That is
well inside UEVR's supported range and is the single most load-bearing fact about this project.

## 2. Two changes, and it reached the menu

Both made with the game closed, because engines rewrite config on exit. Backups beside each file as
`*.backup-before-claude-2026-09-19`.

- `GameUserSettings.ini` — every `sg.*` group to **0**, ray tracing **off**, VSync and motion blur
  off, and **windowed 1280×720** (`ScreenMode=Windowed`, `FullscreenMode=2`, and the three
  resolution key pairs, which all have to agree).
- `Engine.ini` — a new `[ConsoleVariables]` block raising **`g.TimeoutForBlockOnRenderFence` to
  600000** (ten minutes instead of two). Insurance, not the fix: the point is that a slow machine
  should be allowed to finish compiling rather than be killed for taking too long. **Remove this on
  a normal machine.**

Result: main menu reached, **client area measured at exactly 1280×720** `[measured 2026-09-19]` —
measured, not assumed, per the standing rule.

⚠️ **Hazard for every future session: `CONTINUE` is the highlighted item and `NEW GAME` is directly
below it**, against a save file with 97 hours on it. Never blind key-count downward on this menu.
Recorded in the control profile.

## 3. UEVR injects, initialises, and runs in stereo

Build: **`UEVR_AFW_v1.0-beta.5+7-c40eaab8`** — ⚠️ note the zip is named `beta.6` but the binary
reports **beta.5 plus 7 commits**. Trust the binary `[measured 2026-09-19]`. Installed to
`D:\UEVR\AFW-beta6\`, hashes recorded in the dossier.

- The game **appears in the injector's process list without administrator rights**
  `[verified-live 2026-09-19, n=1]`, which removes a UAC prompt from every future session.
- `Hooked DirectX 12`, then **121 consecutive `Failed to initialize Framework on DirectX 12`**
  errors over about two seconds — **and then `Framework initialized`.** The failures are retries, not
  a fault. Worth writing down precisely because a session reading that log cold would call it broken.
- **OpenXR came up**: `Requested runtime: openxr_loader.dll`, instance created, full extension list
  enumerated (hand tracking, eye gaze, local floor…).
- **The in-game overlay reports stereo**: `Render Resolution (per-eye): 1668 x 1856`,
  `Total Render Resolution: 3336 x 1856` `[verified-live 2026-09-19, n=1]`.

**So there is nothing game-specific blocking UEVR on Silent Hill 2.** It hooks, it initialises, it
opens a stereo session.

## 4. What is NOT established

- **Nothing has been seen in a headset**, and this machine has none. The log ends in
  `xrEndFrame failed: XR_ERROR_TIME_INVALID` / `xrBeginFrame failed: XR_FRAME_DISCARDED` — frame
  *timing* errors, which is exactly what an OpenXR session with no compositor pacing it produces.
  **That is expected here and is not evidence of a problem** `[inferred-static 2026-09-19]`.
- **Anti-tamper is now answered and the executable is clean** `[measured 2026-09-19]` — no packer,
  no protector, ordinary entropy, MSVC CRT entry point, both TLS callbacks empty, 1,015 named
  imports, CFG not enforced, zero signature hits across all 142 MB (dossier §8i). ⚠️ Static evidence
  proves the absence of *known* protections, not of a bespoke check, so the step to "injection
  works" stays `[hypothesis]` — corroborated live once here. ⚠️ SH2R is publicly reported to have
  shipped with Denuvo and **this build has none**, so the risk is a future patch re-adding it: a
  later session that cannot inject should check the Steam build ID before blaming its own code.
- Whether the picture is *correct* in stereo — geometry, scale, UI — is untouched. Only that a
  stereo session exists.
- The dev PC is far below the game's minimum spec, so **no frame rate measured here means anything.**

## 5. The decision this session recorded

**PureDark's AFW build is OPTIONAL, not a dependency** (Tefa, 2026-09-19). It is distributed as
binaries only — there is no source in the archive — so "building on it" could only ever have meant
"our users must also download it", which is exactly the extra download the project set out to avoid.
Checked for licence-key or DRM strings: **none found** `[inferred-static 2026-09-19]`. The mod
targets plain upstream UEVR and treats AFW as a comfort add-on.

## 6. Why the native approach is the right call — corrected after the static pass

⚠️ **An earlier draft of this paragraph, written before the full read came back, got three things
wrong and they are withdrawn here rather than quietly edited:** the profile is **49,718 lines of Lua,
not ~6,200**; it does **not** copy the pose every frame (that path is the montage fallback — the tick
is either/or); and the IK is **not** hand-rolled, it calls Unreal's own `K2_TwoBoneIK`. All three
`[disproved 2026-09-19]`. Most of that Lua is also a **shared cross-game framework**, not SH2 code.

**The argument survives, on better ground.** The objection was never that they wrote their own solver —
they sensibly call Unreal's. It is that **every input to and output from that solver crosses the script
boundary, per arm, per frame, by reflection**, on a poseable puppet that carries James's entire clothed
body. Their own source says so: a fast path exists *"if you are calling this function on the tick and
need the best performance"* because the thorough one was too slow; and `ik.lua:1511` records that one
conversion *"amplifies the 0.036° of real controller movement into 0.220°"*, fixed by smoothing —
**latency spent to hide numerical error.** That is Tefa's "janky" as a measured defect rather than taste.

⭐ **And the sharpest point is unchanged: on a poseable mesh the game's own animation graph does not
run**, so the game's interaction hand animations can only ever be copied in by hand — which is exactly
what their montage fallback is doing. Letting the real graph run and applying IK on top gets that for
free. Full corrected map, including the SH2-specific object graph and bone names: dossier §8e.
