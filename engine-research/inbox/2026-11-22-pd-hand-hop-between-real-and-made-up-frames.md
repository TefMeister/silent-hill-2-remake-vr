# The hand that hops against the wall on alternate frames: every way the carried hand can disagree, and what the b038 log says (2026-11-22, the reader, files only)

From: the background reader of the 2026-11-22 live session. Nothing launched. Read: `handpaste.cpp`, `drawnpose.cpp`,
`carry_math.h`, `smooth.cpp`, `smooth_math.h`, `warpscale.cpp`, `ik.cpp`, `ghosthand.cpp`, note 2026-11-19, and the
b038 wear log (`2026-11-22-1638-tefa-wear-b038/uevr-log.txt`, 16:38-16:55, 101 status windows of 10 s).
The file in force: `sh2vr_warpscale.txt` = `1.0 0 0 1 1.0 0.004 1 0` = scale 1, no mode switching, the hands' pass
on, near line 1 m, "moving against the room" 0.004, **7th = 1** (carry by the hand the game drew; a refused carry
repeats the last good one), **8th = 0** (the hands of the view's own record). The log's start lines confirm all of it
`[measured]`.

The measure this answers: the gun hand hops back and forth against the wall by about 6 px of a 1440-wide picture
(about 12 at full size) on alternate frames while moving; Ghost Hand: "the game's hand jumped 563 times while the
controller was calm; left hand wanders 3.2 cm".

## 1. How a made-up picture gets its hand (as the code stands)

Call N draws eye E for real and makes up eye X. The hand pixels of X's last real picture (tick N-1) are carried by
the hand's own rigid move from the record of tick N-1 to the record of tick N, between X's real view then and X's
target view now. A whole step, never half; both records hold the hand AFTER smoothing (`ik.cpp`: the record is
written after `smooth::apply`). So in one eye the hand should read: real N-1, carried to N, real N+1.

## 2. The ways it can disagree, with the log's verdict

| # | way | what the log says |
| --- | --- | --- |
| 1 | **The hand is right and the WALL is one tick old.** The carry puts the hand where it is at tick N; if the warp leaves the wall where it was at N-1 (the 1 m rule for near things, or the fault of the other inbox file), the hand hops against the wall by one tick of wall motion on every made-up frame, only while James moves. | Not something the log can see. Fits "while moving". Size by arithmetic: a sideways walk of 1.3 m a second, a wall 1.5 to 2.5 m away = 11 to 19 px a tick at full size. **Leading explanation** `[hypothesis]`. |
| 2 | A carry refused (no record, turned or moved too far, over 5 cm from the raw controller's) = a one-picture jump. | In play 0 to 16 refusals in about 1,440 carries per 10 s, typically under 5, and every one kept the last good carry (none fell back to the raw controller) `[measured]`. Far too rare for a steady alternate-frame hop. **Ruled out as the steady hop.** |
| 3 | No record for the picture, or the wrong one. | Every view matched in every window of play; unmatched only around the five loads (33-35 each) `[measured]`. **Ruled out.** |
| 4 | The carry is by the raw controller, not the drawn hand (7th number 0, or a silent fallback). | 1,420-1,440 of 1,440 carries a window "by the drawn hand" `[measured]`. The 7th number is in force. **Ruled out.** |
| 5 | The record is not the pose that was really drawn (so the "right" pair is the wrong pair). | Not seen by the log. The log's yardstick: the raw controller's carry differed from the drawn hand's by 1 to 7 mm on average per picture (0.5 mm in the calmest windows, up to the 50 mm cap) `[measured]`. 2 to 5 mm at arm's length is 6 to 15 px at full size: the same size as the hop. So IF the picture really follows the raw controller and not the record, this is the hop. Decided by one live flip (section 4, step 2). `[hypothesis]` |
| 6 | Smoothing on one end and not the other. | Not in the code: both records are after smoothing, the left hand is never smoothed at either end `[inferred-static]`. The `smooth:` lines (35 of them, strength 4, "turn per frame raw / drawn") read 0.5 to 1.4: no evidence either way, because "held still" there means under 20 degrees a second, which includes slow steady moves a filter passes unchanged `[measured, not diagnostic]`. |
| 7 | The record's tick is one off the picture's (the 8th number's subject). | Harmless at a steady speed: a constant offset cancels in the step between two records. It only shows under a change of speed, about 2 px for hand tremor or for James starting to walk (arithmetic). Cannot make 12 px. **Not the hop** `[inferred-static]`. The 8th number stays a test switch only. |
| 8 | A sign or convention error in the carry. | Would give an error that grows with walking speed and with every hand move, and the raw-against-drawn yardstick would sit far above 1-7 mm. `tests/carry_test.cpp` agrees with Unreal's own matrices within 0.04 mm. **Unlikely** `[inferred-static]`. |
| 9 | Half-step against full-step. | No half step exists in the code `[inferred-static]`. |
| 10 | The pixel is not recognised as hand in that picture, so the warp's own (one tick old, turn-only) hand shows. A hand pixel counts only when its motion over two ticks differs from a still thing's by more than 0.004 of the picture (about 5 px). | By definition this can only cost up to that bar: about 3 px a tick at full size. A shimmer at slow moves, not 12 px `[inferred-static]`. |
| 11 | The warp's leftover copy of the hand is not where our clean-up looks. `handpaste.cpp` `source_of()` assumes the warp lands EVERY near pixel by the turn alone. The warp's programs (read today, see the other file) do that only for near pixels it marked as moving; near pixels it marked still are landed with the camera's move as well. While walking, a hand the warp calls "still" is then left somewhere else (tens of px away) and not painted over. | Not in the log. Would look like a second faint hand or gun on alternate frames ("trails", "not solid"), not a hop of the one hand. The tint (4th number 2) shows it: a second gun with no red on it. `[hypothesis]` |
| 12 | The search for the source pixel stops within 0.002 of the picture (about 5 px) instead of exactly. | A few px of wobble at depth edges at worst `[inferred-static]`. |
| 13 | A long gun held in two hands turns about the grip toward the left hand at view time; the record holds the right HAND, not the gun (note 2026-11-19: the gun sits 15-16 degrees out of the drawn right hand while two-handed). The gun's pixels are carried by a blend of the two hands' moves. | Tefa carried the shotgun in this run (the log's weapon lines, 16:40 and 16:44). Affects the gun's far end, not a hand against a wall `[inferred-static]`. |

## 3. The Ghost Hand line is not about made-up frames

`ghosthand.cpp` writes, once a game tick, the real controllers (room) and the game's hand BONES (world, turned into
the room). It never sees a made-up picture. So "the game's hand jumped 563 times while the controller was calm" is
about the hands as the game itself draws them. Three things in the code produce such jumps with nothing wrong
`[inferred-static]`:

- the right hand is smoothed at strength 4 while a gun is held: it keeps moving after the controller has stopped;
- the left hand's bone is put on the gun's front grip while two-handed (`frontgrip.cpp`), away from its controller;
- the world-to-room turn is only refreshed while the hands are more than 15 cm apart, and it pairs the controller
  components (the pose of the previous view) with the headset runtime's pose of now: a stick turn or a walk with the
  hands together makes the game hand appear to jump in the room.

"Left hand wanders 3.2 cm" fits the second. The reader could not open the 16:38 report through the sanctioned tool
(the run name was not found; "latest" is a later, 114 s session with 4 jumps) and did not read the raw recording
(locked). To make the count mean something: `begone.py ask ghost-hand <that run> ...` for which hand jumps, and
whether the jumps sit in stretches where James walks or turns by stick.

## 4. What settles it, cheapest first

1. **From the video, no game:** measure the hand against the picture's own edge instead of against the wall, in a
   stretch where James walks with the head fairly still. Steady in the picture while the wall steps = way 1 (it is
   the world, the other inbox file). Hopping in the picture = the carry (ways 5, 11).
2. **Live, 20 s each, the 7th number:** `... 1 0` (now), `... 0 0` (raw controller's carry), `... 2 0` (b032's way).
   The hop smaller with 0 = the record is not what is drawn (way 5). Larger with 0 = the record is right.
3. **Live, the 4th number 2 (tint) while walking:** green = carried hand, red = painted-over leftover, magenta = no
   background found. A second gun or hand with no tint on it = way 11.
4. **Live, the 4th number 0 for 20 s:** our pass off. The hand then shows the warp's own one-tick-old hand on made-up
   frames; if the hop against the wall is the SAME size, our carry was adding nothing there and the wall is the one
   that moves wrongly.

## Tags

- Counts of carries, refusals, matches and the raw-against-drawn difference `[measured, b038 log, 16:40-16:55]`.
- Everything about what the code does `[inferred-static]`.
- Which way is the real one `[not established]`; way 1 first, then 5 and 11 `[hypothesis]`.
