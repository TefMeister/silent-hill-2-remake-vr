# 2026-11-19: the long gun that curled up by itself, and a hand carry that no longer jumps (b033)

/lm, Developer PC, Fable, the OpenXR Simulator with VR Bench playing the hands, the UEVR connector in, DLSS on
(Balanced, as Tefa left it), the frame warp on. Tefa away: nothing here is worn.

## 1. What Tefa saw

Headset, b031, 2026-11-19: *"when i grab a two handed gun it just starts moving on its own like curving upwards and
stays in that position in my right hand"*. And on b032: the hands and gun *"every now and then just jittery, not all
the time"*.

## 2. The curl: found, measured, cured

**Tefa's own log already ruled out the obvious.** In the wear's log the status line "the gun's start line is N deg off
the left hand" stays at 0.1 to 2.8 degrees: where the barrel POINTS followed the left hand the whole time
`[measured, the b031 wear log, 12 status lines]`. So the fault had to be in a turn nobody measured.

**The cause** (the reader's static reading, then proven live). `twohand::on_view` runs at every view. It read the gun
where the LAST view had left it, then made two shortest-way turns about the right hand's grip: one so the hands' line
points at the left hand, one (the "drawn gun" step of 2026-11-06) so the gun's front grip spot meets the left hand as
drawn. Each is aimed at a target, so the pointing never drifts. But two shortest-way turns about different axes leave
a small turn about the hands' line behind, and nothing ever took it out: the next view started from the turned gun.
The header comment said "never adds to our own last write"; that was true of the direction only.

**Measured on the simulator, b032** `[verified-live 2026-11-19, n=1]`: shotgun two-handed, the left hand going round
a 6 cm circle twice every four seconds, the gun's turn against the hand it hangs on (the mesh's RelativeRotation, read
through the connector at the same point of each circle): +1.2, +2.5, +3.5 ... **+11.9 degrees after 20 s, about 0.6
degrees a second, never stopping.** Hands held still: no creep (the drawn hand sat 0.0 cm from the grip spot). Letting
go of the left grip put the gun back exactly (the saved place in the hand was restored every time).

**The cure (b033).** Each view first puts the gun back in its own place in the right hand (the relative transform
read at the grab, the same one the let-go already restored), reads where that is in the world, and makes the two turns
from there. The result depends on this view's hands alone. Switch: `settings.h` `TWO_HAND_FROM_ITS_PLACE` (false = the
old way, for an A/B). One game call more per view.

**Measured on the simulator, b033** `[verified-live 2026-11-19, n=1]`: the same circling for 30 s: the reading is the
same at every sample (pitch +0.1, yaw -16.1, roll +5.9 against the gun in one hand: the two-hand aim itself), no
creep. The picture: the shotgun forward, the left hand on the pump.

**New proof line in the log**, every 3 s while two-handed: "the gun turned out of its place in the right hand: mean N
deg, max N (follows the hands; must not creep)". It read 15 to 16 degrees mean during the circling: the drawn right
hand does not take the whole turn with the gun. Not new, never measured before, not looked into
`[measured, n=1; cause not known]`.

**NOT established:** that this is all of what Tefa saw. The simulator's hands move in a way a person's do not; the
creep needs the hands to keep moving, and "stays in that position" fits a creep that stops when the hands stop. Tefa's
eyes on b033 decide (`[ID SH2-002]`).

## 3. The jitter now and then: one lead closed in code

A made-up picture's hands are carried by the two poses the arm solve drew (b030 to b032). When that carry was
refused (no record for one of the two frames, a turn or step over the limit, or more than 5 cm from the raw
controller's) the hand fell back to the RAW controller's move for that one picture, which is not where the smoothed,
drawn hand went: a one-picture jump `[hypothesis]`. In b033 a refused carry repeats that hand's last good carry (the
hand goes on as it went) for at most 3 made-up pictures in a row; after that the raw controller as before.

- The log now counts refusals by reason: "handpaste: N hand carries refused (no record .., no carry between the poses
  .., turned too far .., moved too far .., too far from the raw controller's ..); N kept the last good carry, N fell
  back to the raw controller".
- Live A/B for the headset: the 7th number in `sh2vr_warpscale.txt`: 1 = the new way (installed), 2 = b032's way,
  0 = b029's raw carry.
- Simulator, 30 s of circling: 1 refusal (turned too far), kept `[measured, n=1]`. Whether this is the jitter Tefa
  sees is NOT known: `[ID SH2-001]`.

## 4. DLSS Balanced

The game's settings file holds DLSS Balanced now (Tefa's menu change of 00:20 was saved before the game closed).
Started that way with the warp on the game ran normally on the simulator, 3 runs into play, 60 to 80 pictures a second
in the armory `[verified-live 2026-11-19, simulator, n=3]`. So the close Tefa hit is the LIVE change in the menu (DLSS
torn down and rebuilt under the warp), not Balanced itself. The live change was not repeated here (it needs the
settings menu driven by hand). Nothing was changed back: the mod's launcher keeps the player's own DLSS level.

## 5. The run, and what it taught the tools

- Five starts: two died about 20 s in with the known occasional start crash (access violation in the game's own code,
  `Scene state passed to FSceneView constructor is null` as the injector's last line; once on b032, once on b033,
  our file not in the stack) `[measured, 2 of 5 today; 1 of 5 on 2026-11-18]`. Logs kept in the SH2 logs folder.
- **A whole run was lost to a black screen that was "PRESS ANY BUTTON TO PLAY" never passing.** With VR Bench
  switched on, the hand file the last game's run left behind is still obeyed, and it swallowed the simulator's own
  trigger pull. The game then refuses every weapon ("the game refused the draw") and the eyes stay black with a small
  mark on a dark panel. `toplay.sh` now takes that file away first. (KNOWN.md.)
- New tools in `dev-archive/tools/`: `saveswap.py` (start an unattended run from a chosen save, put every save back,
  proven by hash; done by hand twice before), `sim/draw.py` (a holster draw with VR Bench), `sim/press.py` (one VR
  button), `sim/twohand_hold.py` (a long gun held in two hands: still, circling, turning, waving),
  `sim/gun_in_hand.py` (reads the gun's turn in the hand through the connector: the number that caught the creep).
- The main menu has a LOAD GAME line: a cleaner way to pick a save than the file swap, not recorded as a route yet.
- A second `build.sh --deploy` from this clone (CRLF check-out) replaced no script: the line-ending fix of 2026-11-18
  holds `[verified-live 2026-11-19, n=1]`.

## 6. Installed

b033 of the builds history (`Two hands/v0.1.0-b033`), sh2vr.dll sha256 d9bcffb6..., stamped. Roll back:
`builds.py restore silent-hill-2-remake-vr 32`. Deploy backup: `_backup-2026-11-19-1135-before-sh2vr-deploy`.
