# 2026-11-05b: the left hand on the long guns' front with the game's own grip, and the stiff arms

Home PC, Opus, the OpenXR Simulator (Tefa: "yes, go ahead" to launching for this job). sh2vr.dll e0722d0c installed,
backup `D:/SH2 builds/_backup-2026-11-05-1621-before-sh2vr-deploy` (and `_backup-2026-11-05-1604-...`, which holds the
intermediate build, not 0.1.0's; 0.1.0's plugin is in its release folder). Not worn.

Tefa: *"the only way i can see the hand firmly gripping the rifle and shotgun front handle, is to use the games own grip
independant of the wrist - which also needs IK fixing as elbows bend weird"*, then: *"wrists don't bend with hand
movement and upper and forearm don't follow the motion controller like a hand would bend, but it's not like broken
wrong, more like really rigid and wooden"*.

## Found (read live with the UEVR MCP)

- **The guns carry the game's own left-hand spot.** The shotgun mesh's bones: Holder, bone_root, bone_trigger, FX_muzzle,
  **Holder_L**, FX_Chamber, **bone_forearm** (the pump), bone_chamber, bone_magazine, bone_shell_02, bone_shell_01. The
  game's left-hand IK puts the body's hand bone exactly on Holder_L: 5.7 26.1 -1.4 cm from the gun's frame for the socket,
  5.8 26.2 -1.5 for the hand, the same turn to 0.001 `[measured 2026-11-05, n=2]`.
- The weapon actor's root stays on the hidden body at `hand_r_socket`; the visible mesh rides OUR arm mesh's `hand_r_bn`
  (the pack's offset).
- The hidden body is no reliable source while playing: the MCP's held aim never made the game aim (IsInRangedAim false;
  our button layer and the pack keep the aim stance away), and its left hand wandered 20-30 cm off the gun in most
  readings. `SHDebug_Character_Combat_ForceRangedAim` does nothing in this build.
- **The arms never used the real twist bones.** Each arm has `lowerarm_twist_02_{l,r}_bn` (child of the lower arm) and
  `upperarm_twist_02_{l,r}_bn_Offset` / `bn1` / `bn2` under the upper arm. The pack's IK turned only `r_sleeve_master`,
  `r_wrist_Offset` (all weights 0) and `l_wrist_Offset` (no side bend), and only the left arm let the hand's turn move the
  elbow. That fits "rigid and wooden" `[inferred-static]`.

## Built

- `frontgrip.cpp` (new): while two-handed (twohand.cpp, and the locked rifle in rhbolt.cpp) the drawn left HAND BONE is
  placed on the gun's own Holder_L, read live each view, so it also rides the pump; the shotgun's slide-by-hand still moves
  it back along the barrel. New `ik::override_left_bone`: the hand bone straight at that place and turn, no controller
  offset; the elbow still reads the real controller. Proof line: `twohand: status: the drawn left hand is N cm from the
  game's grip spot`.
- Fingers: F2 captures the game's own finger pose when the body's hand is ON Holder_L (refused otherwise); none is saved
  yet (the bad captures moved to `D:/SH2 builds/sh2vr_front_grips.json.bad-captures-2026-11-05`). Until one is, the left
  fingers hold the pack's poses `grip_left_weapon_shotgun` / `grip_left_weapon_rifle_offhand` while two-handed.
- Arms (`data/ik_parameters.json`, now deployed by build.sh; old file `D:/SH2 builds/ik_parameters.json.before-2026-11-05-arms`):
  both forearms get `lowerarm_twist_02_*_bn` at half the wrist's twist; both wrist helpers follow 40 % of the hand's bend
  both ways (pitch and yaw); both arms let the hand's turn swing the elbow, at 0.5 (left was 0.35, right off). First
  guesses, to judge in the headset `[hypothesis]`.

## Proved

- `[verified-live 2026-11-05, simulator, n=1]`: two hands taken on the shotgun (22 cm, 23 deg off the barrel), and the drawn
  left hand **0.0 cm from the game's grip spot**. The picture was too dark to judge by eye.
- A later run with the finger fallback: no fault, frame time unchanged (about 22 ms). The two-hand press was refused there
  (60 deg off the barrel), and the drawn left hand stayed 20 cm from the right however the simulated controller moved, so
  the arms were not following the simulator in that run `[measured 2026-11-05, n=1]`; cause unknown, not ours as far as
  seen (the same build had worked 20 minutes before).

## To wear

Shotgun and rifle out, the left grip on the front: does the hand sit firmly on the pump / the rifle's front and stay there
whatever the wrist does, do the fingers close round it, does it ride the pump? Arms: do the wrists and forearms bend with the
hand now, and do the elbows move more like a real arm? Each new setting is a number in `ik_parameters.json`.
