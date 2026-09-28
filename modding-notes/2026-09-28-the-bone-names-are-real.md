# 2026-09-28: the bone names are real

Home PC, `/lm`, evening. Tefa brought the game into the first street scene; Claude attached UEVR
(nightly AFW beta.6, from outside the game folder) with a small read-only script.

## What we learned

- **Every bone and object name our plan relies on is the game's own.** 388 bones on James; the arm
  chain, head, spine and hand sockets all exist with the expected parents `[verified-live 2026-09-28, n=1]`.
- **A prediction came true:** the reader, working only from CharlotteLiu's saved settings, said the
  upper arms would be bones 56 and 126. They are. So the community profiles' skeleton is the current one.
- **Useful extras:** Epic's IK-target bones (`ik_hand_l/r`), and `parent_cam` / `eye_l` / `eye_r` on
  the head: candidates for where the VR camera should sit.
- **CharlotteLiu's mod** (read by the reader) uses the same names, runs Native Stereo, and names the
  push/climb blueprints the game-animation test should watch.

## Not established

- Where a held weapon attaches (nothing was held). Whether `PushableComponent` lives where the profiles
  say (it was empty here). Nothing about how the game's interaction animations look from James's eyes.

## Also

- The game now opens in a 1920×1080 window on the home PC (Tefa lowered settings tonight); the notes'
  1280×720 figure is the dev PC's.
- The home PC already had this repo since 2026-09-20; the board wrongly said otherwise until tonight.
