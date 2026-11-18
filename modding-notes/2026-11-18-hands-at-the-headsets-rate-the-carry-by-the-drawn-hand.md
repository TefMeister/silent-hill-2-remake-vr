# Weapons and hands at the headset's rate: why the gun doubles on a quicker move, and the carry by the drawn hand (2026-11-18, Developer PC, /pd, Fable)

**The game was not launched and nothing here has been run in it.** Read, measured on clips, tested by numbers on the
shipped code, built, installed as b030, unrun.

Tefa's top wish (2026-11-16, after wearing b028): *"at the moment it is still like weapons have smaller framerate than
the game and it makes them unusable ... when i move my motion controller even a little quicker, i still get that
transporting double model images"*; wanted: the gun and hands as steady as the headset's own rate, or failing that at
the game's rate. Continues `2026-11-13e-why-the-hands-jump-in-warp-mode-2-and-the-mode-per-frame.md` (the hands' own
pass after the warp, `handpaste.cpp`).

## 1. Tefa's old clip, seconds 5 to 10, every frame

`0725(1).mp4` from Tefa's Downloads, copied to `dev-archive/recon/2026-11-18-hands-at-headset-rate/` (the mp4 itself
is kept out of git, the six sheets of all 150 frames are in). `[measured, n=1 clip]`:

- The clip is 960 x 480, ONE view, 30 pictures a second. The headset shows 72: a clip like this samples two of every
  five headset frames, so it cannot show whether the hands were drawn on every headset frame. What it can show is a
  gun in two places inside one picture.
- 149 of the 150 frames: one gun, whole, clean edges, through slow hand moves and head turns across the street.
- Frame 44 (the one quick wrist flick in the five seconds): the slide is drawn twice. So that beta doubled on a
  quick move too; the clip is mostly slow moves.
- The hands in it are jbusfield's Lua forearm hands, hung on the controllers by UEVR itself, unsmoothed. Today's are
  our C++ IK arms with the gun hand smoothed (b023 on).
- Not readable from the clip: the beta's number and its settings (no data in the file; Tefa: "the first betas then
  beta 5"). What is known of beta 5 against 6 is in `2026-11-13d` §2 (beta 5 filled holes along motion vectors).

Against our own build: no clip of b026-b029 was found on this PC. The newest on disk is
Tefa's compile of the 2026-11-14 clips (b023-b025, BEFORE the smoothing went in): 36 consecutive frames of the pistol
during a turn, the gun single and whole in every one (sheet in the recon folder), which fits Tefa's "this works well"
of that night. The complaint came at the first wear with b023's smoothing in (b028, 2026-11-16).

## 2. The cause, in our own code `[inferred-static + verified-numerically 2026-11-18]`

On a made-up frame `handpaste.cpp` takes the hand's pixels from that eye's last real picture and carries them by "how
the hand moved since". Up to b029 that move was the RAW controller's (`API::VR::get_pose`), read at the moment the
warp is called. Two things are wrong with it:

1. **Since b023 the right hand and its gun are drawn smoothed** (`smooth.cpp`, inside the arm solve). The raw
   controller's step is not the drawn gun's step whenever the hand speeds up or slows down. The made-up frame shows
   `drawn(k-1) + raw step`, the next real frame shows `drawn(k)`: the gap flips on and off every other frame, which
   is a double gun.
2. **The moment it is read.** The arms are posed on the game thread; the warp is called a frame or two later on the
   render thread. A step read one frame off misses by the hand's change of speed, which is as large.

`dev-archive/native/tests/handcarry_test.cpp` runs the shipped filter (`smooth_math.h`) on made-up moves. The largest
gap between the made-up frame's gun and the drawn gun, in pixels of the eye picture (gun 45 cm from the eye, a point
20 cm ahead of the hand, 26 px a degree), at 72 frames a second:

| move | raw carry, gun (b023-b029) | raw carry, no smoothing | drawn carry, on time | drawn carry, 1 frame late |
| --- | --- | --- | --- | --- |
| slow drift, 10 cm in 1 s | 1.4 | 0.0 | 0.0 | 0.4 |
| "a little quicker", 20 cm in 0.4 s | **11.2** | 0.0 | 0.0 | 5.2 |
| quick, 30 cm in 0.25 s | **29.8** | 0.0 | 0.0 | 20.6 |
| wrist turn, 30 degrees in 0.3 s | 9.5 | 0.0 | 0.0 | 4.4 |
| aim across a room | 20.8 | 0.0 | 0.0 | 10.4 |

