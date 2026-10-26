# Gold Digger: the two Ghost Hand wears side by side

From: `/gd`, 2026-10-26 (shelf `hands`, items `begone-ghost-hand_2026-10-24_16-57-54` and `_2026-10-25_18-28-04`).

The board knows the 2026-10-25 18:28 wear (slide 1 of 1, game hand jumped 30 times while the controller was calm).
The 2026-10-24 wear is not on the board. Side by side:

| | 2026-10-24 (630 s) | 2026-10-25 (502 s) |
| --- | --- | --- |
| game hand jumped while controller calm | 4 | 30 |
| a controller jumped (tracking glitches) | 2 | 87 |
| tracking lost (of which hands stacked) | 2 (2) | 6 (4) |
| left hand usual gap / wander | 13.3 cm / 0.0 cm | 13.3 cm / 1.5 cm |
| right hand usual gap / wander | 9.3 cm / 0.0 cm | 9.3 cm / 0.0 cm |
| frames a second | 30.7 | 35.8 |

`[measured 2026-10-24/25, n=2 wears]`

- The **controller itself** glitched 87 times on the 25th against 2 on the 24th. A jump of the real controller is the
  tracking (room light, a hand behind the other), not the mod. Read the 30 game-hand jumps against that before
  tuning: some of them may just follow a controller glitch.
- The **usual gap** is identical to the millimetre in both wears (13.3 cm left, 9.3 cm right): a fixed offset between
  the game hand and the controller, most likely the grip offsets in `settings.h`, not drift. Worth checking it is the
  offset you meant when the fit is judged.

No new row; it belongs to the existing `[VR USER]` "hand reload distances and fit" row.
