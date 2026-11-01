# Gold Digger 2026-11-01: the frame rate halved between two runs on 2026-10-31 and stayed there

From: `/gd` (inside `/fullscan`), 16 shelved Ghost Hand reports (2026-10-29 to 10-31).

Ghost Hand reports give the frames a second: 50.4-51.4 in all six 10-29 runs, 47.4-48.5 in the five 10-31 runs from
13:20 to 13:40, then **29.2 at 13:57 and 29.4-30.3 in every run after** (14:30, 14:46, 18:51, 19:00)
`[measured 2026-10-31, n=11 runs]`. With it, "a controller jumped" went from 2-9 per run to 66-196 (controller reads at
half the rate look like jumps), so those counts are not tracking faults.

Something changed between 13:40 and 13:57 on 10-31. Candidates `[hypothesis]`: the 21 Sacraments stutter patch
(installed 2026-10-31; a frame cap or a lock to 30), a UEVR or game setting, or the headset's refresh / Virtual
Desktop. Proposed: check the install time of `silenttraverse.asi` against 13:40-13:57, and on the next launch read the
frame rate with the patch on and off (F8), which the stutter-patch row already plans.