At 55 frames a second every number is about a third larger. Melee (light smoothing) is half the gun's. The full table
is in the recon folder (`tests-output-2026-11-18.txt`). This is a model of the fault, not a measurement of it in the
game: the "no smoothing" column is 0 only because the model reads the raw step exactly on time, which b025 did not.

## 3. The decision: (a) or (b)

- **(a) the hands drawn on every HEADSET frame, whatever the game's rate.** The headset shows a new picture only when
  one is handed in, and UEVR hands one in per game frame. Hands at 72 while the game runs at 55 means handing in
  pictures from a thread of our own, between the game's frames: the last world picture again, with the hands pasted
  at the newest controller reading. That is our own build of UEVR's open side (its frame loop), the size of the
  parked "our own frame warp" row: weeks, and it can break everything UEVR's loop does today (the runtime's pacing,
  the warp's own switch, cutscenes). A pasted hand is also only the side of the hand the last real picture saw.
- **(b) the hands at the game's own rate, exactly.** With the warp every game frame already gives the headset both
  eyes (one drawn, one made up). If the made-up eye's hands are exactly where the game drew them that frame, the
  hands move at the game's rate with nothing doubled, and at a steady 72 that IS the headset's rate. No new loop,
  nothing of UEVR's replaced.

**Chosen: (b), done exactly, now.** (a) stays written down (the board's parked own-warp row) for the case that 72
cannot be held; the 72-solid row is what makes (b) equal (a).

## 4. What is built (b030)

**The carry by the hand the game drew.** New `drawnpose.cpp` + `carry_math.h`; `handpaste.cpp`, `ik.cpp`,
`warpscale.cpp`, `plugin.cpp` touched.

- `ik.cpp` publishes each hand as this tick's solve placed it (target place and turn, world; the smoothing, the slide
  / pump / bolt overrides and the two-hand turn are in it). Not published while an animation plays the arms.
- The game thread records, at the end of our `on_post_calculate_stereo_view_offset`, the eye's place and both hands
  in a ring of 32. The key is the eye's place in the warp's own world, built the way UEVR's open side builds the
  view it hands the warp (PureDark's public fork, AFW branch, `FFakeStereoRenderingHook.cpp` right after the plugins'
  callback, read 2026-11-18): `position = (y, z, -x) / (world to metres * VR_WorldScale)`, `view-to-world =
  translate(position) * yawPitchRoll(-yaw, pitch, -roll)`, and `update_camera_data` copies that into the call.
- The render thread, at each warp call, finds the record whose place is the call's own view-to-world place (the
  drawn eye). Several records at one place (a dead-still head: the simulator): the view's turn decides, then the
  distance behind the newest record that the last sure match had.
- The carry for the made-up eye: `view then -> world -> the hand's move between the two drawn poses -> world -> view
  now`, straight between the two views' own matrices; the pass's shader is unchanged (it reads the same 3 x 4).
- Guards: a carry over 25 degrees or 25 cm a frame, or more than 5 cm from the raw controller's, is not used: the raw
  one is, and it is counted. So a wrong convention or a mismatched frame falls back to b029's behaviour and shows in
  the log.
- Switch: the 7th number in `sh2vr_warpscale.txt`: 1 = this (the default; the file in the profile has six numbers),
  0 = the raw controller as b020-b029. Live, read once a second.
- The proof line, every 10 s while the warp runs: `drawnpose: N views matched to their frame (M not, K with the head
  dead still), L records behind; hands carried: A by the drawn hand, B by the raw controller; the raw carry differed
  by X mm on average, Y mm at most`.

