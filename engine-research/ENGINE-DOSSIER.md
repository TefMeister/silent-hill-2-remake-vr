# Engine dossier — Silent Hill 2 (2024 remake)

What is actually known, and how well. Confidence tags follow the house vocabulary:
`[verified-live]`, `[measured]`, `[verified-numerically]`, `[compile-verified]`,
`[inferred-static]`, `[reported]`, `[hypothesis]`, `[disproved]`.

⚠️ **Read the tags on this page before planning anything.** This dossier was opened on 2026-09-18
with the game never launched, never owned-and-checked from here, and no file of it ever opened. Almost
everything below is `[reported]` or `[hypothesis]`. **There is currently not one `[verified-live]`
line on this page**, and that is the single most important fact about it.

## 1. The game

*Silent Hill 2*, Bloober Team, published by Konami, released October 2024. A remake of Team Silent's
2001 original.

- **Engine: Unreal Engine 5** `[reported]` — widely stated publicly; not confirmed from the shipped
  files here.
- **Platforms:** PC (Steam, Epic) and PlayStation 5 `[reported]`.
- **Owned / installed on either machine?** Not checked. Nothing in this project has looked at a real
  install.
- **Which UE5 minor version** the shipping build uses is unknown, and it matters: UEVR's per-version
  behaviour differs. `[hypothesis]` that it is within UEVR's supported range — untested.

## 2. Why this project exists at all

UE4 and UE5 have **UEVR** (praydog), a general-purpose injector that already knows how Unreal
delivers cameras, projections and stereo. The standing rule on this account about picking new games
is to **name the engine first**, because a UE4/UE5 pick is far cheaper than a bespoke engine.

This is the only Unreal project on the account, and therefore the cheapest VR target on it
`[inferred-static]` — inferred from how much cheaper UEVR makes the first stereo image, not from any
measurement on this game.

## 3. The route in — what has been named, and what has been checked

Three specific things were named by Tefa on 2026-09-18 as the starting point. **None of the three has
been looked at from here.** Listing them is not endorsing them.

| Named | What it is claimed to be | Status here |
| --- | --- | --- |
| **jbusfield's UEVR profile** for this game | an existing published profile | `[reported]` — not located, read, or tested |
| **CharlotteLiu's mod** built on top of it | a mod layered on that profile | `[reported]` — not located or read |
| **PureDark's "AFW" build of UEVR** | a modified UEVR to build on | `[reported]` — **`[not judged]` for buildability.** Nothing known here about what it is, whether it is a fork we can build on, how it is distributed, or what depending on it would mean under the redistribute-nothing rule |

**The stated intention:** study the first two, depend on neither, and reimplement. That is the same
study-then-reimplement pattern used on every other project here, and it is what keeps a mod ours to
release.

⚠️ **Do not plan around the AFW build until a `/gr` pass has actually examined it.** The rest of the
plan works on plain upstream UEVR, which is the safe default and what everything below assumes.

## 4. Anti-tamper — ANSWERED 2026-09-19: the executable is clean (see §8i)

Whether the shipping build carries anti-tamper that interferes with injection has **not been
checked** `[hypothesis]`. A recent commercial title usually does something. This is worth resolving
early because it is the one finding that could invalidate the whole approach rather than merely cost
time.

## 5. Windowed mode — owed on first launch

Standing rule: on a game's first live session, before other work, prove it runs as shipped, then runs
with our smallest file present, then **make it playable in a 1280×720 window** — found through the
game's own setting (config file, registry, command line or menu), measured, backed up, and recorded
here.

**Not done. The game has never been launched for this project.** Prefer a config or command-line
route: in-game "keep these settings?" countdowns revert when nobody is there to click.

## 6. What the mod is meant to become

Design detail lives in the idea repository, not here. In summary, two threads are settled enough to
name:

- **Difficulty and dread** — fewer enemies, stronger and faster, placed differently; the radio silent
  until you are already being hit; combat music only once it has started; Pyramid Head driven off by
  damage rather than by a scripted beat (he does not die — he walks out of the door).
- **Everything on your body, nothing in a menu** — pistol right hip, map left hip, shotgun right
  shoulder, rifle left shoulder, all taken with the right hand; ammo at the left hip taken with the
  left hand; the torch stowed centred **above the head**, taken and held with the left hand.

⚠️ **Whether a UEVR profile can reach the actions these need** — swap weapon, use item — is the
central unknown for the second thread `[hypothesis]`. Knowing where head and hands are is what UEVR
is for; making the game act on a gesture is the part that has to be proven.

## 7. Dead ends

None yet. **This section earns its keep only when something has actually been tried**, and nothing
has. Research sessions read this section first, so leaving it honestly empty is better than filling it
with guesses.

## 8. ⭐⭐⭐ FIRST LIVE SESSION — IT RUNS, AND UEVR IS ALREADY IN IT (2026-09-19, `/lm`, dev PC)

