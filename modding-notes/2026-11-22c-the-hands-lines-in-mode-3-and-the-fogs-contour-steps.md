# 2026-11-22c: the hands' fine lines in the warp's "both" mode, the fog's contour steps, the shimmer (b042, b043, b044)

/lm, Build PC, Fable, 20:25 to about 22:00. Pretend headset (OpenXR Simulator) for the pictures, the real headset
lying still for the cost. The job: the Lanes hand-on `20261122-205500-sh2-b041-hands-lines-shimmer-fog`.
Tefa at the start: he used "Debug Framewarp" (the fork's menu) for the green hands of his second video.
Work files (clips, sheets): logs folder `2026-11-22-2030-hands-lines-shimmer-fog/`. Scripts and five pictures kept in
`dev-archive/recon/2026-11-22-hands-lines-shimmer-fog/`. The reader's two notes are dossier sections 22 and 23.

## How full-size pictures were got (the hand-on's open risk)
The simulator's own captures are 160 px wide. Instead: the simulator window made 3440 x 1440 and kept on top
(`simtop.ps1`), each eye then 1712 x 1381 on screen (from 1280 x 1400), and the screen recorded at 60 a second,
lossless (`grab.sh`, `flick.py`). The right hand swung by the simulator's controller file (`swing2.py`: 5 s, 0.6 Hz,
8 cm, raised in front of plain fog). The simulator moves a hand about 20-34 times a second, so each move is a jump of
a few cm: harsher than a real hand at 72 a second.
The b041 start that "stopped at the title screen" did not repeat: 7 of 7 simulator starts and 2 of 2 real-headset
starts walked into play in 100-116 s `[measured 2026-11-22, n=9]`.

## A. The hands in mode 3
**What b041 shows** `[measured, simulator, full-size, town start]`: hands still = clean. A hand moving = on the
made-up pictures the hand and sleeve are striped (a comb pattern), their edge is dotted, there is a pale thin line
just outside the edge, and black dashes sit beside the fingers. Real pictures are clean. With the warp off
(rendering method 0) there is only a soft halo from DLSS, no dots and no line: the hard marks are the warp's.
The reader, from the warp's own programs `[inferred-static]`: in mode 3 the warp drops the moving pixels of the eye's
last picture (a hand-shaped hole), builds the hand from the other eye, and replaces an "edge band" 1/390 of the
picture wide left and right of every hole. That band is the fine line; its width is a constant, no setting.

**The warp's options, one at a time, two recordings each** `[measured, n=2 each]`: `VR_AFW_FixObjectMotionVector`
off, `VR_AFW_FixMovingObjectBrightnessFlickering` off, `VR_AFW_UltraResponsive` off, `VR_AFW_FixObjectMotionRange`
1 and 8: no change (the count of stand-out pixels around the hand stays in the same 240-390 band; a weak measure,
the pictures agree). Mode 1: clearly worse (900). Mode 2 with our hand pass: clean (100-125).
⚠️ The first batch of this test was worthless: the game had put up "Your controller has been disconnected" and I
counted a paused picture (my miss: not looked at before measuring). Redone with a "is it moving" check.

**b042: our own hand pass after a mode 3 call.** `handpaste.cpp` drew nothing in mode 3 by its own condition. Now it
runs there too (9th number of `sh2vr_warpscale.txt`, 0 = off; `HANDPASTE_IN_MODE3`). The pasted hand (from the eye's
last real picture, carried by the drawn hand's move) covers the warp's striped hand. New for mode 3, in the pixel
program: a pixel that is not hand but has pasted hand within a band's width of it (max(3 px, 0.0034 of the width),
four directions), and every black hole, takes the picture from beside it, away from the hand; a gap inside the hand
takes the hand's colour. Result `[measured, simulator]`: the stripes, the dots and the black dashes are gone; on the
fastest simulator jumps small skin-coloured slivers remain at the fingertips (the warp's other-eye hand where our
carried hand is a few pixels off). Whether a real hand at 72 a second leaves any: Tefa's eyes.
A development aid went in with it: a file `sh2vr_handpaste.hlsl` in the profile replaces the pixel program live.

