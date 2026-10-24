# 2026-10-24 — finger poses ported to C++ (and the finding that the arms had lost them)

/pd, home PC. The board row said: "Forearm hands: finger poses on grip/trigger (`hands_animation.lua`), hide on pause.
Low priority while Tefa uses IK Arms." Reading `ik.lua` to port it showed the priority was wrong.

## The finding

`ik.lua` line 755 (`Rig:initHandAnimations`) calls `handsAnimation.createAnimationHandler(animID, component, ...)` on
the IK arm mesh for both hands (`left_arms`, `right_arms`, from the first profile in `hands_parameters.json`). So the
finger poses were never only for the forearm hands: **the IK arms Tefa plays with had them too**, driven by the same
`hands_animation.lua` (grip and trigger curl from the controller, thumb on the rest, a weapon's own grip pose while it is
held). When the C++ IK took over on 2026-10-24 00:30 and `ik.lua` stood down, nothing called `createAnimationHandler`
any more, and `hands_animation.lua` does nothing while it has no handler. **Since that build the arms have had no finger
poses** `[inferred-static 2026-10-24]` (not seen in the headset: the gun in the right hand may have hidden it; the left
hand would have shown it).

## The port

New `src/fingers.cpp` / `fingers.h`: `hands_animation.lua` plus the pose half of `animation.lua` (`animate`,
`lerpAnimation`, `pose`, `updateAnimation`, `resetAnimation`, the lerp from `libs/core/lerp.lua`), the Lua names kept in
comments. Reads `hands_parameters.json` → `animations` (first entry, "Shared"): `poses` (lists of `[position, on|off]`)
and `positions` (per bone `[pitch, yaw, roll]` for `off` and `on`). A finger change lerps 0 → 1 over 0.1 s with
`KismetMathLibrary.RLerp` per bone, written through `anim::set_local_rotator` (the already-ported
`setBoneSpaceLocalRotator`). Input comes from the plugin's `on_xinput_get_state` (trigger > 100, shoulder button,
thumbrest action `/actions/default/in/ThumbrestTouch<Side>` on that side's joystick source), as the Lua read it.

Both kinds of hands register: `hands.cpp` adds each hand's `AnimationID` from the profile; `ik.cpp` adds both IDs on its
one mesh (the Lua gave the first unused mesh to the first profile key). `remove` on destroy.

**Which grip pose a hand holds still comes from the Lua side** (the attachments / accessories modules are not ported):
`sh2vr_bridge.lua` now sends `gripL=` / `gripR=` (`-` none, `+` the default grip, else the grip animation's name such as
`pistol`) and `mont=` (how many hand montages have ended), read from a 3-line getter added to `hands_animation.lua`
(`getEffectiveAttachment`, the accessory override else the attachment's grip animation, exactly what
`updateAnimationState` used). The C++ re-poses a hand when its word changes or the montage count moves. One change from
the Lua, on purpose: a hand takes its pose the moment it registers (the Lua waited for the next grip change).

`[compile-verified 2026-10-24]`, installed with `build.sh --deploy` beside the fire doors. `hands_animation.lua` joined
the PATCHED list in `build.sh`. **Unrun.**

## What the next run tells

- Heartbeat line: `finger poses on 2 hand(s)` once the arms exist (0 means the profile has no AnimationIDs or the
  parameters file did not load).
- `fingers: left_arms poses on ...` / `right_arms ...` once each.
- Bridge line now reads `... gripL=-;gripR=pistol;mont=0` (the right hand holding the pistol).
- In the headset: fingers open with no weapon, curl on the trigger and the shoulder button, the thumb drops on the
  rest; with the pistol out the right hand takes `grip_right_weapon_pistol` and the trigger finger follows the trigger.
- Lost finger poses after a cutscene would mean the montage counter is not moving (the `on_module_montage_change`
  callback name or signature differs); the log line `bridge: the Lua side is talking` shows the raw string.

## Not established

- Whether the IK solve and the finger writes fight: the Lua wrote fingers at input time and solved arms at tick, as the
  C++ does now, so the order is the same; unproven here.
- The thumbrest action: if UEVR has no such action bound for the Quest controllers, the thumb simply never drops (the
  Lua had the same dependency).