Write-up: `modding-notes/2026-09-19-first-launch-and-uevr-injection.md`. Evidence:
`dev-archive/recon/2026-09-19-first-launch-and-uevr-injection/` (menu screenshot, UEVR overlay
screenshot, the full injection log).

### 8a. The engine version, free from a crash

**Unreal Engine 5.1.1** — `5.1.1-0+++UE5+Release-5.1`, `BuildVersion ++UE5+Release-5.1-CL-0`
`[measured 2026-09-19]`. Read out of the crash context of the first launch, not guessed from release
notes. **Well inside UEVR's supported range**, which is the fact the whole project rests on.
Game build: Konami, `1.0.0.5 (21/01/2025)`, in-game version string `v.1.1.256.834`.

### 8b. The first launch crashed, and the crash was a TIMEOUT

`LowLevelFatalError [RenderingThread.cpp:1273] GameThread timed out waiting for RenderThread after
120.00 secs` `[verified-live 2026-09-19, n=1]`. **Not a renderer fault** — the render thread was
still working when the game thread gave up, on shipped defaults of 1920×1080 fullscreen, every
`sg.*` group at 3 and **`Raytracing=On`**, on an i7-3770S + GTX 1660 SUPER. ⚠️ **A session that
reads "LowLevelFatalError" as a broken install will go looking in the wrong place.**

### 8c. ⭐ The 121 DirectX 12 errors in the UEVR log are RETRIES, not a failure

The injection log contains `Hooked DirectX 12`, then **121 consecutive
`Failed to initialize Framework on DirectX 12`** over roughly two seconds, and then
**`Framework initialized`** `[verified-live 2026-09-19, n=1]`. Recorded here because reading that log
cold, the obvious conclusion is that UEVR failed, and it did not.

### 8d. UEVR injects and opens a stereo OpenXR session