`[verified-numerically 2026-11-18]` `tests/carry_test.cpp`: 20,000 made-up eyes, hands and gun points, the truth
built a second way in Unreal's own axes (its rotation matrix from a rotator), the views built as UEVR builds them
(glm's yawPitchRoll written out): UEVR's view of a point = Unreal's own within 0.04 mm, a gun point carried from the
last real picture lands on the drawn gun within 0.06 mm (0.35 px). 3 km from the map's origin the views' floats cost
0.6 mm (3 px): not this game's streets, noted. `[compile-verified 2026-11-18]` the plugin, 26 of 26 test lines pass.

Also: `tools/build.sh --deploy` no longer rewrites an installed script that differs only in line endings (this
clone checks out CRLF; the deploy had replaced seven scripts that way; they were put back byte for byte from the
deploy's own backup, and the builds list sees only `sh2vr.dll` changed).

## 5. NOT established (all of it needs the game)

- That the call's view really is the record's, number for number (another plugin changing the view after ours, the
  world scale read as text): the log's "matched / not" counts say it at once. Not matched = the raw carry, as b029.
- That the picture the warp gets was drawn with that tick's hands (the injector picks the view of this frame or the
  one before with its own `offset`; if the hands lag the view by a frame the carry is a frame early).
- The long gun in two hands and the map are re-placed at view time (`twohand::on_view`), after the solve: the gun
  there may move a little differently from the hand the carry follows.
- Fingers, the slide, the pump: they move against the hand; they are carried with the hand, as before.
- While an animation plays the arms (reloads the game animates) the carry is the raw controller's, as before.
- How it looks. The numbers say the doubling from the smoothing and the timing goes; the eyes decide.

**What would show the derivation is wrong, not a knob:** with the tint on (4th number 2) and the hand swung on the
simulator, the green pasted hand sits beside the drawn one on made-up frames while the log says "matched" and a
small "differed": then the frame the hands belong to is off by one (the second point above), and the fix is to pair
the record one tick later, not to tune anything.

## 6. The test, when a game run is allowed (simulator first)

1. Hotel-room save, a pistol in hand, warp on. Read the log's `drawnpose:` line after 20 s: "matched" should be
   nearly all, "by the drawn hand" nearly all.
2. Swing the simulator's right hand 2 Hz (`dev-archive/tools/warp-study/`), the flip measure, file's 7th number 0
   then 1: the hands' alternation against the room's shift should fall. The log's "differed by" is the size of what
   was wrong before (expected: millimetres while the hand speeds up, near 0 held still).
3. Then Tefa: the gun waved a little quicker, and quick; `1.0 0 0 1 1.0 0.004 0` puts b029's carry back live.

## 7. The simulator run (2026-11-18 23:25 to 2026-11-19 00:10, /lm, Fable; the OpenXR Simulator, no headset)

Five starts through the launcher (`SH2VR.exe play --sim --no-install`), `toplay.sh` into play. One start crashed
22 s in (the known start crash, dossier 13; our file not in the stack `[measured, the crash report]`); kept with its log.
Tefa (live): the Continue save kills James quickly. No game function loads a chosen slot (`SaveManager` shows none), so
for the run the armory save (slot 3) was copied over the Continue slot with the game closed; the save folder was backed
up first and put back after, the same by hash.

**1. The key did not fit in play, and why** `[verified-live 2026-11-18, n=2 starts]`. In the main menu every view
matched its record. In play none did: the installed injector (beta 6) hands the warp views measured from ANOTHER
ORIGIN than the game's own, a plain shift, different per load (first save: 310.995, 1.020, 92.265 m; the armory:
409.005, -181.453, 543.561 m; the menu: none). The public source read for §4 (the fork's AFW branch, August) does not
do this, so the installed build is newer than that source. Cure, b031: `drawnpose.cpp` learns the shift (record minus
call is the same from one call to the next only for the right pairing; three calls running confirm it; thirty misses
drop it, for a load) and takes it off the hands too. After it `[verified-live, n=3 starts, about 15 minutes of play]`:
every view matched (0 not), with the simulated head swaying every match was the only one, always exactly 2.0
records behind; the hands were carried by the drawn hand on all but 1 or 2 pictures in 10 s.

**2. How far off the old carry was** `[measured live, the plugin's own line, n=12 ten-second lines]`. A melee weapon in
hand (the pistol asked for through the weapon manager; the smoothing reported melee), the simulated right hand swung
at 1 Hz (it moves in jumps, 35 a second: harsh): the raw controller's carry was 2.8 to 3.9 mm from the drawn hand's on
average and 17 to 49 mm at most, on the frames where both existed. 3 mm at 40 cm is about 11 px in the headset.
With the hand still: 0.0.

**3. The pictures** (`dev-archive/tools/warp-study/pair.py`, new: the hand's place in the left picture minus the right,
whose frame-to-frame flip is the made-up eye's error alone; the simulator's 22 consecutive frames, 160 px wide).
`[measured, simulator]`, flip in x, px of the capture:

| setting | round A (n=3 each) | round B (n=3 each) | round C, fast wide swing (n=3 + 2 or 3) |
| --- | --- | --- | --- |
| new carry, the view's own tick | 0.71 0.76 1.06 | 1.44 1.25 1.70 | 2.61 0.83 1.83, 1.35 1.07 |
| new carry, hands one tick later | | 0.63 0.87 1.54 | 2.83 1.06 1.38, 2.09 1.14 0.99 |
| new carry, hands one tick earlier | | 1.82 1.17 2.12 | 2.06 1.08 2.43, 2.70 1.64 1.96 |
| old carry (raw controller) | 1.49 1.31 1.26 | 0.94 1.29 2.23 | 2.13 2.20 2.61 |

The new carry is lower than the old in rounds A and C and level in B; "one tick earlier" is the worst of the three
pairings; "own tick" against "one tick later" cannot be told apart at this size. **Not settled on the simulator:
whether the hands in a picture are the view's own tick's or the next one's.** The switch for it is live: the 8th
number in `sh2vr_warpscale.txt` (-1, 0, 1), beside the 7th (1 = the drawn hand, 0 = the raw controller).

**4. Also seen.** The frame rate on the simulator in the armory was 44 to 62 a second. The reader's reading of the
fork (which tick a call's view belongs to: the picture's own while the game thread is at most 2 ticks ahead, else 2
later) fits the constant "2.0 records behind". The counts of calls "with a velocity picture" were 0 at the 2000th and
6000th call of the first start (older logs: about half); not followed up `[hypothesis: counted before play began]`.

**Installed: b031** = b030 + the learned origin + the tick switch; `sh2vr_warpscale.txt` = `1.0 0 0 1 1.0 0.004 1 0`.
NOT established: a gun in hand (the run had a melee weapon), how it looks in the headset, the tick question above.

## 8. Tefa wears b031 and b032 (2026-11-19 00:00-00:30, /ms, Opus; the real headset, Tefa driving)

The game was started by the session on Tefa's word ("please launch the game"); Tefa played.

- **b031** `[reported, n=1 wear]`: *"when moving it quickly it still leaves like a trail of guns behind it, but when
  just holding it out it feels really good and smooth"*. The log: 72 frames a second (13.9 ms), every view matched,
  1436-1440 of 1440 hands carried by the drawn hand `[measured]`. The tick switch (8th number 1, hands one tick later)
  live: *"still the same trail"*: put back to 0.
- The tint (4th number 2), Tefa's screenshots in the recon folder: green = our pasted hand where it is now, red = the
  warp's leftover copy painted over. **Corrected the same session:** I first read the red gun shape as the old gun left
  in place. With the tint, red marks the painted area whatever fills it, so that reading proved nothing. Still found
  and fixed (b032): with the head still the search for the background beyond the copy stepped 0 and never left it;
  it now also searches away from where each hand went and sideways (`settings::HANDPASTE_FILL_STEP`), magenta where it
  finds nothing. No magenta in Tefa's three b032 screenshots: a background was found everywhere `[measured, n=3]`.
  `tests/shader_compile.py` (new) compiles the pass with the same call the plugin makes, at the desk.
- **b032** `[reported, n=1 wear]`: *"it felt pretty damn good honestly!"*.
- Open: whether the remaining trail on quick waves is a second gun inside one picture (ours, fixable) or 72 separate
  pictures strung together by the eye (the display: only more pictures for the hands, option (a), or a slight hand-only
  blur would help). One untinted screenshot mid-wave tells them apart; not taken yet.
- New faults from Tefa, same wear: James's movement seems off ("the roomscale might be off"), and a two-handed gun
  grabbed "just starts moving on its own like curving upwards and stays in that position in my right hand" (seen in
  the run before the last). Board rows.
- The game closed itself once at 00:20 (UEVR log ends at a DLSS release, no crash report written).
