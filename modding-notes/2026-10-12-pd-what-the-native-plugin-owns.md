# 2026-10-12 (dev PC, `/pd`, no game launch): what the native plugin owns, and how the game's own animations survive

**The game was not launched. This is a design decision made from the dossier (§8e–§11) and UEVR's public plugin
header (`include/uevr/API.h`, praydog, MIT, read 2026-10-12, plugin API 2.39).** Settles the board's two linked
`[PD]` rows: "decide what the NATIVE plugin owns" and "accept or change the code-shape proposal".

## The decision

**The C++ plugin owns everything that runs every frame. Lua owns nothing that runs every frame.**

Per game tick (`on_post_engine_tick`, once per tick, never per eye — §8h):

1. **The game keeps animating James as it always does.** His real mesh (`Pawn.Mesh`, `CH_JamesAnimBP_C`) is not
   touched: the game's animation graph runs, his shadow stays right, his weapons and items stay attached as the
   game expects.
2. **A separate arms mesh** (a `PoseableMeshComponent` made once from James's mesh, everything but the arms hidden
   once at creation with `HideBoneByName` on the branch bones) **copies the real mesh's finished pose** with ONE call,
   `CopyPoseFromSkeletalComponent`. That single call is what brings the game's own interaction animations across:
   push, pull, lever, crawl, reload, everything.
3. **Our own two-bone IK, in C++,** puts the hands on the controllers (`get_grip_pose` from the UEVR VR table):
   shoulder → elbow → wrist rotations for about eight bones, written with `SetBoneRotationByName`. No Kismet maths,
   no `K2_TwoBoneIK` call: the maths runs in registers.
4. **A blend weight per arm, 0 to 1:** 1 in ordinary play (hands where your hands are), eased down to 0 while the
   game plays one of its own interaction animations (the copied pose shows through), and eased back up afterwards.
   **That is Tefa's "use the game's own animations, then hand the arms back smoothly" (2026-09-26) as one number.**

**So the answer to the sharpest question ("can the game's animation graph keep running with IK on top?") is yes,
by construction, without hooking anything:** the graph runs on the real mesh; we copy and adjust a second mesh.
`[inferred-static 2026-10-12]` — built only from engine calls the community profiles already use successfully
(`CopyPoseFromSkeletalComponent`, `SetBoneRotationByName`, `HideBoneByName`; §8e).

### Why this is not the community profile's design again

They used the same pieces, but (§8e): every bone of a whole clothed body was walked per frame to collapse it,
~30 reflection calls per arm per frame went into Kismet maths, and the pose copy was only a FALLBACK for montages,
so interaction animations never reached the arms. Here: the body is hidden once, not per frame; the maths is native;
the copy happens every tick, so the game's animations are always the base layer and IK is the adjustment.
Per tick that is one copy plus about sixteen bone writes, all from C++.

### What Lua is left with

- **Development probes** (quick to change while the game runs, as `sh2_bone_dump.lua` was), each behind a switch
  and archived once answered (estate rule).
- **Nothing per frame, and nothing a player needs.** Event-shaped things (cutscene start/end, an interaction
  starting, a weapon equipped) go native too: UEVR's `hook_ptr` takes a pre/post hook on any UFunction, so the
  plugin can hear `SHCharacterStatics::IsCharacterInCutscene`-type events itself. Lua would only stay for an event
  whose UFunction we cannot find, and only until we find it.

### Fallback if the copy route disappoints

If the copy is a frame late (it reads the source's pose as of that moment; post-engine-tick should be after the
animation tasks finish `[hypothesis]`) or costs too much: hook the real mesh's post-animation step and write the arm
bones into its own transforms (no second mesh). More fragile (engine internals, not public calls), so second.

## The code shape (the reader's 2026-09-28 proposal): ACCEPTED, with the arms folder made concrete

`dev-archive/plugin/src/`:
- `game/` — `game_names.hpp` (every class, bone and property name in one place) and the object-graph walks (§8e,
  §10: `Pawn.Mesh`, `Combat.WeaponManageCmbSubcomp.EquippedWeapon`, `Items`, `View`...).
- `arms/` — `arms_mesh` (make the poseable mesh, hide the rest once), `pose_copy`, `ik_two_bone` (pure maths, no
  UEVR, unit-tested numerically like every other project's maths), `blend` (the per-arm weight and its easing).
- `body/` — what the real mesh shows to the player's eye (head hidden for the camera, §8e).
- `bridge/` — the only files that call the UEVR API (callbacks, VR poses, `call_function`, `hook_ptr`).
- `util/` — log, settings.
One settings table (`profile/data/settings.json` read once; names for every number); probes only behind a switch;
800/1,500-line limits.

## Not established (and the check for each)

- **Weapons:** the game attaches a held weapon to `hand_r_bn` of the REAL mesh (§8e, from the profiles; not seen
  live, §10). With the real arms out of the player's view the gun would float where the game's hand is. Either
  re-attach the weapon to the arms mesh's `hand_r_bn` once per equip (`K2_AttachTo`) or move it each tick; the first
  live session with a gun decides. `[hypothesis]`
- **Shadows and the coat:** the real mesh keeps casting James's shadow (good); the arms mesh's coat sleeves are
  copied, not simulated — whether they look stiff is a headset judgement.
- **Timing:** one frame of lag between the game's pose and the copy would show as the arms trailing in an
  interaction animation; the jitter recorder (`jitterlog.h`, estate rule) logs pose read and bone write per tick.
- **AFW vs Native Stereo:** both supported (§8h); the plugin is per tick, so the same in both `[hypothesis]`.

## What this unblocks (new board rows)

- `[PD]` the two-bone IK solver in C++ with a numerical self-test (no game needed).
- `[PD]` the plugin skeleton: UEVR's MIT headers vendored, the `on_post_engine_tick` callback, log, settings,
  jitter recorder wired, builds as a UEVR plugin DLL.
- `[FLAT]` first load: the plugin logs ticks inside the game, makes the arms mesh, copies the pose with IK weight 0:
  the arms should move exactly as James's do, seen from his eyes.