- Build in use: **`UEVR_AFW_v1.0-beta.5+7-c40eaab8`**. ⚠️ **The archive is named
  `UEVR-nightly_AFW_v1.0-beta.6.zip` but the binary reports beta.5 plus 7 commits** — trust the
  binary `[measured 2026-09-19]`. Installed at `D:\UEVR\AFW-beta6\`; `UEVRBackend.dll` sha256
  `d3ff93e7f327016da2dfcbb571b7651f614e3786053af4323b7f8f6a38516178`, `PDAFWPlugin.dll`
  `5f3cfc38903ac438241a3c47cb4bc3ac329cb39b056e33a63c20045fe15336f9`.
- **The game appears in the injector's process list WITHOUT administrator rights**
  `[verified-live 2026-09-19, n=1]` — so no UAC prompt is needed, which matters for unattended runs.
- OpenXR instance created, full extension list enumerated. **In-game overlay reports
  `Render Resolution (per-eye): 1668 x 1856`, `Total Render Resolution: 3336 x 1856`**
  `[verified-live 2026-09-19, n=1]`.
- **Nothing game-specific blocks UEVR here.** UEVR writes its per-game profile to
  `%APPDATA%\UnrealVRMod\SHProto-Win64-Shipping\`; nothing is installed beside the game executable.
- ⚠️ The log ends in `xrEndFrame failed: XR_ERROR_TIME_INVALID` / `xrBeginFrame failed:
  XR_FRAME_DISCARDED`. These are frame **timing** errors from an OpenXR session with no compositor
  pacing it, which is the expected state on a machine with no headset `[inferred-static 2026-09-19]`.
  **Do not read them as a fault.**

### 8e. ⭐⭐ HOW THE COMMUNITY PROFILE REALLY DOES THE BODY — corrected, and it strengthens the native case

⚠️ **This section replaces a first version written earlier the same day from a partial read. Three
of its claims were wrong and are withdrawn below.** The corrected map comes from a full static pass
(inbox drops `2026-09-19-reader-anti-tamper-static-audit.md` and
`2026-09-19-reader-community-profile-body-and-hands-map.md`, drained 2026-09-19; their full text,
including the complete 45/46-bone lists, stays recoverable in this repo's git history).

**Withdrawn, all `[disproved 2026-09-19]`:**

| written earlier | actually |
| --- | --- |
| "~6,200 lines of Lua" | **49,718 lines** `[measured 2026-09-19, wc -l]` — the 6,197 counted was five files out of ~75 |
| "copies the pose onto a puppet every frame" | `CopyPoseFromSkeletalComponent` has **three call sites and is the FALLBACK**. The tick (`ik.lua:407-457`) is either/or: **montage playing → copy the pose; otherwise → run the IK solver.** In ordinary play the pose is never copied |
| "hand-rolled two-bone IK in Lua" | it calls **Unreal's own solver**, `UKismetAnimationLibrary:K2_TwoBoneIK`, and disables IK outright if `/Script/AnimGraphRuntime.KismetAnimationLibrary` is missing (`ik.lua:387`) |

Also corrected: the per-frame writes are **`SetBoneRotationByName`** (×16) and `SetBoneScaleByName`
(×8), not `SetBoneTransformByName` — all three uses of that are in the restore-after-montage path.

**⭐ And the single most useful structural finding: most of that Lua is not Silent Hill at all.**
`scripts/libs/` carries live references to `/Script/AtomicHeart.*`, `/Script/Indiana.HUDWidget`,
`/Script/GunfireRuntime.RangedWeapon` and `/Script/Phoenix.*` — it is a **shared cross-game UEVR
hands framework** carried between projects. Only `main.lua`, `melee.lua`, `camera.lua`, `data/*.json`
and `sh2r.dll` are SH2-specific `[inferred-static 2026-09-19]`. **That split is the map of what is
worth learning from: the framework is the low-value part; the JSON numbers and `main.lua`'s object
paths are the weeks of work.**

**What is confirmed:** a `PoseableMeshComponent` is created from the game's mesh, with a set-then-clear
`SetMasterPoseComponent`/`SetLeaderPoseComponent` pair to force one update
(`uevr_utils.lua:3397-3441`). **There is no first-person mesh** — body, arms and animation source are
all `Pawn.Mesh`, James's third-person body, with everything not-an-arm scaled away.

**The engine interface a native plugin would call instead:** `PoseableMeshComponent`,
`SkeletalMeshComponent`, `MotionControllerComponent`, **`KismetAnimationLibrary`** (the solver),
`KismetMathLibrary`, `KismetSystemLibrary`; bone API `Get/SetBoneRotationByName`,
`Get/SetBoneLocationByName`, `SetBoneScaleByName`, `GetBoneName`, `GetParentBone`, `GetNumBones`,
`BoneIsChildOf`, `HideBoneByName`, `TransformTo/FromBoneSpace`; `K2_AttachTo` for attachment;
`AddComponentByClass` for creation.

**SH2-specific knowledge — the expensive part, and the part worth having:**
- Arm chain `clavicle_01_{l,r}_bn → upperarm_{l,r}_bn → lowerarm_{l,r}_bn → hand_{l,r}_bn`;
  `spine_05_bn` for the body-hide trick; head hidden with `HideBoneByName("neck_02_bn")`.
  ⚠️ **James wears a coat**, so there are about a dozen jacket-sleeve cloth bones per arm.
- Sockets `hand_l_socket` / `hand_r_socket`, `MuzzleSocket` on weapons — but **weapons attach to the
  BONE `hand_r_bn`, not a socket.**
- The object graph they had to discover:
  `pawn.Mesh.AnimScriptInstance.WeaponManageCmbSubcomp.EquippedWeapon`,
  `pawn.Items.EquipmentActors[]` → `/Script/SHProto.SHFlashlight`,
  `pawn.Items.ItemExecutive.ItemContext.Mesh`, `pawn:GetGameplayInputMode()`,
  `pawn.Movement.PushableComponent`, `pawn.CameraOverlapHandler`,
  `pawn.View:OverrideControlRotation(FRotator, pawn)`, `pawn.RootComponent.CapsuleHalfHeight`,
  `SHCharacterStatics::IsCharacterInCutscene(pawn)`. Weapons identified by name prefix
  `WeaponPistol` / `WeaponShotgun` / `WeaponRifle`.
- Numbers that cannot be derived, only tuned: right end-bone offset `(-5.725, 7.3769, 0.4778)` rot
  `(8, 140, -180)`; left `(-7.7141, -10.0053, 4.0928)` / `(-22, 50, 0)`; arms mesh offset
  `(-13.287, 0, -18.1721)` rot `(0, -90, 0)`; shoulder-width scale `1.74`; wrist twist `0.35`, max 75°.
  Shipped mode `hands_type: 3` = **IKArms**.

**The two native DLLs, both narrower than they look:**
- **`sh2r.dll`** hooks **exactly one** function, `MeleeWeaponEnvTrace`, located by **string-reference
  scan** (so a patch that moves that string breaks it). Build path leaks the project name
  **`SH2R-UEVR`**, built on GitHub Actions with praydog's **kananlib**. It raises Lua events
  (`OnMeleeTraceSuccess` with `Enemy`/`Glass`/`Environment`, `OnMeleeHitLeg`); the Lua then does a raw
  **`write_qword` at a hard-coded struct offset** to swap in `PistolDamage_C` for kneecapping — the
  most build-fragile thing in the whole profile. **It touches nothing to do with hands, body or IK.**
- **`uevr_utils.dll`** is a generic UFunction/UProperty bridge over **JSON on UEVR's custom-event
  channel**, and it exists for one reason its own docs give: *"the TArray issue that may exist in
  native lua calls."* **Three live call sites in 49k lines** — `SphereOverlapComponents`,
  `ComponentOverlapComponents`, `GetAllSocketNames`, all of which take or return a `TArray`.

**⭐⭐ Where the Lua is fighting the engine — and this is the real argument for native, stronger than
the one it replaces.** Nine places were flagged; the load-bearing ones, each backed by the profile's
**own source comments** `[inferred-static 2026-09-19, not profiled]`:
1. **A hand is made out of a whole body, per frame.** A `PoseableMeshComponent` carries all of James,
   so collapsing the rest walks **every bone** (`GetNumBones` → `GetBoneName` → `BoneIsChildOf` → a
   5-call transform write) — 6+ reflection calls per bone across a full UE5 skeleton **with a coat**.
   They know: there is a `parentPathOnly` fast mode *"if you are calling this function on the tick and
   need the best performance"*, and SH2 ships with it on. **The thorough version was too slow to run
   per frame.**
2. **The whole `KismetMathLibrary` layer is pure tax** — roughly 30 reflection calls per arm per
   frame for vector maths a native plugin does in registers.
3. **Precision is lost and then papered over.** `ik.lua:1511` says one component-space conversion
   *"amplifies the 0.036° of real controller movement into 0.220°… The noise is entirely in that one
   conversion."* The fix is NLERP smoothing — **latency spent to hide numerical error, not comfort.**
   ⭐ **This is the most direct evidence for Tefa's "janky" instinct being a real defect and not taste.**
4. `BoundsScale = 16.0` on every hand with their own note that *"it fixes flickering but >1 causes a
   perfomance hit with dynamic shadows… a better way to do this should be found"*, plus a whole
   `flicker_fixer.lua`.
5. The body is hidden by **scaling bones to 0.001**, with the ordering admitted as superstition:
   *"Applying them out of order results in a destroyed mesh."* The head is hidden a second way and
   the pawn meshes a third, through four independent switches because none works everywhere alone.

⚠️ **What none of this establishes:** nothing was seen running. The bone names come from **their
config, not the game's own skeleton** — if that config is stale the names are wrong and nothing here
would reveal it. The DLLs were read by strings/imports/exports and **not decompiled**, so the hooking
method is inferred. `EBoneSpaces` values are used inconsistently across two of their files and were
**not** confirmed against the SDK — re-derive those before relying on them. No performance claim here
was profiled.

**⭐ The conclusion for our design, restated on the corrected facts:** the objection is not that they
wrote their own IK — they sensibly call Unreal's. It is that **every input to and output from that
solver crosses the script boundary, per arm, per frame, by reflection**, on a puppet mesh that carries
an entire clothed body. Tefa's wish for **the game's own interaction hand animations** stays the
sharpest argument: on a poseable mesh the game's animation graph does not run, so those animations
can only ever be copied in by hand — which is precisely what their montage fallback path is doing.
Native work that lets the real animation graph run and applies IK on top gets that for free.

### 8f. Settings this machine needs, and what to undo elsewhere

`%LOCALAPPDATA%\SilentHill2\Saved\Config\Windows\`, edited with the game **closed** (engines
rewrite config on exit); backups alongside as `*.backup-before-claude-2026-09-19`:

- `GameUserSettings.ini`: all `sg.*` groups `0`, `Raytracing=Off`, `VSync=False`,
  `IngameMotionBlur=False`, `ScreenMode=Windowed`, `FullscreenMode=2` and **all three** resolution
  key pairs set to 1280×720 — they must agree or the window does not take.
- `Engine.ini`: `[ConsoleVariables]` → `g.TimeoutForBlockOnRenderFence=600000`.
  **Remove this on a normal machine** — it exists only so a below-minimum-spec PC is allowed to
  finish compiling instead of being killed at 120 s.
- Verified: **client area exactly 1280×720** `[measured 2026-09-19]`.

### 8g. ⚠️ Menu hazard — `NEW GAME` sits directly under the default highlight

The main menu is `CONTINUE` (highlighted) / `NEW GAME` / `LOAD GAME` / `SETTINGS` / `CREDITS` /
`EXIT`, against a save with 97 hours on it. **Never blind key-count downward on this menu** —
navigate, capture, verify the highlight, then commit. Same family as the Enslaved `NEW JOURNEY` trap.
Recorded in `ai-game-control-profiles/profiles/silent-hill-2-remake.json`.

### 8h. The AFW decision

> **Superseded 2026-09-26 (Tefa): we build ON the UEVR nightly AFW build** (`v1.0-beta.6`). The text below
> is the 2026-09-19 decision, kept for the record. Still true: we redistribute nothing of AFW, so players
> fetch that build themselves. See `modding-notes/2026-09-26-both-community-profiles-and-the-game-animation-idea.md`.
> **The AFW build also offers Native Stereo, Synchronized Sequential and AFR** in its Rendering Method menu
> `[inferred-static 2026-09-26, strings]`. **Supported (Tefa, 2026-09-26): Native Stereo and AFW only**; Sequential
> and AFR are not. Per-frame work still runs once per game tick, not once per eye, because AFW draws one eye per
> frame too. Test in Native Stereo AND AFW.


**AFW is OPTIONAL, not a dependency** (Tefa, 2026-09-19). The archive is binaries only — no source —
so depending on it would force every user to fetch a third-party build, which is the opposite of the
project's stated aim. Scanned for licence/DRM strings: **none found** `[inferred-static 2026-09-19]`.
The mod targets plain upstream UEVR; AFW stays a comfort add-on that must never be required.

### 8i. ✅ ANTI-TAMPER: NOTHING IS GUARDING THIS EXECUTABLE (2026-09-19, static)

Folded from `inbox/2026-09-19-reader-anti-tamper-static-audit.md`. All `[measured 2026-09-19]`
(Python + pefile 2024.8.26 + capstone 5.0.7):

- **No packer or protector.** All nine sections carry stock MSVC names; no `.vmp`, no Themida, no
  Steam `.bind`.
- **Entropy is ordinary.** `.text` mean 6.35 over 1,508 windows; the only three windows above 7.2 are
  **OpenSSL** — the region literally contains *"Montgomery Multiplication … CRYPTOGAMS"*. Not
  virtualised code.
- **Entry point is the 4-instruction MSVC CRT stub**, not redirected into an unpacker.
- **Both TLS callbacks disassembled** — they are MSVC's `__dyn_tls_init` / `__dyn_tls_dtor`. The
  classic anti-debug hiding place, and it is empty.
- **1,015 named imports across 46 DLLs** plus 17 delay-loaded. A packer collapses this to a handful.
- **Anti-debug surface is ordinary Unreal crash handling** (`IsDebuggerPresent`, `DebugBreak`,
  `SetUnhandledExceptionFilter`, `AddVectoredExceptionHandler`, `CreateToolhelp32Snapshot`).
  **Absent entirely:** every `Nt*` routine, `CheckRemoteDebuggerPresent`, `WriteProcessMemory`,
  `SetThreadContext`.
- **CFG is instrumented but NOT enforced at load** (`GuardFlags 0x100`, no function table,
  `DllCharacteristics` lacks `GUARD_CF`) — friendly for hooking.
- A byte sweep of all 142 MB found **zero** hits for Denuvo, VMProtect, Themida, Arxan, EAC,
  BattlEye, SecuROM, SteamStub. The one `AntiCheat` hit is the stock property name
  `bAntiCheatProtected`.
- **Both executables are `NotSigned`** — nothing for a loaded DLL to invalidate.
- `.noes.dat` is 16 bytes: a BOM plus `¯\_(ツ)_/¯`. It is a **depot file**, and neither executable
  references `noes` in ASCII or UTF-16LE. Inert.

⚠️ **The claim "therefore injection works" is `[hypothesis]` from static evidence alone** — an audit
can prove the absence of *known* protections, not of a bespoke check buried in 98 MB. It is now
corroborated live once: UEVR injected and initialised `[verified-live 2026-09-19, n=1]` (§8d).

⚠️ **The real risk is the future, not the present.** SH2R is publicly reported to have shipped with
Denuvo; **this build has none.** So a later session that suddenly cannot inject should
**check the Steam build ID before blaming its own code** — a patch may have re-added it.

## 9. ⭐ The game's own interaction animations: nobody has tried it (2026-09-26, static)

Both community profiles were read for one question: do they use the game's own push, pull, lever and
crawl animations as first-person arms? **No.** Their animation lists (34 and 37 entries) hold only
combat, struggles, faces and the save point `[measured 2026-09-26]`. jbusfield treats pushing as a
cutscene and switches roomscale off (`main.lua:946-960`); CharlotteLiu **replaces** the interactions
with hand-built physical grabs per object type (`unlock_door.lua`, 5,487 lines: latches, keys, sliding
doors, drawers, the pushable wardrobe, a cart, levers) `[inferred-static 2026-09-26]`.

So Tefa's idea is new, and only the game can say whether it works. The mechanism it needs, copying the
game's pose onto the VR arms while an animation plays, already exists in both profiles for fights
(§8e). The open question is whether the interaction animations look right from James's eyes, since they
were made for a camera behind him. First test, `[FLAT]`: head-bone camera, full body visible, trigger a
wardrobe push, a lever and a crawl gap, and look. Details:
`modding-notes/2026-09-26-both-community-profiles-and-the-game-animation-idea.md`.

The jbusfield archive Tefa supplied is the same version §8e studied (49,718 Lua lines, identical)
`[measured 2026-09-26]`.

## 10. ✅ The bone names are the game's own — checked live (2026-09-28, `/lm`, home PC)

A one-shot UEVR Lua probe (`dev-archive/probes/archive/sh2_bone_dump.lua`, now retired) read James's
real skeleton from the running game, first street scene, no weapon held. Full output:
`dev-archive/recon/2026-09-28-live-bone-dump/sh2_bone_dump.txt`. All `[verified-live 2026-09-28, n=1]`:

- **Mesh:** `JamesSunderland_2022_08_29`, **388 bones**, on `Pawn.Mesh` (`CharacterMesh0`) of
  `SHCharacterPlay_BP_C`.
- **Every name in §8e exists, with the parent chain §8e assumed:** `spine_05_bn → clavicle_01_{r,l}_bn →
  upperarm → lowerarm → hand_{r,l}_bn`; `neck_02_bn → head_bn`; sockets `hand_{l,r}_socket` are real
  bones under the hands.
- ⭐ **The reader predicted, from CharlotteLiu's saved pickers, that `upperarm_r_bn` would be index 56 and
  `upperarm_l_bn` 126. Both are.** So the shipping skeleton is the one both community profiles knew,
  not a stale one. A genuine prediction, made before the dump was read.
- **New and useful:** `ik_hand_root / ik_hand_l / ik_hand_r` (Epic's IK-target bones, under `Root`),
  `parent_cam` and `eye_l / eye_r` under `head_bn` (candidate head-camera anchors), the coat's
  sleeve rig (`{r,l}_wrist_Offset → {r,l}_sleeve_master → six sleeve bones`, plus `{r,l}_jacket_sleeve`)
  and many `*_corrective_*` helper bones.
- **Object graph confirmed:** `Mesh.AnimScriptInstance` = `CH_JamesAnimBP_C`; `WeaponManageCmbSubcomp`
  lives under the pawn's `Combat` component; `Items`, `CharMoveComp` (the `Movement` field),
  `View` and `CameraOverlapHandler` all resolve.
- **Not settled by this run:** `EquippedWeapon` was nil (nothing held yet), so "weapons attach to the
  bone `hand_r_bn`" is still only from the profiles; `Movement.PushableComponent` was nil (nothing
  pushable nearby, or a different path) `[hypothesis]` which.

## 11. Reader findings folded 2026-09-28 (static)

From inbox drops `2026-09-28-reader-charlotteliu-settings-vs-code.md` and
`2026-09-28-reader-code-shape-proposal.md` (full text in git history):

- **CharlotteLiu's mod agrees with jbusfield on every bone name** `[measured 2026-09-28, their files]`;
  `ik_parameters.json` is byte-identical. It built on jbusfield's config. Flashlight rides `hand_l_bn`.
  It runs **Native Stereo** (native-stereo fix, near plane 0.01); jbusfield's carries AFW settings:
  one known-good starting config per supported mode `[reported]`.
- **Interaction blueprints to watch for the game-animation test:** `Pushable_Base_ABP_C`,
  `PushNClimbDesiredSpot_BP_C`, plus the switch, light-switch and item-base blueprints; classes
  `SHItem`, `SHItemWeaponRanged/Melee`, `SHDoorAttachment`, `SHSlidingDoor`, `SHAnimMontagePlayer`.
- **All of its code is its own** (no licence text anywhere): 87 Lua files, 94,159 lines, plus
  `SH2.dll` (weapon put-away block, ammo reads/writes) `[inferred-static, strings only]`. Learn from its
  data and names only; reimplement everything else.
- **Code shape proposal (not yet accepted):** the per-frame arm path only in C++, once per game tick,
  never per eye; `dev-archive/plugin/src/{game,arms,body,bridge,util}`, one `settings` table backed by
  `profile/data/settings.json`, one `game_names.hpp`, probes built only behind a switch, Lua as small
  event glue; 800/1,500-line limits, aim under 400 `[hypothesis]` until a first build.

## 12. ⭐ UEVR runs on OpenXR only (Tefa, 2026-09-28)

**Rule: for Silent Hill 2, UEVR uses the OpenXR runtime, never OpenVR.** The profile's `config.txt`
now says `Frontend_RequestedRuntime=openxr_loader.dll` (backup beside it), and UEVR's
`openxr_loader.dll` was copied into the `UnrealVRMod` folder. On 2026-09-28 the default asked for OpenVR,
which failed (`Hmd Not Found`), and the OpenXR fallback then logged `Could not load openxr_loader.dll`
`[measured 2026-09-28, UEVR log]`. So nothing VR loaded that night. ⚠️ Whether the loader copy fixes
that is **not checked**: the next attach must show `Requested runtime: openxr_loader.dll` and a
created OpenXR session in UEVR's log before anything else is tested.

## 13. Performance: which switches the game has (2026-09-28, static)

Read from the shipping executable with `dev-archive/tools/list_cvars.py` (3,143 setting names found;
filtered list in `dev-archive/recon/2026-09-28-performance-switches/cvars-found.json`). ⚠️ All
`[inferred-static 2026-09-28]`: a name being in the program does **not** prove the game honours it
(shipping builds can lock a setting, and the game's own menu may overwrite it). Only a live
before-and-after in the headset settles each one. **No Silent Hill-specific settings exist**; it is all
stock Unreal 5.1, so standard Unreal knowledge applies.

| What | Settings present | Menu already covers it? | VR verdict to test |
| --- | --- | --- | --- |
| Motion blur | `r.MotionBlurQuality`, `r.DefaultFeature.MotionBlur` | yes (off tonight) | off; sickness risk |
| Depth of field | `r.DepthOfFieldQuality`, `r.DOF.*` | no | off; eyes focus themselves |
| Film grain | `r.FilmGrain`, `r.Tonemapper.GrainQuantization` | no | off |
| Colour fringing | `r.SceneColorFringeQuality` | no | off |
| Lens flares | `r.LensFlareQuality`, `r.DefaultFeature.LensFlare` | yes (on tonight) | off |
| Vignette | inside `r.Tonemapper.Quality` (no own switch) | no | test the tonemapper level; it also carries grain |
| Panini (flat-screen lens) | `r.Upscale.Panini.*` | yes (off) | off; wrong in VR |
| Lumen lighting | `r.DynamicGlobalIlluminationMethod`, `r.Lumen.DiffuseIndirect.Allow`, `r.Lumen.Reflections.Allow`, `r.Lumen.ScreenProbeGather.DownsampleFactor`, `…RadianceCache`, `r.Lumen.HardwareRayTracing`, `r.ReflectionMethod` | only ray tracing on/off | turn DOWN, not off: likely the single biggest cost; off wrecks the look |
| Volumetric fog | `r.VolumetricFog`, `.GridPixelSize`, `.GridSizeZ` | no | KEEP; try coarser grid only. The fog is Silent Hill |
| Shadows | `r.Shadow.Virtual.Enable`, `.DistanceScale`, `.MaxResolution`, `.CSM.MaxCascades`, `r.ContactShadows`, `r.CapsuleShadows` | quality level | shorter distance first |
| Reflections / AO | `r.SSR.Quality`, `r.AmbientOcclusionLevels` | yes (both off tonight) | off; per-eye wrong anyway |
| Bloom | `r.BloomQuality` | no | keep, maybe lower |
| Resolution / upscaling | `r.ScreenPercentage`, `r.NGX.DLSS.Enable`, `r.FidelityFX.FSR.Enabled`, `r.TemporalAA.Upscaler`, `r.AntiAliasingMethod` | yes (DLSS) | keep DLSS; tune percentage per eye |
| Distance detail | `r.ViewDistanceScale`, `r.StaticMeshLODDistanceScale`, `r.Streaming.PoolSize`, `r.Nanite` | partly | small drops only |
| Translucency | `r.SeparateTranslucency*` | yes | test |

**Order to measure (biggest likely win first):** rendering method (Native Stereo vs AFW) → DLSS
level / screen percentage → Lumen settings → shadows → volumetric fog grid → the camera effects (cheap
each, but free wins and better comfort). One change at a time, frame time before and after, headset
connected, UEVR on OpenXR. Then the keepers are set by our C++ plugin, not left as a config file.

## 14. First headset baseline, and the Ultra+ licence (2026-09-28, `/lm`, home PC)

**Baseline:** Blue Creek Apartments, Tefa's settings (everything max except ray tracing, motion blur
and lens flares off, resolution scale 100%, DLSS quality level 3), UEVR Native Stereo, **on OpenVR**
(see below): **37.2 fps average, slowest 1% ~33 fps, every frame over the 72 fps budget** `[measured
2026-09-28, n=1, 70 s]`; GPU 99% busy, ~256 W, 67 °C, no thermal throttling. ⚠️ **37.2 is exactly half
of ~74**, which is what a runtime's half-rate reprojection lock looks like, so the true unlocked rate
lies somewhere between 37 and 72 `[hypothesis]`. The GPU being fully busy says it genuinely could not
hold 72 here. Recorder: `dev-archive/probes/archive/sh2_perf.lua` (UEVR Lua: per-tick frame times,
10 s summaries, plus a command file for changing console settings live); log in
`dev-archive/recon/2026-09-28-performance-switches/sh2_perf_log.txt`.

**⚠️ OpenXR did not take.** The injector writes `Frontend_RequestedRuntime` into the profile's
`config.txt` on every attach from its OWN saved choice (`OpenXRRadio` / `OpenVRRadio` in
`%LOCALAPPDATA%\praydog\UEVRInjector_Path_<hash>\1.0.0.0\user.config`, one folder per injector
location), so editing `config.txt` alone is overwritten `[verified-live 2026-09-28, n=1]`. Fix next
time: set `OpenXRRadio=True`, `OpenVRRadio=False` in that file (the one for
`tools/UEVR-nightly-AFW-beta.6`), with the injector closed.

**Ultra+ (the "Ultra Team" graphics mod, v5.0.4) — ⚠️ read its licence before learning from it.**
It is UE4SS + Lua + a pak, and its `LICENSE.txt` is proprietary: no modifying, reverse engineering or
derivative works, and it names *"CVar manipulation … integration into … third-party modifications
(including … game mods)"* as forbidden, *"regardless of whether … functionally reimplemented or
recreated by reference."* So: **players may use it alongside our mod for themselves (personal use is
allowed), but we must not build our settings from it.** Our tuning comes from Unreal's own documented
settings, measured ourselves in the headset (§13). Its settings list was read only as far as the
main config's option names; no values or code were taken. Whether it runs alongside UEVR is untested.
It was only unpacked into a scratch folder, never installed.

## 15. ⭐ First tuning pass in the headset, OpenXR (2026-09-28, `/lm`, home PC)

Same spot (Blue Creek Apartments), Tefa's max settings, UEVR Native Stereo, **OpenXR** (fixed by Tefa
in the injector), Quest 3 via Virtual Desktop, render size **2688×2880 per eye**. Each step is added ON TOP
of the ones before, held ~30 s; figures are the steady 10 s windows `[measured 2026-09-28, n=1 run
each]`. GPU stayed at 67–70 °C, no throttling. Driver: `dev-archive/tools/sh2_tune_steps.py` +
the `sh2_perf.lua` recorder; raw log `dev-archive/recon/2026-09-28-performance-switches/sh2_perf_log-run2-openxr.txt`.

| Step (cumulative) | avg fps | slowest 1% | gain |
| --- | --- | --- | --- |
| s0 max settings (no ray tracing) | 47.9 | 43.4 | — |
| s1 camera effects off (DOF, grain, fringe, blur, flares) | 49.3 | 44.8 | +1.4 |
| s2 shadows: distance 0.6, max resolution 1024 | 56.8 | 51.0 | **+7.5** |
| s3 fog grid coarser (pixel size 16, depth 64) | 60.2 | 53.7 | +3.4 |
| s4 Lumen screen-probe downsample 32 | 62.2 | 56.5 | +2.0 |
| s5 Lumen reflections off | 65.2 | 59.2 | +3.0 |
| s6 screen-space reflections + ambient occlusion off | 68.5 | 61.0 | +3.3 |
| s7 view distance 0.7 | 67.9 | 60.6 | 0 (noise) |
| s9 `r.ScreenPercentage 75` | 62.0 | 55.5 | **−6**: it overrides DLSS's own lower internal size |
| s10 `r.ScreenPercentage 60` | 72.0 (at the cap) | 66.4 | +4 over s7 |

- **Biggest single win: shadows.** Then fog grid, Lumen reflections, SSR/AO, Lumen downsample.
- **Resolution is the other big lever**, but `r.ScreenPercentage` fights DLSS: 75 was slower than DLSS's
  own setting. UEVR's `OpenXR_ResolutionScale` did NOT change live through `set_mod_value` (needs a
  runtime reinitialise) `[verified-live 2026-09-28, n=1]`. The per-eye size is set by Virtual Desktop's
  resolution choice; that is the cleaner place to lower it.
- **Not yet solid 72:** the slowest 1% was still ~66. ⚠️ **Nothing here has been judged by eye in the
  headset** — Lumen reflections off, coarser fog and 60% resolution may look noticeably worse. That
  judgement is Tefa's and comes before any of this goes into the plugin.
- `r.ViewDistanceScale 0.7` bought nothing in this indoor room; retest outdoors.

### 15b. Walking, the fog, and the flashlight (2026-09-29, Tefa in the headset, same tuned set)

All `[measured 2026-09-29, n=1 each]`, Tefa's own movement and flashlight presses, raw log as above:

- **Walking indoors (Blue Creek): 51–64 fps, slowest 1% 38–50.** Standing still in one view had read 72.
- **Outside in the fog: pinned at the 72 cap, slowest 1% ~66**, flashlight on or off (daylight: it casts
  no shadows, Tefa). ⭐ Tefa: *"outside the graphics look.... simple. fog had no 'foggyness' to it"* — most
  likely our coarser fog grid (s3) `[hypothesis]`. **There is headroom outside to spend on the fog.**
- **Dark room: ~55 fps with the flashlight on, ~60 with it off**, so the flashlight costs ~1.3 ms there;
  the room itself is the larger cost. Three local-light shadow settings changed nothing measurable
  (`r.Shadow.Virtual.ResolutionLodBiasLocal` 1 and 2, `r.Shadow.Virtual.SMRT.RayCountLocal 2` +
  `SamplesPerRayLocal 1`), nor did `r.VolumetricFog.InjectShadowedLightsSeparately 0` — all ~55.
- **The tuned set survives a save load** in these tests: re-applying it in the dark room changed nothing.
  An earlier drop (72 → 61 before the walk) stays unexplained `[hypothesis]`: a menu, or the game
  re-applying its settings.
- **Tefa's direction (2026-09-29):** *"i want my 72fps above all, then graphics, the parts that matter,
  tuned up as much as possible."* → Design for the plugin: **settings per place** (fog detail back up
  outside, cheaper lighting inside), plus a **frame-rate governor** that raises detail while there is
  room and lowers it the moment it slips under 72. Proved possible tonight: every change here was made
  live, mid-game, through console commands.