**Cost, real headset lying still, 2688 x 2880 an eye, f4decomp paused** `[measured, 20 s each, the plugin's "alive"
average, off / on twice]`: 13.9-14.0 ms without the pass, 14.6-14.9 with it over the whole picture: 0.6 to 1.0 ms,
72 a second down to about 67, the card at 96-97% either way. **b044**: the pass draws and copies only a box around
each hand (controller then and now, the weapon's line, the shoulder); a hand outside the picture gets none. On the
simulator with both hands raised the boxes are 63-70% of the picture; with the controllers out of view nothing is
drawn. NOT measured: the cost in real play with the hands up (expect about two thirds of the above while they are in
view) `[open]`.

## C. The fog's "contour lines" (Tefa: "like contours on the map")
- The eyes' pictures are 8 bits a colour (format 87), and the town's fog is so slow a gradient that it is drawn in
  flat one-level steps: 88% of neighbouring fog pixels are exactly equal; painting odd levels white and even ones
  black gives a clean contour map (`fog-odd-even-levels-contour-map.png`) `[measured, simulator window]`.
- The steps are in the picture as it reaches our code, not made by the simulator's window: a test view built into
  the new pass paints the incoming picture's odd levels white, and the contour rings are there
  (`fog-test-view-1-as-it-came.jpg`) `[measured]`. Where the game's own dither is lost is not known; the reader's
  guesses (the tonemapper's grain sized for a 10-bit in-between picture; the injector forcing the game to 8 bits)
  are `[hypothesis]`. `r.Tonemapper.GrainQuantization` reads 1, `r.FilmGrain` 0.
- **The pass (`deband.cpp`, new):** a pixel whose four neighbours at a random direction and distance (up to 12 px)
  average out to within one level of it, none more than three levels away, gets that average plus one level of fine
  grain (new every frame). Textured things are left alone. `sh2vr_deband.txt`: "on radius threshold grain [test view]".
- b042's first version did nothing at the gentle setting (it asked all four neighbours to be within 1.5 levels: the
  steps are about 10 px apart) and b043's test view showed it reached only the made-up eye: the drawn eye's picture
  is shown from the engine's scene render target, not from the texture in the warp call (the reader, dossier 23).
  **b044** writes the drawn eye's result into the scene render target's left, eye-sized part: test view 2 then shows
  the pass in both eyes in every frame (`fog-test-view-2-after-the-pass-both-eyes.jpg`)
  `[verified-live 2026-11-22, simulator, n=1]`. Real headset lying still: it runs (scene render target 5376 x 2880,
  format 90), no measurable cost (13.9 ms with and without) `[measured, n=1]`.
- NOT established: that these steps are what Tefa sees as contour lines in the headset (it fits his words; only his
  eyes can say), and whether Virtual Desktop's video stream bands the fog again after us. The pass is OFF in the dll
  (`DEBAND_ON = false`) and switched ON by the file for his wear.
- "Denser / more high quality" fog: not touched this session.

## B. The shimmer on fine patterns (grate, puddles): measured, not cured
Simulator, right eye, view still, 2 s, back-and-forth flicker per pixel in boxes (`flick.py`; this spot's grate is
small and far, the trees and wires are the strong patches) `[measured, n=1 unless said; run-to-run spread on the
trees 0.31-0.54]`:

| what | trees | wires | fog | pavement | grate |
| --- | --- | --- | --- | --- | --- |
| b042 as built (n=3) | 0.31-0.54 | 0.25-0.36 | 0.11 | 0.09-0.10 | 0.09-0.11 |
| warp off (rendering method 0, n=2) | 0.16 | 0.24 | 0.14 | 0.09 | 0.07 |
| ghosting fix off | 1.65 | 1.42 | 1.14 | 0.73 | 1.15 |
| `r.Tonemapper.Sharpen` 0 (is 0.25) | 0.43 | 0.39 | 0.11 | 0.09 | 0.11 |
| `r.ViewTextureMipBias.Offset` 0 (is -0.3) | 0.39 | 0.34 | 0.09 | 0.07 | 0.09 |
| `VR_AFW_IgnoreMotionThreshold` 5 / 10 / 100 | 0.26-0.34 | 0.23-0.26 | 0.10-0.11 | 0.10 | 0.08-0.09 |
| warp mode 2 | 0.31 | 0.24 | 0.10 | 0.09 | 0.08 |

Read: the warp's made-up pictures about double the flicker of thin things (leaves, the grate) against both eyes
drawn; no warp mode or threshold changes that; the sharpening does nothing; the texture bias a little, within the
spread. So the shimmer is partly the price of the warp (its made-up picture is thrown forward pixel by pixel,
unfiltered: the reader) and partly DLSS on a pattern near pixel size. Not tried: the DLSS preset
(`r.NGX.DLSS.Preset` reads 5), a higher render size, the reflection values for the puddles. A measure at the grate
itself needs the view moved to it (it is 80 px wide from the town start spot on the simulator).

## Things learned about driving the game (also in KNOWN.md)
- "Your controller has been disconnected" comes up on the simulator when its hands are moved far or parked low:
  Enter hides it and leaves the pause menu; the pause menu does not close on Enter or the connector's pad A; the
  live `InGameMenuWidget_BP_C /Engine/Transient...` object's `ReturnToGame()` closes it.
- The simulator's window is the place for full-size pictures (above).
- config.txt after each of tonight's 9 quits again held the plugin's live switches; `cfgfix.sh` puts the three
  starting lines back before every start. The plugin-side cure (row SH2-032) is not built.

## Mode 3 runs tonight (the crash count)
9 runs in mode 3, 3 to 12 minutes each, simulator 7 and real headset 2 (both lying still): no crash, no hang
`[measured]`. The 19:54 crash stays n=1.

## Open after this
1. Tefa's eyes on b044: the hands' lines, the fog with the pass on and off, the frame rate with hands up.
2. The hand pass's cost in play; tighter boxes (the arm as a slanted strip, not a box to the corner) if it shows.
3. The slivers at the fingertips on fast moves, if his eyes find them: the warp's other-eye hand peeking past ours.
4. The shimmer: the DLSS preset and render size at the grate and a puddle, with the view on them.
5. The fog "denser / more high quality".
6. SH2-032, the plugin putting config.txt's starting values back itself.

## 22:00 to 23:45: Tefa in the headset (b044, then b045 with live changes); b046 built, installed, unworn

Real headset, f4decomp held. His screenshots: logs folder `2026-11-22-2030-hands-lines-shimmer-fog/tefa/`; the run's
log `uevr-log_2026-11-22_2259_b045-tefa-wear-first-enemy.txt`. His words are quoted; everything else is my reading.

**b044 was wrong on the hands, and the fog pass too gentle** `[reported, Tefa, worn, n=1]`:
- "i can still see the fog lines and the contours around hands, like nothing changed". The log showed both passes
  running. With the tint: green hands, a yellow band, "a thick red line on the side of hand when turning with right
  stick and hands are very framey ... like lower framerate- teleporty"; "the hands now feel like framey when we first
  got rid of the shimmer around them + now the shimmer around them is also there". So our PASTED hand in mode 3 is not
  where the hand is on made-up frames (not found why; the simulator pictures had only checked that it was clean, not
  where it sat: my miss). With the pass off the hands were smooth again ("yes").
- **What the lines are** (pass off = b041's hands): "they are on both sides of the hand with both eyes open ... right
  eye open only, i can see the lines on the right side of my hand"; "looks like a see-through glass like line, not
  hand colour, like a ghost hand is on both sides of my real James hand, barely seeable"; "lines when head turning and
  walking"; "it flickers really quickly"; none when everything is still. The same dotted line stands beside a sign
  post and a pole (his screenshot 22:48). Reading: a near thing hides a patch of the world from the other eye; mode 3
  has no fresh picture for that patch on a made-up frame and fills it with this eye's old picture unmoved (the
  reader, dossier 22): a stale hand-shaped patch on the side away from the other eye `[hypothesis that fits every
  point; the tint on the simulator showed the patch hand-shaped and in that place: measured]`.

**b045: the patch refilled, no pasted hands in mode 3.** A pixel whose spot the other eye cannot see (what that eye
sees in that direction is nearer than 0.6 of the spot's distance) takes this eye's last real picture moved by the
camera, by depth (`still_src`, the reader's). The whole picture; cost on the real headset lying still about 0.2 ms
(14.1 against 13.9) `[measured, n=1]`. Tefa: "weaker for sure", "definitely noticably weaker!"; the post "did have
a bit of it when i moved ... definitely better"; left: thin dotted arcs beside the hand, dots along the sleeve and
the plank "quite noticably when i turn the hand in front of me", and "enemies and other moving things do have that
shimmer around them, so i could see the enemy outline throught the fog before i saw the enemy".
Then live, by the development shader file (no restart): the patch 4 pixels wider (five looks at the other eye's
depth, the nearest counts) and a filter for single dark dots (a pixel darker than 0.6 of both its left and right
neighbours takes their mean). Tefa: arcs "weaker, nearly gone", dots "weaker, a lot weaker"; left: a thin dark ring
across the left palm (the thumb's outline on the palm) and "sometimes the hand also flickers like gets dark but
without the black dots all over, just darker and then goes back to normal". The filter then also up/down (for the
ring): loaded live at 23:35, NOT judged by him.

**The fog pass, his settings** (`sh2vr_deband.txt`: on radius threshold grain view smoothing):
| setting | Tefa |
| --- | --- |
| 1 12 1 1 (b044, per-colour average, one grain) | "i didn't see the difference" |
| 1 24 3 3 | "now it's better, but it is too foggy now can't see anyhting" |
| 1 16 1.5 2 | "the lines are gone now" but "still less fog would be better", "the bottom part of the fog looks like colourful a little like rainbow colours" |
| 1 12 1 2, then 1 8 0.75 1.5 | asked for less fog still; off = "lines are back as well now" |
| b045: 1 12 1 2 0 0 (grain only, one brightness amount) | "Fog looks like it has lines, so need to put the denser fog back on" |
| b045: 1 16 1.5 2 0 1 (smoothing back, one brightness amount) | "lines are gone nearly, still see them sometimes when i really focus on finding them, but fog still looks colourful" |
So: the grain alone does not cure it, the smoothing does, and one shared brightness amount leaves the fog tinted
(the steps of red, green and blue fall at different places `[hypothesis]`).

**b046 (installed, unworn):** the live shader's version baked in (wider patch, the dot filter both ways); deband
smooths each colour by itself and gives each its OWN grain. File left at `1 16 1.5 2 0 1`.

**Also from the wear:** one eye sees a puddle brighter than the other (he asks for an "eye fix" like the RE games');
double doors: pushing one leaf opens the other; the game's hurt red at the picture's edge is shown in VR; "traversal
stutters when running around the town" with 21 Sacraments installed and on (`LoadPlugins=1`): the log counts 33
stalls of 115-170 ms in 45 minutes (one more of 766 ms was my shader loading). Mode 3 on the real headset: 45 minutes
in play, no crash.

**Decided by Tefa 2026-11-22:** no warp of our own; PureDark gave permission to reverse engineer his frame warp
with AI for this game only, so we keep his warp, read it, and work before and after it.
