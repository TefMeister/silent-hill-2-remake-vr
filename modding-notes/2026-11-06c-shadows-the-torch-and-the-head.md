# Shadows: why the torch never lit the gun, and the head back in the player's shadow (2026-11-06 20:55-21:15)

Home PC, Fable, the Quest 3 connected through Virtual Desktop with nobody wearing it (Tefa: "i got the headset
connected for the shadow work if you can do it without me wearing it"). The game was driven and read through the
UEVR connector; every change below was made live and read back; the pictures are the desktop view of one eye.
Board row (Tefa 2026-10-23): *"weapons must cast shadows when the flashlight shines on them (they do not now); fix
the player's shadow if at all possible, with the head showing and the hands looking right."*

## 1. The torch and the weapons: lighting channels `[verified-live 2026-11-06, headset runtime, n=1]`

| what | lighting channels | shadow flags |
| --- | --- | --- |
| the flashlight's main spot light (`Flashlight_BP_C.Light`, 67 cd-ish, cone 16/30, 14 m) | **0 and 2** | casts dynamic shadows |
| the flashlight's fill lights (BackLight rect, CloseLight point, VeryCloseSpotlight) | 1 / 0 / 1 | intensity 0 at the time |
| James's body (`CharacterMesh0`, 388 bones, 23 material slots) | **1 only** | casts, not hidden-shadow |
| the flashlight's own mesh | 1 only | |
| the pistol's mesh (child of OUR arm mesh's `hand_r_bn`) | **1 only** | casts dynamic shadows |
| our poseable arm mesh | **0 only** | casts dynamic shadows |

So the game keeps James and everything he holds on channel 1 and lights him with his own fill lights, while the
torch lights the world on channels 0 and 2: by design the torch never touches the player or his gun. In VR the gun
is in a hand the torch does light (our arms are channel 0), so the gun looked unlit and cast nothing. Not a
shadow setting at all: a channel.

**The fix:** the weapon mesh gets channel 0 added (`SetLightingChannels(true, true, false)`; 1 kept so the fill
lights still reach it). Measured on the desk, torch on, cone opened to 85 degrees so it reached the pistol at the
edge of view: the pistol's corner of the picture 29.5 against 27.6 with the channel off, the rest of the picture
identical (33.2 / 33.2) `[measured 2026-11-06, n=1 each way, repeated once: 29.6 / 27.5]`. A small number because
the cone barely reached it; the shadow itself needs the torch pointed at the gun, which is Tefa's look.

Also learned: F toggles the torch (whole picture 33.2 on, 26.9 off) and the torch WAS on; the pack's "hidden arms
still block the flashlight" comment is about its own arm mesh being on channel 0, which is right for VR hands.

## 2. The head in the player's shadow `[verified-live 2026-11-06, n=1]`

The pack hides the head by collapsing `neck_02_bn` (`HideBoneByName`), which removes the head from every shadow:
the headless shadow. First try: a second skeletal mesh on the pawn following the body's pose
(`SetLeaderPoseComponent`), hidden from the camera, `bCastHiddenShadow`, channels 0 and 1, the arm bones hidden on
it. It followed the pose exactly (neck and hand positions identical to the body's), but `IsBoneHiddenByName` said
its neck was hidden too: **a follower shares the leader's bone visibility**, so the head cannot be hidden on the
body and shown on the copy by bones.

What worked: hide the head by MATERIAL instead. The game ships `MI_Invisible` and already uses it on three of
James's own slots. The head is 15 of the body's 23 slots (lacrimal, teeth, eye shadow, face skin, facial hair,
eyes, eight hair slots, eyebrows; the hands are "Skin_Hands", the clothes Shirt / Jacket / Pants). Set those to
`MI_Invisible`, show the neck bone again, switch the body's own `CastShadow` off; the copy (its own materials are
the asset's real ones) casts the complete shadow, head included, and the arms stay out of it because the pack still
hides the arm bones on the body and the copy follows. The camera picture afterwards showed no trace of a head
(the camera sits inside it) `[verified-live, n=1]`. The body's visible torso and legs are untouched.

## 3. What is built (sh2vr.dll 875fcee4, deployed, not yet run by the plugin itself)

`shadows.cpp`, once a second: the equipped weapon's mesh onto channel 0 (once per weapon); the body's head slots
found by their material names (`Lacrimal`, `Teeth`, `Eye`, `Face`, `Hair`) and set invisible, the neck bone shown,
the body's shadow off, re-applied whenever the game or the pack undoes it; the shadow body made once per pawn.
Settings `SHADOWS_*`. Log lines: "the WeaponPistol_BP_C's mesh put on the torch's lighting channel (channels 010 ->
110)", "the body's head is 15 of 23 material slots (...)", "body fix 1: 17 thing(s) put right", "the shadow body
made on the pawn: ... hidden 1, casts hidden shadow 1, channels 110".

## Not established

- That the shadow body's shadow is actually drawn with the head: the copy reported `bRecentlyRendered` true and
  the bone positions match, but the shadow itself was out of the desk picture's view. Tefa's look: in the headset,
  find the player's shadow on a floor or wall under a room light and look for the head; point the torch at a wall
  with the gun in front of it and look for the gun's shadow.
- Whether the game resets the body's materials on damage or a level change (the plugin re-applies within a second).
- Whether our arms should also get channel 1 so James's own fill lights reach the VR hands ("the hands looking
  right" in LIGHT, not shadow): not changed; a one-line setting if Tefa finds the hands too dark.
- The fill lights read intensity 0 while the torch was on; not followed up.

## Driving notes (the headset idle on the desk)

- UEVR attached at the warning screen came up fine (ten "Failed to initialize Framework" lines first, then the
  plugin alive at 31 ms/frame, OpenXR, 2688x2880); attached during the splash it hung and the game died (the
  21:4x KNOWN line). Enter by PostMessage passes the warning and the "press any button" in VR; `ContinueGame` on
  the main menu widget loads the save; `1` takes the pistol out; `F` toggles the torch.
- State-o-matiC said "frozen: the window does not answer Windows" while the plugin's frame counter ran on: in VR
  the game's window stops answering, so that verdict is a false alarm there (tool idea for the dev PC).
